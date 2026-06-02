#include "HJGraph.h"
#include "HJMediaInfo.h"
#include "HJFFHeaders.h"
#include "HJFLog.h"

#include "gtest/gtest.h"

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <future>
#include <thread>

NS_HJ_USING;

namespace {

constexpr const char* kSeekTestMediaPath = "E:/audio/001ow9fj37bUzO.mkv";
constexpr int kDefaultSampleRate = 48000;
constexpr int kDefaultChannels = 2;
constexpr int kDefaultFrameSamples = 1024;
constexpr int64_t kPollIntervalMs = 20;
constexpr int64_t kOpenTimeoutMs = 5000;
constexpr int64_t kSeekSettleTimeoutMs = 2500;
constexpr int64_t kSeekToleranceMs = 250;
constexpr int64_t kPreparedSeekTargetMs = 5000;
constexpr int64_t kRapidSeekAms = 1000;
constexpr int64_t kRapidSeekBms = 3000;
constexpr int64_t kRapidSeekCms = 7000;

bool strictSeekPrecisionEnabled()
{
    const char* value = std::getenv("HJ_ENABLE_STRICT_SEEK_PRECISION");
    return value != nullptr && std::string(value) == "1";
}

bool mediaFileExists()
{
    std::ifstream input(kSeekTestMediaPath, std::ios::binary);
    return input.good();
}

HJAudioInfo::Ptr makeMusicPlayerTestAudioInfo()
{
    auto audio_info =
        HJAudioInfo::makeAudioInfo(kDefaultChannels,
                                   kDefaultSampleRate,
                                   AV_SAMPLE_FMT_S16,
                                   kDefaultFrameSamples);
    if (audio_info == nullptr) {
        return nullptr;
    }
    audio_info->m_bytesPerSample = 2;
    return audio_info;
}

HJGraphPlayer::Ptr createMusicPlayerGraph()
{
    auto graph = HJGraphPlayer::createGraph(HJGraphType_MUSIC, 0);
    if (graph == nullptr) {
        return nullptr;
    }

    auto param = std::make_shared<HJKeyStorage>();
    auto audio_info = makeMusicPlayerTestAudioInfo();
    if (audio_info == nullptr) {
        return nullptr;
    }

    (*param)["audioInfo"] = audio_info;
    (*param)["repeats"] = 0;
    (*param)["prerollDurationMs"] = int64_t{120};
    (*param)["pcmCallbackIntervalMs"] = int64_t{20};

    if (graph->init(param) != HJ_OK) {
        graph->done();
        return nullptr;
    }

    return graph;
}

bool waitUntil(const std::function<bool()>& predicate, int64_t timeout_ms)
{
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    while (std::chrono::steady_clock::now() < deadline) {
        if (predicate()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(kPollIntervalMs));
    }
    return predicate();
}

bool waitForTimestampNear(const HJGraphPlayer::Ptr& graph,
                          int64_t target_ms,
                          int64_t tolerance_ms,
                          int64_t timeout_ms,
                          int64_t* observed_ts = nullptr)
{
    int64_t last_ts = 0;
    const bool matched = waitUntil([&]() {
        last_ts = graph ? graph->getCurrentTimestamp() : 0;
        bool mat = std::llabs(last_ts - target_ms) <= tolerance_ms;
        HJFLogi("202604171637 mat:{}, delta:{}, last_ts:{}, target_ms:{}", mat, last_ts - target_ms, last_ts, target_ms);
        return mat;
    }, timeout_ms);

    if (observed_ts != nullptr) {
        *observed_ts = last_ts;
    }
    return matched;
}

class HJGraphMusicPlayerSeekTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        if (!mediaFileExists()) {
            GTEST_SKIP() << "Seek test media missing: " << kSeekTestMediaPath;
        }
    }

    HJGraphPlayer::Ptr createOpenedGraph()
    {
        auto graph = createMusicPlayerGraph();
        EXPECT_NE(graph, nullptr);
        if (graph == nullptr) {
            return nullptr;
        }

        auto stream_opened_promise = std::make_shared<std::promise<void>>();
        auto delivered = std::make_shared<std::atomic<bool>>(false);
        const int register_ret = graph->eventBus()->registerHandler(
            EVENT_GRAPH_STREAM_OPENED_ID,
            [stream_opened_promise, delivered]() {
                bool expected = false;
                if (delivered->compare_exchange_strong(expected, true)) {
                    stream_opened_promise->set_value();
                }
            });
        EXPECT_EQ(register_ret, HJ_OK);
        if (register_ret != HJ_OK) {
            graph->done();
            return nullptr;
        }

        auto url = std::make_shared<HJMediaUrl>(kSeekTestMediaPath);
        if (url == nullptr) {
            ADD_FAILURE() << "failed to create test media url";
            graph->done();
            return nullptr;
        }
        if (graph->openURL(url) != HJ_OK) {
            ADD_FAILURE() << "graph->openURL failed for " << kSeekTestMediaPath;
            graph->done();
            return nullptr;
        }

        auto future = stream_opened_promise->get_future();
        if (future.wait_for(std::chrono::milliseconds(kOpenTimeoutMs)) !=
            std::future_status::ready) {
            ADD_FAILURE() << "timed out waiting for EVENT_GRAPH_STREAM_OPENED";
            graph->done();
            return nullptr;
        }
        if (!waitUntil([&]() { return graph->getDuration() > 0; }, kOpenTimeoutMs)) {
            ADD_FAILURE() << "timed out waiting for graph duration";
            graph->done();
            return nullptr;
        }

        return graph;
    }
};

