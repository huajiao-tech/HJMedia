#include "HJFFDemuxer.h"
#include "HJMediaInfo.h"
#include "HJFLog.h"

#include "gtest/gtest.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

NS_HJ_USING;

namespace {

constexpr const char* kDemuxerSeekTestMediaPath = "E:/audio/001ow9fj37bUzO.mkv";
constexpr int64_t kDemuxerSeekToleranceMs = 250;
constexpr int64_t kDemuxerSeekTargetAms = 5000;
constexpr int64_t kDemuxerSeekTargetBms = 8000;
constexpr int kFramesToInspectAfterSeek = 12;

bool strictFfDemuxerSeekPrecisionEnabled()
{
    return true;
    //const char* value = std::getenv("HJ_ENABLE_FFDEMUXER_SEEK_PRECISION");
    //return value != nullptr && std::string(value) == "1";
}

bool demuxerSeekMediaFileExists()
{
    std::ifstream input(kDemuxerSeekTestMediaPath, std::ios::binary);
    return input.good();
}

HJMediaUrl::Ptr makeDemuxerSeekMediaUrl()
{
    return std::make_shared<HJMediaUrl>(kDemuxerSeekTestMediaPath);
}

std::vector<int64_t> readAudioPtsSequence(HJFFDemuxer& demuxer, int frames_to_read)
{
    std::vector<int64_t> pts_values;
    for (int i = 0; i < frames_to_read; ++i) {
        HJMediaFrame::Ptr frame = nullptr;
        const int ret = demuxer.getFrame(frame);
        if (ret == HJ_EOF || ret < HJ_OK || frame == nullptr) {
            break;
        }
        if (!frame->isAudio() || !frame->isValidFrame()) {
            continue;
        }
        pts_values.push_back(frame->getPTS());
    }
    return pts_values;
}

class HJFFDemuxerSeekPrecisionTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        if (!demuxerSeekMediaFileExists()) {
            GTEST_SKIP() << "Seek test media missing: " << kDemuxerSeekTestMediaPath;
        }

        demuxer = std::make_shared<HJFFDemuxer>();
        ASSERT_NE(demuxer, nullptr);

        auto url = makeDemuxerSeekMediaUrl();
        ASSERT_NE(url, nullptr);
        ASSERT_EQ(demuxer->init(url), HJ_OK);
    }

    void TearDown() override
    {
        if (demuxer) {
            demuxer->done();
            demuxer = nullptr;
        }
    }

    void assertSeekPtsNearTarget(int64_t target_ms)
    {
        ASSERT_NE(demuxer, nullptr);

        const int64_t duration_ms = demuxer->getDuration();
        ASSERT_GT(duration_ms, 0);
        target_ms = std::min(target_ms, duration_ms);

        ASSERT_EQ(demuxer->seek(target_ms), HJ_OK);

        const std::vector<int64_t> pts_values =
            readAudioPtsSequence(*demuxer, kFramesToInspectAfterSeek);
        ASSERT_FALSE(pts_values.empty()) << "No audio frames available after seek("
                                         << target_ms << ")";

        const int64_t first_pts = pts_values.front();
        const auto nearest_it = std::min_element(
            pts_values.begin(),
            pts_values.end(),
            [target_ms](int64_t lhs, int64_t rhs) {
                return std::llabs(lhs - target_ms) < std::llabs(rhs - target_ms);
            });
        ASSERT_NE(nearest_it, pts_values.end());
        const int64_t nearest_pts = *nearest_it;

        HJFLogi("HJFFDemuxer seek target:{} first_pts:{} nearest_pts:{} inspected:{}",
                target_ms,
                first_pts,
                nearest_pts,
                pts_values.size());

        EXPECT_LE(std::llabs(first_pts - target_ms), kDemuxerSeekToleranceMs)
            << "first demuxed audio pts after seek(" << target_ms << ") was "
            << first_pts << "ms";
        EXPECT_LE(std::llabs(nearest_pts - target_ms), kDemuxerSeekToleranceMs)
            << "closest demuxed audio pts within first " << pts_values.size()
            << " frames after seek(" << target_ms << ") was " << nearest_pts << "ms";
    }

    HJFFDemuxer::Ptr demuxer;
};

TEST_F(HJFFDemuxerSeekPrecisionTest, SeekToFiveSecondsYieldsAudioPtsNearTarget)
{
    if (!strictFfDemuxerSeekPrecisionEnabled()) {
        GTEST_SKIP() << "Set HJ_ENABLE_FFDEMUXER_SEEK_PRECISION=1 to run strict HJFFDemuxer seek validation";
    }

    //assertSeekPtsNearTarget(kDemuxerSeekTargetAms);
    for (size_t i = 0; i < 10; i++)
    {
        int64_t rand_ts = HJUtilitys::random(0, 192518);
        assertSeekPtsNearTarget(rand_ts);
    }
}

TEST_F(HJFFDemuxerSeekPrecisionTest, SeekToEightSecondsYieldsAudioPtsNearTarget)
{
    if (!strictFfDemuxerSeekPrecisionEnabled()) {
        GTEST_SKIP() << "Set HJ_ENABLE_FFDEMUXER_SEEK_PRECISION=1 to run strict HJFFDemuxer seek validation";
    }

    assertSeekPtsNearTarget(kDemuxerSeekTargetBms);
}

} // namespace