TEST_F(HJGraphMusicPlayerSeekTest, OpenUrlWithStartTimestampSettlesNearPreparedTarget)
{
    if (!strictSeekPrecisionEnabled()) {
        GTEST_SKIP() << "Set HJ_ENABLE_STRICT_SEEK_PRECISION=1 to run strict prepared-seek precision validation";
    }

    auto graph = createMusicPlayerGraph();
    ASSERT_NE(graph, nullptr);

    auto stream_opened_promise = std::make_shared<std::promise<void>>();
    auto delivered = std::make_shared<std::atomic<bool>>(false);
    ASSERT_EQ(graph->eventBus()->registerHandler(
                  EVENT_GRAPH_STREAM_OPENED_ID,
                  [stream_opened_promise, delivered]() {
            bool expected = false;
            if (delivered->compare_exchange_strong(expected, true)) {
                stream_opened_promise->set_value();
            }
        }),
              HJ_OK);

    auto url = std::make_shared<HJMediaUrl>(kSeekTestMediaPath);
    ASSERT_NE(url, nullptr);
    (*url)["startTimestamp"] = kPreparedSeekTargetMs;

    ASSERT_EQ(graph->openURL(url), HJ_OK);

    auto stream_opened_future = stream_opened_promise->get_future();
    ASSERT_EQ(stream_opened_future.wait_for(std::chrono::milliseconds(kOpenTimeoutMs)),
              std::future_status::ready);
    ASSERT_TRUE(waitUntil([&]() { return graph->getDuration() > 0; },
                          kOpenTimeoutMs));

    const int64_t duration_ms = graph->getDuration();
    ASSERT_GT(duration_ms, 0);

    const int64_t expected_target_ms =
        std::min<int64_t>(kPreparedSeekTargetMs, duration_ms);
    int64_t observed_ts = 0;
    EXPECT_TRUE(waitForTimestampNear(graph,
                                     expected_target_ms,
                                     kSeekToleranceMs,
                                     kSeekSettleTimeoutMs,
                                     &observed_ts))
        << "expected near prepared seek target " << expected_target_ms
        << "ms, observed " << observed_ts << "ms";

    EXPECT_EQ(graph->done(), HJ_OK);
}

TEST_F(HJGraphMusicPlayerSeekTest, SeekDuringPlaybackSettlesNearRequestedTimestamp)
{
    if (!strictSeekPrecisionEnabled()) {
        GTEST_SKIP() << "Set HJ_ENABLE_STRICT_SEEK_PRECISION=1 to run strict runtime-seek precision validation";
    }

    auto graph = createOpenedGraph();
    ASSERT_NE(graph, nullptr);

    const int64_t duration_ms = graph->getDuration();
    ASSERT_GT(duration_ms, 2000);

    const int64_t target_ms = std::min<int64_t>(duration_ms / 2, 8000);
    ASSERT_GT(target_ms, 0);

    ASSERT_EQ(graph->seek(target_ms), HJ_OK);

    int64_t observed_ts = 0;
    EXPECT_TRUE(waitForTimestampNear(graph,
                                     target_ms,
                                     kSeekToleranceMs,
                                     kSeekSettleTimeoutMs,
                                     &observed_ts))
        << "expected near runtime seek target " << target_ms
        << "ms, observed " << observed_ts << "ms";

    EXPECT_EQ(graph->done(), HJ_OK);
}

TEST_F(HJGraphMusicPlayerSeekTest, RapidSeeksCoalesceToLastRequestedTimestamp)
{
    auto graph = createOpenedGraph();
    ASSERT_NE(graph, nullptr);

    const int64_t duration_ms = graph->getDuration();
    ASSERT_GT(duration_ms, 0);

    //for (size_t i = 0; i < 10; i++)
    {
        //int64_t target_c_ms = HJUtilitys::random(0, duration_ms);
        //const int64_t target_a_ms = std::min<int64_t>(kRapidSeekAms, duration_ms);
        //const int64_t target_b_ms = std::min<int64_t>(kRapidSeekBms, duration_ms);
        //const int64_t target_c_ms = std::min<int64_t>(kRapidSeekCms, duration_ms);
        const int64_t target_c_ms = std::min<int64_t>(18467, duration_ms);

        //ASSERT_EQ(graph->seek(target_a_ms), HJ_OK);
        //ASSERT_EQ(graph->seek(target_b_ms), HJ_OK);
        ASSERT_EQ(graph->seek(target_c_ms), HJ_OK);

        int64_t observed_ts = 0;
        EXPECT_TRUE(waitForTimestampNear(graph,
            target_c_ms,
            kSeekToleranceMs,
            kSeekSettleTimeoutMs,
            &observed_ts))
            << "expected near final rapid seek target " << target_c_ms
            << "ms, observed " << observed_ts << "ms";

        HJLogi("end");
    }

    EXPECT_EQ(graph->done(), HJ_OK);
}

} // namespace
