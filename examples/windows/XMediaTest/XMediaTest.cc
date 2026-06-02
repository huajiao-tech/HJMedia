//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN         //��ֹ windows.h ���� winsock.h, ������WinSock2.h��ͻ
#endif
#include "HJUtils.h"
#include "HJMedias.h"
#include "HJCores.h"
#include "HJGuis.h"
#include "HJImguiUtils.h"
#include "HJFLog.h"
#include "HJNetManager.h"
#include "HJNets.h"
#include "HJLocalServer.h"
#include "HJGlobalSettings.h"
#include "HJMov2Live.h"
#include "HJContext.h"
#include "HJDataSourceKit.h"
#include "HJGraph.h"
#include "HJGraphAudioMixer.h"
#include "HJPluginAudioMixer.h"
#include "HJPluginFDKAACEncoder.h"
#include "../XMediaTools/HJGraphPlayerView.h"

#include "gtest/gtest.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <future>
#include <thread>

#if defined(HJ_OS_WIN32)
#include <crtdbg.h>
#elif defined(HJ_OS_MACOS)
#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>
#endif

using namespace HJ;

namespace {

size_t buildPluginKeyHash(const std::string& i_name, HJMediaType i_type, int i_track_id)
{
    size_t h1 = std::hash<std::string>{}(i_name);
    size_t h2 = std::hash<int>{}(static_cast<int>(i_type));
    size_t h3 = std::hash<int>{}(i_track_id);
    size_t seed = h1;
    seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= h3 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
}

int64_t samplesToPtsMs(int64_t i_samples, int i_sample_rate)
{
    if (i_sample_rate <= 0) {
        return 0;
    }
    return (i_samples * 1000 + i_sample_rate / 2) / i_sample_rate;
}

HJAudioInfo::Ptr makeAacEncoderTestAudioInfo(int i_aac_type)
{
    auto audio_info = HJAudioInfo::makeAudioInfo(
        2,
        48000,
        AV_SAMPLE_FMT_S16,
        HJ_AUDIO_MIXER_OUTPUT_SAMPLES);
    if (audio_info == nullptr) {
        return nullptr;
    }

    HJMediaFrame::setCodecIdAAC(audio_info->m_codecID);
    audio_info->setBitrate(128000);
    //(*audio_info)["aacType"] = i_aac_type;
    return audio_info;
}

HJMediaFrame::Ptr makeAacEncoderTestInputFrame(const HJAudioInfo::Ptr& i_audio_info)
{
    auto frame = HJMediaFrame::makeSilenceAudioFrame(i_audio_info);
    if (frame == nullptr) {
        return nullptr;
    }

    frame->setPTSDTS(0, 0, HJTimeBase{ 1, i_audio_info->m_samplesRate });
    frame->setDuration(i_audio_info->m_sampleCnt, HJTimeBase{ 1, i_audio_info->m_samplesRate });
    return frame;
}

bool isAdtsPacket(const HJMediaFrame::Ptr& i_frame)
{
    if (i_frame == nullptr) {
        return false;
    }

    AVPacket* packet = i_frame->getAVPacket();
    if (packet == nullptr || packet->data == nullptr || packet->size < 2) {
        return false;
    }

    return packet->data[0] == 0xFF && (packet->data[1] & 0xF0) == 0xF0;
}

TEST(HJAudioMixerConfigTest, DefaultsToAacOutputAndRawAacPackaging)
{
    HJAudioMixerConfig config{};

    EXPECT_EQ(config.m_out_type, 0);
    EXPECT_EQ(config.m_aac_type, 0);
}

class HJTestSourcePlugin : public HJPlugin
{
public:
    explicit HJTestSourcePlugin(const std::string& i_name)
        : HJPlugin(i_name) {}

    int pushFrame(const HJMediaFrame::Ptr& i_frame)
    {
        auto frame = i_frame;
        deliverToOutputs(frame);
        return HJ_OK;
    }

protected:
    int runTask(int64_t* o_delay = nullptr) override
    {
        if (o_delay != nullptr) {
            *o_delay = 0;
        }
        return HJ_WOULD_BLOCK;
    }
};

class HJTestSinkPlugin : public HJPlugin
{
public:
    explicit HJTestSinkPlugin(const std::string& i_name)
        : HJPlugin(i_name) {}

    int deliver(size_t i_src_key_hash, HJMediaFrame::Ptr& i_media_frame) override
    {
        (void)i_src_key_hash;
        if (i_media_frame == nullptr) {
            return HJErrInvalidParams;
        }
        m_frames.push_back(i_media_frame->deepDup());
        return HJ_OK;
    }

    const std::vector<HJMediaFrame::Ptr>& getFrames() const
    {
        return m_frames;
    }

protected:
    int runTask(int64_t* o_delay = nullptr) override
    {
        if (o_delay != nullptr) {
            *o_delay = 0;
        }
        return HJ_WOULD_BLOCK;
    }

private:
    std::vector<HJMediaFrame::Ptr> m_frames;
};

class HJTestAudioMixerPlugin : public HJPluginAudioMixer
{
public:
    explicit HJTestAudioMixerPlugin(const std::string& i_name, HJKeyStorage::Ptr i_graph_info = nullptr)
        : HJPluginAudioMixer(i_name, 0, i_graph_info) {}

    int runOnce(int64_t* o_delay = nullptr)
    {
        return HJPluginAudioMixer::runTask(o_delay);
    }

    size_t queuedFrameCount(size_t i_input_key_hash)
    {
        auto input = getInput(i_input_key_hash);
        return input ? input->mediaFrames.size() : 0;
    }

    int64_t previewPts(size_t i_input_key_hash)
    {
        auto frame = preview(i_input_key_hash);
        return frame ? frame->getPTS() : HJ_NOTS_VALUE;
    }

protected:
    void postTask(int64_t i_delay = 0) override
    {
        (void)i_delay;
    }
};

class HJTestFDKAACEncoderPlugin : public HJPluginFDKAACEncoder
{
public:
    explicit HJTestFDKAACEncoderPlugin(const std::string& i_name, HJKeyStorage::Ptr i_graph_info = nullptr)
        : HJPluginFDKAACEncoder(i_name, 0, i_graph_info) {}

    int runOnce(int64_t* o_delay = nullptr)
    {
        return HJPluginFDKAACEncoder::runTask(o_delay);
    }

protected:
    void postTask(int64_t i_delay = 0) override
    {
        (void)i_delay;
    }
};

int encodeSingleFrameWithFdkaacPlugin(int i_aac_type, HJMediaFrame::Ptr& o_output_frame)
{
    auto audio_info = makeAacEncoderTestAudioInfo(i_aac_type);
    if (audio_info == nullptr) {
        return HJErrInvalidParams;
    }

    auto graph = std::make_shared<HJGraph>("HJPluginFDKAACEncoderGraph");
    if (graph == nullptr) {
        return HJErrFatal;
    }

    auto graph_info = std::make_shared<HJKeyStorage>();
    (*graph_info)["graph"] = graph;

    auto encoder = std::make_shared<HJTestFDKAACEncoderPlugin>("HJPluginFDKAACEncoderTest", graph_info);
    auto source = std::make_shared<HJTestSourcePlugin>("HJPluginFDKAACEncoderSource");
    auto sink = std::make_shared<HJTestSinkPlugin>("HJPluginFDKAACEncoderSink");
    if (encoder == nullptr || source == nullptr || sink == nullptr) {
        return HJErrFatal;
    }

    auto init_param = std::make_shared<HJKeyStorage>();
    (*init_param)["createThread"] = false;
    (*init_param)["audioInfo"] = audio_info;

    int ret = encoder->init(init_param);
    if (ret != HJ_OK) {
        return ret;
    }

    ret = source->addOutputPlugin(encoder, HJMEDIA_TYPE_AUDIO);
    if (ret != HJ_OK) {
        return ret;
    }
    ret = encoder->addInputPlugin(source, HJMEDIA_TYPE_AUDIO);
    if (ret != HJ_OK) {
        return ret;
    }
    ret = encoder->addOutputPlugin(sink, HJMEDIA_TYPE_AUDIO);
    if (ret != HJ_OK) {
        return ret;
    }

    auto input_frame = makeAacEncoderTestInputFrame(audio_info);
    if (input_frame == nullptr) {
        return HJErrInvalidParams;
    }

    ret = source->pushFrame(input_frame);
    if (ret != HJ_OK) {
        return ret;
    }

    int64_t delay_ms = 0;
    ret = encoder->runOnce(&delay_ms);
    if (ret != HJ_OK) {
        return ret;
    }

    const auto& frames = sink->getFrames();
    if (frames.empty()) {
        return HJ_WOULD_BLOCK;
    }

    o_output_frame = frames.back();
    return o_output_frame != nullptr ? HJ_OK : HJErrInvalidData;
}

TEST(HJPluginFDKAACEncoderTest, RawAccessUnitsCarryCodecExtradata)
{
    HJMediaFrame::Ptr output_frame{};
    ASSERT_EQ(encodeSingleFrameWithFdkaacPlugin(0, output_frame), HJ_OK);
    ASSERT_NE(output_frame, nullptr);
    ASSERT_NE(output_frame->getAudioInfo(), nullptr);
    EXPECT_TRUE(HJMediaFrame::isCodecMatchAAC(output_frame->getAudioInfo()->m_codecID));
    EXPECT_NE(output_frame->getAudioInfo()->getCodecParams(), nullptr);
}

TEST(HJPluginFDKAACEncoderTest, AdtsModePrefixesEncodedPacketsWithAdtsHeader)
{
    HJMediaFrame::Ptr output_frame{};
    ASSERT_EQ(encodeSingleFrameWithFdkaacPlugin(2, output_frame), HJ_OK);
    ASSERT_NE(output_frame, nullptr);
    EXPECT_TRUE(isAdtsPacket(output_frame));
}

TEST(HJPluginFDKAACEncoderTest, InvalidAacTypeFallsBackToRawAccessUnits)
{
    HJMediaFrame::Ptr output_frame{};
    ASSERT_EQ(encodeSingleFrameWithFdkaacPlugin(99, output_frame), HJ_OK);
    ASSERT_NE(output_frame, nullptr);
    ASSERT_NE(output_frame->getAudioInfo(), nullptr);
    EXPECT_FALSE(isAdtsPacket(output_frame));
    EXPECT_NE(output_frame->getAudioInfo()->getCodecParams(), nullptr);
}

HJMediaFrame::Ptr makeTestAudioFrame(const HJAudioInfo::Ptr& i_audio_info, int64_t i_pts_ms)
{
    auto frame_info = std::dynamic_pointer_cast<HJAudioInfo>(i_audio_info->dup());
    if (frame_info == nullptr) {
        return nullptr;
    }
    frame_info->m_sampleCnt = i_audio_info->m_sampleCnt;
    frame_info->m_samplesPerFrame = i_audio_info->m_samplesPerFrame;

    auto frame = HJMediaFrame::makeSilenceAudioFrame(frame_info);
    if (frame == nullptr) {
        return nullptr;
    }

    frame->setPTSDTS(i_pts_ms, i_pts_ms);
    frame->setDuration(samplesToPtsMs(frame_info->m_sampleCnt, frame_info->m_samplesRate), HJ_TIMEBASE_MS);
    return frame;
}

int fillPlanarFloatTestAudioFrame(const HJMediaFrame::Ptr& i_frame, float i_value)
{
    if (i_frame == nullptr) {
        return HJErrInvalidParams;
    }

    AVFrame* avf = i_frame->getAVFrame();
    if (avf == nullptr || avf->format != AV_SAMPLE_FMT_FLTP ||
        avf->extended_data == nullptr || avf->nb_samples <= 0 ||
        avf->ch_layout.nb_channels <= 0) {
        return HJErrInvalidParams;
    }

    const int writable_ret = av_frame_make_writable(avf);
    if (writable_ret < HJ_OK) {
        return writable_ret;
    }

    for (int channel = 0; channel < avf->ch_layout.nb_channels; ++channel) {
        float* samples = reinterpret_cast<float*>(avf->extended_data[channel]);
        std::fill(samples, samples + avf->nb_samples, i_value);
    }
    return HJ_OK;
}

int fillPlanarFloatRampTestAudioFrame(const HJMediaFrame::Ptr& i_frame,
                                      float i_start_value,
                                      float i_end_value)
{
    if (i_frame == nullptr) {
        return HJErrInvalidParams;
    }

    AVFrame* avf = i_frame->getAVFrame();
    if (avf == nullptr || avf->format != AV_SAMPLE_FMT_FLTP ||
        avf->extended_data == nullptr || avf->nb_samples <= 0 ||
        avf->ch_layout.nb_channels <= 0) {
        return HJErrInvalidParams;
    }

    const int writable_ret = av_frame_make_writable(avf);
    if (writable_ret < HJ_OK) {
        return writable_ret;
    }

    const float step = avf->nb_samples > 1
                           ? (i_end_value - i_start_value) /
                                 static_cast<float>(avf->nb_samples - 1)
                           : 0.0f;
    for (int channel = 0; channel < avf->ch_layout.nb_channels; ++channel) {
        float* samples = reinterpret_cast<float*>(avf->extended_data[channel]);
        for (int sample_index = 0; sample_index < avf->nb_samples; ++sample_index) {
            samples[sample_index] =
                i_start_value + step * static_cast<float>(sample_index);
        }
    }
    return HJ_OK;
}

HJMediaFrame::Ptr makePlanarFloatTestAudioFrame(const HJAudioInfo::Ptr& i_audio_info,
                                                int64_t i_pts_ms,
                                                float i_value)
{
    auto frame = makeTestAudioFrame(i_audio_info, i_pts_ms);
    if (frame == nullptr) {
        return nullptr;
    }

    if (fillPlanarFloatTestAudioFrame(frame, i_value) != HJ_OK) {
        return nullptr;
    }
    return frame;
}

HJMediaFrame::Ptr makePlanarFloatRampTestAudioFrame(
    const HJAudioInfo::Ptr& i_audio_info, int64_t i_pts_ms,
    float i_start_value, float i_end_value)
{
    auto frame = makeTestAudioFrame(i_audio_info, i_pts_ms);
    if (frame == nullptr) {
        return nullptr;
    }

    if (fillPlanarFloatRampTestAudioFrame(frame, i_start_value, i_end_value) !=
        HJ_OK) {
        return nullptr;
    }
    return frame;
}

float readPlanarFloatSample(const HJMediaFrame::Ptr& i_frame, int i_channel,
                            int i_sample_index)
{
    if (i_frame == nullptr) {
        return 0.0f;
    }

    AVFrame* avf = i_frame->getAVFrame();
    if (avf == nullptr || avf->format != AV_SAMPLE_FMT_FLTP ||
        avf->extended_data == nullptr || i_channel < 0 ||
        i_channel >= avf->ch_layout.nb_channels || i_sample_index < 0 ||
        i_sample_index >= avf->nb_samples) {
        return 0.0f;
    }

    const float* samples =
        reinterpret_cast<const float*>(avf->extended_data[i_channel]);
    return samples[i_sample_index];
}

std::vector<uint8_t> makePlanarS16PcmBuffer(int i_channels, int i_samples,
                                            const std::vector<int16_t>& i_values)
{
    std::vector<uint8_t> pcm;
    if (i_channels <= 0 || i_samples <= 0 ||
        static_cast<size_t>(i_channels) != i_values.size()) {
        return pcm;
    }

    const size_t plane_size = static_cast<size_t>(i_samples) * sizeof(int16_t);
    pcm.resize(plane_size * static_cast<size_t>(i_channels));
    for (int channel = 0; channel < i_channels; ++channel) {
        int16_t* plane = reinterpret_cast<int16_t*>(
            pcm.data() + static_cast<size_t>(channel) * plane_size);
        std::fill(plane, plane + i_samples, i_values[static_cast<size_t>(channel)]);
    }
    return pcm;
}

} // namespace

TEST(HJLiveResourceFetcherTest, ParseReplayPageHtmlReturnsReplayItem)
{
    const std::string html = R"hj(
<div node-type="replayItem"
     data-uid="10001"
     data-liveid="20002"
     data-sn="replay_sn"
     data-m3u8="https://media.example.com/replay/index.m3u8"
     data-nickname="ReplayNick">
    <ul class="replay_info">
        <li title="Replay Title"><span>Title</span></li>
    </ul>
    <img src="https://img.example.com/replay.jpg" />
</div>
)hj";

    const std::vector<HJLiveResourceItem> items =
        HJLiveResourceFetcher::parseItemsFromPageHtml(
            "https://admin.huajiao.com/Resource/Replay/index",
            html);
    ASSERT_EQ(items.size(), 1u);
    EXPECT_EQ(items[0].m_page_type, "replay");
    EXPECT_EQ(items[0].m_content_type, "replay");
    EXPECT_EQ(items[0].m_uid, "10001");
    EXPECT_EQ(items[0].m_liveid, "20002");
    EXPECT_EQ(items[0].m_sn, "replay_sn");
    EXPECT_EQ(items[0].m_m3u8, "https://media.example.com/replay/index.m3u8");
    EXPECT_EQ(items[0].m_stream_status, "html");
}

TEST(HJLiveResourceFetcherTest, ParseLiveResourceHtmlReturnsLiveItem)
{
    const std::string html = R"hj(
<div node-type="replayItem"
     data-uid="30003"
     data-liveid="40004"
     data-sn="live_sn"
     data-flvurl="https://media.example.com/live/stream.flv?token=1"
     data-m3u8="https://media.example.com/live/index.m3u8"
     data-nickname="LiveNick">
    <ul class="replay_info">
        <li title="Live Title"><span>Title</span></li>
    </ul>
    <img src="https://img.example.com/live.jpg" />
</div>
)hj";

    const std::vector<HJLiveResourceItem> items =
        HJLiveResourceFetcher::parseItemsFromPageHtml(
            "https://admin.huajiao.com/Resource/LiveResource/index",
            html);
    ASSERT_EQ(items.size(), 1u);
    EXPECT_EQ(items[0].m_page_type, "live_resource");
    EXPECT_EQ(items[0].m_content_type, "live");
    EXPECT_EQ(items[0].m_flvurl,
              "https://media.example.com/live/stream.flv?token=1");
    EXPECT_EQ(items[0].m_m3u8, "https://media.example.com/live/index.m3u8");
    EXPECT_EQ(items[0].m_stream_status, "html");
}

TEST(HJLiveResourceFetcherTest, ChoosePlayableUrlUsesPageTypePriority)
{
    HJLiveResourceItem replay_item;
    replay_item.m_page_type = "replay";
    replay_item.m_content_type = "replay";
    replay_item.m_flvurl = "https://media.example.com/replay.flv";
    replay_item.m_m3u8 = "https://media.example.com/replay.m3u8";
    EXPECT_EQ(HJLiveResourceFetcher::choosePlayableUrl(replay_item, true),
              replay_item.m_m3u8);

    HJLiveResourceItem live_item;
    live_item.m_page_type = "live_resource";
    live_item.m_content_type = "live";
    live_item.m_flvurl = "https://media.example.com/live.flv";
    live_item.m_m3u8 = "https://media.example.com/live.m3u8";
    EXPECT_EQ(HJLiveResourceFetcher::choosePlayableUrl(live_item, false),
              live_item.m_flvurl);
}

TEST(HJPluginAudioMixerTest, InitWithConfigReturnsWithoutDeadlock)
{
    HJAudioInfo::Ptr output_info = std::make_shared<HJAudioInfo>();
    output_info->m_samplesRate = HJ_SAMPLE_RATE_DEFAULT;
    output_info->setChannels(HJ_CHANNEL_DEFAULT);
    output_info->m_sampleFmt = HJ_SAMPLE_FORMAT_DEFAULT;

    HJAudioMixerConfig config{};
    config.m_output_info = output_info;
    config.m_max_inputs = 2;

    auto mixer_thread = HJLooperThread::quickStart("HJPluginAudioMixerTestThread");
    ASSERT_NE(mixer_thread, nullptr);

    auto plugin = HJPluginAudioMixer::Create<HJPluginAudioMixer>("HJPluginAudioMixerTest");
    ASSERT_NE(plugin, nullptr);

    auto param = std::make_shared<HJKeyStorage>();
    (*param)["thread"] = mixer_thread;
    (*param)["audioMixerConfig"] = config;

    std::promise<int> init_promise;
    std::future<int> init_future = init_promise.get_future();
    std::thread init_thread([plugin, param, promise = std::move(init_promise)]() mutable {
        try {
            promise.set_value(plugin->init(param));
        }
        catch (...) {
            promise.set_exception(std::current_exception());
        }
    });
    init_thread.detach();

    const auto wait_status = init_future.wait_for(std::chrono::seconds(2));
    EXPECT_EQ(wait_status, std::future_status::ready);
    if (wait_status == std::future_status::ready) {
        const int init_result = init_future.get();
        if (init_result == HJ_OK) {
            EXPECT_EQ(plugin->getStatus(), HJSTATUS_Inited);
            EXPECT_EQ(plugin->done(), HJ_OK);
        }
    }

    mixer_thread->done();
}

TEST(HJPluginAudioMixerTest, RunTaskUsesSampleTimelineWithoutAccumulatedMsDrift)
{
    constexpr int kSampleRate = 48000;
    constexpr int kFrameSamples = 1024;
    constexpr int kFrameCount = 5;

    HJAudioInfo::Ptr output_info = std::make_shared<HJAudioInfo>();
    output_info->m_samplesRate = kSampleRate;
    output_info->setChannels(HJ_CHANNEL_DEFAULT);
    output_info->m_sampleFmt = HJ_SAMPLE_FORMAT_DEFAULT;
    output_info->m_sampleCnt = kFrameSamples;
    output_info->m_samplesPerFrame = kFrameSamples;

    HJAudioMixerConfig config{};
    config.m_output_info = output_info;
    config.m_max_inputs = 1;
    config.m_frame_samples = kFrameSamples;
    config.m_sync_window_ms = 0;
    config.m_enable_compand = false;
    config.m_enable_limiter = false;

    auto mixer = std::make_shared<HJTestAudioMixerPlugin>("HJPluginAudioMixerPreciseTimelineTest");
    ASSERT_NE(mixer, nullptr);

    auto init_param = std::make_shared<HJKeyStorage>();
    (*init_param)["createThread"] = false;
    (*init_param)["audioMixerConfig"] = config;
    ASSERT_EQ(mixer->init(init_param), HJ_OK);

    auto source = std::make_shared<HJTestSourcePlugin>("HJPluginAudioMixerSourceTest");
    auto sink = std::make_shared<HJTestSinkPlugin>("HJPluginAudioMixerSinkTest");
    ASSERT_NE(source, nullptr);
    ASSERT_NE(sink, nullptr);

    ASSERT_EQ(source->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addOutputPlugin(sink, HJMEDIA_TYPE_AUDIO), HJ_OK);

    HJAudioMixerInputDesc input_desc{};
    input_desc.m_input_id = "input0";
    input_desc.m_input_info = std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    ASSERT_NE(input_desc.m_input_info, nullptr);

    const size_t input_key_hash = buildPluginKeyHash(source->getName(), HJMEDIA_TYPE_AUDIO, 0);
    ASSERT_EQ(mixer->bindInput(0, input_key_hash, input_desc), HJ_OK);

    std::vector<int64_t> expected_pts_ms;
    expected_pts_ms.reserve(kFrameCount);
    for (int i = 0; i < kFrameCount; ++i) {
        const int64_t pts_ms = samplesToPtsMs(static_cast<int64_t>(i) * kFrameSamples, kSampleRate);
        expected_pts_ms.push_back(pts_ms);

        auto frame = makeTestAudioFrame(output_info, pts_ms);
        ASSERT_NE(frame, nullptr);
        ASSERT_EQ(source->pushFrame(frame), HJ_OK);
    }

    int64_t delay_ms = 0;
    for (int i = 0; i < kFrameCount; ++i) {
        ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
        EXPECT_GE(delay_ms, 21);
        EXPECT_LE(delay_ms, 22);
    }
    EXPECT_EQ(mixer->runOnce(&delay_ms), HJ_WOULD_BLOCK);

    const auto& out_frames = sink->getFrames();
    ASSERT_EQ(out_frames.size(), expected_pts_ms.size());
    for (size_t i = 0; i < expected_pts_ms.size(); ++i) {
        ASSERT_NE(out_frames[i], nullptr);
        EXPECT_EQ(out_frames[i]->getPTS(), expected_pts_ms[i]);
    }

    EXPECT_EQ(mixer->done(), HJ_OK);
}

TEST(HJPluginAudioMixerTest, StartupDoesNotConsumeFirstFrameOutsideSyncWindow)
{
    constexpr int kSampleRate = 48000;
    constexpr int kFrameSamples = 1024;
    constexpr int64_t kLateInputPtsMs = 100;

    HJAudioInfo::Ptr output_info = std::make_shared<HJAudioInfo>();
    output_info->m_samplesRate = kSampleRate;
    output_info->setChannels(HJ_CHANNEL_DEFAULT);
    output_info->m_sampleFmt = HJ_SAMPLE_FORMAT_DEFAULT;
    output_info->m_sampleCnt = kFrameSamples;
    output_info->m_samplesPerFrame = kFrameSamples;

    HJAudioMixerConfig config{};
    config.m_output_info = output_info;
    config.m_max_inputs = 2;
    config.m_frame_samples = kFrameSamples;
    config.m_sync_window_ms = 10;
    config.m_late_threshold_ms = 60;
    config.m_enable_compand = false;
    config.m_enable_limiter = false;

    auto mixer = std::make_shared<HJTestAudioMixerPlugin>("HJPluginAudioMixerStartupWindowTest");
    auto source0 = std::make_shared<HJTestSourcePlugin>("HJPluginAudioMixerStartupWindowSource0");
    auto source1 = std::make_shared<HJTestSourcePlugin>("HJPluginAudioMixerStartupWindowSource1");
    auto sink = std::make_shared<HJTestSinkPlugin>("HJPluginAudioMixerStartupWindowSink");
    ASSERT_NE(mixer, nullptr);
    ASSERT_NE(source0, nullptr);
    ASSERT_NE(source1, nullptr);
    ASSERT_NE(sink, nullptr);

    auto init_param = std::make_shared<HJKeyStorage>();
    (*init_param)["createThread"] = false;
    (*init_param)["audioMixerConfig"] = config;
    ASSERT_EQ(mixer->init(init_param), HJ_OK);

    ASSERT_EQ(source0->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(source1->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source0, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source1, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addOutputPlugin(sink, HJMEDIA_TYPE_AUDIO), HJ_OK);

    HJAudioMixerInputDesc input_desc0{};
    input_desc0.m_input_id = "input0";
    input_desc0.m_input_info = std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    HJAudioMixerInputDesc input_desc1{};
    input_desc1.m_input_id = "input1";
    input_desc1.m_input_info = std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    ASSERT_NE(input_desc0.m_input_info, nullptr);
    ASSERT_NE(input_desc1.m_input_info, nullptr);

    const size_t input0_key_hash = buildPluginKeyHash(source0->getName(), HJMEDIA_TYPE_AUDIO, 0);
    const size_t input1_key_hash = buildPluginKeyHash(source1->getName(), HJMEDIA_TYPE_AUDIO, 0);
    ASSERT_EQ(mixer->bindInput(0, input0_key_hash, input_desc0), HJ_OK);
    ASSERT_EQ(mixer->bindInput(1, input1_key_hash, input_desc1), HJ_OK);

    auto frame0 = makeTestAudioFrame(output_info, 0);
    auto frame1 = makeTestAudioFrame(output_info, kLateInputPtsMs);
    ASSERT_NE(frame0, nullptr);
    ASSERT_NE(frame1, nullptr);
    ASSERT_EQ(source0->pushFrame(frame0), HJ_OK);
    ASSERT_EQ(source1->pushFrame(frame1), HJ_OK);

    int64_t delay_ms = 0;
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    const auto& out_frames = sink->getFrames();
    ASSERT_EQ(out_frames.size(), 1u);
    ASSERT_NE(out_frames[0], nullptr);
    EXPECT_EQ(out_frames[0]->getPTS(), 0);
    EXPECT_EQ(mixer->queuedFrameCount(input0_key_hash), 0u);
    EXPECT_EQ(mixer->queuedFrameCount(input1_key_hash), 1u);
    EXPECT_EQ(mixer->previewPts(input1_key_hash), kLateInputPtsMs);

    EXPECT_EQ(mixer->done(), HJ_OK);
}

TEST(HJPluginAudioMixerTest, ZeroVolumeInputDoesNotBlockSteadyStateMix)
{
    constexpr int kSampleRate = 48000;
    constexpr int kFrameSamples = 1024;
    const int64_t kSecondPtsMs = samplesToPtsMs(kFrameSamples, kSampleRate);

    HJAudioInfo::Ptr output_info = std::make_shared<HJAudioInfo>();
    output_info->m_samplesRate = kSampleRate;
    output_info->setChannels(HJ_CHANNEL_DEFAULT);
    output_info->m_sampleFmt = HJ_SAMPLE_FORMAT_DEFAULT;
    output_info->m_sampleCnt = kFrameSamples;
    output_info->m_samplesPerFrame = kFrameSamples;

    HJAudioMixerConfig config{};
    config.m_output_info = output_info;
    config.m_max_inputs = 2;
    config.m_frame_samples = kFrameSamples;
    config.m_sync_window_ms = 0;
    config.m_late_threshold_ms = 200;
    config.m_enable_compand = false;
    config.m_enable_limiter = false;

    auto mixer =
        std::make_shared<HJTestAudioMixerPlugin>("HJPluginAudioMixerZeroVolumeTest");
    auto source0 =
        std::make_shared<HJTestSourcePlugin>("HJPluginAudioMixerZeroVolumeSource0");
    auto source1 =
        std::make_shared<HJTestSourcePlugin>("HJPluginAudioMixerZeroVolumeSource1");
    auto sink =
        std::make_shared<HJTestSinkPlugin>("HJPluginAudioMixerZeroVolumeSink");
    ASSERT_NE(mixer, nullptr);
    ASSERT_NE(source0, nullptr);
    ASSERT_NE(source1, nullptr);
    ASSERT_NE(sink, nullptr);

    auto init_param = std::make_shared<HJKeyStorage>();
    (*init_param)["createThread"] = false;
    (*init_param)["audioMixerConfig"] = config;
    ASSERT_EQ(mixer->init(init_param), HJ_OK);

    ASSERT_EQ(source0->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(source1->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source0, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source1, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addOutputPlugin(sink, HJMEDIA_TYPE_AUDIO), HJ_OK);

    HJAudioMixerInputDesc input_desc0{};
    input_desc0.m_input_id = "input0";
    input_desc0.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    HJAudioMixerInputDesc input_desc1{};
    input_desc1.m_input_id = "input1";
    input_desc1.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    ASSERT_NE(input_desc0.m_input_info, nullptr);
    ASSERT_NE(input_desc1.m_input_info, nullptr);

    const size_t input0_key_hash =
        buildPluginKeyHash(source0->getName(), HJMEDIA_TYPE_AUDIO, 0);
    const size_t input1_key_hash =
        buildPluginKeyHash(source1->getName(), HJMEDIA_TYPE_AUDIO, 0);
    ASSERT_EQ(mixer->bindInput(0, input0_key_hash, input_desc0), HJ_OK);
    ASSERT_EQ(mixer->bindInput(1, input1_key_hash, input_desc1), HJ_OK);

    auto first_frame0 = makeTestAudioFrame(output_info, 0);
    auto first_frame1 = makeTestAudioFrame(output_info, 0);
    ASSERT_NE(first_frame0, nullptr);
    ASSERT_NE(first_frame1, nullptr);
    ASSERT_EQ(source0->pushFrame(first_frame0), HJ_OK);
    ASSERT_EQ(source1->pushFrame(first_frame1), HJ_OK);

    int64_t delay_ms = 0;
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(sink->getFrames().size(), 1u);

    ASSERT_EQ(mixer->setInputVolume(1, 0.0f), HJ_OK);

    auto second_frame0 = makeTestAudioFrame(output_info, kSecondPtsMs);
    ASSERT_NE(second_frame0, nullptr);
    ASSERT_EQ(source0->pushFrame(second_frame0), HJ_OK);

    EXPECT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    const auto& out_frames = sink->getFrames();
    ASSERT_EQ(out_frames.size(), 2u);
    ASSERT_NE(out_frames[1], nullptr);
    EXPECT_EQ(out_frames[1]->getPTS(), kSecondPtsMs);
    EXPECT_EQ(mixer->queuedFrameCount(input0_key_hash), 0u);
    EXPECT_EQ(mixer->queuedFrameCount(input1_key_hash), 0u);
    EXPECT_EQ(mixer->previewPts(input1_key_hash), HJ_NOTS_VALUE);

    EXPECT_EQ(mixer->done(), HJ_OK);
}

TEST(HJPluginAudioMixerTest, StarvationGapUsesFadeOutAndRecoveryUsesFadeIn)
{
    constexpr int kSampleRate = 48000;
    constexpr int kFrameSamples = 480;
    constexpr int64_t kRecoveryPtsMs = 20;
    constexpr float kFrameStartAmplitude = 0.1f;
    constexpr float kFrameEndAmplitude = 1.0f;

    HJAudioInfo::Ptr output_info = std::make_shared<HJAudioInfo>();
    output_info->m_samplesRate = kSampleRate;
    output_info->setChannels(HJ_CHANNEL_DEFAULT);
    output_info->m_sampleFmt = AV_SAMPLE_FMT_FLTP;
    output_info->m_sampleCnt = kFrameSamples;
    output_info->m_samplesPerFrame = kFrameSamples;

    HJAudioMixerConfig config{};
    config.m_output_info = output_info;
    config.m_max_inputs = 1;
    config.m_frame_samples = kFrameSamples;
    config.m_sync_window_ms = 0;
    config.m_late_threshold_ms = 0;
    config.m_enable_compand = false;
    config.m_enable_limiter = false;

    auto mixer = std::make_shared<HJTestAudioMixerPlugin>("HJPluginAudioMixerDeclickTest");
    auto source = std::make_shared<HJTestSourcePlugin>("HJPluginAudioMixerDeclickSource");
    auto sink = std::make_shared<HJTestSinkPlugin>("HJPluginAudioMixerDeclickSink");
    ASSERT_NE(mixer, nullptr);
    ASSERT_NE(source, nullptr);
    ASSERT_NE(sink, nullptr);

    auto init_param = std::make_shared<HJKeyStorage>();
    (*init_param)["createThread"] = false;
    (*init_param)["audioMixerConfig"] = config;
    ASSERT_EQ(mixer->init(init_param), HJ_OK);

    ASSERT_EQ(source->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addOutputPlugin(sink, HJMEDIA_TYPE_AUDIO), HJ_OK);

    HJAudioMixerInputDesc input_desc{};
    input_desc.m_input_id = "input0";
    input_desc.m_input_info = std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    ASSERT_NE(input_desc.m_input_info, nullptr);

    const size_t input_key_hash = buildPluginKeyHash(source->getName(), HJMEDIA_TYPE_AUDIO, 0);
    ASSERT_EQ(mixer->bindInput(0, input_key_hash, input_desc), HJ_OK);

    auto startup_frame = makePlanarFloatRampTestAudioFrame(
        output_info, 0, kFrameStartAmplitude, kFrameEndAmplitude);
    auto recovery_frame = makePlanarFloatTestAudioFrame(
        output_info, kRecoveryPtsMs, kFrameEndAmplitude);
    ASSERT_NE(startup_frame, nullptr);
    ASSERT_NE(recovery_frame, nullptr);
    ASSERT_EQ(source->pushFrame(startup_frame), HJ_OK);

    int64_t delay_ms = 0;
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    ASSERT_EQ(source->pushFrame(recovery_frame), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    const auto& out_frames = sink->getFrames();
    ASSERT_EQ(out_frames.size(), 3u);
    ASSERT_NE(out_frames[0], nullptr);
    ASSERT_NE(out_frames[1], nullptr);
    ASSERT_NE(out_frames[2], nullptr);

    const float steady_first = readPlanarFloatSample(out_frames[0], 0, 0);
    const float steady_last = readPlanarFloatSample(out_frames[0], 0, kFrameSamples - 1);
    const float gap_first = readPlanarFloatSample(out_frames[1], 0, 0);
    const float gap_mid = readPlanarFloatSample(out_frames[1], 0, 120);
    const float gap_tail = readPlanarFloatSample(out_frames[1], 0, kFrameSamples - 1);
    const float recovery_first = readPlanarFloatSample(out_frames[2], 0, 0);
    const float recovery_mid = readPlanarFloatSample(out_frames[2], 0, 120);
    const float recovery_tail = readPlanarFloatSample(out_frames[2], 0, kFrameSamples - 1);

    EXPECT_NEAR(steady_first, kFrameStartAmplitude, 0.02f);
    EXPECT_NEAR(steady_last, kFrameEndAmplitude, 0.02f);
    EXPECT_NEAR(gap_first, steady_last, 0.02f);
    EXPECT_GT(std::fabs(gap_first - steady_first), 0.5f);
    EXPECT_GT(gap_first, gap_mid);
    EXPECT_GT(gap_mid, gap_tail);
    EXPECT_GT(gap_tail, 0.35f);
    EXPECT_LT(gap_tail, 0.65f);
    EXPECT_NEAR(recovery_first, gap_tail, 0.02f);
    EXPECT_GT(recovery_mid, recovery_first);
    EXPECT_GT(recovery_tail, 0.95f);

    EXPECT_EQ(mixer->done(), HJ_OK);
}

TEST(HJPluginAudioMixerTest, StarvationGapStopsConcealmentAfterBudgetExhausted)
{
    constexpr int kSampleRate = 48000;
    constexpr int kFrameSamples = 480;
    constexpr int64_t kFutureRecoveryPtsMs = 100;
    constexpr float kAmplitude = 1.0f;

    HJAudioInfo::Ptr output_info = std::make_shared<HJAudioInfo>();
    output_info->m_samplesRate = kSampleRate;
    output_info->setChannels(HJ_CHANNEL_DEFAULT);
    output_info->m_sampleFmt = AV_SAMPLE_FMT_FLTP;
    output_info->m_sampleCnt = kFrameSamples;
    output_info->m_samplesPerFrame = kFrameSamples;

    HJAudioMixerConfig config{};
    config.m_output_info = output_info;
    config.m_max_inputs = 1;
    config.m_frame_samples = kFrameSamples;
    config.m_sync_window_ms = 0;
    config.m_late_threshold_ms = 0;
    config.m_enable_compand = false;
    config.m_enable_limiter = false;

    auto mixer = std::make_shared<HJTestAudioMixerPlugin>("HJPluginAudioMixerGapBudgetTest");
    auto source = std::make_shared<HJTestSourcePlugin>("HJPluginAudioMixerGapBudgetSource");
    auto sink = std::make_shared<HJTestSinkPlugin>("HJPluginAudioMixerGapBudgetSink");
    ASSERT_NE(mixer, nullptr);
    ASSERT_NE(source, nullptr);
    ASSERT_NE(sink, nullptr);

    auto init_param = std::make_shared<HJKeyStorage>();
    (*init_param)["createThread"] = false;
    (*init_param)["audioMixerConfig"] = config;
    ASSERT_EQ(mixer->init(init_param), HJ_OK);

    ASSERT_EQ(source->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addOutputPlugin(sink, HJMEDIA_TYPE_AUDIO), HJ_OK);

    HJAudioMixerInputDesc input_desc{};
    input_desc.m_input_id = "input0";
    input_desc.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    ASSERT_NE(input_desc.m_input_info, nullptr);

    const size_t input_key_hash =
        buildPluginKeyHash(source->getName(), HJMEDIA_TYPE_AUDIO, 0);
    ASSERT_EQ(mixer->bindInput(0, input_key_hash, input_desc), HJ_OK);

    auto startup_frame = makePlanarFloatTestAudioFrame(output_info, 0, kAmplitude);
    auto future_frame =
        makePlanarFloatTestAudioFrame(output_info, kFutureRecoveryPtsMs, kAmplitude);
    ASSERT_NE(startup_frame, nullptr);
    ASSERT_NE(future_frame, nullptr);
    ASSERT_EQ(source->pushFrame(startup_frame), HJ_OK);
    ASSERT_EQ(source->pushFrame(future_frame), HJ_OK);

    int64_t delay_ms = 0;
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    const auto& out_frames = sink->getFrames();
    ASSERT_EQ(out_frames.size(), 5u);

    const float first_gap_first = readPlanarFloatSample(out_frames[1], 0, 0);
    const float second_gap_first = readPlanarFloatSample(out_frames[2], 0, 0);
    const float post_budget_first = readPlanarFloatSample(out_frames[4], 0, 0);
    const float post_budget_mid =
        readPlanarFloatSample(out_frames[4], 0, kFrameSamples / 2);
    const float post_budget_tail =
        readPlanarFloatSample(out_frames[4], 0, kFrameSamples - 1);

    EXPECT_GT(first_gap_first, 0.9f);
    EXPECT_GT(second_gap_first, 0.45f);
    EXPECT_LT(std::fabs(post_budget_first), 0.02f);
    EXPECT_LT(std::fabs(post_budget_mid), 0.02f);
    EXPECT_LT(std::fabs(post_budget_tail), 0.02f);

    EXPECT_EQ(mixer->done(), HJ_OK);
}

TEST(HJPluginAudioMixerTest, RunTaskDropsAllLateFramesBeforeConsumingCurrentFrame)
{
    constexpr int kSampleRate = 48000;
    constexpr int kFrameSamples = 1024;
    constexpr int64_t kCurrentPtsMs = 21;

    HJAudioInfo::Ptr output_info = std::make_shared<HJAudioInfo>();
    output_info->m_samplesRate = kSampleRate;
    output_info->setChannels(HJ_CHANNEL_DEFAULT);
    output_info->m_sampleFmt = HJ_SAMPLE_FORMAT_DEFAULT;
    output_info->m_sampleCnt = kFrameSamples;
    output_info->m_samplesPerFrame = kFrameSamples;

    HJAudioMixerConfig config{};
    config.m_output_info = output_info;
    config.m_max_inputs = 1;
    config.m_frame_samples = kFrameSamples;
    config.m_sync_window_ms = 0;
    config.m_late_threshold_ms = 0;
    config.m_enable_compand = false;
    config.m_enable_limiter = false;

    auto mixer = std::make_shared<HJTestAudioMixerPlugin>("HJPluginAudioMixerLateDropTest");
    auto source = std::make_shared<HJTestSourcePlugin>("HJPluginAudioMixerLateDropSource");
    auto sink = std::make_shared<HJTestSinkPlugin>("HJPluginAudioMixerLateDropSink");
    ASSERT_NE(mixer, nullptr);
    ASSERT_NE(source, nullptr);
    ASSERT_NE(sink, nullptr);

    auto init_param = std::make_shared<HJKeyStorage>();
    (*init_param)["createThread"] = false;
    (*init_param)["audioMixerConfig"] = config;
    ASSERT_EQ(mixer->init(init_param), HJ_OK);

    ASSERT_EQ(source->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addOutputPlugin(sink, HJMEDIA_TYPE_AUDIO), HJ_OK);

    HJAudioMixerInputDesc input_desc{};
    input_desc.m_input_id = "input0";
    input_desc.m_input_info = std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    ASSERT_NE(input_desc.m_input_info, nullptr);

    const size_t input_key_hash = buildPluginKeyHash(source->getName(), HJMEDIA_TYPE_AUDIO, 0);
    ASSERT_EQ(mixer->bindInput(0, input_key_hash, input_desc), HJ_OK);

    auto startup_frame = makeTestAudioFrame(output_info, 0);
    ASSERT_NE(startup_frame, nullptr);
    ASSERT_EQ(source->pushFrame(startup_frame), HJ_OK);

    int64_t delay_ms = 0;
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    const int64_t stale_pts_ms[] = { 0, 1, 2 };
    for (const int64_t stale_pts : stale_pts_ms) {
        auto stale_frame = makeTestAudioFrame(output_info, stale_pts);
        ASSERT_NE(stale_frame, nullptr);
        ASSERT_EQ(source->pushFrame(stale_frame), HJ_OK);
    }
    auto current_frame = makeTestAudioFrame(output_info, kCurrentPtsMs);
    ASSERT_NE(current_frame, nullptr);
    ASSERT_EQ(source->pushFrame(current_frame), HJ_OK);

    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    const auto& out_frames = sink->getFrames();
    ASSERT_EQ(out_frames.size(), 2u);
    ASSERT_NE(out_frames[1], nullptr);
    EXPECT_EQ(out_frames[1]->getPTS(), kCurrentPtsMs);
    EXPECT_EQ(mixer->queuedFrameCount(input_key_hash), 0u);
    EXPECT_EQ(mixer->previewPts(input_key_hash), HJ_NOTS_VALUE);

    EXPECT_EQ(mixer->done(), HJ_OK);
}

TEST(HJPluginAudioMixerTest, SyncWindowControlsPastDropWindowInsteadOfLateThreshold)
{
    constexpr int kSampleRate = 48000;
    constexpr int kFrameSamples = 1024;
    constexpr int64_t kCurrentPtsMs = 21;

    HJAudioInfo::Ptr output_info = std::make_shared<HJAudioInfo>();
    output_info->m_samplesRate = kSampleRate;
    output_info->setChannels(HJ_CHANNEL_DEFAULT);
    output_info->m_sampleFmt = HJ_SAMPLE_FORMAT_DEFAULT;
    output_info->m_sampleCnt = kFrameSamples;
    output_info->m_samplesPerFrame = kFrameSamples;

    HJAudioMixerConfig config{};
    config.m_output_info = output_info;
    config.m_max_inputs = 1;
    config.m_frame_samples = kFrameSamples;
    config.m_sync_window_ms = 20;
    config.m_late_threshold_ms = 200;
    config.m_enable_compand = false;
    config.m_enable_limiter = false;

    auto mixer = std::make_shared<HJTestAudioMixerPlugin>("HJPluginAudioMixerThresholdSplitTest");
    auto source = std::make_shared<HJTestSourcePlugin>("HJPluginAudioMixerThresholdSplitSource");
    auto sink = std::make_shared<HJTestSinkPlugin>("HJPluginAudioMixerThresholdSplitSink");
    ASSERT_NE(mixer, nullptr);
    ASSERT_NE(source, nullptr);
    ASSERT_NE(sink, nullptr);

    auto init_param = std::make_shared<HJKeyStorage>();
    (*init_param)["createThread"] = false;
    (*init_param)["audioMixerConfig"] = config;
    ASSERT_EQ(mixer->init(init_param), HJ_OK);

    ASSERT_EQ(source->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addOutputPlugin(sink, HJMEDIA_TYPE_AUDIO), HJ_OK);

    HJAudioMixerInputDesc input_desc{};
    input_desc.m_input_id = "input0";
    input_desc.m_input_info = std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    ASSERT_NE(input_desc.m_input_info, nullptr);

    const size_t input_key_hash = buildPluginKeyHash(source->getName(), HJMEDIA_TYPE_AUDIO, 0);
    ASSERT_EQ(mixer->bindInput(0, input_key_hash, input_desc), HJ_OK);

    auto startup_frame = makeTestAudioFrame(output_info, 0);
    ASSERT_NE(startup_frame, nullptr);
    ASSERT_EQ(source->pushFrame(startup_frame), HJ_OK);

    int64_t delay_ms = 0;
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    auto stale_frame = makeTestAudioFrame(output_info, 0);
    auto current_frame = makeTestAudioFrame(output_info, kCurrentPtsMs);
    ASSERT_NE(stale_frame, nullptr);
    ASSERT_NE(current_frame, nullptr);
    ASSERT_EQ(source->pushFrame(stale_frame), HJ_OK);
    ASSERT_EQ(source->pushFrame(current_frame), HJ_OK);

    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    const auto& out_frames = sink->getFrames();
    ASSERT_EQ(out_frames.size(), 2u);
    ASSERT_NE(out_frames[1], nullptr);
    EXPECT_EQ(out_frames[1]->getPTS(), kCurrentPtsMs);
    EXPECT_EQ(mixer->queuedFrameCount(input_key_hash), 0u);
    EXPECT_EQ(mixer->previewPts(input_key_hash), HJ_NOTS_VALUE);

    EXPECT_EQ(mixer->done(), HJ_OK);
}

TEST(HJPluginAudioMixerTest, StartupTimeoutUsesSilenceForMissingInput)
{
    constexpr int kSampleRate = 48000;
    constexpr int kFrameSamples = 1024;
    constexpr int kLateThresholdMs = 60;

    HJAudioInfo::Ptr output_info = std::make_shared<HJAudioInfo>();
    output_info->m_samplesRate = kSampleRate;
    output_info->setChannels(HJ_CHANNEL_DEFAULT);
    output_info->m_sampleFmt = HJ_SAMPLE_FORMAT_DEFAULT;
    output_info->m_sampleCnt = kFrameSamples;
    output_info->m_samplesPerFrame = kFrameSamples;

    HJAudioMixerConfig config{};
    config.m_output_info = output_info;
    config.m_max_inputs = 2;
    config.m_frame_samples = kFrameSamples;
    config.m_sync_window_ms = 20;
    config.m_late_threshold_ms = kLateThresholdMs;
    config.m_enable_compand = false;
    config.m_enable_limiter = false;

    auto mixer = std::make_shared<HJTestAudioMixerPlugin>("HJPluginAudioMixerStartupThresholdTest");
    auto source0 = std::make_shared<HJTestSourcePlugin>("HJPluginAudioMixerStartupThresholdSource0");
    auto source1 = std::make_shared<HJTestSourcePlugin>("HJPluginAudioMixerStartupThresholdSource1");
    auto sink = std::make_shared<HJTestSinkPlugin>("HJPluginAudioMixerStartupThresholdSink");
    ASSERT_NE(mixer, nullptr);
    ASSERT_NE(source0, nullptr);
    ASSERT_NE(source1, nullptr);
    ASSERT_NE(sink, nullptr);

    auto init_param = std::make_shared<HJKeyStorage>();
    (*init_param)["createThread"] = false;
    (*init_param)["audioMixerConfig"] = config;
    ASSERT_EQ(mixer->init(init_param), HJ_OK);

    ASSERT_EQ(source0->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(source1->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source0, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source1, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addOutputPlugin(sink, HJMEDIA_TYPE_AUDIO), HJ_OK);

    HJAudioMixerInputDesc input_desc0{};
    input_desc0.m_input_id = "input0";
    input_desc0.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    HJAudioMixerInputDesc input_desc1{};
    input_desc1.m_input_id = "input1";
    input_desc1.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    ASSERT_NE(input_desc0.m_input_info, nullptr);
    ASSERT_NE(input_desc1.m_input_info, nullptr);

    const size_t input0_key_hash =
        buildPluginKeyHash(source0->getName(), HJMEDIA_TYPE_AUDIO, 0);
    const size_t input1_key_hash =
        buildPluginKeyHash(source1->getName(), HJMEDIA_TYPE_AUDIO, 0);
    ASSERT_EQ(mixer->bindInput(0, input0_key_hash, input_desc0), HJ_OK);
    ASSERT_EQ(mixer->bindInput(1, input1_key_hash, input_desc1), HJ_OK);

    auto first_frame = makeTestAudioFrame(output_info, 0);
    auto second_frame = makeTestAudioFrame(output_info, samplesToPtsMs(kFrameSamples, kSampleRate));
    auto third_frame = makeTestAudioFrame(output_info, samplesToPtsMs(kFrameSamples * 2, kSampleRate));
    ASSERT_NE(first_frame, nullptr);
    ASSERT_NE(second_frame, nullptr);
    ASSERT_NE(third_frame, nullptr);

    ASSERT_EQ(source0->pushFrame(first_frame), HJ_OK);

    int64_t delay_ms = 0;
    EXPECT_EQ(mixer->runOnce(&delay_ms), HJ_WOULD_BLOCK);
    EXPECT_TRUE(sink->getFrames().empty());

    ASSERT_EQ(source0->pushFrame(second_frame), HJ_OK);
    ASSERT_EQ(source0->pushFrame(third_frame), HJ_OK);

    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    const auto& out_frames = sink->getFrames();
    ASSERT_EQ(out_frames.size(), 1u);
    ASSERT_NE(out_frames[0], nullptr);
    EXPECT_EQ(out_frames[0]->getPTS(), 0);
    EXPECT_EQ(mixer->queuedFrameCount(input0_key_hash), 2u);
    EXPECT_EQ(mixer->queuedFrameCount(input1_key_hash), 0u);

    EXPECT_EQ(mixer->done(), HJ_OK);
}

TEST(HJPluginAudioMixerTest, RebindInputWaitsForJoinInsteadOfAdvancingWithSilence)
{
    constexpr int kSampleRate = 48000;
    constexpr int kFrameSamples = 1024;
    const int64_t kSecondPtsMs = samplesToPtsMs(kFrameSamples, kSampleRate);
    const int64_t kThirdPtsMs = samplesToPtsMs(kFrameSamples * 2, kSampleRate);

    HJAudioInfo::Ptr output_info = std::make_shared<HJAudioInfo>();
    output_info->m_samplesRate = kSampleRate;
    output_info->setChannels(HJ_CHANNEL_DEFAULT);
    output_info->m_sampleFmt = HJ_SAMPLE_FORMAT_DEFAULT;
    output_info->m_sampleCnt = kFrameSamples;
    output_info->m_samplesPerFrame = kFrameSamples;

    HJAudioMixerConfig config{};
    config.m_output_info = output_info;
    config.m_max_inputs = 2;
    config.m_frame_samples = kFrameSamples;
    config.m_sync_window_ms = 1;
    config.m_late_threshold_ms = 200;
    config.m_enable_compand = false;
    config.m_enable_limiter = false;

    auto mixer =
        std::make_shared<HJTestAudioMixerPlugin>("HJPluginAudioMixerRebindJoinTest");
    auto source0 =
        std::make_shared<HJTestSourcePlugin>("HJPluginAudioMixerRebindJoinSource0");
    auto source1 =
        std::make_shared<HJTestSourcePlugin>("HJPluginAudioMixerRebindJoinSource1");
    auto sink =
        std::make_shared<HJTestSinkPlugin>("HJPluginAudioMixerRebindJoinSink");
    ASSERT_NE(mixer, nullptr);
    ASSERT_NE(source0, nullptr);
    ASSERT_NE(source1, nullptr);
    ASSERT_NE(sink, nullptr);

    auto init_param = std::make_shared<HJKeyStorage>();
    (*init_param)["createThread"] = false;
    (*init_param)["audioMixerConfig"] = config;
    ASSERT_EQ(mixer->init(init_param), HJ_OK);

    ASSERT_EQ(source0->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(source1->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source0, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source1, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addOutputPlugin(sink, HJMEDIA_TYPE_AUDIO), HJ_OK);

    HJAudioMixerInputDesc input_desc0{};
    input_desc0.m_input_id = "input0";
    input_desc0.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    HJAudioMixerInputDesc input_desc1{};
    input_desc1.m_input_id = "input1";
    input_desc1.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    ASSERT_NE(input_desc0.m_input_info, nullptr);
    ASSERT_NE(input_desc1.m_input_info, nullptr);

    const size_t input0_key_hash =
        buildPluginKeyHash(source0->getName(), HJMEDIA_TYPE_AUDIO, 0);
    const size_t input1_key_hash =
        buildPluginKeyHash(source1->getName(), HJMEDIA_TYPE_AUDIO, 0);
    ASSERT_EQ(mixer->bindInput(0, input0_key_hash, input_desc0), HJ_OK);
    ASSERT_EQ(mixer->bindInput(1, input1_key_hash, input_desc1), HJ_OK);

    auto first_frame0 = makeTestAudioFrame(output_info, 0);
    auto first_frame1 = makeTestAudioFrame(output_info, 0);
    auto second_frame0 = makeTestAudioFrame(output_info, kSecondPtsMs);
    auto third_frame0 = makeTestAudioFrame(output_info, kThirdPtsMs);
    auto third_frame1 = makeTestAudioFrame(output_info, kThirdPtsMs);
    ASSERT_NE(first_frame0, nullptr);
    ASSERT_NE(first_frame1, nullptr);
    ASSERT_NE(second_frame0, nullptr);
    ASSERT_NE(third_frame0, nullptr);
    ASSERT_NE(third_frame1, nullptr);

    ASSERT_EQ(source0->pushFrame(first_frame0), HJ_OK);
    ASSERT_EQ(source1->pushFrame(first_frame1), HJ_OK);

    int64_t delay_ms = 0;
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(sink->getFrames().size(), 1u);

    ASSERT_EQ(mixer->unbindInput(1), HJ_OK);

    ASSERT_EQ(source0->pushFrame(second_frame0), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(sink->getFrames().size(), 2u);
    ASSERT_EQ(sink->getFrames()[1]->getPTS(), kSecondPtsMs);

    ASSERT_EQ(mixer->bindInput(1, input1_key_hash, input_desc1), HJ_OK);
    ASSERT_EQ(source1->pushFrame(third_frame1), HJ_OK);

    EXPECT_EQ(mixer->runOnce(&delay_ms), HJ_WOULD_BLOCK);
    EXPECT_EQ(sink->getFrames().size(), 2u);
    EXPECT_EQ(mixer->queuedFrameCount(input1_key_hash), 1u);
    EXPECT_EQ(mixer->previewPts(input1_key_hash), kThirdPtsMs);

    ASSERT_EQ(source0->pushFrame(third_frame0), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    const auto& out_frames = sink->getFrames();
    ASSERT_EQ(out_frames.size(), 3u);
    ASSERT_NE(out_frames[2], nullptr);
    EXPECT_EQ(out_frames[2]->getPTS(), kThirdPtsMs);
    EXPECT_EQ(mixer->queuedFrameCount(input0_key_hash), 0u);
    EXPECT_EQ(mixer->queuedFrameCount(input1_key_hash), 0u);
    EXPECT_EQ(mixer->previewPts(input0_key_hash), HJ_NOTS_VALUE);
    EXPECT_EQ(mixer->previewPts(input1_key_hash), HJ_NOTS_VALUE);

    EXPECT_EQ(mixer->done(), HJ_OK);
}

TEST(HJPluginAudioMixerTest, RebindInputJoinTimeoutFallsBackToSilence)
{
    constexpr int kSampleRate = 48000;
    constexpr int kFrameSamples = 1024;
    constexpr int kLateThresholdMs = 60;
    const int64_t kSecondPtsMs = samplesToPtsMs(kFrameSamples, kSampleRate);
    const int64_t kThirdPtsMs = samplesToPtsMs(kFrameSamples * 2, kSampleRate);
    const int64_t kFourthPtsMs = samplesToPtsMs(kFrameSamples * 3, kSampleRate);
    const int64_t kFifthPtsMs = samplesToPtsMs(kFrameSamples * 4, kSampleRate);

    HJAudioInfo::Ptr output_info = std::make_shared<HJAudioInfo>();
    output_info->m_samplesRate = kSampleRate;
    output_info->setChannels(HJ_CHANNEL_DEFAULT);
    output_info->m_sampleFmt = HJ_SAMPLE_FORMAT_DEFAULT;
    output_info->m_sampleCnt = kFrameSamples;
    output_info->m_samplesPerFrame = kFrameSamples;

    HJAudioMixerConfig config{};
    config.m_output_info = output_info;
    config.m_max_inputs = 2;
    config.m_frame_samples = kFrameSamples;
    config.m_sync_window_ms = 50;
    config.m_late_threshold_ms = kLateThresholdMs;
    config.m_enable_compand = false;
    config.m_enable_limiter = false;

    auto mixer = std::make_shared<HJTestAudioMixerPlugin>(
        "HJPluginAudioMixerRebindTimeoutTest");
    auto source0 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerRebindTimeoutSource0");
    auto source1 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerRebindTimeoutSource1");
    auto sink =
        std::make_shared<HJTestSinkPlugin>("HJPluginAudioMixerRebindTimeoutSink");
    ASSERT_NE(mixer, nullptr);
    ASSERT_NE(source0, nullptr);
    ASSERT_NE(source1, nullptr);
    ASSERT_NE(sink, nullptr);

    auto init_param = std::make_shared<HJKeyStorage>();
    (*init_param)["createThread"] = false;
    (*init_param)["audioMixerConfig"] = config;
    ASSERT_EQ(mixer->init(init_param), HJ_OK);

    ASSERT_EQ(source0->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(source1->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source0, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source1, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addOutputPlugin(sink, HJMEDIA_TYPE_AUDIO), HJ_OK);

    HJAudioMixerInputDesc input_desc0{};
    input_desc0.m_input_id = "input0";
    input_desc0.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    HJAudioMixerInputDesc input_desc1{};
    input_desc1.m_input_id = "input1";
    input_desc1.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    ASSERT_NE(input_desc0.m_input_info, nullptr);
    ASSERT_NE(input_desc1.m_input_info, nullptr);

    const size_t input0_key_hash =
        buildPluginKeyHash(source0->getName(), HJMEDIA_TYPE_AUDIO, 0);
    const size_t input1_key_hash =
        buildPluginKeyHash(source1->getName(), HJMEDIA_TYPE_AUDIO, 0);
    ASSERT_EQ(mixer->bindInput(0, input0_key_hash, input_desc0), HJ_OK);
    ASSERT_EQ(mixer->bindInput(1, input1_key_hash, input_desc1), HJ_OK);

    auto first_frame0 = makeTestAudioFrame(output_info, 0);
    auto first_frame1 = makeTestAudioFrame(output_info, 0);
    auto second_frame0 = makeTestAudioFrame(output_info, kSecondPtsMs);
    auto third_frame0 = makeTestAudioFrame(output_info, kThirdPtsMs);
    auto fourth_frame0 = makeTestAudioFrame(output_info, kFourthPtsMs);
    auto fifth_frame0 = makeTestAudioFrame(output_info, kFifthPtsMs);
    ASSERT_NE(first_frame0, nullptr);
    ASSERT_NE(first_frame1, nullptr);
    ASSERT_NE(second_frame0, nullptr);
    ASSERT_NE(third_frame0, nullptr);
    ASSERT_NE(fourth_frame0, nullptr);
    ASSERT_NE(fifth_frame0, nullptr);

    ASSERT_EQ(source0->pushFrame(first_frame0), HJ_OK);
    ASSERT_EQ(source1->pushFrame(first_frame1), HJ_OK);

    int64_t delay_ms = 0;
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(sink->getFrames().size(), 1u);

    ASSERT_EQ(mixer->unbindInput(1), HJ_OK);

    ASSERT_EQ(source0->pushFrame(second_frame0), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(sink->getFrames().size(), 2u);
    ASSERT_EQ(sink->getFrames()[1]->getPTS(), kSecondPtsMs);

    ASSERT_EQ(mixer->bindInput(1, input1_key_hash, input_desc1), HJ_OK);

    ASSERT_EQ(source0->pushFrame(third_frame0), HJ_OK);
    EXPECT_EQ(mixer->runOnce(&delay_ms), HJ_WOULD_BLOCK);
    EXPECT_EQ(sink->getFrames().size(), 2u);

    ASSERT_EQ(source0->pushFrame(fourth_frame0), HJ_OK);
    EXPECT_EQ(mixer->runOnce(&delay_ms), HJ_WOULD_BLOCK);
    EXPECT_EQ(sink->getFrames().size(), 2u);

    ASSERT_EQ(source0->pushFrame(fifth_frame0), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    const auto &out_frames = sink->getFrames();
    ASSERT_EQ(out_frames.size(), 3u);
    ASSERT_NE(out_frames[2], nullptr);
    EXPECT_EQ(out_frames[2]->getPTS(), kThirdPtsMs);
    EXPECT_EQ(mixer->queuedFrameCount(input0_key_hash), 2u);
    EXPECT_EQ(mixer->queuedFrameCount(input1_key_hash), 0u);
    EXPECT_EQ(mixer->previewPts(input0_key_hash), kFourthPtsMs);
    EXPECT_EQ(mixer->previewPts(input1_key_hash), HJ_NOTS_VALUE);

    EXPECT_EQ(mixer->done(), HJ_OK);
}

TEST(HJPluginAudioMixerTest,
     RebindLateJoinFramesWhileExistingInputStarvedDoesNotAdvanceWithSilence)
{
    constexpr int kSampleRate = 48000;
    constexpr int kFrameSamples = 1024;
    const int64_t kSecondPtsMs = samplesToPtsMs(kFrameSamples, kSampleRate);
    const int64_t kThirdPtsMs = samplesToPtsMs(kFrameSamples * 2, kSampleRate);
    const int64_t kFourthPtsMs = samplesToPtsMs(kFrameSamples * 3, kSampleRate);
    const int64_t kFifthPtsMs = samplesToPtsMs(kFrameSamples * 4, kSampleRate);
    const int64_t kSixthPtsMs = samplesToPtsMs(kFrameSamples * 5, kSampleRate);

    HJAudioInfo::Ptr output_info = std::make_shared<HJAudioInfo>();
    output_info->m_samplesRate = kSampleRate;
    output_info->setChannels(HJ_CHANNEL_DEFAULT);
    output_info->m_sampleFmt = HJ_SAMPLE_FORMAT_DEFAULT;
    output_info->m_sampleCnt = kFrameSamples;
    output_info->m_samplesPerFrame = kFrameSamples;

    HJAudioMixerConfig config{};
    config.m_output_info = output_info;
    config.m_max_inputs = 2;
    config.m_frame_samples = kFrameSamples;
    config.m_sync_window_ms = 50;
    config.m_late_threshold_ms = 500;
    config.m_enable_compand = false;
    config.m_enable_limiter = false;

    auto mixer = std::make_shared<HJTestAudioMixerPlugin>(
        "HJPluginAudioMixerRebindLateJoinStarvedTest");
    auto source0 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerRebindLateJoinStarvedSource0");
    auto source1 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerRebindLateJoinStarvedSource1");
    auto sink = std::make_shared<HJTestSinkPlugin>(
        "HJPluginAudioMixerRebindLateJoinStarvedSink");
    ASSERT_NE(mixer, nullptr);
    ASSERT_NE(source0, nullptr);
    ASSERT_NE(source1, nullptr);
    ASSERT_NE(sink, nullptr);

    auto init_param = std::make_shared<HJKeyStorage>();
    (*init_param)["createThread"] = false;
    (*init_param)["audioMixerConfig"] = config;
    ASSERT_EQ(mixer->init(init_param), HJ_OK);

    ASSERT_EQ(source0->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(source1->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source0, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source1, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addOutputPlugin(sink, HJMEDIA_TYPE_AUDIO), HJ_OK);

    HJAudioMixerInputDesc input_desc0{};
    input_desc0.m_input_id = "input0";
    input_desc0.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    HJAudioMixerInputDesc input_desc1{};
    input_desc1.m_input_id = "input1";
    input_desc1.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    ASSERT_NE(input_desc0.m_input_info, nullptr);
    ASSERT_NE(input_desc1.m_input_info, nullptr);

    const size_t input0_key_hash =
        buildPluginKeyHash(source0->getName(), HJMEDIA_TYPE_AUDIO, 0);
    const size_t input1_key_hash =
        buildPluginKeyHash(source1->getName(), HJMEDIA_TYPE_AUDIO, 0);
    ASSERT_EQ(mixer->bindInput(0, input0_key_hash, input_desc0), HJ_OK);
    ASSERT_EQ(mixer->bindInput(1, input1_key_hash, input_desc1), HJ_OK);

    auto first_frame0 = makeTestAudioFrame(output_info, 0);
    auto first_frame1 = makeTestAudioFrame(output_info, 0);
    auto second_frame0 = makeTestAudioFrame(output_info, kSecondPtsMs);
    auto third_frame0 = makeTestAudioFrame(output_info, kThirdPtsMs);
    auto fourth_frame0 = makeTestAudioFrame(output_info, kFourthPtsMs);
    auto fifth_frame0 = makeTestAudioFrame(output_info, kFifthPtsMs);
    auto stale_rejoin_frame1a = makeTestAudioFrame(output_info, kFourthPtsMs);
    auto stale_rejoin_frame1b = makeTestAudioFrame(output_info, kFourthPtsMs);
    auto current_rejoin_frame0 = makeTestAudioFrame(output_info, kSixthPtsMs);
    auto current_rejoin_frame1 = makeTestAudioFrame(output_info, kSixthPtsMs);
    ASSERT_NE(first_frame0, nullptr);
    ASSERT_NE(first_frame1, nullptr);
    ASSERT_NE(second_frame0, nullptr);
    ASSERT_NE(third_frame0, nullptr);
    ASSERT_NE(fourth_frame0, nullptr);
    ASSERT_NE(fifth_frame0, nullptr);
    ASSERT_NE(stale_rejoin_frame1a, nullptr);
    ASSERT_NE(stale_rejoin_frame1b, nullptr);
    ASSERT_NE(current_rejoin_frame0, nullptr);
    ASSERT_NE(current_rejoin_frame1, nullptr);

    ASSERT_EQ(source0->pushFrame(first_frame0), HJ_OK);
    ASSERT_EQ(source1->pushFrame(first_frame1), HJ_OK);

    int64_t delay_ms = 0;
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(sink->getFrames().size(), 1u);
    ASSERT_EQ(sink->getFrames()[0]->getPTS(), 0);

    ASSERT_EQ(mixer->unbindInput(1), HJ_OK);

    ASSERT_EQ(source0->pushFrame(second_frame0), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(source0->pushFrame(third_frame0), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(source0->pushFrame(fourth_frame0), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(source0->pushFrame(fifth_frame0), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    const auto &single_input_frames = sink->getFrames();
    ASSERT_EQ(single_input_frames.size(), 5u);
    EXPECT_EQ(single_input_frames[4]->getPTS(), kFifthPtsMs);

    ASSERT_EQ(mixer->bindInput(1, input1_key_hash, input_desc1), HJ_OK);
    ASSERT_EQ(source1->pushFrame(stale_rejoin_frame1a), HJ_OK);
    ASSERT_EQ(source1->pushFrame(stale_rejoin_frame1b), HJ_OK);

    EXPECT_EQ(mixer->runOnce(&delay_ms), HJ_WOULD_BLOCK);
    EXPECT_EQ(sink->getFrames().size(), 5u);
    EXPECT_EQ(mixer->queuedFrameCount(input0_key_hash), 0u);
    EXPECT_EQ(mixer->queuedFrameCount(input1_key_hash), 0u);
    EXPECT_EQ(mixer->previewPts(input0_key_hash), HJ_NOTS_VALUE);
    EXPECT_EQ(mixer->previewPts(input1_key_hash), HJ_NOTS_VALUE);

    EXPECT_EQ(mixer->runOnce(&delay_ms), HJ_WOULD_BLOCK);
    EXPECT_EQ(sink->getFrames().size(), 5u);

    ASSERT_EQ(source0->pushFrame(current_rejoin_frame0), HJ_OK);
    ASSERT_EQ(source1->pushFrame(current_rejoin_frame1), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    const auto &out_frames = sink->getFrames();
    ASSERT_EQ(out_frames.size(), 6u);
    ASSERT_NE(out_frames[5], nullptr);
    EXPECT_EQ(out_frames[5]->getPTS(), kSixthPtsMs);
    EXPECT_EQ(mixer->queuedFrameCount(input0_key_hash), 0u);
    EXPECT_EQ(mixer->queuedFrameCount(input1_key_hash), 0u);
    EXPECT_EQ(mixer->previewPts(input0_key_hash), HJ_NOTS_VALUE);
    EXPECT_EQ(mixer->previewPts(input1_key_hash), HJ_NOTS_VALUE);

    EXPECT_EQ(mixer->done(), HJ_OK);
}

TEST(HJPluginAudioMixerTest, RebindInputDropsStaleFramesBeforeCurrentJoinFrame)
{
    constexpr int kSampleRate = 48000;
    constexpr int kFrameSamples = 1024;
    const int64_t kSecondPtsMs = samplesToPtsMs(kFrameSamples, kSampleRate);
    const int64_t kThirdPtsMs = samplesToPtsMs(kFrameSamples * 2, kSampleRate);

    HJAudioInfo::Ptr output_info = std::make_shared<HJAudioInfo>();
    output_info->m_samplesRate = kSampleRate;
    output_info->setChannels(HJ_CHANNEL_DEFAULT);
    output_info->m_sampleFmt = HJ_SAMPLE_FORMAT_DEFAULT;
    output_info->m_sampleCnt = kFrameSamples;
    output_info->m_samplesPerFrame = kFrameSamples;

    HJAudioMixerConfig config{};
    config.m_output_info = output_info;
    config.m_max_inputs = 2;
    config.m_frame_samples = kFrameSamples;
    config.m_sync_window_ms = 50;
    config.m_late_threshold_ms = 200;
    config.m_enable_compand = false;
    config.m_enable_limiter = false;

    auto mixer = std::make_shared<HJTestAudioMixerPlugin>(
        "HJPluginAudioMixerRebindStaleJoinTest");
    auto source0 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerRebindStaleJoinSource0");
    auto source1 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerRebindStaleJoinSource1");
    auto sink = std::make_shared<HJTestSinkPlugin>(
        "HJPluginAudioMixerRebindStaleJoinSink");
    ASSERT_NE(mixer, nullptr);
    ASSERT_NE(source0, nullptr);
    ASSERT_NE(source1, nullptr);
    ASSERT_NE(sink, nullptr);

    auto init_param = std::make_shared<HJKeyStorage>();
    (*init_param)["createThread"] = false;
    (*init_param)["audioMixerConfig"] = config;
    ASSERT_EQ(mixer->init(init_param), HJ_OK);

    ASSERT_EQ(source0->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(source1->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source0, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source1, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addOutputPlugin(sink, HJMEDIA_TYPE_AUDIO), HJ_OK);

    HJAudioMixerInputDesc input_desc0{};
    input_desc0.m_input_id = "input0";
    input_desc0.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    HJAudioMixerInputDesc input_desc1{};
    input_desc1.m_input_id = "input1";
    input_desc1.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    ASSERT_NE(input_desc0.m_input_info, nullptr);
    ASSERT_NE(input_desc1.m_input_info, nullptr);

    const size_t input0_key_hash =
        buildPluginKeyHash(source0->getName(), HJMEDIA_TYPE_AUDIO, 0);
    const size_t input1_key_hash =
        buildPluginKeyHash(source1->getName(), HJMEDIA_TYPE_AUDIO, 0);
    ASSERT_EQ(mixer->bindInput(0, input0_key_hash, input_desc0), HJ_OK);
    ASSERT_EQ(mixer->bindInput(1, input1_key_hash, input_desc1), HJ_OK);

    auto first_frame0 = makeTestAudioFrame(output_info, 0);
    auto first_frame1 = makeTestAudioFrame(output_info, 0);
    auto second_frame0 = makeTestAudioFrame(output_info, kSecondPtsMs);
    auto stale_frame1 = makeTestAudioFrame(output_info, 0);
    auto third_frame0 = makeTestAudioFrame(output_info, kThirdPtsMs);
    auto third_frame1 = makeTestAudioFrame(output_info, kThirdPtsMs);
    ASSERT_NE(first_frame0, nullptr);
    ASSERT_NE(first_frame1, nullptr);
    ASSERT_NE(second_frame0, nullptr);
    ASSERT_NE(stale_frame1, nullptr);
    ASSERT_NE(third_frame0, nullptr);
    ASSERT_NE(third_frame1, nullptr);

    ASSERT_EQ(source0->pushFrame(first_frame0), HJ_OK);
    ASSERT_EQ(source1->pushFrame(first_frame1), HJ_OK);

    int64_t delay_ms = 0;
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(sink->getFrames().size(), 1u);

    ASSERT_EQ(mixer->unbindInput(1), HJ_OK);

    ASSERT_EQ(source0->pushFrame(second_frame0), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(sink->getFrames().size(), 2u);
    ASSERT_EQ(sink->getFrames()[1]->getPTS(), kSecondPtsMs);

    ASSERT_EQ(mixer->bindInput(1, input1_key_hash, input_desc1), HJ_OK);
    ASSERT_EQ(source1->pushFrame(stale_frame1), HJ_OK);
    ASSERT_EQ(source1->pushFrame(third_frame1), HJ_OK);
    ASSERT_EQ(source0->pushFrame(third_frame0), HJ_OK);

    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    const auto &out_frames = sink->getFrames();
    ASSERT_EQ(out_frames.size(), 3u);
    ASSERT_NE(out_frames[2], nullptr);
    EXPECT_EQ(out_frames[2]->getPTS(), kThirdPtsMs);
    EXPECT_EQ(mixer->queuedFrameCount(input0_key_hash), 0u);
    EXPECT_EQ(mixer->queuedFrameCount(input1_key_hash), 0u);
    EXPECT_EQ(mixer->previewPts(input0_key_hash), HJ_NOTS_VALUE);
    EXPECT_EQ(mixer->previewPts(input1_key_hash), HJ_NOTS_VALUE);

    EXPECT_EQ(mixer->done(), HJ_OK);
}

TEST(HJPluginAudioMixerTest, SyncWindowOneDropsPreviousFrameAtHalfWindowBoundary)
{
    constexpr int kSampleRate = 48000;
    constexpr int kFrameSamples = 1024;
    const int64_t kCurrentPtsMs = samplesToPtsMs(kFrameSamples, kSampleRate);
    const int64_t kPreviousPtsMs = kCurrentPtsMs - 1;

    HJAudioInfo::Ptr output_info = std::make_shared<HJAudioInfo>();
    output_info->m_samplesRate = kSampleRate;
    output_info->setChannels(HJ_CHANNEL_DEFAULT);
    output_info->m_sampleFmt = HJ_SAMPLE_FORMAT_DEFAULT;
    output_info->m_sampleCnt = kFrameSamples;
    output_info->m_samplesPerFrame = kFrameSamples;

    HJAudioMixerConfig config{};
    config.m_output_info = output_info;
    config.m_max_inputs = 1;
    config.m_frame_samples = kFrameSamples;
    config.m_sync_window_ms = 1;
    config.m_late_threshold_ms = 0;
    config.m_enable_compand = false;
    config.m_enable_limiter = false;

    auto mixer = std::make_shared<HJTestAudioMixerPlugin>(
        "HJPluginAudioMixerSyncWindowOneBoundaryTest");
    auto source = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerSyncWindowOneBoundarySource");
    auto sink = std::make_shared<HJTestSinkPlugin>(
        "HJPluginAudioMixerSyncWindowOneBoundarySink");
    ASSERT_NE(mixer, nullptr);
    ASSERT_NE(source, nullptr);
    ASSERT_NE(sink, nullptr);

    auto init_param = std::make_shared<HJKeyStorage>();
    (*init_param)["createThread"] = false;
    (*init_param)["audioMixerConfig"] = config;
    ASSERT_EQ(mixer->init(init_param), HJ_OK);

    ASSERT_EQ(source->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addOutputPlugin(sink, HJMEDIA_TYPE_AUDIO), HJ_OK);

    HJAudioMixerInputDesc input_desc{};
    input_desc.m_input_id = "input0";
    input_desc.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    ASSERT_NE(input_desc.m_input_info, nullptr);

    const size_t input_key_hash =
        buildPluginKeyHash(source->getName(), HJMEDIA_TYPE_AUDIO, 0);
    ASSERT_EQ(mixer->bindInput(0, input_key_hash, input_desc), HJ_OK);

    auto startup_frame = makeTestAudioFrame(output_info, 0);
    auto previous_frame = makeTestAudioFrame(output_info, kPreviousPtsMs);
    auto current_frame = makeTestAudioFrame(output_info, kCurrentPtsMs);
    ASSERT_NE(startup_frame, nullptr);
    ASSERT_NE(previous_frame, nullptr);
    ASSERT_NE(current_frame, nullptr);

    ASSERT_EQ(source->pushFrame(startup_frame), HJ_OK);

    int64_t delay_ms = 0;
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    ASSERT_EQ(source->pushFrame(previous_frame), HJ_OK);
    ASSERT_EQ(source->pushFrame(current_frame), HJ_OK);

    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    const auto &out_frames = sink->getFrames();
    ASSERT_EQ(out_frames.size(), 2u);
    ASSERT_NE(out_frames[1], nullptr);
    EXPECT_EQ(out_frames[1]->getPTS(), kCurrentPtsMs);
    EXPECT_EQ(mixer->queuedFrameCount(input_key_hash), 0u);
    EXPECT_EQ(mixer->previewPts(input_key_hash), HJ_NOTS_VALUE);

    EXPECT_EQ(mixer->done(), HJ_OK);
}

TEST(HJPluginAudioMixerTest,
     RebindInputTwiceStillDropsStaleFramesAndWaitsForSecondJoin)
{
    constexpr int kSampleRate = 48000;
    constexpr int kFrameSamples = 1024;
    const int64_t kSecondPtsMs = samplesToPtsMs(kFrameSamples, kSampleRate);
    const int64_t kThirdPtsMs = samplesToPtsMs(kFrameSamples * 2, kSampleRate);
    const int64_t kFourthPtsMs = samplesToPtsMs(kFrameSamples * 3, kSampleRate);

    HJAudioInfo::Ptr output_info = std::make_shared<HJAudioInfo>();
    output_info->m_samplesRate = kSampleRate;
    output_info->setChannels(HJ_CHANNEL_DEFAULT);
    output_info->m_sampleFmt = HJ_SAMPLE_FORMAT_DEFAULT;
    output_info->m_sampleCnt = kFrameSamples;
    output_info->m_samplesPerFrame = kFrameSamples;

    HJAudioMixerConfig config{};
    config.m_output_info = output_info;
    config.m_max_inputs = 2;
    config.m_frame_samples = kFrameSamples;
    config.m_sync_window_ms = 50;
    config.m_late_threshold_ms = 200;
    config.m_enable_compand = false;
    config.m_enable_limiter = false;

    auto mixer = std::make_shared<HJTestAudioMixerPlugin>(
        "HJPluginAudioMixerRebindTwiceTest");
    auto source0 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerRebindTwiceSource0");
    auto source1 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerRebindTwiceSource1");
    auto sink =
        std::make_shared<HJTestSinkPlugin>("HJPluginAudioMixerRebindTwiceSink");
    ASSERT_NE(mixer, nullptr);
    ASSERT_NE(source0, nullptr);
    ASSERT_NE(source1, nullptr);
    ASSERT_NE(sink, nullptr);

    auto init_param = std::make_shared<HJKeyStorage>();
    (*init_param)["createThread"] = false;
    (*init_param)["audioMixerConfig"] = config;
    ASSERT_EQ(mixer->init(init_param), HJ_OK);

    ASSERT_EQ(source0->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(source1->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source0, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source1, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addOutputPlugin(sink, HJMEDIA_TYPE_AUDIO), HJ_OK);

    HJAudioMixerInputDesc input_desc0{};
    input_desc0.m_input_id = "input0";
    input_desc0.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    HJAudioMixerInputDesc input_desc1{};
    input_desc1.m_input_id = "input1";
    input_desc1.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    ASSERT_NE(input_desc0.m_input_info, nullptr);
    ASSERT_NE(input_desc1.m_input_info, nullptr);

    const size_t input0_key_hash =
        buildPluginKeyHash(source0->getName(), HJMEDIA_TYPE_AUDIO, 0);
    const size_t input1_key_hash =
        buildPluginKeyHash(source1->getName(), HJMEDIA_TYPE_AUDIO, 0);
    ASSERT_EQ(mixer->bindInput(0, input0_key_hash, input_desc0), HJ_OK);
    ASSERT_EQ(mixer->bindInput(1, input1_key_hash, input_desc1), HJ_OK);

    auto first_frame0 = makeTestAudioFrame(output_info, 0);
    auto first_frame1 = makeTestAudioFrame(output_info, 0);
    auto second_frame0 = makeTestAudioFrame(output_info, kSecondPtsMs);
    auto third_frame0 = makeTestAudioFrame(output_info, kThirdPtsMs);
    auto third_frame1 = makeTestAudioFrame(output_info, kThirdPtsMs);
    auto stale_frame1 = makeTestAudioFrame(output_info, 0);
    auto fourth_frame0 = makeTestAudioFrame(output_info, kFourthPtsMs);
    auto fourth_frame1 = makeTestAudioFrame(output_info, kFourthPtsMs);
    ASSERT_NE(first_frame0, nullptr);
    ASSERT_NE(first_frame1, nullptr);
    ASSERT_NE(second_frame0, nullptr);
    ASSERT_NE(third_frame0, nullptr);
    ASSERT_NE(third_frame1, nullptr);
    ASSERT_NE(stale_frame1, nullptr);
    ASSERT_NE(fourth_frame0, nullptr);
    ASSERT_NE(fourth_frame1, nullptr);

    ASSERT_EQ(source0->pushFrame(first_frame0), HJ_OK);
    ASSERT_EQ(source1->pushFrame(first_frame1), HJ_OK);

    int64_t delay_ms = 0;
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(sink->getFrames().size(), 1u);

    ASSERT_EQ(mixer->unbindInput(1), HJ_OK);

    ASSERT_EQ(source0->pushFrame(second_frame0), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(sink->getFrames().size(), 2u);
    ASSERT_EQ(sink->getFrames()[1]->getPTS(), kSecondPtsMs);

    ASSERT_EQ(mixer->bindInput(1, input1_key_hash, input_desc1), HJ_OK);
    ASSERT_EQ(source1->pushFrame(third_frame1), HJ_OK);
    EXPECT_EQ(mixer->runOnce(&delay_ms), HJ_WOULD_BLOCK);
    EXPECT_EQ(sink->getFrames().size(), 2u);

    ASSERT_EQ(source0->pushFrame(third_frame0), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(sink->getFrames().size(), 3u);
    ASSERT_EQ(sink->getFrames()[2]->getPTS(), kThirdPtsMs);

    ASSERT_EQ(mixer->unbindInput(1), HJ_OK);
    ASSERT_EQ(mixer->bindInput(1, input1_key_hash, input_desc1), HJ_OK);
    ASSERT_EQ(source1->pushFrame(stale_frame1), HJ_OK);
    ASSERT_EQ(source1->pushFrame(fourth_frame1), HJ_OK);
    EXPECT_EQ(mixer->runOnce(&delay_ms), HJ_WOULD_BLOCK);
    EXPECT_EQ(sink->getFrames().size(), 3u);
    EXPECT_EQ(mixer->queuedFrameCount(input1_key_hash), 1u);
    EXPECT_EQ(mixer->previewPts(input1_key_hash), kFourthPtsMs);

    ASSERT_EQ(source0->pushFrame(fourth_frame0), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    const auto &out_frames = sink->getFrames();
    ASSERT_EQ(out_frames.size(), 4u);
    ASSERT_NE(out_frames[3], nullptr);
    EXPECT_EQ(out_frames[3]->getPTS(), kFourthPtsMs);
    EXPECT_EQ(mixer->queuedFrameCount(input0_key_hash), 0u);
    EXPECT_EQ(mixer->queuedFrameCount(input1_key_hash), 0u);
    EXPECT_EQ(mixer->previewPts(input0_key_hash), HJ_NOTS_VALUE);
    EXPECT_EQ(mixer->previewPts(input1_key_hash), HJ_NOTS_VALUE);

    EXPECT_EQ(mixer->done(), HJ_OK);
}

TEST(HJPluginAudioMixerTest, ZeroLateThresholdRebindDoesNotWaitForJoin)
{
    constexpr int kSampleRate = 48000;
    constexpr int kFrameSamples = 1024;
    const int64_t kSecondPtsMs = samplesToPtsMs(kFrameSamples, kSampleRate);
    const int64_t kThirdPtsMs = samplesToPtsMs(kFrameSamples * 2, kSampleRate);

    HJAudioInfo::Ptr output_info = std::make_shared<HJAudioInfo>();
    output_info->m_samplesRate = kSampleRate;
    output_info->setChannels(HJ_CHANNEL_DEFAULT);
    output_info->m_sampleFmt = HJ_SAMPLE_FORMAT_DEFAULT;
    output_info->m_sampleCnt = kFrameSamples;
    output_info->m_samplesPerFrame = kFrameSamples;

    HJAudioMixerConfig config{};
    config.m_output_info = output_info;
    config.m_max_inputs = 2;
    config.m_frame_samples = kFrameSamples;
    config.m_sync_window_ms = 50;
    config.m_late_threshold_ms = 0;
    config.m_enable_compand = false;
    config.m_enable_limiter = false;

    auto mixer = std::make_shared<HJTestAudioMixerPlugin>(
        "HJPluginAudioMixerZeroLateThresholdJoinTest");
    auto source0 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerZeroLateThresholdJoinSource0");
    auto source1 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerZeroLateThresholdJoinSource1");
    auto sink = std::make_shared<HJTestSinkPlugin>(
        "HJPluginAudioMixerZeroLateThresholdJoinSink");
    ASSERT_NE(mixer, nullptr);
    ASSERT_NE(source0, nullptr);
    ASSERT_NE(source1, nullptr);
    ASSERT_NE(sink, nullptr);

    auto init_param = std::make_shared<HJKeyStorage>();
    (*init_param)["createThread"] = false;
    (*init_param)["audioMixerConfig"] = config;
    ASSERT_EQ(mixer->init(init_param), HJ_OK);

    ASSERT_EQ(source0->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(source1->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source0, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source1, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addOutputPlugin(sink, HJMEDIA_TYPE_AUDIO), HJ_OK);

    HJAudioMixerInputDesc input_desc0{};
    input_desc0.m_input_id = "input0";
    input_desc0.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    HJAudioMixerInputDesc input_desc1{};
    input_desc1.m_input_id = "input1";
    input_desc1.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    ASSERT_NE(input_desc0.m_input_info, nullptr);
    ASSERT_NE(input_desc1.m_input_info, nullptr);

    const size_t input0_key_hash =
        buildPluginKeyHash(source0->getName(), HJMEDIA_TYPE_AUDIO, 0);
    const size_t input1_key_hash =
        buildPluginKeyHash(source1->getName(), HJMEDIA_TYPE_AUDIO, 0);
    ASSERT_EQ(mixer->bindInput(0, input0_key_hash, input_desc0), HJ_OK);
    ASSERT_EQ(mixer->bindInput(1, input1_key_hash, input_desc1), HJ_OK);

    auto first_frame0 = makeTestAudioFrame(output_info, 0);
    auto first_frame1 = makeTestAudioFrame(output_info, 0);
    auto second_frame0 = makeTestAudioFrame(output_info, kSecondPtsMs);
    auto third_frame1 = makeTestAudioFrame(output_info, kThirdPtsMs);
    ASSERT_NE(first_frame0, nullptr);
    ASSERT_NE(first_frame1, nullptr);
    ASSERT_NE(second_frame0, nullptr);
    ASSERT_NE(third_frame1, nullptr);

    ASSERT_EQ(source0->pushFrame(first_frame0), HJ_OK);
    ASSERT_EQ(source1->pushFrame(first_frame1), HJ_OK);

    int64_t delay_ms = 0;
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(sink->getFrames().size(), 1u);

    ASSERT_EQ(mixer->unbindInput(1), HJ_OK);
    ASSERT_EQ(source0->pushFrame(second_frame0), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(sink->getFrames().size(), 2u);
    ASSERT_EQ(sink->getFrames()[1]->getPTS(), kSecondPtsMs);

    ASSERT_EQ(mixer->bindInput(1, input1_key_hash, input_desc1), HJ_OK);
    ASSERT_EQ(source1->pushFrame(third_frame1), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    const auto &out_frames = sink->getFrames();
    ASSERT_EQ(out_frames.size(), 3u);
    ASSERT_NE(out_frames[2], nullptr);
    EXPECT_EQ(out_frames[2]->getPTS(), kThirdPtsMs);
    EXPECT_EQ(mixer->queuedFrameCount(input0_key_hash), 0u);
    EXPECT_EQ(mixer->queuedFrameCount(input1_key_hash), 0u);
    EXPECT_EQ(mixer->previewPts(input0_key_hash), HJ_NOTS_VALUE);
    EXPECT_EQ(mixer->previewPts(input1_key_hash), HJ_NOTS_VALUE);

    EXPECT_EQ(mixer->done(), HJ_OK);
}

TEST(HJPluginAudioMixerTest, RebindMutedInputRestoredVolumeRejoinsWaitLogic)
{
    constexpr int kSampleRate = 48000;
    constexpr int kFrameSamples = 1024;
    const int64_t kSecondPtsMs = samplesToPtsMs(kFrameSamples, kSampleRate);
    const int64_t kThirdPtsMs = samplesToPtsMs(kFrameSamples * 2, kSampleRate);
    const int64_t kFourthPtsMs = samplesToPtsMs(kFrameSamples * 3, kSampleRate);

    HJAudioInfo::Ptr output_info = std::make_shared<HJAudioInfo>();
    output_info->m_samplesRate = kSampleRate;
    output_info->setChannels(HJ_CHANNEL_DEFAULT);
    output_info->m_sampleFmt = HJ_SAMPLE_FORMAT_DEFAULT;
    output_info->m_sampleCnt = kFrameSamples;
    output_info->m_samplesPerFrame = kFrameSamples;

    HJAudioMixerConfig config{};
    config.m_output_info = output_info;
    config.m_max_inputs = 2;
    config.m_frame_samples = kFrameSamples;
    config.m_sync_window_ms = 50;
    config.m_late_threshold_ms = 200;
    config.m_enable_compand = false;
    config.m_enable_limiter = false;

    auto mixer = std::make_shared<HJTestAudioMixerPlugin>(
        "HJPluginAudioMixerMutedRejoinTest");
    auto source0 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerMutedRejoinSource0");
    auto source1 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerMutedRejoinSource1");
    auto sink =
        std::make_shared<HJTestSinkPlugin>("HJPluginAudioMixerMutedRejoinSink");
    ASSERT_NE(mixer, nullptr);
    ASSERT_NE(source0, nullptr);
    ASSERT_NE(source1, nullptr);
    ASSERT_NE(sink, nullptr);

    auto init_param = std::make_shared<HJKeyStorage>();
    (*init_param)["createThread"] = false;
    (*init_param)["audioMixerConfig"] = config;
    ASSERT_EQ(mixer->init(init_param), HJ_OK);

    ASSERT_EQ(source0->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(source1->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source0, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source1, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addOutputPlugin(sink, HJMEDIA_TYPE_AUDIO), HJ_OK);

    HJAudioMixerInputDesc input_desc0{};
    input_desc0.m_input_id = "input0";
    input_desc0.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    HJAudioMixerInputDesc input_desc1{};
    input_desc1.m_input_id = "input1";
    input_desc1.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    input_desc1.m_volume = 0.0f;
    ASSERT_NE(input_desc0.m_input_info, nullptr);
    ASSERT_NE(input_desc1.m_input_info, nullptr);

    const size_t input0_key_hash =
        buildPluginKeyHash(source0->getName(), HJMEDIA_TYPE_AUDIO, 0);
    const size_t input1_key_hash =
        buildPluginKeyHash(source1->getName(), HJMEDIA_TYPE_AUDIO, 0);
    ASSERT_EQ(mixer->bindInput(0, input0_key_hash, input_desc0), HJ_OK);
    ASSERT_EQ(mixer->bindInput(1, input1_key_hash, input_desc1), HJ_OK);

    auto first_frame0 = makeTestAudioFrame(output_info, 0);
    auto second_frame0 = makeTestAudioFrame(output_info, kSecondPtsMs);
    auto third_frame0 = makeTestAudioFrame(output_info, kThirdPtsMs);
    auto fourth_frame0 = makeTestAudioFrame(output_info, kFourthPtsMs);
    auto fourth_frame1 = makeTestAudioFrame(output_info, kFourthPtsMs);
    ASSERT_NE(first_frame0, nullptr);
    ASSERT_NE(second_frame0, nullptr);
    ASSERT_NE(third_frame0, nullptr);
    ASSERT_NE(fourth_frame0, nullptr);
    ASSERT_NE(fourth_frame1, nullptr);

    ASSERT_EQ(source0->pushFrame(first_frame0), HJ_OK);

    int64_t delay_ms = 0;
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(sink->getFrames().size(), 1u);

    ASSERT_EQ(mixer->unbindInput(1), HJ_OK);
    ASSERT_EQ(mixer->bindInput(1, input1_key_hash, input_desc1), HJ_OK);

    ASSERT_EQ(source0->pushFrame(second_frame0), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(sink->getFrames().size(), 2u);
    ASSERT_EQ(sink->getFrames()[1]->getPTS(), kSecondPtsMs);

    ASSERT_EQ(mixer->setInputVolume(1, 1.0f), HJ_OK);
    ASSERT_EQ(source1->pushFrame(fourth_frame1), HJ_OK);
    EXPECT_EQ(mixer->runOnce(&delay_ms), HJ_WOULD_BLOCK);
    EXPECT_EQ(sink->getFrames().size(), 2u);
    EXPECT_EQ(mixer->queuedFrameCount(input1_key_hash), 1u);
    EXPECT_EQ(mixer->previewPts(input1_key_hash), kFourthPtsMs);

    ASSERT_EQ(source0->pushFrame(third_frame0), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(sink->getFrames().size(), 3u);
    ASSERT_NE(sink->getFrames()[2], nullptr);
    EXPECT_EQ(sink->getFrames()[2]->getPTS(), kThirdPtsMs);
    EXPECT_EQ(mixer->queuedFrameCount(input1_key_hash), 0u);
    EXPECT_EQ(mixer->previewPts(input1_key_hash), HJ_NOTS_VALUE);

    ASSERT_EQ(source0->pushFrame(fourth_frame0), HJ_OK);
    EXPECT_EQ(mixer->runOnce(&delay_ms), HJ_WOULD_BLOCK);

    const auto &out_frames = sink->getFrames();
    ASSERT_EQ(out_frames.size(), 3u);
    EXPECT_EQ(mixer->queuedFrameCount(input0_key_hash), 1u);
    EXPECT_EQ(mixer->queuedFrameCount(input1_key_hash), 0u);
    EXPECT_EQ(mixer->previewPts(input0_key_hash), kFourthPtsMs);
    EXPECT_EQ(mixer->previewPts(input1_key_hash), HJ_NOTS_VALUE);

    EXPECT_EQ(mixer->done(), HJ_OK);
}

TEST(HJPluginAudioMixerTest, FlushInputClearsQueuedFramesAndResetsPublishedState)
{
    constexpr int kSampleRate = 48000;
    constexpr int kFrameSamples = 1024;
    constexpr int64_t kFuturePtsMs = 100;

    HJAudioInfo::Ptr output_info = std::make_shared<HJAudioInfo>();
    output_info->m_samplesRate = kSampleRate;
    output_info->setChannels(HJ_CHANNEL_DEFAULT);
    output_info->m_sampleFmt = HJ_SAMPLE_FORMAT_DEFAULT;
    output_info->m_sampleCnt = kFrameSamples;
    output_info->m_samplesPerFrame = kFrameSamples;

    auto graph = std::make_shared<HJGraph>("HJPluginAudioMixerFlushGraph");
    ASSERT_NE(graph, nullptr);

    auto graph_info = std::make_shared<HJKeyStorage>();
    (*graph_info)["graph"] = graph;

    HJAudioMixerConfig config{};
    config.m_output_info = output_info;
    config.m_max_inputs = 1;
    config.m_frame_samples = kFrameSamples;
    config.m_sync_window_ms = 0;
    config.m_late_threshold_ms = 200;
    config.m_enable_compand = false;
    config.m_enable_limiter = false;

    auto mixer = std::make_shared<HJTestAudioMixerPlugin>("HJPluginAudioMixerFlushTest", graph_info);
    auto source = std::make_shared<HJTestSourcePlugin>("HJPluginAudioMixerFlushSource");
    auto sink = std::make_shared<HJTestSinkPlugin>("HJPluginAudioMixerFlushSink");
    ASSERT_NE(mixer, nullptr);
    ASSERT_NE(source, nullptr);
    ASSERT_NE(sink, nullptr);

    std::vector<HJGraphAudioMixerInputState> input_states;
    auto input_state_token = graph->eventBus()->subscribe(EVENT_GRAPH_AUDIO_MIXER_INPUT_STATE_ID,
        [&input_states](const HJGraphAudioMixerInputState& i_state) -> HJRet {
            input_states.push_back(i_state);
            return HJ_OK;
        });
    ASSERT_NE(input_state_token, nullptr);

    auto init_param = std::make_shared<HJKeyStorage>();
    (*init_param)["createThread"] = false;
    (*init_param)["audioMixerConfig"] = config;
    ASSERT_EQ(mixer->init(init_param), HJ_OK);

    ASSERT_EQ(source->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addOutputPlugin(sink, HJMEDIA_TYPE_AUDIO), HJ_OK);

    HJAudioMixerInputDesc input_desc{};
    input_desc.m_input_id = "input0";
    input_desc.m_input_info = std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    ASSERT_NE(input_desc.m_input_info, nullptr);

    const size_t input_key_hash = buildPluginKeyHash(source->getName(), HJMEDIA_TYPE_AUDIO, 0);
    ASSERT_EQ(mixer->bindInput(0, input_key_hash, input_desc), HJ_OK);

    auto startup_frame = makeTestAudioFrame(output_info, 0);
    ASSERT_NE(startup_frame, nullptr);
    ASSERT_EQ(source->pushFrame(startup_frame), HJ_OK);

    int64_t delay_ms = 0;
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    auto future_frame = makeTestAudioFrame(output_info, kFuturePtsMs);
    ASSERT_NE(future_frame, nullptr);
    ASSERT_EQ(source->pushFrame(future_frame), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    ASSERT_FALSE(input_states.empty());
    EXPECT_TRUE(std::any_of(input_states.begin(), input_states.end(),
        [](const HJGraphAudioMixerInputState& i_state) {
            return i_state.m_input_id == "input0" && i_state.m_starved;
        }));
    EXPECT_EQ(mixer->queuedFrameCount(input_key_hash), 1u);
    EXPECT_EQ(mixer->previewPts(input_key_hash), kFuturePtsMs);

    ASSERT_EQ(mixer->flushInput(0), HJ_OK);
    ASSERT_FALSE(input_states.empty());
    const auto& last_state = input_states.back();
    EXPECT_EQ(last_state.m_input_id, "input0");
    EXPECT_TRUE(last_state.m_attached);
    EXPECT_FALSE(last_state.m_starved);
    EXPECT_FALSE(last_state.m_eof);
    EXPECT_EQ(mixer->queuedFrameCount(input_key_hash), 0u);
    EXPECT_EQ(mixer->previewPts(input_key_hash), HJ_NOTS_VALUE);

    graph->eventBus()->unSubscribe(EVENT_GRAPH_AUDIO_MIXER_INPUT_STATE_ID, input_state_token);
    EXPECT_EQ(mixer->done(), HJ_OK);
}

TEST(HJPluginAudioMixerTest, FlushInputAfterRebindPreservesTimelineAndJoinWait)
{
    constexpr int kSampleRate = 48000;
    constexpr int kFrameSamples = 1024;
    const int64_t kSecondPtsMs = samplesToPtsMs(kFrameSamples, kSampleRate);
    const int64_t kThirdPtsMs = samplesToPtsMs(kFrameSamples * 2, kSampleRate);
    const int64_t kFourthPtsMs = samplesToPtsMs(kFrameSamples * 3, kSampleRate);

    auto graph = std::make_shared<HJGraph>("HJPluginAudioMixerFlushRebindGraph");
    ASSERT_NE(graph, nullptr);

    auto graph_info = std::make_shared<HJKeyStorage>();
    (*graph_info)["graph"] = graph;

    HJAudioInfo::Ptr output_info = std::make_shared<HJAudioInfo>();
    output_info->m_samplesRate = kSampleRate;
    output_info->setChannels(HJ_CHANNEL_DEFAULT);
    output_info->m_sampleFmt = HJ_SAMPLE_FORMAT_DEFAULT;
    output_info->m_sampleCnt = kFrameSamples;
    output_info->m_samplesPerFrame = kFrameSamples;

    HJAudioMixerConfig config{};
    config.m_output_info = output_info;
    config.m_max_inputs = 2;
    config.m_frame_samples = kFrameSamples;
    config.m_sync_window_ms = 50;
    config.m_late_threshold_ms = 200;
    config.m_enable_compand = false;
    config.m_enable_limiter = false;

    auto mixer = std::make_shared<HJTestAudioMixerPlugin>(
        "HJPluginAudioMixerFlushRebindTest", graph_info);
    auto source0 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerFlushRebindSource0");
    auto source1 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerFlushRebindSource1");
    auto sink = std::make_shared<HJTestSinkPlugin>(
        "HJPluginAudioMixerFlushRebindSink");
    ASSERT_NE(mixer, nullptr);
    ASSERT_NE(source0, nullptr);
    ASSERT_NE(source1, nullptr);
    ASSERT_NE(sink, nullptr);

    std::vector<HJGraphAudioMixerStats> stats_events;
    auto stats_token = graph->eventBus()->subscribe(
        EVENT_GRAPH_AUDIO_MIXER_STATS_ID,
        [&stats_events](const HJGraphAudioMixerStats& i_stats) -> HJRet {
            stats_events.push_back(i_stats);
            return HJ_OK;
        });
    ASSERT_NE(stats_token, nullptr);

    auto init_param = std::make_shared<HJKeyStorage>();
    (*init_param)["createThread"] = false;
    (*init_param)["audioMixerConfig"] = config;
    ASSERT_EQ(mixer->init(init_param), HJ_OK);

    ASSERT_EQ(source0->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(source1->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source0, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source1, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addOutputPlugin(sink, HJMEDIA_TYPE_AUDIO), HJ_OK);

    HJAudioMixerInputDesc input_desc0{};
    input_desc0.m_input_id = "input0";
    input_desc0.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    HJAudioMixerInputDesc input_desc1{};
    input_desc1.m_input_id = "input1";
    input_desc1.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    ASSERT_NE(input_desc0.m_input_info, nullptr);
    ASSERT_NE(input_desc1.m_input_info, nullptr);

    const size_t input0_key_hash =
        buildPluginKeyHash(source0->getName(), HJMEDIA_TYPE_AUDIO, 0);
    const size_t input1_key_hash =
        buildPluginKeyHash(source1->getName(), HJMEDIA_TYPE_AUDIO, 0);
    ASSERT_EQ(mixer->bindInput(0, input0_key_hash, input_desc0), HJ_OK);
    ASSERT_EQ(mixer->bindInput(1, input1_key_hash, input_desc1), HJ_OK);

    auto first_frame0 = makeTestAudioFrame(output_info, 0);
    auto first_frame1 = makeTestAudioFrame(output_info, 0);
    auto second_frame0 = makeTestAudioFrame(output_info, kSecondPtsMs);
    auto third_frame0 = makeTestAudioFrame(output_info, kThirdPtsMs);
    auto fourth_frame1 = makeTestAudioFrame(output_info, kFourthPtsMs);
    ASSERT_NE(first_frame0, nullptr);
    ASSERT_NE(first_frame1, nullptr);
    ASSERT_NE(second_frame0, nullptr);
    ASSERT_NE(third_frame0, nullptr);
    ASSERT_NE(fourth_frame1, nullptr);

    ASSERT_EQ(source0->pushFrame(first_frame0), HJ_OK);
    ASSERT_EQ(source1->pushFrame(first_frame1), HJ_OK);

    int64_t delay_ms = 0;
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(sink->getFrames().size(), 1u);

    ASSERT_EQ(mixer->unbindInput(1), HJ_OK);
    ASSERT_EQ(source0->pushFrame(second_frame0), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(sink->getFrames().size(), 2u);
    ASSERT_EQ(sink->getFrames()[1]->getPTS(), kSecondPtsMs);

    ASSERT_EQ(mixer->bindInput(1, input1_key_hash, input_desc1), HJ_OK);
    ASSERT_EQ(source1->pushFrame(fourth_frame1), HJ_OK);
    EXPECT_EQ(mixer->runOnce(&delay_ms), HJ_WOULD_BLOCK);
    EXPECT_EQ(mixer->queuedFrameCount(input1_key_hash), 1u);
    EXPECT_EQ(mixer->previewPts(input1_key_hash), kFourthPtsMs);

    ASSERT_EQ(mixer->flushInput(1), HJ_OK);
    EXPECT_EQ(mixer->queuedFrameCount(input1_key_hash), 0u);
    EXPECT_EQ(mixer->previewPts(input1_key_hash), HJ_NOTS_VALUE);

    ASSERT_EQ(source0->pushFrame(third_frame0), HJ_OK);
    EXPECT_EQ(mixer->runOnce(&delay_ms), HJ_WOULD_BLOCK);
    EXPECT_EQ(sink->getFrames().size(), 2u);
    EXPECT_EQ(mixer->previewPts(input0_key_hash), kThirdPtsMs);

    ASSERT_EQ(source1->pushFrame(fourth_frame1), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    const auto& out_frames = sink->getFrames();
    ASSERT_EQ(out_frames.size(), 3u);
    ASSERT_NE(out_frames[2], nullptr);
    EXPECT_EQ(out_frames[2]->getPTS(), kThirdPtsMs);
    ASSERT_FALSE(stats_events.empty());
    EXPECT_EQ(stats_events.back().m_output_frames, 3u);
    EXPECT_EQ(stats_events.back().m_mix_pts, kFourthPtsMs);

    graph->eventBus()->unSubscribe(EVENT_GRAPH_AUDIO_MIXER_STATS_ID, stats_token);
    EXPECT_EQ(mixer->done(), HJ_OK);
}

TEST(HJPluginAudioMixerTest, ClearFrameAfterRebindResetsTimelineAndOutputCounter)
{
    constexpr int kSampleRate = 48000;
    constexpr int kFrameSamples = 1024;
    const int64_t kSecondPtsMs = samplesToPtsMs(kFrameSamples, kSampleRate);
    const int64_t kFourthPtsMs = samplesToPtsMs(kFrameSamples * 3, kSampleRate);

    auto graph = std::make_shared<HJGraph>("HJPluginAudioMixerClearRebindGraph");
    ASSERT_NE(graph, nullptr);

    auto graph_info = std::make_shared<HJKeyStorage>();
    (*graph_info)["graph"] = graph;

    HJAudioInfo::Ptr output_info = std::make_shared<HJAudioInfo>();
    output_info->m_samplesRate = kSampleRate;
    output_info->setChannels(HJ_CHANNEL_DEFAULT);
    output_info->m_sampleFmt = HJ_SAMPLE_FORMAT_DEFAULT;
    output_info->m_sampleCnt = kFrameSamples;
    output_info->m_samplesPerFrame = kFrameSamples;

    HJAudioMixerConfig config{};
    config.m_output_info = output_info;
    config.m_max_inputs = 2;
    config.m_frame_samples = kFrameSamples;
    config.m_sync_window_ms = 50;
    config.m_late_threshold_ms = 200;
    config.m_enable_compand = false;
    config.m_enable_limiter = false;

    auto mixer = std::make_shared<HJTestAudioMixerPlugin>(
        "HJPluginAudioMixerClearRebindTest", graph_info);
    auto source0 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerClearRebindSource0");
    auto source1 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerClearRebindSource1");
    auto sink = std::make_shared<HJTestSinkPlugin>(
        "HJPluginAudioMixerClearRebindSink");
    ASSERT_NE(mixer, nullptr);
    ASSERT_NE(source0, nullptr);
    ASSERT_NE(source1, nullptr);
    ASSERT_NE(sink, nullptr);

    std::vector<HJGraphAudioMixerStats> stats_events;
    auto stats_token = graph->eventBus()->subscribe(
        EVENT_GRAPH_AUDIO_MIXER_STATS_ID,
        [&stats_events](const HJGraphAudioMixerStats& i_stats) -> HJRet {
            stats_events.push_back(i_stats);
            return HJ_OK;
        });
    ASSERT_NE(stats_token, nullptr);

    auto init_param = std::make_shared<HJKeyStorage>();
    (*init_param)["createThread"] = false;
    (*init_param)["audioMixerConfig"] = config;
    ASSERT_EQ(mixer->init(init_param), HJ_OK);

    ASSERT_EQ(source0->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(source1->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source0, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source1, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addOutputPlugin(sink, HJMEDIA_TYPE_AUDIO), HJ_OK);

    HJAudioMixerInputDesc input_desc0{};
    input_desc0.m_input_id = "input0";
    input_desc0.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    HJAudioMixerInputDesc input_desc1{};
    input_desc1.m_input_id = "input1";
    input_desc1.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    ASSERT_NE(input_desc0.m_input_info, nullptr);
    ASSERT_NE(input_desc1.m_input_info, nullptr);

    const size_t input0_key_hash =
        buildPluginKeyHash(source0->getName(), HJMEDIA_TYPE_AUDIO, 0);
    const size_t input1_key_hash =
        buildPluginKeyHash(source1->getName(), HJMEDIA_TYPE_AUDIO, 0);
    ASSERT_EQ(mixer->bindInput(0, input0_key_hash, input_desc0), HJ_OK);
    ASSERT_EQ(mixer->bindInput(1, input1_key_hash, input_desc1), HJ_OK);

    auto first_frame0 = makeTestAudioFrame(output_info, 0);
    auto first_frame1 = makeTestAudioFrame(output_info, 0);
    auto second_frame0 = makeTestAudioFrame(output_info, kSecondPtsMs);
    auto second_frame1 = makeTestAudioFrame(output_info, kSecondPtsMs);
    auto fourth_frame1 = makeTestAudioFrame(output_info, kFourthPtsMs);
    auto restart_frame0 = makeTestAudioFrame(output_info, 0);
    auto restart_frame1 = makeTestAudioFrame(output_info, 0);
    ASSERT_NE(first_frame0, nullptr);
    ASSERT_NE(first_frame1, nullptr);
    ASSERT_NE(second_frame0, nullptr);
    ASSERT_NE(second_frame1, nullptr);
    ASSERT_NE(fourth_frame1, nullptr);
    ASSERT_NE(restart_frame0, nullptr);
    ASSERT_NE(restart_frame1, nullptr);

    ASSERT_EQ(source0->pushFrame(first_frame0), HJ_OK);
    ASSERT_EQ(source1->pushFrame(first_frame1), HJ_OK);

    int64_t delay_ms = 0;
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    ASSERT_EQ(source0->pushFrame(second_frame0), HJ_OK);
    ASSERT_EQ(source1->pushFrame(second_frame1), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(sink->getFrames().size(), 2u);
    ASSERT_EQ(sink->getFrames()[1]->getPTS(), kSecondPtsMs);

    ASSERT_EQ(mixer->unbindInput(1), HJ_OK);
    ASSERT_EQ(mixer->bindInput(1, input1_key_hash, input_desc1), HJ_OK);
    ASSERT_EQ(source1->pushFrame(fourth_frame1), HJ_OK);
    EXPECT_EQ(mixer->runOnce(&delay_ms), HJ_WOULD_BLOCK);
    EXPECT_EQ(mixer->queuedFrameCount(input1_key_hash), 1u);
    EXPECT_EQ(mixer->previewPts(input1_key_hash), kFourthPtsMs);

    ASSERT_EQ(mixer->flush(input1_key_hash), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    EXPECT_EQ(mixer->queuedFrameCount(input1_key_hash), 0u);
    EXPECT_EQ(mixer->previewPts(input1_key_hash), HJ_NOTS_VALUE);

    ASSERT_EQ(source0->pushFrame(restart_frame0), HJ_OK);
    ASSERT_EQ(source1->pushFrame(restart_frame1), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    const auto& out_frames = sink->getFrames();
    ASSERT_EQ(out_frames.size(), 3u);
    ASSERT_NE(out_frames[2], nullptr);
    EXPECT_EQ(out_frames[2]->getPTS(), 0);
    ASSERT_FALSE(stats_events.empty());
    EXPECT_EQ(stats_events.back().m_output_frames, 1u);
    EXPECT_EQ(stats_events.back().m_mix_pts, kSecondPtsMs);

    graph->eventBus()->unSubscribe(EVENT_GRAPH_AUDIO_MIXER_STATS_ID, stats_token);
    EXPECT_EQ(mixer->done(), HJ_OK);
}

TEST(HJPluginAudioMixerTest, RebindInputEofDoesNotBlockExistingSteadyMix)
{
    constexpr int kSampleRate = 48000;
    constexpr int kFrameSamples = 1024;
    const int64_t kSecondPtsMs = samplesToPtsMs(kFrameSamples, kSampleRate);
    const int64_t kThirdPtsMs = samplesToPtsMs(kFrameSamples * 2, kSampleRate);

    auto graph = std::make_shared<HJGraph>("HJPluginAudioMixerRebindEofGraph");
    ASSERT_NE(graph, nullptr);

    auto graph_info = std::make_shared<HJKeyStorage>();
    (*graph_info)["graph"] = graph;

    HJAudioInfo::Ptr output_info = std::make_shared<HJAudioInfo>();
    output_info->m_samplesRate = kSampleRate;
    output_info->setChannels(HJ_CHANNEL_DEFAULT);
    output_info->m_sampleFmt = HJ_SAMPLE_FORMAT_DEFAULT;
    output_info->m_sampleCnt = kFrameSamples;
    output_info->m_samplesPerFrame = kFrameSamples;

    HJAudioMixerConfig config{};
    config.m_output_info = output_info;
    config.m_max_inputs = 2;
    config.m_frame_samples = kFrameSamples;
    config.m_sync_window_ms = 50;
    config.m_late_threshold_ms = 200;
    config.m_enable_compand = false;
    config.m_enable_limiter = false;

    auto mixer = std::make_shared<HJTestAudioMixerPlugin>(
        "HJPluginAudioMixerRebindEofTest", graph_info);
    auto source0 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerRebindEofSource0");
    auto source1 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerRebindEofSource1");
    auto sink = std::make_shared<HJTestSinkPlugin>(
        "HJPluginAudioMixerRebindEofSink");
    ASSERT_NE(mixer, nullptr);
    ASSERT_NE(source0, nullptr);
    ASSERT_NE(source1, nullptr);
    ASSERT_NE(sink, nullptr);

    std::vector<HJGraphAudioMixerInputState> input_states;
    auto input_state_token = graph->eventBus()->subscribe(
        EVENT_GRAPH_AUDIO_MIXER_INPUT_STATE_ID,
        [&input_states](const HJGraphAudioMixerInputState& i_state) -> HJRet {
            input_states.push_back(i_state);
            return HJ_OK;
        });
    ASSERT_NE(input_state_token, nullptr);

    auto init_param = std::make_shared<HJKeyStorage>();
    (*init_param)["createThread"] = false;
    (*init_param)["audioMixerConfig"] = config;
    ASSERT_EQ(mixer->init(init_param), HJ_OK);

    ASSERT_EQ(source0->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(source1->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source0, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source1, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addOutputPlugin(sink, HJMEDIA_TYPE_AUDIO), HJ_OK);

    HJAudioMixerInputDesc input_desc0{};
    input_desc0.m_input_id = "input0";
    input_desc0.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    HJAudioMixerInputDesc input_desc1{};
    input_desc1.m_input_id = "input1";
    input_desc1.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    ASSERT_NE(input_desc0.m_input_info, nullptr);
    ASSERT_NE(input_desc1.m_input_info, nullptr);

    const size_t input0_key_hash =
        buildPluginKeyHash(source0->getName(), HJMEDIA_TYPE_AUDIO, 0);
    const size_t input1_key_hash =
        buildPluginKeyHash(source1->getName(), HJMEDIA_TYPE_AUDIO, 0);
    ASSERT_EQ(mixer->bindInput(0, input0_key_hash, input_desc0), HJ_OK);
    ASSERT_EQ(mixer->bindInput(1, input1_key_hash, input_desc1), HJ_OK);

    auto first_frame0 = makeTestAudioFrame(output_info, 0);
    auto first_frame1 = makeTestAudioFrame(output_info, 0);
    auto second_frame0 = makeTestAudioFrame(output_info, kSecondPtsMs);
    auto third_frame0 = makeTestAudioFrame(output_info, kThirdPtsMs);
    auto eof_frame1 = HJMediaFrame::makeEofFrame(HJMEDIA_TYPE_AUDIO);
    ASSERT_NE(first_frame0, nullptr);
    ASSERT_NE(first_frame1, nullptr);
    ASSERT_NE(second_frame0, nullptr);
    ASSERT_NE(third_frame0, nullptr);
    ASSERT_NE(eof_frame1, nullptr);

    ASSERT_EQ(source0->pushFrame(first_frame0), HJ_OK);
    ASSERT_EQ(source1->pushFrame(first_frame1), HJ_OK);

    int64_t delay_ms = 0;
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(sink->getFrames().size(), 1u);

    ASSERT_EQ(mixer->unbindInput(1), HJ_OK);
    ASSERT_EQ(source0->pushFrame(second_frame0), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(sink->getFrames().size(), 2u);
    ASSERT_EQ(sink->getFrames()[1]->getPTS(), kSecondPtsMs);

    ASSERT_EQ(mixer->bindInput(1, input1_key_hash, input_desc1), HJ_OK);
    ASSERT_EQ(source1->pushFrame(eof_frame1), HJ_OK);
    ASSERT_EQ(source0->pushFrame(third_frame0), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    const auto& out_frames = sink->getFrames();
    ASSERT_EQ(out_frames.size(), 3u);
    ASSERT_NE(out_frames[2], nullptr);
    EXPECT_EQ(out_frames[2]->getPTS(), kThirdPtsMs);
    EXPECT_EQ(mixer->queuedFrameCount(input1_key_hash), 0u);
    EXPECT_EQ(mixer->previewPts(input1_key_hash), HJ_NOTS_VALUE);
    EXPECT_TRUE(std::any_of(input_states.begin(), input_states.end(),
        [](const HJGraphAudioMixerInputState& i_state) {
            return i_state.m_input_id == "input1" && i_state.m_eof;
        }));

    graph->eventBus()->unSubscribe(EVENT_GRAPH_AUDIO_MIXER_INPUT_STATE_ID, input_state_token);
    EXPECT_EQ(mixer->done(), HJ_OK);
}

TEST(HJPluginAudioMixerTest, ClearThenSingleInputEofDoesNotBlockRestartedDualInputMix)
{
    constexpr int kSampleRate = 48000;
    constexpr int kFrameSamples = 1024;
    const int64_t kSecondPtsMs = samplesToPtsMs(kFrameSamples, kSampleRate);
    const int64_t kFutureJoinPtsMs = samplesToPtsMs(kFrameSamples * 5, kSampleRate);

    auto graph = std::make_shared<HJGraph>("HJPluginAudioMixerClearEofRestartGraph");
    ASSERT_NE(graph, nullptr);

    auto graph_info = std::make_shared<HJKeyStorage>();
    (*graph_info)["graph"] = graph;

    HJAudioInfo::Ptr output_info = std::make_shared<HJAudioInfo>();
    output_info->m_samplesRate = kSampleRate;
    output_info->setChannels(HJ_CHANNEL_DEFAULT);
    output_info->m_sampleFmt = HJ_SAMPLE_FORMAT_DEFAULT;
    output_info->m_sampleCnt = kFrameSamples;
    output_info->m_samplesPerFrame = kFrameSamples;

    HJAudioMixerConfig config{};
    config.m_output_info = output_info;
    config.m_max_inputs = 2;
    config.m_frame_samples = kFrameSamples;
    config.m_sync_window_ms = 50;
    config.m_late_threshold_ms = 200;
    config.m_enable_compand = false;
    config.m_enable_limiter = false;

    auto mixer = std::make_shared<HJTestAudioMixerPlugin>(
        "HJPluginAudioMixerClearEofRestartTest", graph_info);
    auto source0 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerClearEofRestartSource0");
    auto source1 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerClearEofRestartSource1");
    auto sink = std::make_shared<HJTestSinkPlugin>(
        "HJPluginAudioMixerClearEofRestartSink");
    ASSERT_NE(mixer, nullptr);
    ASSERT_NE(source0, nullptr);
    ASSERT_NE(source1, nullptr);
    ASSERT_NE(sink, nullptr);

    auto init_param = std::make_shared<HJKeyStorage>();
    (*init_param)["createThread"] = false;
    (*init_param)["audioMixerConfig"] = config;
    ASSERT_EQ(mixer->init(init_param), HJ_OK);

    ASSERT_EQ(source0->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(source1->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source0, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source1, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addOutputPlugin(sink, HJMEDIA_TYPE_AUDIO), HJ_OK);

    HJAudioMixerInputDesc input_desc0{};
    input_desc0.m_input_id = "input0";
    input_desc0.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    HJAudioMixerInputDesc input_desc1{};
    input_desc1.m_input_id = "input1";
    input_desc1.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    ASSERT_NE(input_desc0.m_input_info, nullptr);
    ASSERT_NE(input_desc1.m_input_info, nullptr);

    const size_t input0_key_hash =
        buildPluginKeyHash(source0->getName(), HJMEDIA_TYPE_AUDIO, 0);
    const size_t input1_key_hash =
        buildPluginKeyHash(source1->getName(), HJMEDIA_TYPE_AUDIO, 0);
    ASSERT_EQ(mixer->bindInput(0, input0_key_hash, input_desc0), HJ_OK);
    ASSERT_EQ(mixer->bindInput(1, input1_key_hash, input_desc1), HJ_OK);

    auto first_frame0 = makeTestAudioFrame(output_info, 0);
    auto first_frame1 = makeTestAudioFrame(output_info, 0);
    auto second_frame0 = makeTestAudioFrame(output_info, kSecondPtsMs);
    auto restart_frame0 = makeTestAudioFrame(output_info, 0);
    auto restart_second_frame0 = makeTestAudioFrame(output_info, kSecondPtsMs);
    auto restart_second_frame1 = makeTestAudioFrame(output_info, kSecondPtsMs);
    auto future_frame1 = makeTestAudioFrame(output_info, kFutureJoinPtsMs);
    auto eof_frame1 = HJMediaFrame::makeEofFrame(HJMEDIA_TYPE_AUDIO);
    ASSERT_NE(first_frame0, nullptr);
    ASSERT_NE(first_frame1, nullptr);
    ASSERT_NE(second_frame0, nullptr);
    ASSERT_NE(restart_frame0, nullptr);
    ASSERT_NE(restart_second_frame0, nullptr);
    ASSERT_NE(restart_second_frame1, nullptr);
    ASSERT_NE(future_frame1, nullptr);
    ASSERT_NE(eof_frame1, nullptr);

    ASSERT_EQ(source0->pushFrame(first_frame0), HJ_OK);
    ASSERT_EQ(source1->pushFrame(first_frame1), HJ_OK);

    int64_t delay_ms = 0;
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(sink->getFrames().size(), 1u);

    ASSERT_EQ(source0->pushFrame(second_frame0), HJ_OK);
    ASSERT_EQ(mixer->unbindInput(1), HJ_OK);
    ASSERT_EQ(mixer->bindInput(1, input1_key_hash, input_desc1), HJ_OK);
    ASSERT_EQ(source1->pushFrame(future_frame1), HJ_OK);
    EXPECT_EQ(mixer->runOnce(&delay_ms), HJ_WOULD_BLOCK);
    EXPECT_EQ(mixer->queuedFrameCount(input1_key_hash), 1u);
    EXPECT_EQ(mixer->previewPts(input1_key_hash), kFutureJoinPtsMs);

    ASSERT_EQ(mixer->flush(input1_key_hash), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    ASSERT_EQ(source1->pushFrame(eof_frame1), HJ_OK);
    ASSERT_EQ(source0->pushFrame(restart_frame0), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    ASSERT_EQ(source0->pushFrame(restart_second_frame0), HJ_OK);
    ASSERT_EQ(source1->pushFrame(restart_second_frame1), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    const auto& out_frames = sink->getFrames();
    ASSERT_EQ(out_frames.size(), 3u);
    ASSERT_NE(out_frames[1], nullptr);
    ASSERT_NE(out_frames[2], nullptr);
    EXPECT_EQ(out_frames[1]->getPTS(), 0);
    EXPECT_EQ(out_frames[2]->getPTS(), kSecondPtsMs);
    EXPECT_EQ(mixer->queuedFrameCount(input0_key_hash), 0u);
    EXPECT_EQ(mixer->queuedFrameCount(input1_key_hash), 0u);
    EXPECT_EQ(mixer->previewPts(input0_key_hash), HJ_NOTS_VALUE);
    EXPECT_EQ(mixer->previewPts(input1_key_hash), HJ_NOTS_VALUE);

    EXPECT_EQ(mixer->done(), HJ_OK);
}

TEST(HJPluginAudioMixerTest,
     ClearFrameFlushesOtherQueuedStaleAndFutureFrames)
{
    constexpr int kSampleRate = 48000;
    constexpr int kFrameSamples = 1024;
    const int64_t kSecondPtsMs = samplesToPtsMs(kFrameSamples, kSampleRate);
    const int64_t kFuturePtsMs = samplesToPtsMs(kFrameSamples * 5, kSampleRate);

    HJAudioInfo::Ptr output_info = std::make_shared<HJAudioInfo>();
    output_info->m_samplesRate = kSampleRate;
    output_info->setChannels(HJ_CHANNEL_DEFAULT);
    output_info->m_sampleFmt = HJ_SAMPLE_FORMAT_DEFAULT;
    output_info->m_sampleCnt = kFrameSamples;
    output_info->m_samplesPerFrame = kFrameSamples;

    HJAudioMixerConfig config{};
    config.m_output_info = output_info;
    config.m_max_inputs = 2;
    config.m_frame_samples = kFrameSamples;
    config.m_sync_window_ms = 50;
    config.m_late_threshold_ms = 200;
    config.m_enable_compand = false;
    config.m_enable_limiter = false;

    auto mixer = std::make_shared<HJTestAudioMixerPlugin>(
        "HJPluginAudioMixerClearQueueResetTest");
    auto source0 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerClearQueueResetSource0");
    auto source1 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerClearQueueResetSource1");
    auto sink = std::make_shared<HJTestSinkPlugin>(
        "HJPluginAudioMixerClearQueueResetSink");
    ASSERT_NE(mixer, nullptr);
    ASSERT_NE(source0, nullptr);
    ASSERT_NE(source1, nullptr);
    ASSERT_NE(sink, nullptr);

    auto init_param = std::make_shared<HJKeyStorage>();
    (*init_param)["createThread"] = false;
    (*init_param)["audioMixerConfig"] = config;
    ASSERT_EQ(mixer->init(init_param), HJ_OK);

    ASSERT_EQ(source0->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(source1->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source0, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source1, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addOutputPlugin(sink, HJMEDIA_TYPE_AUDIO), HJ_OK);

    HJAudioMixerInputDesc input_desc0{};
    input_desc0.m_input_id = "input0";
    input_desc0.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    HJAudioMixerInputDesc input_desc1{};
    input_desc1.m_input_id = "input1";
    input_desc1.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    ASSERT_NE(input_desc0.m_input_info, nullptr);
    ASSERT_NE(input_desc1.m_input_info, nullptr);

    const size_t input0_key_hash =
        buildPluginKeyHash(source0->getName(), HJMEDIA_TYPE_AUDIO, 0);
    const size_t input1_key_hash =
        buildPluginKeyHash(source1->getName(), HJMEDIA_TYPE_AUDIO, 0);
    ASSERT_EQ(mixer->bindInput(0, input0_key_hash, input_desc0), HJ_OK);
    ASSERT_EQ(mixer->bindInput(1, input1_key_hash, input_desc1), HJ_OK);

    auto first_frame0 = makeTestAudioFrame(output_info, 0);
    auto first_frame1 = makeTestAudioFrame(output_info, 0);
    auto stale_frame0 = makeTestAudioFrame(output_info, 0);
    auto future_frame0 = makeTestAudioFrame(output_info, kFuturePtsMs);
    auto restart_frame0 = makeTestAudioFrame(output_info, 0);
    auto restart_frame1 = makeTestAudioFrame(output_info, 0);
    ASSERT_NE(first_frame0, nullptr);
    ASSERT_NE(first_frame1, nullptr);
    ASSERT_NE(stale_frame0, nullptr);
    ASSERT_NE(future_frame0, nullptr);
    ASSERT_NE(restart_frame0, nullptr);
    ASSERT_NE(restart_frame1, nullptr);

    ASSERT_EQ(source0->pushFrame(first_frame0), HJ_OK);
    ASSERT_EQ(source1->pushFrame(first_frame1), HJ_OK);

    int64_t delay_ms = 0;
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(sink->getFrames().size(), 1u);

    ASSERT_EQ(source0->pushFrame(stale_frame0), HJ_OK);
    ASSERT_EQ(source0->pushFrame(future_frame0), HJ_OK);
    ASSERT_EQ(mixer->flush(input1_key_hash), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    EXPECT_EQ(mixer->queuedFrameCount(input0_key_hash), 0u);
    EXPECT_EQ(mixer->queuedFrameCount(input1_key_hash), 0u);
    EXPECT_EQ(mixer->previewPts(input0_key_hash), HJ_NOTS_VALUE);
    EXPECT_EQ(mixer->previewPts(input1_key_hash), HJ_NOTS_VALUE);

    ASSERT_EQ(source0->pushFrame(restart_frame0), HJ_OK);
    ASSERT_EQ(source1->pushFrame(restart_frame1), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    const auto& out_frames = sink->getFrames();
    ASSERT_EQ(out_frames.size(), 2u);
    ASSERT_NE(out_frames[1], nullptr);
    EXPECT_EQ(out_frames[1]->getPTS(), 0);

    EXPECT_EQ(mixer->done(), HJ_OK);
}

TEST(HJPluginAudioMixerTest,
     ZeroLateThresholdClearRestartStillMixesImmediately)
{
    constexpr int kSampleRate = 48000;
    constexpr int kFrameSamples = 1024;
    const int64_t kSecondPtsMs = samplesToPtsMs(kFrameSamples, kSampleRate);

    HJAudioInfo::Ptr output_info = std::make_shared<HJAudioInfo>();
    output_info->m_samplesRate = kSampleRate;
    output_info->setChannels(HJ_CHANNEL_DEFAULT);
    output_info->m_sampleFmt = HJ_SAMPLE_FORMAT_DEFAULT;
    output_info->m_sampleCnt = kFrameSamples;
    output_info->m_samplesPerFrame = kFrameSamples;

    HJAudioMixerConfig config{};
    config.m_output_info = output_info;
    config.m_max_inputs = 2;
    config.m_frame_samples = kFrameSamples;
    config.m_sync_window_ms = 50;
    config.m_late_threshold_ms = 0;
    config.m_enable_compand = false;
    config.m_enable_limiter = false;

    auto mixer = std::make_shared<HJTestAudioMixerPlugin>(
        "HJPluginAudioMixerZeroLateClearRestartTest");
    auto source0 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerZeroLateClearRestartSource0");
    auto source1 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerZeroLateClearRestartSource1");
    auto sink = std::make_shared<HJTestSinkPlugin>(
        "HJPluginAudioMixerZeroLateClearRestartSink");
    ASSERT_NE(mixer, nullptr);
    ASSERT_NE(source0, nullptr);
    ASSERT_NE(source1, nullptr);
    ASSERT_NE(sink, nullptr);

    auto init_param = std::make_shared<HJKeyStorage>();
    (*init_param)["createThread"] = false;
    (*init_param)["audioMixerConfig"] = config;
    ASSERT_EQ(mixer->init(init_param), HJ_OK);

    ASSERT_EQ(source0->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(source1->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source0, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source1, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addOutputPlugin(sink, HJMEDIA_TYPE_AUDIO), HJ_OK);

    HJAudioMixerInputDesc input_desc0{};
    input_desc0.m_input_id = "input0";
    input_desc0.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    HJAudioMixerInputDesc input_desc1{};
    input_desc1.m_input_id = "input1";
    input_desc1.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    ASSERT_NE(input_desc0.m_input_info, nullptr);
    ASSERT_NE(input_desc1.m_input_info, nullptr);

    const size_t input0_key_hash =
        buildPluginKeyHash(source0->getName(), HJMEDIA_TYPE_AUDIO, 0);
    const size_t input1_key_hash =
        buildPluginKeyHash(source1->getName(), HJMEDIA_TYPE_AUDIO, 0);
    ASSERT_EQ(mixer->bindInput(0, input0_key_hash, input_desc0), HJ_OK);
    ASSERT_EQ(mixer->bindInput(1, input1_key_hash, input_desc1), HJ_OK);

    auto first_frame0 = makeTestAudioFrame(output_info, 0);
    auto first_frame1 = makeTestAudioFrame(output_info, 0);
    auto second_frame0 = makeTestAudioFrame(output_info, kSecondPtsMs);
    auto restart_frame0 = makeTestAudioFrame(output_info, 0);
    ASSERT_NE(first_frame0, nullptr);
    ASSERT_NE(first_frame1, nullptr);
    ASSERT_NE(second_frame0, nullptr);
    ASSERT_NE(restart_frame0, nullptr);

    ASSERT_EQ(source0->pushFrame(first_frame0), HJ_OK);
    ASSERT_EQ(source1->pushFrame(first_frame1), HJ_OK);

    int64_t delay_ms = 0;
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(sink->getFrames().size(), 1u);

    ASSERT_EQ(source0->pushFrame(second_frame0), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(sink->getFrames().size(), 2u);
    ASSERT_NE(sink->getFrames()[1], nullptr);
    EXPECT_EQ(sink->getFrames()[1]->getPTS(), kSecondPtsMs);

    ASSERT_EQ(mixer->flush(input1_key_hash), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    ASSERT_EQ(source0->pushFrame(restart_frame0), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    const auto& out_frames = sink->getFrames();
    ASSERT_EQ(out_frames.size(), 3u);
    ASSERT_NE(out_frames[2], nullptr);
    EXPECT_EQ(out_frames[2]->getPTS(), 0);

    EXPECT_EQ(mixer->done(), HJ_OK);
}

TEST(HJPluginAudioMixerTest,
     ClearResetMutedInputRestoredVolumeRejoinsJoinWait)
{
    constexpr int kSampleRate = 48000;
    constexpr int kFrameSamples = 1024;
    const int64_t kSecondPtsMs = samplesToPtsMs(kFrameSamples, kSampleRate);
    const int64_t kThirdPtsMs = samplesToPtsMs(kFrameSamples * 2, kSampleRate);

    HJAudioInfo::Ptr output_info = std::make_shared<HJAudioInfo>();
    output_info->m_samplesRate = kSampleRate;
    output_info->setChannels(HJ_CHANNEL_DEFAULT);
    output_info->m_sampleFmt = HJ_SAMPLE_FORMAT_DEFAULT;
    output_info->m_sampleCnt = kFrameSamples;
    output_info->m_samplesPerFrame = kFrameSamples;

    HJAudioMixerConfig config{};
    config.m_output_info = output_info;
    config.m_max_inputs = 2;
    config.m_frame_samples = kFrameSamples;
    config.m_sync_window_ms = 50;
    config.m_late_threshold_ms = 200;
    config.m_enable_compand = false;
    config.m_enable_limiter = false;

    auto mixer = std::make_shared<HJTestAudioMixerPlugin>(
        "HJPluginAudioMixerClearMutedRejoinTest");
    auto source0 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerClearMutedRejoinSource0");
    auto source1 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerClearMutedRejoinSource1");
    auto sink = std::make_shared<HJTestSinkPlugin>(
        "HJPluginAudioMixerClearMutedRejoinSink");
    ASSERT_NE(mixer, nullptr);
    ASSERT_NE(source0, nullptr);
    ASSERT_NE(source1, nullptr);
    ASSERT_NE(sink, nullptr);

    auto init_param = std::make_shared<HJKeyStorage>();
    (*init_param)["createThread"] = false;
    (*init_param)["audioMixerConfig"] = config;
    ASSERT_EQ(mixer->init(init_param), HJ_OK);

    ASSERT_EQ(source0->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(source1->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source0, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source1, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addOutputPlugin(sink, HJMEDIA_TYPE_AUDIO), HJ_OK);

    HJAudioMixerInputDesc input_desc0{};
    input_desc0.m_input_id = "input0";
    input_desc0.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    HJAudioMixerInputDesc input_desc1{};
    input_desc1.m_input_id = "input1";
    input_desc1.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    input_desc1.m_volume = 0.0f;
    ASSERT_NE(input_desc0.m_input_info, nullptr);
    ASSERT_NE(input_desc1.m_input_info, nullptr);

    const size_t input0_key_hash =
        buildPluginKeyHash(source0->getName(), HJMEDIA_TYPE_AUDIO, 0);
    const size_t input1_key_hash =
        buildPluginKeyHash(source1->getName(), HJMEDIA_TYPE_AUDIO, 0);
    ASSERT_EQ(mixer->bindInput(0, input0_key_hash, input_desc0), HJ_OK);
    ASSERT_EQ(mixer->bindInput(1, input1_key_hash, input_desc1), HJ_OK);

    auto first_frame0 = makeTestAudioFrame(output_info, 0);
    auto restart_frame0 = makeTestAudioFrame(output_info, 0);
    auto second_frame0 = makeTestAudioFrame(output_info, kSecondPtsMs);
    auto third_frame1 = makeTestAudioFrame(output_info, kThirdPtsMs);
    ASSERT_NE(first_frame0, nullptr);
    ASSERT_NE(restart_frame0, nullptr);
    ASSERT_NE(second_frame0, nullptr);
    ASSERT_NE(third_frame1, nullptr);

    ASSERT_EQ(source0->pushFrame(first_frame0), HJ_OK);

    int64_t delay_ms = 0;
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(sink->getFrames().size(), 1u);

    ASSERT_EQ(mixer->flush(input1_key_hash), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    ASSERT_EQ(source0->pushFrame(restart_frame0), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(sink->getFrames().size(), 2u);
    ASSERT_NE(sink->getFrames()[1], nullptr);
    EXPECT_EQ(sink->getFrames()[1]->getPTS(), 0);

    ASSERT_EQ(mixer->setInputVolume(1, 1.0f), HJ_OK);
    ASSERT_EQ(source1->pushFrame(third_frame1), HJ_OK);
    EXPECT_EQ(mixer->runOnce(&delay_ms), HJ_WOULD_BLOCK);
    EXPECT_EQ(sink->getFrames().size(), 2u);
    EXPECT_EQ(mixer->queuedFrameCount(input1_key_hash), 1u);
    EXPECT_EQ(mixer->previewPts(input1_key_hash), kThirdPtsMs);

    ASSERT_EQ(source0->pushFrame(second_frame0), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    const auto& out_frames = sink->getFrames();
    ASSERT_EQ(out_frames.size(), 3u);
    ASSERT_NE(out_frames[2], nullptr);
    EXPECT_EQ(out_frames[2]->getPTS(), kSecondPtsMs);
    EXPECT_EQ(mixer->queuedFrameCount(input1_key_hash), 0u);
    EXPECT_EQ(mixer->previewPts(input1_key_hash), HJ_NOTS_VALUE);

    EXPECT_EQ(mixer->done(), HJ_OK);
}

TEST(HJPluginAudioMixerTest,
     ClearThenDualEofDoesNotRepublishStaleMixerStats)
{
    constexpr int kSampleRate = 48000;
    constexpr int kFrameSamples = 1024;

    auto graph = std::make_shared<HJGraph>(
        "HJPluginAudioMixerClearDualEofEventsGraph");
    ASSERT_NE(graph, nullptr);

    auto graph_info = std::make_shared<HJKeyStorage>();
    (*graph_info)["graph"] = graph;

    HJAudioInfo::Ptr output_info = std::make_shared<HJAudioInfo>();
    output_info->m_samplesRate = kSampleRate;
    output_info->setChannels(HJ_CHANNEL_DEFAULT);
    output_info->m_sampleFmt = HJ_SAMPLE_FORMAT_DEFAULT;
    output_info->m_sampleCnt = kFrameSamples;
    output_info->m_samplesPerFrame = kFrameSamples;

    HJAudioMixerConfig config{};
    config.m_output_info = output_info;
    config.m_max_inputs = 2;
    config.m_frame_samples = kFrameSamples;
    config.m_sync_window_ms = 50;
    config.m_late_threshold_ms = 200;
    config.m_enable_compand = false;
    config.m_enable_limiter = false;

    auto mixer = std::make_shared<HJTestAudioMixerPlugin>(
        "HJPluginAudioMixerClearDualEofEventsTest", graph_info);
    auto source0 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerClearDualEofEventsSource0");
    auto source1 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerClearDualEofEventsSource1");
    auto sink = std::make_shared<HJTestSinkPlugin>(
        "HJPluginAudioMixerClearDualEofEventsSink");
    ASSERT_NE(mixer, nullptr);
    ASSERT_NE(source0, nullptr);
    ASSERT_NE(source1, nullptr);
    ASSERT_NE(sink, nullptr);

    std::vector<HJGraphAudioMixerInputState> input_states;
    auto input_state_token = graph->eventBus()->subscribe(
        EVENT_GRAPH_AUDIO_MIXER_INPUT_STATE_ID,
        [&input_states](const HJGraphAudioMixerInputState& i_state) -> HJRet {
            input_states.push_back(i_state);
            return HJ_OK;
        });
    ASSERT_NE(input_state_token, nullptr);

    std::vector<HJGraphAudioMixerStats> stats_events;
    auto stats_token = graph->eventBus()->subscribe(
        EVENT_GRAPH_AUDIO_MIXER_STATS_ID,
        [&stats_events](const HJGraphAudioMixerStats& i_stats) -> HJRet {
            stats_events.push_back(i_stats);
            return HJ_OK;
        });
    ASSERT_NE(stats_token, nullptr);

    auto init_param = std::make_shared<HJKeyStorage>();
    (*init_param)["createThread"] = false;
    (*init_param)["audioMixerConfig"] = config;
    ASSERT_EQ(mixer->init(init_param), HJ_OK);

    ASSERT_EQ(source0->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(source1->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source0, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source1, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addOutputPlugin(sink, HJMEDIA_TYPE_AUDIO), HJ_OK);

    HJAudioMixerInputDesc input_desc0{};
    input_desc0.m_input_id = "input0";
    input_desc0.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    HJAudioMixerInputDesc input_desc1{};
    input_desc1.m_input_id = "input1";
    input_desc1.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    ASSERT_NE(input_desc0.m_input_info, nullptr);
    ASSERT_NE(input_desc1.m_input_info, nullptr);

    const size_t input0_key_hash =
        buildPluginKeyHash(source0->getName(), HJMEDIA_TYPE_AUDIO, 0);
    const size_t input1_key_hash =
        buildPluginKeyHash(source1->getName(), HJMEDIA_TYPE_AUDIO, 0);
    ASSERT_EQ(mixer->bindInput(0, input0_key_hash, input_desc0), HJ_OK);
    ASSERT_EQ(mixer->bindInput(1, input1_key_hash, input_desc1), HJ_OK);

    auto first_frame0 = makeTestAudioFrame(output_info, 0);
    auto first_frame1 = makeTestAudioFrame(output_info, 0);
    auto eof_frame0 = HJMediaFrame::makeEofFrame(HJMEDIA_TYPE_AUDIO);
    auto eof_frame1 = HJMediaFrame::makeEofFrame(HJMEDIA_TYPE_AUDIO);
    ASSERT_NE(first_frame0, nullptr);
    ASSERT_NE(first_frame1, nullptr);
    ASSERT_NE(eof_frame0, nullptr);
    ASSERT_NE(eof_frame1, nullptr);

    ASSERT_EQ(source0->pushFrame(first_frame0), HJ_OK);
    ASSERT_EQ(source1->pushFrame(first_frame1), HJ_OK);

    int64_t delay_ms = 0;
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(sink->getFrames().size(), 1u);
    ASSERT_FALSE(stats_events.empty());
    EXPECT_EQ(stats_events.back().m_output_frames, 1u);
    EXPECT_EQ(stats_events.back().m_active_inputs, 2u);

    ASSERT_EQ(mixer->flush(input1_key_hash), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    input_states.clear();
    stats_events.clear();

    ASSERT_EQ(source0->pushFrame(eof_frame0), HJ_OK);
    ASSERT_EQ(source1->pushFrame(eof_frame1), HJ_OK);
    EXPECT_EQ(mixer->runOnce(&delay_ms), HJ_WOULD_BLOCK);

    EXPECT_TRUE(stats_events.empty());
    EXPECT_TRUE(std::any_of(input_states.begin(), input_states.end(),
        [](const HJGraphAudioMixerInputState& i_state) {
            return i_state.m_input_id == "input0" && i_state.m_eof &&
                   !i_state.m_starved;
        }));
    EXPECT_TRUE(std::any_of(input_states.begin(), input_states.end(),
        [](const HJGraphAudioMixerInputState& i_state) {
            return i_state.m_input_id == "input1" && i_state.m_eof &&
                   !i_state.m_starved;
        }));

    graph->eventBus()->unSubscribe(EVENT_GRAPH_AUDIO_MIXER_INPUT_STATE_ID,
                                   input_state_token);
    graph->eventBus()->unSubscribe(EVENT_GRAPH_AUDIO_MIXER_STATS_ID,
                                   stats_token);
    EXPECT_EQ(mixer->done(), HJ_OK);
}

TEST(HJPluginAudioMixerTest,
     ClearAfterConcealmentAndFutureFrameResetsPublishedEvents)
{
    constexpr int kSampleRate = 48000;
    constexpr int kFrameSamples = 1024;
    const int64_t kSecondPtsMs = samplesToPtsMs(kFrameSamples, kSampleRate);
    const int64_t kFuturePtsMs = samplesToPtsMs(kFrameSamples * 5, kSampleRate);

    auto graph = std::make_shared<HJGraph>(
        "HJPluginAudioMixerClearConcealmentFutureGraph");
    ASSERT_NE(graph, nullptr);

    auto graph_info = std::make_shared<HJKeyStorage>();
    (*graph_info)["graph"] = graph;

    HJAudioInfo::Ptr output_info = std::make_shared<HJAudioInfo>();
    output_info->m_samplesRate = kSampleRate;
    output_info->setChannels(HJ_CHANNEL_DEFAULT);
    output_info->m_sampleFmt = HJ_SAMPLE_FORMAT_DEFAULT;
    output_info->m_sampleCnt = kFrameSamples;
    output_info->m_samplesPerFrame = kFrameSamples;

    HJAudioMixerConfig config{};
    config.m_output_info = output_info;
    config.m_max_inputs = 2;
    config.m_frame_samples = kFrameSamples;
    config.m_sync_window_ms = 50;
    config.m_late_threshold_ms = 0;
    config.m_enable_compand = false;
    config.m_enable_limiter = false;

    auto mixer = std::make_shared<HJTestAudioMixerPlugin>(
        "HJPluginAudioMixerClearConcealmentFutureTest", graph_info);
    auto source0 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerClearConcealmentFutureSource0");
    auto source1 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerClearConcealmentFutureSource1");
    auto sink = std::make_shared<HJTestSinkPlugin>(
        "HJPluginAudioMixerClearConcealmentFutureSink");
    ASSERT_NE(mixer, nullptr);
    ASSERT_NE(source0, nullptr);
    ASSERT_NE(source1, nullptr);
    ASSERT_NE(sink, nullptr);

    std::vector<HJGraphAudioMixerInputState> input_states;
    auto input_state_token = graph->eventBus()->subscribe(
        EVENT_GRAPH_AUDIO_MIXER_INPUT_STATE_ID,
        [&input_states](const HJGraphAudioMixerInputState& i_state) -> HJRet {
            input_states.push_back(i_state);
            return HJ_OK;
        });
    ASSERT_NE(input_state_token, nullptr);

    std::vector<HJGraphAudioMixerStats> stats_events;
    auto stats_token = graph->eventBus()->subscribe(
        EVENT_GRAPH_AUDIO_MIXER_STATS_ID,
        [&stats_events](const HJGraphAudioMixerStats& i_stats) -> HJRet {
            stats_events.push_back(i_stats);
            return HJ_OK;
        });
    ASSERT_NE(stats_token, nullptr);

    auto init_param = std::make_shared<HJKeyStorage>();
    (*init_param)["createThread"] = false;
    (*init_param)["audioMixerConfig"] = config;
    ASSERT_EQ(mixer->init(init_param), HJ_OK);

    ASSERT_EQ(source0->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(source1->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source0, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source1, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addOutputPlugin(sink, HJMEDIA_TYPE_AUDIO), HJ_OK);

    HJAudioMixerInputDesc input_desc0{};
    input_desc0.m_input_id = "input0";
    input_desc0.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    HJAudioMixerInputDesc input_desc1{};
    input_desc1.m_input_id = "input1";
    input_desc1.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    ASSERT_NE(input_desc0.m_input_info, nullptr);
    ASSERT_NE(input_desc1.m_input_info, nullptr);

    const size_t input0_key_hash =
        buildPluginKeyHash(source0->getName(), HJMEDIA_TYPE_AUDIO, 0);
    const size_t input1_key_hash =
        buildPluginKeyHash(source1->getName(), HJMEDIA_TYPE_AUDIO, 0);
    ASSERT_EQ(mixer->bindInput(0, input0_key_hash, input_desc0), HJ_OK);
    ASSERT_EQ(mixer->bindInput(1, input1_key_hash, input_desc1), HJ_OK);

    auto first_frame0 = makeTestAudioFrame(output_info, 0);
    auto first_frame1 = makeTestAudioFrame(output_info, 0);
    auto second_frame0 = makeTestAudioFrame(output_info, kSecondPtsMs);
    auto future_frame1 = makeTestAudioFrame(output_info, kFuturePtsMs);
    auto restart_frame0 = makeTestAudioFrame(output_info, 0);
    auto restart_frame1 = makeTestAudioFrame(output_info, 0);
    ASSERT_NE(first_frame0, nullptr);
    ASSERT_NE(first_frame1, nullptr);
    ASSERT_NE(second_frame0, nullptr);
    ASSERT_NE(future_frame1, nullptr);
    ASSERT_NE(restart_frame0, nullptr);
    ASSERT_NE(restart_frame1, nullptr);

    ASSERT_EQ(source0->pushFrame(first_frame0), HJ_OK);
    ASSERT_EQ(source1->pushFrame(first_frame1), HJ_OK);

    int64_t delay_ms = 0;
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(sink->getFrames().size(), 1u);

    ASSERT_EQ(source0->pushFrame(second_frame0), HJ_OK);
    ASSERT_EQ(source1->pushFrame(future_frame1), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_FALSE(stats_events.empty());
    EXPECT_EQ(stats_events.back().m_output_frames, 2u);
    EXPECT_EQ(stats_events.back().m_starved_inputs, 1u);
    EXPECT_TRUE(std::any_of(input_states.begin(), input_states.end(),
        [](const HJGraphAudioMixerInputState& i_state) {
            return i_state.m_input_id == "input1" && i_state.m_starved &&
                   !i_state.m_eof;
        }));

    ASSERT_EQ(mixer->flush(input1_key_hash), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    input_states.clear();
    stats_events.clear();

    ASSERT_EQ(source0->pushFrame(restart_frame0), HJ_OK);
    ASSERT_EQ(source1->pushFrame(restart_frame1), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    ASSERT_EQ(sink->getFrames().size(), 3u);
    ASSERT_NE(sink->getFrames().back(), nullptr);
    EXPECT_EQ(sink->getFrames().back()->getPTS(), 0);
    ASSERT_EQ(stats_events.size(), 1u);
    EXPECT_EQ(stats_events.back().m_output_frames, 1u);
    EXPECT_EQ(stats_events.back().m_mix_pts,
              samplesToPtsMs(kFrameSamples, kSampleRate));
    EXPECT_EQ(stats_events.back().m_starved_inputs, 0u);
    EXPECT_EQ(stats_events.back().m_active_inputs, 2u);
    EXPECT_TRUE(std::none_of(input_states.begin(), input_states.end(),
        [](const HJGraphAudioMixerInputState& i_state) {
            return i_state.m_starved || i_state.m_eof;
        }));

    graph->eventBus()->unSubscribe(EVENT_GRAPH_AUDIO_MIXER_INPUT_STATE_ID,
                                   input_state_token);
    graph->eventBus()->unSubscribe(EVENT_GRAPH_AUDIO_MIXER_STATS_ID,
                                   stats_token);
    EXPECT_EQ(mixer->done(), HJ_OK);
}

TEST(HJPluginAudioMixerTest,
     RepeatedClearRestartResetsStatsAndInputStateCounters)
{
    constexpr int kSampleRate = 48000;
    constexpr int kFrameSamples = 1024;

    auto graph = std::make_shared<HJGraph>(
        "HJPluginAudioMixerRepeatedClearEventsGraph");
    ASSERT_NE(graph, nullptr);

    auto graph_info = std::make_shared<HJKeyStorage>();
    (*graph_info)["graph"] = graph;

    HJAudioInfo::Ptr output_info = std::make_shared<HJAudioInfo>();
    output_info->m_samplesRate = kSampleRate;
    output_info->setChannels(HJ_CHANNEL_DEFAULT);
    output_info->m_sampleFmt = HJ_SAMPLE_FORMAT_DEFAULT;
    output_info->m_sampleCnt = kFrameSamples;
    output_info->m_samplesPerFrame = kFrameSamples;

    HJAudioMixerConfig config{};
    config.m_output_info = output_info;
    config.m_max_inputs = 2;
    config.m_frame_samples = kFrameSamples;
    config.m_sync_window_ms = 50;
    config.m_late_threshold_ms = 200;
    config.m_enable_compand = false;
    config.m_enable_limiter = false;

    auto mixer = std::make_shared<HJTestAudioMixerPlugin>(
        "HJPluginAudioMixerRepeatedClearEventsTest", graph_info);
    auto source0 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerRepeatedClearEventsSource0");
    auto source1 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerRepeatedClearEventsSource1");
    auto sink = std::make_shared<HJTestSinkPlugin>(
        "HJPluginAudioMixerRepeatedClearEventsSink");
    ASSERT_NE(mixer, nullptr);
    ASSERT_NE(source0, nullptr);
    ASSERT_NE(source1, nullptr);
    ASSERT_NE(sink, nullptr);

    std::vector<HJGraphAudioMixerInputState> input_states;
    auto input_state_token = graph->eventBus()->subscribe(
        EVENT_GRAPH_AUDIO_MIXER_INPUT_STATE_ID,
        [&input_states](const HJGraphAudioMixerInputState& i_state) -> HJRet {
            input_states.push_back(i_state);
            return HJ_OK;
        });
    ASSERT_NE(input_state_token, nullptr);

    std::vector<HJGraphAudioMixerStats> stats_events;
    auto stats_token = graph->eventBus()->subscribe(
        EVENT_GRAPH_AUDIO_MIXER_STATS_ID,
        [&stats_events](const HJGraphAudioMixerStats& i_stats) -> HJRet {
            stats_events.push_back(i_stats);
            return HJ_OK;
        });
    ASSERT_NE(stats_token, nullptr);

    auto init_param = std::make_shared<HJKeyStorage>();
    (*init_param)["createThread"] = false;
    (*init_param)["audioMixerConfig"] = config;
    ASSERT_EQ(mixer->init(init_param), HJ_OK);

    ASSERT_EQ(source0->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(source1->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source0, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source1, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addOutputPlugin(sink, HJMEDIA_TYPE_AUDIO), HJ_OK);

    HJAudioMixerInputDesc input_desc0{};
    input_desc0.m_input_id = "input0";
    input_desc0.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    HJAudioMixerInputDesc input_desc1{};
    input_desc1.m_input_id = "input1";
    input_desc1.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    ASSERT_NE(input_desc0.m_input_info, nullptr);
    ASSERT_NE(input_desc1.m_input_info, nullptr);

    const size_t input0_key_hash =
        buildPluginKeyHash(source0->getName(), HJMEDIA_TYPE_AUDIO, 0);
    const size_t input1_key_hash =
        buildPluginKeyHash(source1->getName(), HJMEDIA_TYPE_AUDIO, 0);
    ASSERT_EQ(mixer->bindInput(0, input0_key_hash, input_desc0), HJ_OK);
    ASSERT_EQ(mixer->bindInput(1, input1_key_hash, input_desc1), HJ_OK);

    auto push_restart_cycle = [&](int64_t *io_delay_ms) {
        auto restart_frame0 = makeTestAudioFrame(output_info, 0);
        auto restart_frame1 = makeTestAudioFrame(output_info, 0);
        ASSERT_NE(restart_frame0, nullptr);
        ASSERT_NE(restart_frame1, nullptr);
        ASSERT_EQ(source0->pushFrame(restart_frame0), HJ_OK);
        ASSERT_EQ(source1->pushFrame(restart_frame1), HJ_OK);
        ASSERT_EQ(mixer->runOnce(io_delay_ms), HJ_OK);
    };

    int64_t delay_ms = 0;
    push_restart_cycle(&delay_ms);
    ASSERT_EQ(sink->getFrames().size(), 1u);

    ASSERT_EQ(mixer->flush(input1_key_hash), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    push_restart_cycle(&delay_ms);

    ASSERT_EQ(mixer->flush(input1_key_hash), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    push_restart_cycle(&delay_ms);

    const size_t restart_stats_count = static_cast<size_t>(std::count_if(
        stats_events.begin(), stats_events.end(),
        [kFrameSamples, kSampleRate](const HJGraphAudioMixerStats& i_stats) {
            return i_stats.m_output_frames == 1u &&
                   i_stats.m_mix_pts ==
                       samplesToPtsMs(kFrameSamples, kSampleRate) &&
                   i_stats.m_active_inputs == 2u &&
                   i_stats.m_starved_inputs == 0u;
        }));
    EXPECT_GE(restart_stats_count, 3u);

    const size_t reset_state_count = static_cast<size_t>(std::count_if(
        input_states.begin(), input_states.end(),
        [](const HJGraphAudioMixerInputState& i_state) {
            return i_state.m_input_id == "input1" && i_state.m_attached &&
                   !i_state.m_starved && !i_state.m_eof;
        }));
    EXPECT_GE(reset_state_count, 2u);

    graph->eventBus()->unSubscribe(EVENT_GRAPH_AUDIO_MIXER_INPUT_STATE_ID,
                                   input_state_token);
    graph->eventBus()->unSubscribe(EVENT_GRAPH_AUDIO_MIXER_STATS_ID,
                                   stats_token);
    EXPECT_EQ(mixer->done(), HJ_OK);
}

TEST(HJPluginAudioMixerTest,
     ClearRestartThenLateDropAndEofPublishFreshCurrentCycleEvents)
{
    constexpr int kSampleRate = 48000;
    constexpr int kFrameSamples = 1024;
    const int64_t kSecondPtsMs = samplesToPtsMs(kFrameSamples, kSampleRate);
    const int64_t kLateDropPtsMs = -10;

    auto graph = std::make_shared<HJGraph>(
        "HJPluginAudioMixerClearLateDropEofEventsGraph");
    ASSERT_NE(graph, nullptr);

    auto graph_info = std::make_shared<HJKeyStorage>();
    (*graph_info)["graph"] = graph;

    HJAudioInfo::Ptr output_info = std::make_shared<HJAudioInfo>();
    output_info->m_samplesRate = kSampleRate;
    output_info->setChannels(HJ_CHANNEL_DEFAULT);
    output_info->m_sampleFmt = HJ_SAMPLE_FORMAT_DEFAULT;
    output_info->m_sampleCnt = kFrameSamples;
    output_info->m_samplesPerFrame = kFrameSamples;

    HJAudioMixerConfig config{};
    config.m_output_info = output_info;
    config.m_max_inputs = 2;
    config.m_frame_samples = kFrameSamples;
    config.m_sync_window_ms = 50;
    config.m_late_threshold_ms = 0;
    config.m_enable_compand = false;
    config.m_enable_limiter = false;

    auto mixer = std::make_shared<HJTestAudioMixerPlugin>(
        "HJPluginAudioMixerClearLateDropEofEventsTest", graph_info);
    auto source0 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerClearLateDropEofEventsSource0");
    auto source1 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerClearLateDropEofEventsSource1");
    auto sink = std::make_shared<HJTestSinkPlugin>(
        "HJPluginAudioMixerClearLateDropEofEventsSink");
    ASSERT_NE(mixer, nullptr);
    ASSERT_NE(source0, nullptr);
    ASSERT_NE(source1, nullptr);
    ASSERT_NE(sink, nullptr);

    std::vector<HJGraphAudioMixerInputState> input_states;
    auto input_state_token = graph->eventBus()->subscribe(
        EVENT_GRAPH_AUDIO_MIXER_INPUT_STATE_ID,
        [&input_states](const HJGraphAudioMixerInputState& i_state) -> HJRet {
            input_states.push_back(i_state);
            return HJ_OK;
        });
    ASSERT_NE(input_state_token, nullptr);

    std::vector<HJGraphAudioMixerStats> stats_events;
    auto stats_token = graph->eventBus()->subscribe(
        EVENT_GRAPH_AUDIO_MIXER_STATS_ID,
        [&stats_events](const HJGraphAudioMixerStats& i_stats) -> HJRet {
            stats_events.push_back(i_stats);
            return HJ_OK;
        });
    ASSERT_NE(stats_token, nullptr);

    size_t late_drop_notify_count = 0;
    ASSERT_EQ(
        graph->eventBus()->registerHandler(
            EVENT_PLUGIN_NOTIFY_ID,
            [&late_drop_notify_count, mixer](HJPluginNotifyType i_notify_type,
                                             size_t i_plugin_id) -> HJRet {
                if (i_notify_type ==
                        HJ_PLUGIN_NOTIFY_ERROR_AUDIOMIXER_MIXFRAME &&
                    i_plugin_id == mixer->getID()) {
                    ++late_drop_notify_count;
                }
                return HJ_OK;
            }),
        HJ_OK);

    auto init_param = std::make_shared<HJKeyStorage>();
    (*init_param)["createThread"] = false;
    (*init_param)["audioMixerConfig"] = config;
    ASSERT_EQ(mixer->init(init_param), HJ_OK);

    ASSERT_EQ(source0->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(source1->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source0, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source1, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addOutputPlugin(sink, HJMEDIA_TYPE_AUDIO), HJ_OK);

    HJAudioMixerInputDesc input_desc0{};
    input_desc0.m_input_id = "input0";
    input_desc0.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    HJAudioMixerInputDesc input_desc1{};
    input_desc1.m_input_id = "input1";
    input_desc1.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    ASSERT_NE(input_desc0.m_input_info, nullptr);
    ASSERT_NE(input_desc1.m_input_info, nullptr);

    const size_t input0_key_hash =
        buildPluginKeyHash(source0->getName(), HJMEDIA_TYPE_AUDIO, 0);
    const size_t input1_key_hash =
        buildPluginKeyHash(source1->getName(), HJMEDIA_TYPE_AUDIO, 0);
    ASSERT_EQ(mixer->bindInput(0, input0_key_hash, input_desc0), HJ_OK);
    ASSERT_EQ(mixer->bindInput(1, input1_key_hash, input_desc1), HJ_OK);

    auto first_frame0 = makeTestAudioFrame(output_info, 0);
    auto first_frame1 = makeTestAudioFrame(output_info, 0);
    auto restart_frame0 = makeTestAudioFrame(output_info, 0);
    auto restart_frame1 = makeTestAudioFrame(output_info, 0);
    auto stale_frame0 = makeTestAudioFrame(output_info, kLateDropPtsMs);
    auto current_frame0 = makeTestAudioFrame(output_info, kSecondPtsMs);
    auto eof_frame1 = HJMediaFrame::makeEofFrame(HJMEDIA_TYPE_AUDIO);
    ASSERT_NE(first_frame0, nullptr);
    ASSERT_NE(first_frame1, nullptr);
    ASSERT_NE(restart_frame0, nullptr);
    ASSERT_NE(restart_frame1, nullptr);
    ASSERT_NE(stale_frame0, nullptr);
    ASSERT_NE(current_frame0, nullptr);
    ASSERT_NE(eof_frame1, nullptr);

    ASSERT_EQ(source0->pushFrame(first_frame0), HJ_OK);
    ASSERT_EQ(source1->pushFrame(first_frame1), HJ_OK);

    int64_t delay_ms = 0;
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(sink->getFrames().size(), 1u);

    ASSERT_EQ(mixer->flush(input1_key_hash), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    ASSERT_EQ(source0->pushFrame(restart_frame0), HJ_OK);
    ASSERT_EQ(source1->pushFrame(restart_frame1), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(sink->getFrames().size(), 2u);

    stats_events.clear();
    late_drop_notify_count = 0;

    ASSERT_EQ(source0->pushFrame(stale_frame0), HJ_OK);
    ASSERT_EQ(source0->pushFrame(current_frame0), HJ_OK);
    ASSERT_EQ(source1->pushFrame(eof_frame1), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    ASSERT_EQ(sink->getFrames().size(), 3u);
    ASSERT_NE(sink->getFrames().back(), nullptr);
    EXPECT_EQ(sink->getFrames().back()->getPTS(), kSecondPtsMs);
    ASSERT_EQ(stats_events.size(), 1u);
    EXPECT_EQ(stats_events.back().m_output_frames, 2u);
    EXPECT_EQ(stats_events.back().m_mix_pts,
              samplesToPtsMs(kFrameSamples * 2, kSampleRate));
    EXPECT_EQ(stats_events.back().m_active_inputs, 1u);
    EXPECT_EQ(stats_events.back().m_starved_inputs, 0u);
    EXPECT_EQ(late_drop_notify_count, 1u);
    EXPECT_TRUE(std::any_of(input_states.begin(), input_states.end(),
        [](const HJGraphAudioMixerInputState& i_state) {
            return i_state.m_input_id == "input1" && i_state.m_eof &&
                   !i_state.m_starved;
        }));
    EXPECT_TRUE(std::none_of(input_states.begin(), input_states.end(),
        [](const HJGraphAudioMixerInputState& i_state) {
            return i_state.m_input_id == "input1" && i_state.m_starved;
        }));
    EXPECT_EQ(mixer->queuedFrameCount(input0_key_hash), 0u);
    EXPECT_EQ(mixer->queuedFrameCount(input1_key_hash), 0u);

    graph->eventBus()->unSubscribe(EVENT_GRAPH_AUDIO_MIXER_INPUT_STATE_ID,
                                   input_state_token);
    graph->eventBus()->unSubscribe(EVENT_GRAPH_AUDIO_MIXER_STATS_ID,
                                   stats_token);
    EXPECT_EQ(mixer->done(), HJ_OK);
}

TEST(HJPluginAudioMixerTest,
     ClearThenInputMissingRestartPublishesFreshOutputWithoutStaleState)
{
    constexpr int kSampleRate = 48000;
    constexpr int kFrameSamples = 1024;

    auto graph = std::make_shared<HJGraph>(
        "HJPluginAudioMixerClearInputMissingEventsGraph");
    ASSERT_NE(graph, nullptr);

    auto graph_info = std::make_shared<HJKeyStorage>();
    (*graph_info)["graph"] = graph;

    HJAudioInfo::Ptr output_info = std::make_shared<HJAudioInfo>();
    output_info->m_samplesRate = kSampleRate;
    output_info->setChannels(HJ_CHANNEL_DEFAULT);
    output_info->m_sampleFmt = HJ_SAMPLE_FORMAT_DEFAULT;
    output_info->m_sampleCnt = kFrameSamples;
    output_info->m_samplesPerFrame = kFrameSamples;

    HJAudioMixerConfig config{};
    config.m_output_info = output_info;
    config.m_max_inputs = 2;
    config.m_frame_samples = kFrameSamples;
    config.m_sync_window_ms = 50;
    config.m_late_threshold_ms = 0;
    config.m_enable_compand = false;
    config.m_enable_limiter = false;

    auto mixer = std::make_shared<HJTestAudioMixerPlugin>(
        "HJPluginAudioMixerClearInputMissingEventsTest", graph_info);
    auto source0 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerClearInputMissingEventsSource0");
    auto source1 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerClearInputMissingEventsSource1");
    auto sink = std::make_shared<HJTestSinkPlugin>(
        "HJPluginAudioMixerClearInputMissingEventsSink");
    ASSERT_NE(mixer, nullptr);
    ASSERT_NE(source0, nullptr);
    ASSERT_NE(source1, nullptr);
    ASSERT_NE(sink, nullptr);

    std::vector<HJGraphAudioMixerInputState> input_states;
    auto input_state_token = graph->eventBus()->subscribe(
        EVENT_GRAPH_AUDIO_MIXER_INPUT_STATE_ID,
        [&input_states](const HJGraphAudioMixerInputState& i_state) -> HJRet {
            input_states.push_back(i_state);
            return HJ_OK;
        });
    ASSERT_NE(input_state_token, nullptr);

    std::vector<HJGraphAudioMixerStats> stats_events;
    auto stats_token = graph->eventBus()->subscribe(
        EVENT_GRAPH_AUDIO_MIXER_STATS_ID,
        [&stats_events](const HJGraphAudioMixerStats& i_stats) -> HJRet {
            stats_events.push_back(i_stats);
            return HJ_OK;
        });
    ASSERT_NE(stats_token, nullptr);

    auto init_param = std::make_shared<HJKeyStorage>();
    (*init_param)["createThread"] = false;
    (*init_param)["audioMixerConfig"] = config;
    ASSERT_EQ(mixer->init(init_param), HJ_OK);

    ASSERT_EQ(source0->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(source1->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source0, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source1, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addOutputPlugin(sink, HJMEDIA_TYPE_AUDIO), HJ_OK);

    HJAudioMixerInputDesc input_desc0{};
    input_desc0.m_input_id = "input0";
    input_desc0.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    HJAudioMixerInputDesc input_desc1{};
    input_desc1.m_input_id = "input1";
    input_desc1.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    ASSERT_NE(input_desc0.m_input_info, nullptr);
    ASSERT_NE(input_desc1.m_input_info, nullptr);

    const size_t input0_key_hash =
        buildPluginKeyHash(source0->getName(), HJMEDIA_TYPE_AUDIO, 0);
    const size_t input1_key_hash =
        buildPluginKeyHash(source1->getName(), HJMEDIA_TYPE_AUDIO, 0);
    ASSERT_EQ(mixer->bindInput(0, input0_key_hash, input_desc0), HJ_OK);
    ASSERT_EQ(mixer->bindInput(1, input1_key_hash, input_desc1), HJ_OK);

    auto first_frame0 = makeTestAudioFrame(output_info, 0);
    auto first_frame1 = makeTestAudioFrame(output_info, 0);
    auto restart_frame0 = makeTestAudioFrame(output_info, 0);
    ASSERT_NE(first_frame0, nullptr);
    ASSERT_NE(first_frame1, nullptr);
    ASSERT_NE(restart_frame0, nullptr);

    ASSERT_EQ(source0->pushFrame(first_frame0), HJ_OK);
    ASSERT_EQ(source1->pushFrame(first_frame1), HJ_OK);

    int64_t delay_ms = 0;
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(sink->getFrames().size(), 1u);

    ASSERT_EQ(mixer->flush(input1_key_hash), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    mixer->onInputDisconnected(input1_key_hash);
    input_states.clear();
    stats_events.clear();

    ASSERT_EQ(source0->pushFrame(restart_frame0), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    ASSERT_EQ(sink->getFrames().size(), 2u);
    ASSERT_NE(sink->getFrames().back(), nullptr);
    EXPECT_EQ(sink->getFrames().back()->getPTS(), 0);
    ASSERT_EQ(stats_events.size(), 1u);
    EXPECT_EQ(stats_events.back().m_output_frames, 1u);
    EXPECT_EQ(stats_events.back().m_mix_pts,
              samplesToPtsMs(kFrameSamples, kSampleRate));
    EXPECT_EQ(stats_events.back().m_starved_inputs, 0u);
    EXPECT_TRUE(std::none_of(input_states.begin(), input_states.end(),
        [](const HJGraphAudioMixerInputState& i_state) {
            return i_state.m_input_id == "input1" &&
                   (i_state.m_starved || i_state.m_eof);
        }));

    graph->eventBus()->unSubscribe(EVENT_GRAPH_AUDIO_MIXER_INPUT_STATE_ID,
                                   input_state_token);
    graph->eventBus()->unSubscribe(EVENT_GRAPH_AUDIO_MIXER_STATS_ID,
                                   stats_token);
    EXPECT_EQ(mixer->done(), HJ_OK);
}

TEST(HJPluginAudioMixerTest,
     RepeatedClearEofRestartClearRepublishesFreshResetEvents)
{
    constexpr int kSampleRate = 48000;
    constexpr int kFrameSamples = 1024;

    auto graph = std::make_shared<HJGraph>(
        "HJPluginAudioMixerRepeatedClearEofEventsGraph");
    ASSERT_NE(graph, nullptr);

    auto graph_info = std::make_shared<HJKeyStorage>();
    (*graph_info)["graph"] = graph;

    HJAudioInfo::Ptr output_info = std::make_shared<HJAudioInfo>();
    output_info->m_samplesRate = kSampleRate;
    output_info->setChannels(HJ_CHANNEL_DEFAULT);
    output_info->m_sampleFmt = HJ_SAMPLE_FORMAT_DEFAULT;
    output_info->m_sampleCnt = kFrameSamples;
    output_info->m_samplesPerFrame = kFrameSamples;

    HJAudioMixerConfig config{};
    config.m_output_info = output_info;
    config.m_max_inputs = 2;
    config.m_frame_samples = kFrameSamples;
    config.m_sync_window_ms = 50;
    config.m_late_threshold_ms = 200;
    config.m_enable_compand = false;
    config.m_enable_limiter = false;

    auto mixer = std::make_shared<HJTestAudioMixerPlugin>(
        "HJPluginAudioMixerRepeatedClearEofEventsTest", graph_info);
    auto source0 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerRepeatedClearEofEventsSource0");
    auto source1 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerRepeatedClearEofEventsSource1");
    auto sink = std::make_shared<HJTestSinkPlugin>(
        "HJPluginAudioMixerRepeatedClearEofEventsSink");
    ASSERT_NE(mixer, nullptr);
    ASSERT_NE(source0, nullptr);
    ASSERT_NE(source1, nullptr);
    ASSERT_NE(sink, nullptr);

    std::vector<HJGraphAudioMixerInputState> input_states;
    auto input_state_token = graph->eventBus()->subscribe(
        EVENT_GRAPH_AUDIO_MIXER_INPUT_STATE_ID,
        [&input_states](const HJGraphAudioMixerInputState& i_state) -> HJRet {
            input_states.push_back(i_state);
            return HJ_OK;
        });
    ASSERT_NE(input_state_token, nullptr);

    std::vector<HJGraphAudioMixerStats> stats_events;
    auto stats_token = graph->eventBus()->subscribe(
        EVENT_GRAPH_AUDIO_MIXER_STATS_ID,
        [&stats_events](const HJGraphAudioMixerStats& i_stats) -> HJRet {
            stats_events.push_back(i_stats);
            return HJ_OK;
        });
    ASSERT_NE(stats_token, nullptr);

    auto init_param = std::make_shared<HJKeyStorage>();
    (*init_param)["createThread"] = false;
    (*init_param)["audioMixerConfig"] = config;
    ASSERT_EQ(mixer->init(init_param), HJ_OK);

    ASSERT_EQ(source0->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(source1->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source0, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source1, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addOutputPlugin(sink, HJMEDIA_TYPE_AUDIO), HJ_OK);

    HJAudioMixerInputDesc input_desc0{};
    input_desc0.m_input_id = "input0";
    input_desc0.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    HJAudioMixerInputDesc input_desc1{};
    input_desc1.m_input_id = "input1";
    input_desc1.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    ASSERT_NE(input_desc0.m_input_info, nullptr);
    ASSERT_NE(input_desc1.m_input_info, nullptr);

    const size_t input0_key_hash =
        buildPluginKeyHash(source0->getName(), HJMEDIA_TYPE_AUDIO, 0);
    const size_t input1_key_hash =
        buildPluginKeyHash(source1->getName(), HJMEDIA_TYPE_AUDIO, 0);
    ASSERT_EQ(mixer->bindInput(0, input0_key_hash, input_desc0), HJ_OK);
    ASSERT_EQ(mixer->bindInput(1, input1_key_hash, input_desc1), HJ_OK);

    auto push_cycle = [&](int64_t* io_delay_ms) {
        auto frame0 = makeTestAudioFrame(output_info, 0);
        auto frame1 = makeTestAudioFrame(output_info, 0);
        ASSERT_NE(frame0, nullptr);
        ASSERT_NE(frame1, nullptr);
        ASSERT_EQ(source0->pushFrame(frame0), HJ_OK);
        ASSERT_EQ(source1->pushFrame(frame1), HJ_OK);
        ASSERT_EQ(mixer->runOnce(io_delay_ms), HJ_OK);
    };

    int64_t delay_ms = 0;
    push_cycle(&delay_ms);
    ASSERT_EQ(sink->getFrames().size(), 1u);

    ASSERT_EQ(mixer->flush(input1_key_hash), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    push_cycle(&delay_ms);

    input_states.clear();
    stats_events.clear();

    auto eof_frame0 = HJMediaFrame::makeEofFrame(HJMEDIA_TYPE_AUDIO);
    auto eof_frame1 = HJMediaFrame::makeEofFrame(HJMEDIA_TYPE_AUDIO);
    ASSERT_NE(eof_frame0, nullptr);
    ASSERT_NE(eof_frame1, nullptr);
    ASSERT_EQ(source0->pushFrame(eof_frame0), HJ_OK);
    ASSERT_EQ(source1->pushFrame(eof_frame1), HJ_OK);
    EXPECT_EQ(mixer->runOnce(&delay_ms), HJ_WOULD_BLOCK);

    EXPECT_TRUE(std::any_of(input_states.begin(), input_states.end(),
        [](const HJGraphAudioMixerInputState& i_state) {
            return i_state.m_input_id == "input0" && i_state.m_eof;
        }));
    EXPECT_TRUE(std::any_of(input_states.begin(), input_states.end(),
        [](const HJGraphAudioMixerInputState& i_state) {
            return i_state.m_input_id == "input1" && i_state.m_eof;
        }));
    EXPECT_TRUE(stats_events.empty());
    const size_t state_count_before_second_clear = input_states.size();

    ASSERT_EQ(mixer->flush(input1_key_hash), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    stats_events.clear();

    push_cycle(&delay_ms);

    ASSERT_EQ(sink->getFrames().size(), 3u);
    ASSERT_EQ(stats_events.size(), 1u);
    EXPECT_EQ(stats_events.back().m_output_frames, 1u);
    EXPECT_EQ(stats_events.back().m_mix_pts,
              samplesToPtsMs(kFrameSamples, kSampleRate));
    EXPECT_EQ(stats_events.back().m_starved_inputs, 0u);
    ASSERT_GT(input_states.size(), state_count_before_second_clear);
    EXPECT_TRUE(std::none_of(input_states.begin() + state_count_before_second_clear,
                             input_states.end(),
        [](const HJGraphAudioMixerInputState& i_state) {
            return i_state.m_starved || i_state.m_eof;
        }));

    graph->eventBus()->unSubscribe(EVENT_GRAPH_AUDIO_MIXER_INPUT_STATE_ID,
                                   input_state_token);
    graph->eventBus()->unSubscribe(EVENT_GRAPH_AUDIO_MIXER_STATS_ID,
                                   stats_token);
    EXPECT_EQ(mixer->done(), HJ_OK);
}

TEST(HJPluginAudioMixerTest,
     ClearRebindMissingInputRestartDoesNotRepublishStaleState)
{
    constexpr int kSampleRate = 48000;
    constexpr int kFrameSamples = 1024;

    auto graph = std::make_shared<HJGraph>(
        "HJPluginAudioMixerClearRebindMissingEventsGraph");
    ASSERT_NE(graph, nullptr);

    auto graph_info = std::make_shared<HJKeyStorage>();
    (*graph_info)["graph"] = graph;

    HJAudioInfo::Ptr output_info = std::make_shared<HJAudioInfo>();
    output_info->m_samplesRate = kSampleRate;
    output_info->setChannels(HJ_CHANNEL_DEFAULT);
    output_info->m_sampleFmt = HJ_SAMPLE_FORMAT_DEFAULT;
    output_info->m_sampleCnt = kFrameSamples;
    output_info->m_samplesPerFrame = kFrameSamples;

    HJAudioMixerConfig config{};
    config.m_output_info = output_info;
    config.m_max_inputs = 2;
    config.m_frame_samples = kFrameSamples;
    config.m_sync_window_ms = 50;
    config.m_late_threshold_ms = 0;
    config.m_enable_compand = false;
    config.m_enable_limiter = false;

    auto mixer = std::make_shared<HJTestAudioMixerPlugin>(
        "HJPluginAudioMixerClearRebindMissingEventsTest", graph_info);
    auto source0 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerClearRebindMissingEventsSource0");
    auto source1 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerClearRebindMissingEventsSource1");
    auto sink = std::make_shared<HJTestSinkPlugin>(
        "HJPluginAudioMixerClearRebindMissingEventsSink");
    ASSERT_NE(mixer, nullptr);
    ASSERT_NE(source0, nullptr);
    ASSERT_NE(source1, nullptr);
    ASSERT_NE(sink, nullptr);

    std::vector<HJGraphAudioMixerInputState> input_states;
    auto input_state_token = graph->eventBus()->subscribe(
        EVENT_GRAPH_AUDIO_MIXER_INPUT_STATE_ID,
        [&input_states](const HJGraphAudioMixerInputState& i_state) -> HJRet {
            input_states.push_back(i_state);
            return HJ_OK;
        });
    ASSERT_NE(input_state_token, nullptr);

    std::vector<HJGraphAudioMixerStats> stats_events;
    auto stats_token = graph->eventBus()->subscribe(
        EVENT_GRAPH_AUDIO_MIXER_STATS_ID,
        [&stats_events](const HJGraphAudioMixerStats& i_stats) -> HJRet {
            stats_events.push_back(i_stats);
            return HJ_OK;
        });
    ASSERT_NE(stats_token, nullptr);

    auto init_param = std::make_shared<HJKeyStorage>();
    (*init_param)["createThread"] = false;
    (*init_param)["audioMixerConfig"] = config;
    ASSERT_EQ(mixer->init(init_param), HJ_OK);

    ASSERT_EQ(source0->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(source1->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source0, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source1, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addOutputPlugin(sink, HJMEDIA_TYPE_AUDIO), HJ_OK);

    HJAudioMixerInputDesc input_desc0{};
    input_desc0.m_input_id = "input0";
    input_desc0.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    HJAudioMixerInputDesc input_desc1{};
    input_desc1.m_input_id = "input1";
    input_desc1.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    ASSERT_NE(input_desc0.m_input_info, nullptr);
    ASSERT_NE(input_desc1.m_input_info, nullptr);

    const size_t input0_key_hash =
        buildPluginKeyHash(source0->getName(), HJMEDIA_TYPE_AUDIO, 0);
    const size_t input1_key_hash =
        buildPluginKeyHash(source1->getName(), HJMEDIA_TYPE_AUDIO, 0);
    ASSERT_EQ(mixer->bindInput(0, input0_key_hash, input_desc0), HJ_OK);
    ASSERT_EQ(mixer->bindInput(1, input1_key_hash, input_desc1), HJ_OK);

    auto first_frame0 = makeTestAudioFrame(output_info, 0);
    auto first_frame1 = makeTestAudioFrame(output_info, 0);
    auto restart_frame0 = makeTestAudioFrame(output_info, 0);
    ASSERT_NE(first_frame0, nullptr);
    ASSERT_NE(first_frame1, nullptr);
    ASSERT_NE(restart_frame0, nullptr);

    ASSERT_EQ(source0->pushFrame(first_frame0), HJ_OK);
    ASSERT_EQ(source1->pushFrame(first_frame1), HJ_OK);

    int64_t delay_ms = 0;
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(sink->getFrames().size(), 1u);

    ASSERT_EQ(mixer->flush(input1_key_hash), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(mixer->unbindInput(1), HJ_OK);
    ASSERT_EQ(mixer->bindInput(1, input1_key_hash, input_desc1), HJ_OK);
    mixer->onInputDisconnected(input1_key_hash);

    input_states.clear();
    stats_events.clear();

    ASSERT_EQ(source0->pushFrame(restart_frame0), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    ASSERT_EQ(sink->getFrames().size(), 2u);
    ASSERT_NE(sink->getFrames().back(), nullptr);
    EXPECT_EQ(sink->getFrames().back()->getPTS(), 0);
    ASSERT_EQ(stats_events.size(), 1u);
    EXPECT_EQ(stats_events.back().m_output_frames, 1u);
    EXPECT_EQ(stats_events.back().m_mix_pts,
              samplesToPtsMs(kFrameSamples, kSampleRate));
    EXPECT_EQ(stats_events.back().m_starved_inputs, 0u);
    EXPECT_TRUE(std::none_of(input_states.begin(), input_states.end(),
        [](const HJGraphAudioMixerInputState& i_state) {
            return i_state.m_input_id == "input1" &&
                   (i_state.m_starved || i_state.m_eof);
        }));
    EXPECT_EQ(mixer->queuedFrameCount(input0_key_hash), 0u);
    EXPECT_EQ(mixer->queuedFrameCount(input1_key_hash), 0u);

    graph->eventBus()->unSubscribe(EVENT_GRAPH_AUDIO_MIXER_INPUT_STATE_ID,
                                   input_state_token);
    graph->eventBus()->unSubscribe(EVENT_GRAPH_AUDIO_MIXER_STATS_ID,
                                   stats_token);
    EXPECT_EQ(mixer->done(), HJ_OK);
}

TEST(HJPluginAudioMixerTest,
     ClearRestartMutedInputVolumeRestorePublishesFreshStats)
{
    constexpr int kSampleRate = 48000;
    constexpr int kFrameSamples = 1024;
    const int64_t kSecondPtsMs = samplesToPtsMs(kFrameSamples, kSampleRate);

    auto graph = std::make_shared<HJGraph>(
        "HJPluginAudioMixerClearMutedEventsGraph");
    ASSERT_NE(graph, nullptr);

    auto graph_info = std::make_shared<HJKeyStorage>();
    (*graph_info)["graph"] = graph;

    HJAudioInfo::Ptr output_info = std::make_shared<HJAudioInfo>();
    output_info->m_samplesRate = kSampleRate;
    output_info->setChannels(HJ_CHANNEL_DEFAULT);
    output_info->m_sampleFmt = HJ_SAMPLE_FORMAT_DEFAULT;
    output_info->m_sampleCnt = kFrameSamples;
    output_info->m_samplesPerFrame = kFrameSamples;

    HJAudioMixerConfig config{};
    config.m_output_info = output_info;
    config.m_max_inputs = 2;
    config.m_frame_samples = kFrameSamples;
    config.m_sync_window_ms = 50;
    config.m_late_threshold_ms = 200;
    config.m_enable_compand = false;
    config.m_enable_limiter = false;

    auto mixer = std::make_shared<HJTestAudioMixerPlugin>(
        "HJPluginAudioMixerClearMutedEventsTest", graph_info);
    auto source0 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerClearMutedEventsSource0");
    auto source1 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerClearMutedEventsSource1");
    auto sink = std::make_shared<HJTestSinkPlugin>(
        "HJPluginAudioMixerClearMutedEventsSink");
    ASSERT_NE(mixer, nullptr);
    ASSERT_NE(source0, nullptr);
    ASSERT_NE(source1, nullptr);
    ASSERT_NE(sink, nullptr);

    std::vector<HJGraphAudioMixerInputState> input_states;
    auto input_state_token = graph->eventBus()->subscribe(
        EVENT_GRAPH_AUDIO_MIXER_INPUT_STATE_ID,
        [&input_states](const HJGraphAudioMixerInputState& i_state) -> HJRet {
            input_states.push_back(i_state);
            return HJ_OK;
        });
    ASSERT_NE(input_state_token, nullptr);

    std::vector<HJGraphAudioMixerStats> stats_events;
    auto stats_token = graph->eventBus()->subscribe(
        EVENT_GRAPH_AUDIO_MIXER_STATS_ID,
        [&stats_events](const HJGraphAudioMixerStats& i_stats) -> HJRet {
            stats_events.push_back(i_stats);
            return HJ_OK;
        });
    ASSERT_NE(stats_token, nullptr);

    auto init_param = std::make_shared<HJKeyStorage>();
    (*init_param)["createThread"] = false;
    (*init_param)["audioMixerConfig"] = config;
    ASSERT_EQ(mixer->init(init_param), HJ_OK);

    ASSERT_EQ(source0->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(source1->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source0, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source1, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addOutputPlugin(sink, HJMEDIA_TYPE_AUDIO), HJ_OK);

    HJAudioMixerInputDesc input_desc0{};
    input_desc0.m_input_id = "input0";
    input_desc0.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    HJAudioMixerInputDesc input_desc1{};
    input_desc1.m_input_id = "input1";
    input_desc1.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    input_desc1.m_volume = 0.0f;
    ASSERT_NE(input_desc0.m_input_info, nullptr);
    ASSERT_NE(input_desc1.m_input_info, nullptr);

    const size_t input0_key_hash =
        buildPluginKeyHash(source0->getName(), HJMEDIA_TYPE_AUDIO, 0);
    const size_t input1_key_hash =
        buildPluginKeyHash(source1->getName(), HJMEDIA_TYPE_AUDIO, 0);
    ASSERT_EQ(mixer->bindInput(0, input0_key_hash, input_desc0), HJ_OK);
    ASSERT_EQ(mixer->bindInput(1, input1_key_hash, input_desc1), HJ_OK);

    auto first_frame0 = makeTestAudioFrame(output_info, 0);
    auto second_frame0 = makeTestAudioFrame(output_info, kSecondPtsMs);
    auto second_frame1 = makeTestAudioFrame(output_info, kSecondPtsMs);
    ASSERT_NE(first_frame0, nullptr);
    ASSERT_NE(second_frame0, nullptr);
    ASSERT_NE(second_frame1, nullptr);

    ASSERT_EQ(source0->pushFrame(first_frame0), HJ_OK);

    int64_t delay_ms = 0;
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(sink->getFrames().size(), 1u);

    ASSERT_EQ(mixer->flush(input1_key_hash), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    input_states.clear();
    stats_events.clear();

    ASSERT_EQ(mixer->setInputVolume(1, 1.0f), HJ_OK);
    ASSERT_EQ(source1->pushFrame(second_frame1), HJ_OK);
    EXPECT_EQ(mixer->runOnce(&delay_ms), HJ_WOULD_BLOCK);
    EXPECT_TRUE(stats_events.empty());

    ASSERT_EQ(source0->pushFrame(second_frame0), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    ASSERT_EQ(sink->getFrames().size(), 2u);
    ASSERT_NE(sink->getFrames().back(), nullptr);
    EXPECT_EQ(sink->getFrames().back()->getPTS(), kSecondPtsMs);
    ASSERT_EQ(stats_events.size(), 1u);
    EXPECT_EQ(stats_events.back().m_output_frames, 1u);
    EXPECT_EQ(stats_events.back().m_mix_pts,
              kSecondPtsMs + sink->getFrames().back()->getPTS());
    EXPECT_EQ(stats_events.back().m_active_inputs, 2u);
    EXPECT_EQ(stats_events.back().m_starved_inputs, 0u);
    EXPECT_TRUE(std::none_of(input_states.begin(), input_states.end(),
        [](const HJGraphAudioMixerInputState& i_state) {
            return i_state.m_starved || i_state.m_eof;
        }));
    EXPECT_EQ(mixer->queuedFrameCount(input1_key_hash), 0u);

    graph->eventBus()->unSubscribe(EVENT_GRAPH_AUDIO_MIXER_INPUT_STATE_ID,
                                   input_state_token);
    graph->eventBus()->unSubscribe(EVENT_GRAPH_AUDIO_MIXER_STATS_ID,
                                   stats_token);
    EXPECT_EQ(mixer->done(), HJ_OK);
}

TEST(HJPluginAudioMixerTest,
     ZeroLateThresholdRepeatedClearRestartEofKeepsFreshStats)
{
    constexpr int kSampleRate = 48000;
    constexpr int kFrameSamples = 1024;

    auto graph = std::make_shared<HJGraph>(
        "HJPluginAudioMixerZeroLateRepeatedClearEventsGraph");
    ASSERT_NE(graph, nullptr);

    auto graph_info = std::make_shared<HJKeyStorage>();
    (*graph_info)["graph"] = graph;

    HJAudioInfo::Ptr output_info = std::make_shared<HJAudioInfo>();
    output_info->m_samplesRate = kSampleRate;
    output_info->setChannels(HJ_CHANNEL_DEFAULT);
    output_info->m_sampleFmt = HJ_SAMPLE_FORMAT_DEFAULT;
    output_info->m_sampleCnt = kFrameSamples;
    output_info->m_samplesPerFrame = kFrameSamples;

    HJAudioMixerConfig config{};
    config.m_output_info = output_info;
    config.m_max_inputs = 2;
    config.m_frame_samples = kFrameSamples;
    config.m_sync_window_ms = 50;
    config.m_late_threshold_ms = 0;
    config.m_enable_compand = false;
    config.m_enable_limiter = false;

    auto mixer = std::make_shared<HJTestAudioMixerPlugin>(
        "HJPluginAudioMixerZeroLateRepeatedClearEventsTest", graph_info);
    auto source0 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerZeroLateRepeatedClearEventsSource0");
    auto source1 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerZeroLateRepeatedClearEventsSource1");
    auto sink = std::make_shared<HJTestSinkPlugin>(
        "HJPluginAudioMixerZeroLateRepeatedClearEventsSink");
    ASSERT_NE(mixer, nullptr);
    ASSERT_NE(source0, nullptr);
    ASSERT_NE(source1, nullptr);
    ASSERT_NE(sink, nullptr);

    std::vector<HJGraphAudioMixerInputState> input_states;
    auto input_state_token = graph->eventBus()->subscribe(
        EVENT_GRAPH_AUDIO_MIXER_INPUT_STATE_ID,
        [&input_states](const HJGraphAudioMixerInputState& i_state) -> HJRet {
            input_states.push_back(i_state);
            return HJ_OK;
        });
    ASSERT_NE(input_state_token, nullptr);

    std::vector<HJGraphAudioMixerStats> stats_events;
    auto stats_token = graph->eventBus()->subscribe(
        EVENT_GRAPH_AUDIO_MIXER_STATS_ID,
        [&stats_events](const HJGraphAudioMixerStats& i_stats) -> HJRet {
            stats_events.push_back(i_stats);
            return HJ_OK;
        });
    ASSERT_NE(stats_token, nullptr);

    auto init_param = std::make_shared<HJKeyStorage>();
    (*init_param)["createThread"] = false;
    (*init_param)["audioMixerConfig"] = config;
    ASSERT_EQ(mixer->init(init_param), HJ_OK);

    ASSERT_EQ(source0->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(source1->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source0, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source1, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addOutputPlugin(sink, HJMEDIA_TYPE_AUDIO), HJ_OK);

    HJAudioMixerInputDesc input_desc0{};
    input_desc0.m_input_id = "input0";
    input_desc0.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    HJAudioMixerInputDesc input_desc1{};
    input_desc1.m_input_id = "input1";
    input_desc1.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    ASSERT_NE(input_desc0.m_input_info, nullptr);
    ASSERT_NE(input_desc1.m_input_info, nullptr);

    const size_t input0_key_hash =
        buildPluginKeyHash(source0->getName(), HJMEDIA_TYPE_AUDIO, 0);
    const size_t input1_key_hash =
        buildPluginKeyHash(source1->getName(), HJMEDIA_TYPE_AUDIO, 0);
    ASSERT_EQ(mixer->bindInput(0, input0_key_hash, input_desc0), HJ_OK);
    ASSERT_EQ(mixer->bindInput(1, input1_key_hash, input_desc1), HJ_OK);

    auto push_dual_cycle = [&](int64_t* io_delay_ms) {
        auto frame0 = makeTestAudioFrame(output_info, 0);
        auto frame1 = makeTestAudioFrame(output_info, 0);
        ASSERT_NE(frame0, nullptr);
        ASSERT_NE(frame1, nullptr);
        ASSERT_EQ(source0->pushFrame(frame0), HJ_OK);
        ASSERT_EQ(source1->pushFrame(frame1), HJ_OK);
        ASSERT_EQ(mixer->runOnce(io_delay_ms), HJ_OK);
    };

    auto push_single_cycle = [&](int64_t* io_delay_ms) {
        auto frame0 = makeTestAudioFrame(output_info, 0);
        ASSERT_NE(frame0, nullptr);
        ASSERT_EQ(source0->pushFrame(frame0), HJ_OK);
        ASSERT_EQ(mixer->runOnce(io_delay_ms), HJ_OK);
    };

    int64_t delay_ms = 0;
    push_dual_cycle(&delay_ms);
    ASSERT_EQ(sink->getFrames().size(), 1u);

    ASSERT_EQ(mixer->flush(input1_key_hash), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    input_states.clear();
    stats_events.clear();

    push_single_cycle(&delay_ms);
    ASSERT_EQ(sink->getFrames().size(), 2u);
    ASSERT_EQ(stats_events.size(), 1u);
    EXPECT_EQ(stats_events.back().m_output_frames, 1u);
    EXPECT_EQ(stats_events.back().m_active_inputs, 2u);

    input_states.clear();
    stats_events.clear();

    auto eof_frame0 = HJMediaFrame::makeEofFrame(HJMEDIA_TYPE_AUDIO);
    auto eof_frame1 = HJMediaFrame::makeEofFrame(HJMEDIA_TYPE_AUDIO);
    ASSERT_NE(eof_frame0, nullptr);
    ASSERT_NE(eof_frame1, nullptr);
    ASSERT_EQ(source0->pushFrame(eof_frame0), HJ_OK);
    ASSERT_EQ(source1->pushFrame(eof_frame1), HJ_OK);
    EXPECT_EQ(mixer->runOnce(&delay_ms), HJ_WOULD_BLOCK);
    EXPECT_TRUE(stats_events.empty());
    EXPECT_TRUE(std::any_of(input_states.begin(), input_states.end(),
        [](const HJGraphAudioMixerInputState& i_state) {
            return i_state.m_input_id == "input0" && i_state.m_eof;
        }));
    EXPECT_TRUE(std::any_of(input_states.begin(), input_states.end(),
        [](const HJGraphAudioMixerInputState& i_state) {
            return i_state.m_input_id == "input1" && i_state.m_eof;
        }));
    const size_t state_count_before_second_clear = input_states.size();

    ASSERT_EQ(mixer->flush(input1_key_hash), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    stats_events.clear();
    push_single_cycle(&delay_ms);

    ASSERT_EQ(sink->getFrames().size(), 3u);
    ASSERT_EQ(stats_events.size(), 1u);
    EXPECT_EQ(stats_events.back().m_output_frames, 1u);
    EXPECT_EQ(stats_events.back().m_mix_pts,
              samplesToPtsMs(kFrameSamples, kSampleRate));
    ASSERT_GT(input_states.size(), state_count_before_second_clear);
    EXPECT_TRUE(std::none_of(input_states.begin() + state_count_before_second_clear,
                             input_states.end(),
        [](const HJGraphAudioMixerInputState& i_state) {
            return i_state.m_eof;
        }));
    EXPECT_TRUE(std::any_of(input_states.begin() + state_count_before_second_clear,
                            input_states.end(),
        [](const HJGraphAudioMixerInputState& i_state) {
            return i_state.m_input_id == "input1" && i_state.m_starved;
        }));

    graph->eventBus()->unSubscribe(EVENT_GRAPH_AUDIO_MIXER_INPUT_STATE_ID,
                                   input_state_token);
    graph->eventBus()->unSubscribe(EVENT_GRAPH_AUDIO_MIXER_STATS_ID,
                                   stats_token);
    EXPECT_EQ(mixer->done(), HJ_OK);
}

TEST(HJPluginAudioMixerTest,
     ClearRestartFutureOnlyInputPublishesFreshStarvedThenRecovered)
{
    constexpr int kSampleRate = 48000;
    constexpr int kFrameSamples = 1024;
    const int64_t kSecondPtsMs = samplesToPtsMs(kFrameSamples, kSampleRate);
    const int64_t kThirdPtsMs = samplesToPtsMs(kFrameSamples * 2, kSampleRate);

    auto graph = std::make_shared<HJGraph>(
        "HJPluginAudioMixerClearFutureRecoverEventsGraph");
    ASSERT_NE(graph, nullptr);

    auto graph_info = std::make_shared<HJKeyStorage>();
    (*graph_info)["graph"] = graph;

    HJAudioInfo::Ptr output_info = std::make_shared<HJAudioInfo>();
    output_info->m_samplesRate = kSampleRate;
    output_info->setChannels(HJ_CHANNEL_DEFAULT);
    output_info->m_sampleFmt = HJ_SAMPLE_FORMAT_DEFAULT;
    output_info->m_sampleCnt = kFrameSamples;
    output_info->m_samplesPerFrame = kFrameSamples;

    HJAudioMixerConfig config{};
    config.m_output_info = output_info;
    config.m_max_inputs = 2;
    config.m_frame_samples = kFrameSamples;
    config.m_sync_window_ms = 10;
    config.m_late_threshold_ms = 0;
    config.m_enable_compand = false;
    config.m_enable_limiter = false;

    auto mixer = std::make_shared<HJTestAudioMixerPlugin>(
        "HJPluginAudioMixerClearFutureRecoverEventsTest", graph_info);
    auto source0 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerClearFutureRecoverEventsSource0");
    auto source1 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerClearFutureRecoverEventsSource1");
    auto sink = std::make_shared<HJTestSinkPlugin>(
        "HJPluginAudioMixerClearFutureRecoverEventsSink");
    ASSERT_NE(mixer, nullptr);
    ASSERT_NE(source0, nullptr);
    ASSERT_NE(source1, nullptr);
    ASSERT_NE(sink, nullptr);

    std::vector<HJGraphAudioMixerInputState> input_states;
    auto input_state_token = graph->eventBus()->subscribe(
        EVENT_GRAPH_AUDIO_MIXER_INPUT_STATE_ID,
        [&input_states](const HJGraphAudioMixerInputState& i_state) -> HJRet {
            input_states.push_back(i_state);
            return HJ_OK;
        });
    ASSERT_NE(input_state_token, nullptr);

    std::vector<HJGraphAudioMixerStats> stats_events;
    auto stats_token = graph->eventBus()->subscribe(
        EVENT_GRAPH_AUDIO_MIXER_STATS_ID,
        [&stats_events](const HJGraphAudioMixerStats& i_stats) -> HJRet {
            stats_events.push_back(i_stats);
            return HJ_OK;
        });
    ASSERT_NE(stats_token, nullptr);

    auto init_param = std::make_shared<HJKeyStorage>();
    (*init_param)["createThread"] = false;
    (*init_param)["audioMixerConfig"] = config;
    ASSERT_EQ(mixer->init(init_param), HJ_OK);

    ASSERT_EQ(source0->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(source1->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source0, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source1, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addOutputPlugin(sink, HJMEDIA_TYPE_AUDIO), HJ_OK);

    HJAudioMixerInputDesc input_desc0{};
    input_desc0.m_input_id = "input0";
    input_desc0.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    HJAudioMixerInputDesc input_desc1{};
    input_desc1.m_input_id = "input1";
    input_desc1.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    ASSERT_NE(input_desc0.m_input_info, nullptr);
    ASSERT_NE(input_desc1.m_input_info, nullptr);

    const size_t input0_key_hash =
        buildPluginKeyHash(source0->getName(), HJMEDIA_TYPE_AUDIO, 0);
    const size_t input1_key_hash =
        buildPluginKeyHash(source1->getName(), HJMEDIA_TYPE_AUDIO, 0);
    ASSERT_EQ(mixer->bindInput(0, input0_key_hash, input_desc0), HJ_OK);
    ASSERT_EQ(mixer->bindInput(1, input1_key_hash, input_desc1), HJ_OK);

    auto first_frame0 = makeTestAudioFrame(output_info, 0);
    auto first_frame1 = makeTestAudioFrame(output_info, 0);
    auto restart_frame0 = makeTestAudioFrame(output_info, 0);
    auto restart_second_frame0 = makeTestAudioFrame(output_info, kSecondPtsMs);
    auto restart_third_frame0 = makeTestAudioFrame(output_info, kThirdPtsMs);
    auto future_frame1 = makeTestAudioFrame(output_info, kThirdPtsMs);
    ASSERT_NE(first_frame0, nullptr);
    ASSERT_NE(first_frame1, nullptr);
    ASSERT_NE(restart_frame0, nullptr);
    ASSERT_NE(restart_second_frame0, nullptr);
    ASSERT_NE(restart_third_frame0, nullptr);
    ASSERT_NE(future_frame1, nullptr);

    ASSERT_EQ(source0->pushFrame(first_frame0), HJ_OK);
    ASSERT_EQ(source1->pushFrame(first_frame1), HJ_OK);

    int64_t delay_ms = 0;
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(sink->getFrames().size(), 1u);

    ASSERT_EQ(mixer->flush(input1_key_hash), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    input_states.clear();
    stats_events.clear();

    ASSERT_EQ(source0->pushFrame(restart_frame0), HJ_OK);
    ASSERT_EQ(source1->pushFrame(future_frame1), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(sink->getFrames().size(), 2u);
    ASSERT_FALSE(stats_events.empty());
    EXPECT_EQ(stats_events.back().m_output_frames, 1u);
    EXPECT_EQ(stats_events.back().m_starved_inputs, 1u);
    EXPECT_TRUE(std::any_of(input_states.begin(), input_states.end(),
        [](const HJGraphAudioMixerInputState& i_state) {
            return i_state.m_input_id == "input1" && i_state.m_starved &&
                   !i_state.m_eof;
        }));
    const size_t state_count_after_starved = input_states.size();

    ASSERT_EQ(source0->pushFrame(restart_second_frame0), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(source0->pushFrame(restart_third_frame0), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    ASSERT_EQ(sink->getFrames().size(), 4u);
    ASSERT_FALSE(stats_events.empty());
    EXPECT_EQ(stats_events.back().m_output_frames, 3u);
    EXPECT_EQ(stats_events.back().m_starved_inputs, 0u);
    EXPECT_TRUE(std::any_of(input_states.begin() + state_count_after_starved,
                            input_states.end(),
        [](const HJGraphAudioMixerInputState& i_state) {
            return i_state.m_input_id == "input1" && !i_state.m_starved &&
                   !i_state.m_eof;
        }));
    EXPECT_EQ(std::count_if(input_states.begin(), input_states.end(),
                  [](const HJGraphAudioMixerInputState& i_state) {
                      return i_state.m_input_id == "input1" &&
                             i_state.m_starved;
                  }),
              1);

    graph->eventBus()->unSubscribe(EVENT_GRAPH_AUDIO_MIXER_INPUT_STATE_ID,
                                   input_state_token);
    graph->eventBus()->unSubscribe(EVENT_GRAPH_AUDIO_MIXER_STATS_ID,
                                   stats_token);
    EXPECT_EQ(mixer->done(), HJ_OK);
}

TEST(HJPluginAudioMixerTest, ClearResetMuteUnmuteThenEofPublishesFreshState)
{
    constexpr int kSampleRate = 48000;
    constexpr int kFrameSamples = 1024;
    const int64_t kSecondPtsMs = samplesToPtsMs(kFrameSamples, kSampleRate);

    auto graph = std::make_shared<HJGraph>(
        "HJPluginAudioMixerClearMuteUnmuteEofEventsGraph");
    ASSERT_NE(graph, nullptr);

    auto graph_info = std::make_shared<HJKeyStorage>();
    (*graph_info)["graph"] = graph;

    HJAudioInfo::Ptr output_info = std::make_shared<HJAudioInfo>();
    output_info->m_samplesRate = kSampleRate;
    output_info->setChannels(HJ_CHANNEL_DEFAULT);
    output_info->m_sampleFmt = HJ_SAMPLE_FORMAT_DEFAULT;
    output_info->m_sampleCnt = kFrameSamples;
    output_info->m_samplesPerFrame = kFrameSamples;

    HJAudioMixerConfig config{};
    config.m_output_info = output_info;
    config.m_max_inputs = 2;
    config.m_frame_samples = kFrameSamples;
    config.m_sync_window_ms = 50;
    config.m_late_threshold_ms = 200;
    config.m_enable_compand = false;
    config.m_enable_limiter = false;

    auto mixer = std::make_shared<HJTestAudioMixerPlugin>(
        "HJPluginAudioMixerClearMuteUnmuteEofEventsTest", graph_info);
    auto source0 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerClearMuteUnmuteEofEventsSource0");
    auto source1 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerClearMuteUnmuteEofEventsSource1");
    auto sink = std::make_shared<HJTestSinkPlugin>(
        "HJPluginAudioMixerClearMuteUnmuteEofEventsSink");
    ASSERT_NE(mixer, nullptr);
    ASSERT_NE(source0, nullptr);
    ASSERT_NE(source1, nullptr);
    ASSERT_NE(sink, nullptr);

    std::vector<HJGraphAudioMixerInputState> input_states;
    auto input_state_token = graph->eventBus()->subscribe(
        EVENT_GRAPH_AUDIO_MIXER_INPUT_STATE_ID,
        [&input_states](const HJGraphAudioMixerInputState& i_state) -> HJRet {
            input_states.push_back(i_state);
            return HJ_OK;
        });
    ASSERT_NE(input_state_token, nullptr);

    std::vector<HJGraphAudioMixerStats> stats_events;
    auto stats_token = graph->eventBus()->subscribe(
        EVENT_GRAPH_AUDIO_MIXER_STATS_ID,
        [&stats_events](const HJGraphAudioMixerStats& i_stats) -> HJRet {
            stats_events.push_back(i_stats);
            return HJ_OK;
        });
    ASSERT_NE(stats_token, nullptr);

    auto init_param = std::make_shared<HJKeyStorage>();
    (*init_param)["createThread"] = false;
    (*init_param)["audioMixerConfig"] = config;
    ASSERT_EQ(mixer->init(init_param), HJ_OK);

    ASSERT_EQ(source0->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(source1->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source0, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source1, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addOutputPlugin(sink, HJMEDIA_TYPE_AUDIO), HJ_OK);

    HJAudioMixerInputDesc input_desc0{};
    input_desc0.m_input_id = "input0";
    input_desc0.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    HJAudioMixerInputDesc input_desc1{};
    input_desc1.m_input_id = "input1";
    input_desc1.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    input_desc1.m_volume = 0.0f;
    ASSERT_NE(input_desc0.m_input_info, nullptr);
    ASSERT_NE(input_desc1.m_input_info, nullptr);

    const size_t input0_key_hash =
        buildPluginKeyHash(source0->getName(), HJMEDIA_TYPE_AUDIO, 0);
    const size_t input1_key_hash =
        buildPluginKeyHash(source1->getName(), HJMEDIA_TYPE_AUDIO, 0);
    ASSERT_EQ(mixer->bindInput(0, input0_key_hash, input_desc0), HJ_OK);
    ASSERT_EQ(mixer->bindInput(1, input1_key_hash, input_desc1), HJ_OK);

    auto first_frame0 = makeTestAudioFrame(output_info, 0);
    auto restart_frame0 = makeTestAudioFrame(output_info, 0);
    auto restart_frame1 = makeTestAudioFrame(output_info, 0);
    auto second_frame0 = makeTestAudioFrame(output_info, kSecondPtsMs);
    auto eof_frame1 = HJMediaFrame::makeEofFrame(HJMEDIA_TYPE_AUDIO);
    ASSERT_NE(first_frame0, nullptr);
    ASSERT_NE(restart_frame0, nullptr);
    ASSERT_NE(restart_frame1, nullptr);
    ASSERT_NE(second_frame0, nullptr);
    ASSERT_NE(eof_frame1, nullptr);

    ASSERT_EQ(source0->pushFrame(first_frame0), HJ_OK);

    int64_t delay_ms = 0;
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(sink->getFrames().size(), 1u);

    ASSERT_EQ(mixer->flush(input1_key_hash), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

    input_states.clear();
    stats_events.clear();

    ASSERT_EQ(mixer->setInputVolume(1, 1.0f), HJ_OK);
    ASSERT_EQ(source0->pushFrame(restart_frame0), HJ_OK);
    ASSERT_EQ(source1->pushFrame(restart_frame1), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(sink->getFrames().size(), 2u);
    ASSERT_EQ(stats_events.size(), 1u);
    EXPECT_EQ(stats_events.back().m_output_frames, 1u);
    EXPECT_EQ(stats_events.back().m_starved_inputs, 0u);
    EXPECT_TRUE(std::none_of(input_states.begin(), input_states.end(),
        [](const HJGraphAudioMixerInputState& i_state) {
            return i_state.m_starved || i_state.m_eof;
        }));

    input_states.clear();
    stats_events.clear();

    ASSERT_EQ(source0->pushFrame(second_frame0), HJ_OK);
    ASSERT_EQ(source1->pushFrame(eof_frame1), HJ_OK);
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(sink->getFrames().size(), 3u);
    ASSERT_FALSE(stats_events.empty());
    EXPECT_EQ(stats_events.back().m_starved_inputs, 0u);
    EXPECT_EQ(stats_events.back().m_active_inputs, 1u);
    EXPECT_TRUE(std::any_of(input_states.begin(), input_states.end(),
        [](const HJGraphAudioMixerInputState& i_state) {
            return i_state.m_input_id == "input1" && i_state.m_eof &&
                   !i_state.m_starved;
        }));
    EXPECT_TRUE(std::none_of(input_states.begin(), input_states.end(),
        [](const HJGraphAudioMixerInputState& i_state) {
            return i_state.m_input_id == "input1" && i_state.m_starved;
        }));

    graph->eventBus()->unSubscribe(EVENT_GRAPH_AUDIO_MIXER_INPUT_STATE_ID,
                                   input_state_token);
    graph->eventBus()->unSubscribe(EVENT_GRAPH_AUDIO_MIXER_STATS_ID,
                                   stats_token);
    EXPECT_EQ(mixer->done(), HJ_OK);
}

TEST(HJPluginAudioMixerTest,
     ZeroLateThresholdMultipleClearRestartResetsStatsEachRound)
{
    constexpr int kSampleRate = 48000;
    constexpr int kFrameSamples = 1024;

    auto graph = std::make_shared<HJGraph>(
        "HJPluginAudioMixerZeroLateMultiRestartEventsGraph");
    ASSERT_NE(graph, nullptr);

    auto graph_info = std::make_shared<HJKeyStorage>();
    (*graph_info)["graph"] = graph;

    HJAudioInfo::Ptr output_info = std::make_shared<HJAudioInfo>();
    output_info->m_samplesRate = kSampleRate;
    output_info->setChannels(HJ_CHANNEL_DEFAULT);
    output_info->m_sampleFmt = HJ_SAMPLE_FORMAT_DEFAULT;
    output_info->m_sampleCnt = kFrameSamples;
    output_info->m_samplesPerFrame = kFrameSamples;

    HJAudioMixerConfig config{};
    config.m_output_info = output_info;
    config.m_max_inputs = 2;
    config.m_frame_samples = kFrameSamples;
    config.m_sync_window_ms = 50;
    config.m_late_threshold_ms = 0;
    config.m_enable_compand = false;
    config.m_enable_limiter = false;

    auto mixer = std::make_shared<HJTestAudioMixerPlugin>(
        "HJPluginAudioMixerZeroLateMultiRestartEventsTest", graph_info);
    auto source0 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerZeroLateMultiRestartEventsSource0");
    auto source1 = std::make_shared<HJTestSourcePlugin>(
        "HJPluginAudioMixerZeroLateMultiRestartEventsSource1");
    auto sink = std::make_shared<HJTestSinkPlugin>(
        "HJPluginAudioMixerZeroLateMultiRestartEventsSink");
    ASSERT_NE(mixer, nullptr);
    ASSERT_NE(source0, nullptr);
    ASSERT_NE(source1, nullptr);
    ASSERT_NE(sink, nullptr);

    std::vector<HJGraphAudioMixerStats> stats_events;
    auto stats_token = graph->eventBus()->subscribe(
        EVENT_GRAPH_AUDIO_MIXER_STATS_ID,
        [&stats_events](const HJGraphAudioMixerStats& i_stats) -> HJRet {
            stats_events.push_back(i_stats);
            return HJ_OK;
        });
    ASSERT_NE(stats_token, nullptr);

    auto init_param = std::make_shared<HJKeyStorage>();
    (*init_param)["createThread"] = false;
    (*init_param)["audioMixerConfig"] = config;
    ASSERT_EQ(mixer->init(init_param), HJ_OK);

    ASSERT_EQ(source0->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(source1->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source0, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source1, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addOutputPlugin(sink, HJMEDIA_TYPE_AUDIO), HJ_OK);

    HJAudioMixerInputDesc input_desc0{};
    input_desc0.m_input_id = "input0";
    input_desc0.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    HJAudioMixerInputDesc input_desc1{};
    input_desc1.m_input_id = "input1";
    input_desc1.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    ASSERT_NE(input_desc0.m_input_info, nullptr);
    ASSERT_NE(input_desc1.m_input_info, nullptr);

    const size_t input0_key_hash =
        buildPluginKeyHash(source0->getName(), HJMEDIA_TYPE_AUDIO, 0);
    const size_t input1_key_hash =
        buildPluginKeyHash(source1->getName(), HJMEDIA_TYPE_AUDIO, 0);
    ASSERT_EQ(mixer->bindInput(0, input0_key_hash, input_desc0), HJ_OK);
    ASSERT_EQ(mixer->bindInput(1, input1_key_hash, input_desc1), HJ_OK);

    auto first_frame0 = makeTestAudioFrame(output_info, 0);
    auto first_frame1 = makeTestAudioFrame(output_info, 0);
    ASSERT_NE(first_frame0, nullptr);
    ASSERT_NE(first_frame1, nullptr);
    ASSERT_EQ(source0->pushFrame(first_frame0), HJ_OK);
    ASSERT_EQ(source1->pushFrame(first_frame1), HJ_OK);

    int64_t delay_ms = 0;
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    ASSERT_EQ(sink->getFrames().size(), 1u);

    for (int i = 0; i < 3; ++i) {
        ASSERT_EQ(mixer->flush(input1_key_hash), HJ_OK);
        ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

        stats_events.clear();

        auto restart_frame0 = makeTestAudioFrame(output_info, 0);
        ASSERT_NE(restart_frame0, nullptr);
        ASSERT_EQ(source0->pushFrame(restart_frame0), HJ_OK);
        ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);

        ASSERT_EQ(stats_events.size(), 1u);
        EXPECT_EQ(stats_events.back().m_output_frames, 1u);
        EXPECT_EQ(stats_events.back().m_mix_pts,
                  samplesToPtsMs(kFrameSamples, kSampleRate));
        EXPECT_EQ(stats_events.back().m_active_inputs, 2u);
    }

    ASSERT_EQ(sink->getFrames().size(), 4u);

    graph->eventBus()->unSubscribe(EVENT_GRAPH_AUDIO_MIXER_STATS_ID,
                                   stats_token);
    EXPECT_EQ(mixer->done(), HJ_OK);
}

TEST(HJPluginAudioMixerTest, LateDropReportsNotifyOnlyOncePerRunTask)
{
    constexpr int kSampleRate = 48000;
    constexpr int kFrameSamples = 1024;
    constexpr int64_t kCurrentPtsMs = 21;

    HJAudioInfo::Ptr output_info = std::make_shared<HJAudioInfo>();
    output_info->m_samplesRate = kSampleRate;
    output_info->setChannels(HJ_CHANNEL_DEFAULT);
    output_info->m_sampleFmt = HJ_SAMPLE_FORMAT_DEFAULT;
    output_info->m_sampleCnt = kFrameSamples;
    output_info->m_samplesPerFrame = kFrameSamples;

    auto graph = std::make_shared<HJGraph>("HJPluginAudioMixerNotifyGraph");
    ASSERT_NE(graph, nullptr);

    auto graph_info = std::make_shared<HJKeyStorage>();
    (*graph_info)["graph"] = graph;

    HJAudioMixerConfig config{};
    config.m_output_info = output_info;
    config.m_max_inputs = 1;
    config.m_frame_samples = kFrameSamples;
    config.m_sync_window_ms = 0;
    config.m_late_threshold_ms = 0;
    config.m_enable_compand = false;
    config.m_enable_limiter = false;

    auto mixer = std::make_shared<HJTestAudioMixerPlugin>("HJPluginAudioMixerNotifyTest", graph_info);
    auto source = std::make_shared<HJTestSourcePlugin>("HJPluginAudioMixerNotifySource");
    auto sink = std::make_shared<HJTestSinkPlugin>("HJPluginAudioMixerNotifySink");
    ASSERT_NE(mixer, nullptr);
    ASSERT_NE(source, nullptr);
    ASSERT_NE(sink, nullptr);

    size_t late_drop_notify_count = 0;
    ASSERT_EQ(
        graph->eventBus()->registerHandler(EVENT_PLUGIN_NOTIFY_ID,
            [&late_drop_notify_count, mixer](HJPluginNotifyType i_notify_type, size_t i_plugin_id) -> HJRet {
                if (i_notify_type == HJ_PLUGIN_NOTIFY_ERROR_AUDIOMIXER_MIXFRAME && i_plugin_id == mixer->getID()) {
                    ++late_drop_notify_count;
                }
                return HJ_OK;
            }),
        HJ_OK);

    auto init_param = std::make_shared<HJKeyStorage>();
    (*init_param)["createThread"] = false;
    (*init_param)["audioMixerConfig"] = config;
    ASSERT_EQ(mixer->init(init_param), HJ_OK);

    ASSERT_EQ(source->addOutputPlugin(mixer, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addInputPlugin(source, HJMEDIA_TYPE_AUDIO), HJ_OK);
    ASSERT_EQ(mixer->addOutputPlugin(sink, HJMEDIA_TYPE_AUDIO), HJ_OK);

    HJAudioMixerInputDesc input_desc{};
    input_desc.m_input_id = "input0";
    input_desc.m_input_info = std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    ASSERT_NE(input_desc.m_input_info, nullptr);

    const size_t input_key_hash = buildPluginKeyHash(source->getName(), HJMEDIA_TYPE_AUDIO, 0);
    ASSERT_EQ(mixer->bindInput(0, input_key_hash, input_desc), HJ_OK);

    auto startup_frame = makeTestAudioFrame(output_info, 0);
    ASSERT_NE(startup_frame, nullptr);
    ASSERT_EQ(source->pushFrame(startup_frame), HJ_OK);

    int64_t delay_ms = 0;
    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    EXPECT_EQ(late_drop_notify_count, 0u);

    const int64_t stale_pts_ms[] = { 0, 1, 2 };
    for (const int64_t stale_pts : stale_pts_ms) {
        auto stale_frame = makeTestAudioFrame(output_info, stale_pts);
        ASSERT_NE(stale_frame, nullptr);
        ASSERT_EQ(source->pushFrame(stale_frame), HJ_OK);
    }
    auto current_frame = makeTestAudioFrame(output_info, kCurrentPtsMs);
    ASSERT_NE(current_frame, nullptr);
    ASSERT_EQ(source->pushFrame(current_frame), HJ_OK);

    ASSERT_EQ(mixer->runOnce(&delay_ms), HJ_OK);
    EXPECT_EQ(late_drop_notify_count, 1u);
    EXPECT_EQ(mixer->queuedFrameCount(input_key_hash), 0u);

    EXPECT_EQ(mixer->done(), HJ_OK);
}

TEST(HJGraphAudioMixerTest, DefaultOutputPublishesAacFrameEvent)
{
    constexpr int kSampleRate = 48000;
    constexpr int kFrameSamples = 1024;

    HJAudioInfo::Ptr output_info = std::make_shared<HJAudioInfo>();
    output_info->m_samplesRate = kSampleRate;
    output_info->setChannels(HJ_CHANNEL_DEFAULT);
    output_info->m_sampleFmt = HJ_SAMPLE_FORMAT_DEFAULT;
    output_info->m_sampleCnt = kFrameSamples;
    output_info->m_samplesPerFrame = kFrameSamples;

    HJAudioMixerConfig config{};
    config.m_output_info = output_info;
    config.m_max_inputs = 2;
    config.m_frame_samples = kFrameSamples;
    config.m_sync_window_ms = 0;
    config.m_aac_type = 0;
    config.m_enable_compand = false;
    config.m_enable_limiter = false;

    HJAudioMixerInputDesc input_desc{};
    input_desc.m_input_id = "input0";
    input_desc.m_input_info = std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    ASSERT_NE(input_desc.m_input_info, nullptr);
    config.m_inputs.push_back(input_desc);

    auto mixer = std::make_shared<HJGraphAudioMixer>("HJGraphAudioMixerConfiguredInputTest");
    ASSERT_NE(mixer, nullptr);

    auto output_info_before_init = mixer->queryBus()->query<QUERY_GRAPH_AUDIO_MIXER_OUTPUT_INFO>();
    EXPECT_EQ(output_info_before_init.code, HJErrNotFind);
    EXPECT_EQ(output_info_before_init.value, nullptr);

    auto output_frame_promise =
        std::make_shared<std::promise<HJMediaFrame::Ptr>>();
    auto output_frame_future = output_frame_promise->get_future();
    auto delivered = std::make_shared<std::atomic<bool>>(false);
    auto audio_frame_token = mixer->eventBus()->subscribe(EVENT_GRAPH_AUDIO_FRAME_ID,
        [output_frame_promise, delivered](const HJMediaFrame::Ptr& i_frame) -> HJRet {
            if (i_frame == nullptr) {
                return HJErrInvalidParams;
            }
            bool expected = false;
            if (delivered->compare_exchange_strong(expected, true)) {
                output_frame_promise->set_value(i_frame->deepDup());
            }
            return HJ_OK;
        });
    ASSERT_NE(audio_frame_token, nullptr);

    ASSERT_EQ(mixer->initWithConfig(config), HJ_OK);

    auto mixer_output_info = mixer->queryBus()->query<QUERY_GRAPH_AUDIO_MIXER_OUTPUT_INFO>();
    ASSERT_EQ(mixer_output_info.code, HJ_OK);
    ASSERT_NE(mixer_output_info.value, nullptr);
    EXPECT_EQ(mixer_output_info.value->m_sampleCnt, HJ_AUDIO_MIXER_OUTPUT_SAMPLES);
    EXPECT_EQ(mixer_output_info.value->m_samplesPerFrame, HJ_AUDIO_MIXER_OUTPUT_SAMPLES);

    auto frame = makeTestAudioFrame(output_info, 0);
    ASSERT_NE(frame, nullptr);
    ASSERT_EQ(mixer->pushFrame("input0", frame), HJ_OK);

    EXPECT_EQ(output_frame_future.wait_for(std::chrono::seconds(2)), std::future_status::ready);
    ASSERT_EQ(output_frame_future.wait_for(std::chrono::seconds(0)),
              std::future_status::ready);

    auto output_frame = output_frame_future.get();
    ASSERT_NE(output_frame, nullptr);
    ASSERT_NE(output_frame->getAudioInfo(), nullptr);
    EXPECT_TRUE(HJMediaFrame::isCodecMatchAAC(output_frame->getAudioInfo()->m_codecID));
    EXPECT_NE(output_frame->getAudioInfo()->getCodecParams(), nullptr);
    EXPECT_EQ(output_frame->getPTS(), 0);

    mixer->eventBus()->unSubscribe(EVENT_GRAPH_AUDIO_FRAME_ID, audio_frame_token);
    EXPECT_EQ(mixer->done(), HJ_OK);
}

TEST(HJGraphAudioMixerTest, AdtsOutputPublishesFramesWithAdtsHeader)
{
    constexpr int kSampleRate = 48000;
    constexpr int kFrameSamples = 1024;

    auto output_info =
        HJAudioInfo::makeAudioInfo(2, kSampleRate, AV_SAMPLE_FMT_S16, kFrameSamples);
    ASSERT_NE(output_info, nullptr);
    output_info->m_samplesPerFrame = kFrameSamples;

    HJAudioMixerConfig config{};
    config.m_output_info = output_info;
    config.m_max_inputs = 1;
    config.m_frame_samples = kFrameSamples;
    config.m_sync_window_ms = 0;
    config.m_aac_type = 2;
    config.m_enable_compand = false;
    config.m_enable_limiter = false;

    HJAudioMixerInputDesc input_desc{};
    input_desc.m_input_id = "input0";
    input_desc.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    ASSERT_NE(input_desc.m_input_info, nullptr);
    config.m_inputs.push_back(input_desc);

    auto mixer =
        std::make_shared<HJGraphAudioMixer>("HJGraphAudioMixerAdtsOutputTest");
    ASSERT_NE(mixer, nullptr);

    auto output_frame_promise =
        std::make_shared<std::promise<HJMediaFrame::Ptr>>();
    auto output_frame_future = output_frame_promise->get_future();
    auto delivered = std::make_shared<std::atomic<bool>>(false);
    auto audio_frame_token = mixer->eventBus()->subscribe(
        EVENT_GRAPH_AUDIO_FRAME_ID,
        [output_frame_promise, delivered](const HJMediaFrame::Ptr& i_frame) -> HJRet {
            if (i_frame == nullptr) {
                return HJErrInvalidParams;
            }

            bool expected = false;
            if (delivered->compare_exchange_strong(expected, true)) {
                output_frame_promise->set_value(i_frame->deepDup());
            }
            return HJ_OK;
        });
    ASSERT_NE(audio_frame_token, nullptr);

    ASSERT_EQ(mixer->initWithConfig(config), HJ_OK);

    auto frame = makeTestAudioFrame(output_info, 0);
    ASSERT_NE(frame, nullptr);
    ASSERT_EQ(mixer->pushFrame("input0", frame), HJ_OK);

    EXPECT_EQ(output_frame_future.wait_for(std::chrono::seconds(2)),
              std::future_status::ready);
    ASSERT_EQ(output_frame_future.wait_for(std::chrono::seconds(0)),
              std::future_status::ready);

    auto output_frame = output_frame_future.get();
    ASSERT_NE(output_frame, nullptr);
    EXPECT_TRUE(isAdtsPacket(output_frame));

    mixer->eventBus()->unSubscribe(EVENT_GRAPH_AUDIO_FRAME_ID, audio_frame_token);
    EXPECT_EQ(mixer->done(), HJ_OK);
}

TEST(HJGraphAudioMixerTest, PcmOutputModeAcceptsPlanarS16Input)
{
    constexpr int kSampleRate = 48000;
    constexpr int kFrameSamples = 1024;
    constexpr int kChannels = 2;
    constexpr int16_t kLeftValue = 1000;
    constexpr int16_t kRightValue = -1000;
    constexpr int64_t kPtsMs = kFrameSamples;

    auto output_info =
        HJAudioInfo::makeAudioInfo(kChannels, kSampleRate, AV_SAMPLE_FMT_S16, kFrameSamples);
    ASSERT_NE(output_info, nullptr);
    output_info->m_samplesPerFrame = kFrameSamples;

    HJAudioMixerConfig config{};
    config.m_output_info = output_info;
    config.m_out_type = 1;
    config.m_max_inputs = 1;
    config.m_frame_samples = kFrameSamples;
    config.m_sync_window_ms = 0;
    config.m_enable_compand = false;
    config.m_enable_limiter = false;

    HJAudioMixerInputDesc input_desc{};
    input_desc.m_input_id = "input0";
    input_desc.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(output_info->dup());
    ASSERT_NE(input_desc.m_input_info, nullptr);
    config.m_inputs.push_back(input_desc);

    auto mixer = std::make_shared<HJGraphAudioMixer>(
        "HJGraphAudioMixerPlanarPcmInputTest");
    ASSERT_NE(mixer, nullptr);

    auto output_frame_promise =
        std::make_shared<std::promise<HJMediaFrame::Ptr>>();
    auto output_frame_future = output_frame_promise->get_future();
    auto delivered = std::make_shared<std::atomic<bool>>(false);
    auto audio_frame_token = mixer->eventBus()->subscribe(
        EVENT_GRAPH_AUDIO_FRAME_ID,
        [output_frame_promise, delivered](const HJMediaFrame::Ptr& i_frame) -> HJRet {
            if (i_frame == nullptr) {
                return HJErrInvalidParams;
            }

            bool expected = false;
            if (delivered->compare_exchange_strong(expected, true)) {
                output_frame_promise->set_value(i_frame->deepDup());
            }
            return HJ_OK;
        });
    ASSERT_NE(audio_frame_token, nullptr);

    ASSERT_EQ(mixer->initWithConfig(config), HJ_OK);

    std::vector<uint8_t> pcm = makePlanarS16PcmBuffer(
        kChannels, kFrameSamples, { kLeftValue, kRightValue });
    ASSERT_FALSE(pcm.empty());

    ASSERT_EQ(
        mixer->pushFrame("input0", pcm.data(), kFrameSamples, kChannels,
                         kSampleRate, AV_SAMPLE_FMT_S16P, kPtsMs),
        HJ_OK);

    EXPECT_EQ(output_frame_future.wait_for(std::chrono::seconds(2)),
              std::future_status::ready);
    ASSERT_EQ(output_frame_future.wait_for(std::chrono::seconds(0)),
              std::future_status::ready);

    auto mixed_frame = output_frame_future.get();
    ASSERT_NE(mixed_frame, nullptr);
    ASSERT_NE(mixed_frame->getAudioInfo(), nullptr);
    EXPECT_EQ(mixed_frame->getAudioInfo()->m_sampleFmt, AV_SAMPLE_FMT_S16);
    EXPECT_EQ(mixed_frame->getPTS(), kPtsMs);

    AVFrame* mixed_avf = mixed_frame->getAVFrame();
    ASSERT_NE(mixed_avf, nullptr);
    ASSERT_EQ(mixed_avf->format, AV_SAMPLE_FMT_S16);
    ASSERT_NE(mixed_avf->data[0], nullptr);
    ASSERT_GT(mixed_avf->nb_samples, 0);

    const int16_t* mixed_samples =
        reinterpret_cast<const int16_t*>(mixed_avf->data[0]);
    ASSERT_NE(mixed_samples, nullptr);
    EXPECT_EQ(mixed_samples[0], kLeftValue);
    EXPECT_EQ(mixed_samples[1], kRightValue);

    mixer->eventBus()->unSubscribe(EVENT_GRAPH_AUDIO_FRAME_ID, audio_frame_token);
    EXPECT_EQ(mixer->done(), HJ_OK);
}

TEST(HJGraphAudioMixerTest,
     PcmOutputModeNormalizesConfiguredInputSampleRateToMixerOutputInfo)
{
    constexpr int kInputSampleRate = 44100;
    constexpr int kOutputSampleRate = 48000;
    constexpr int kFrameSamples = 1024;
    constexpr int kChannels = 2;
    constexpr int16_t kLeftValue = 1200;
    constexpr int16_t kRightValue = -800;
    constexpr int64_t kPtsMs = 0;

    auto output_info =
        HJAudioInfo::makeAudioInfo(kChannels, kOutputSampleRate, AV_SAMPLE_FMT_S16,
                                   kFrameSamples);
    ASSERT_NE(output_info, nullptr);
    output_info->m_samplesPerFrame = kFrameSamples;

    auto input_info =
        HJAudioInfo::makeAudioInfo(kChannels, kInputSampleRate, AV_SAMPLE_FMT_S16P,
                                   kFrameSamples);
    ASSERT_NE(input_info, nullptr);
    input_info->m_samplesPerFrame = kFrameSamples;

    HJAudioMixerConfig config{};
    config.m_output_info = output_info;
    config.m_out_type = 1;
    config.m_max_inputs = 1;
    config.m_frame_samples = kFrameSamples;
    config.m_sync_window_ms = 0;
    config.m_enable_compand = false;
    config.m_enable_limiter = false;

    HJAudioMixerInputDesc input_desc{};
    input_desc.m_input_id = "input0";
    input_desc.m_input_info =
        std::dynamic_pointer_cast<HJAudioInfo>(input_info->dup());
    ASSERT_NE(input_desc.m_input_info, nullptr);
    config.m_inputs.push_back(input_desc);

    auto mixer = std::make_shared<HJGraphAudioMixer>(
        "HJGraphAudioMixerNormalizeInputInfoTest");
    ASSERT_NE(mixer, nullptr);

    auto output_frame_promise =
        std::make_shared<std::promise<HJMediaFrame::Ptr>>();
    auto output_frame_future = output_frame_promise->get_future();
    auto delivered = std::make_shared<std::atomic<bool>>(false);
    auto audio_frame_token = mixer->eventBus()->subscribe(
        EVENT_GRAPH_AUDIO_FRAME_ID,
        [output_frame_promise, delivered](const HJMediaFrame::Ptr& i_frame) -> HJRet {
            if (i_frame == nullptr) {
                return HJErrInvalidParams;
            }

            bool expected = false;
            if (delivered->compare_exchange_strong(expected, true)) {
                output_frame_promise->set_value(i_frame->deepDup());
            }
            return HJ_OK;
        });
    ASSERT_NE(audio_frame_token, nullptr);

    ASSERT_EQ(mixer->initWithConfig(config), HJ_OK);

    std::vector<uint8_t> pcm = makePlanarS16PcmBuffer(
        kChannels, kFrameSamples, { kLeftValue, kRightValue });
    ASSERT_FALSE(pcm.empty());

    ASSERT_EQ(
        mixer->pushFrame("input0", pcm.data(), kFrameSamples, kChannels,
                         kInputSampleRate, AV_SAMPLE_FMT_S16P, kPtsMs),
        HJ_OK);

    EXPECT_EQ(output_frame_future.wait_for(std::chrono::seconds(2)),
              std::future_status::ready);
    ASSERT_EQ(output_frame_future.wait_for(std::chrono::seconds(0)),
              std::future_status::ready);

    auto mixed_frame = output_frame_future.get();
    ASSERT_NE(mixed_frame, nullptr);
    ASSERT_NE(mixed_frame->getAudioInfo(), nullptr);
    EXPECT_EQ(mixed_frame->getAudioInfo()->m_samplesRate, kOutputSampleRate);
    EXPECT_EQ(mixed_frame->getAudioInfo()->m_sampleFmt, AV_SAMPLE_FMT_S16);
    EXPECT_EQ(mixed_frame->getAudioInfo()->m_channels, kChannels);
    EXPECT_EQ(mixed_frame->getPTS(), kPtsMs);

    mixer->eventBus()->unSubscribe(EVENT_GRAPH_AUDIO_FRAME_ID, audio_frame_token);
    EXPECT_EQ(mixer->done(), HJ_OK);
}

TEST(HJGraphAudioMixerTest, InitRejectsInvalidConfiguredInputs)
{
    constexpr int kSampleRate = 48000;
    constexpr int kFrameSamples = 1024;

    auto make_output_info = [&]() -> HJAudioInfo::Ptr {
        HJAudioInfo::Ptr output_info = std::make_shared<HJAudioInfo>();
        output_info->m_samplesRate = kSampleRate;
        output_info->setChannels(HJ_CHANNEL_DEFAULT);
        output_info->m_sampleFmt = HJ_SAMPLE_FORMAT_DEFAULT;
        output_info->m_sampleCnt = kFrameSamples;
        output_info->m_samplesPerFrame = kFrameSamples;
        return output_info;
    };

    auto make_input_desc = [&](const std::string& i_input_id) -> HJAudioMixerInputDesc {
        HJAudioMixerInputDesc input_desc{};
        input_desc.m_input_id = i_input_id;
        input_desc.m_input_info = make_output_info();
        return input_desc;
    };

    HJAudioMixerConfig too_many_inputs_config{};
    too_many_inputs_config.m_output_info = make_output_info();
    too_many_inputs_config.m_max_inputs = 1;
    too_many_inputs_config.m_inputs.push_back(make_input_desc("input0"));
    too_many_inputs_config.m_inputs.push_back(make_input_desc("input1"));

    auto too_many_inputs_mixer = std::make_shared<HJGraphAudioMixer>("HJGraphAudioMixerTooManyInputsTest");
    ASSERT_NE(too_many_inputs_mixer, nullptr);
    EXPECT_EQ(too_many_inputs_mixer->initWithConfig(too_many_inputs_config), HJErrInvalidParams);

    HJAudioMixerConfig duplicate_id_config{};
    duplicate_id_config.m_output_info = make_output_info();
    duplicate_id_config.m_max_inputs = 2;
    duplicate_id_config.m_inputs.push_back(make_input_desc("duplicated"));
    duplicate_id_config.m_inputs.push_back(make_input_desc("duplicated"));

    auto duplicate_id_mixer = std::make_shared<HJGraphAudioMixer>("HJGraphAudioMixerDuplicateInputsTest");
    ASSERT_NE(duplicate_id_mixer, nullptr);
    EXPECT_EQ(duplicate_id_mixer->initWithConfig(duplicate_id_config), HJErrInvalidParams);

    HJAudioMixerConfig empty_input_id_config{};
    empty_input_id_config.m_output_info = make_output_info();
    empty_input_id_config.m_max_inputs = 1;
    empty_input_id_config.m_inputs.push_back(make_input_desc(""));

    auto empty_input_id_mixer = std::make_shared<HJGraphAudioMixer>("HJGraphAudioMixerEmptyInputIdTest");
    ASSERT_NE(empty_input_id_mixer, nullptr);
    EXPECT_EQ(empty_input_id_mixer->initWithConfig(empty_input_id_config), HJErrInvalidParams);

    HJAudioMixerConfig missing_input_info_config{};
    missing_input_info_config.m_output_info = make_output_info();
    missing_input_info_config.m_max_inputs = 1;
    HJAudioMixerInputDesc missing_input_info_desc{};
    missing_input_info_desc.m_input_id = "input0";
    missing_input_info_config.m_inputs.push_back(missing_input_info_desc);

    auto missing_input_info_mixer = std::make_shared<HJGraphAudioMixer>("HJGraphAudioMixerMissingInputInfoTest");
    ASSERT_NE(missing_input_info_mixer, nullptr);
    EXPECT_EQ(missing_input_info_mixer->initWithConfig(missing_input_info_config), HJErrInvalidParams);
}

TEST(HJGraphPlayerViewTest, ResolveGraphTypeUsesVodForFileAndReplayInputs)
{
    EXPECT_EQ(
        HJGraphPlayerView::resolveGraphTypeForUrl("E:/movies/720x1280.mp4", HJGraphPlayerView::kPlayerTypeAuto),
        HJGraphType_VOD);
    EXPECT_EQ(
        HJGraphPlayerView::resolveGraphTypeForUrl(
            "https://file-22.huajiao.com/record/main/HJ_0_ali_1_main__h265_271393148_1765772819219_8223_O/replay.m3u8",
            HJGraphPlayerView::kPlayerTypeAuto),
        HJGraphType_VOD);
    EXPECT_EQ(
        HJGraphPlayerView::resolveGraphTypeForUrl(
            "E:/movies/audio/c58733ac51124fe38cdc6540a7b8fa46.mkv",
            HJGraphPlayerView::kPlayerTypeAuto),
        HJGraphType_VOD);
}

TEST(HJGraphPlayerViewTest, ResolveGraphTypeAllowsLiveOverrideAndDetectsLiveUrls)
{
    EXPECT_EQ(
        HJGraphPlayerView::resolveGraphTypeForUrl(
            "https://live-pull-3.huajiao.com/main/HJ_0_tc_3_main__h265_274016599_1774343948963_3244_O.flv?txSecret=4636c70d7181bd0b0ea80be40cea4464&txTime=69C3A999",
            HJGraphPlayerView::kPlayerTypeAuto),
        HJGraphType_LIVESTREAM);
    EXPECT_EQ(
        HJGraphPlayerView::resolveGraphTypeForUrl("E:/movies/720x1280.mp4", HJGraphPlayerView::kPlayerTypeLive),
        HJGraphType_LIVESTREAM);
    EXPECT_EQ(
        HJGraphPlayerView::resolveGraphTypeForUrl("rtmp://localhost/live/livestream", HJGraphPlayerView::kPlayerTypeAuto),
        HJGraphType_LIVESTREAM);
}

// Forward declaration of the function that will run the tests.
bool hasGtestFilterArg(int argc, const char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i] ? argv[i] : "";
        if (arg.find("--gtest_filter=") == 0) {
            return true;
        }
    }
    return false;
}

void runHJExecutorPoolTests(int argc, const char* argv[]) {
    HJFLogi("Running HJExecutorPool Tests...");
    const bool has_cli_filter = hasGtestFilterArg(argc, argv);
    // Initialize Google Test. It requires a non-const char**, so we use const_cast.
    // This is a common practice when interfacing with libraries that don't use const correctly.
    ::testing::InitGoogleTest(&argc, const_cast<char**>(argv));

    // Set a filter to run only the tests from the HJExecutorPoolTest suite.
    // The wildcard '*' ensures all tests within this suite are run.
    //::testing::GTEST_FLAG(filter) = "HJExecutorPoolTest*";
    //::testing::GTEST_FLAG(filter) = "HJSEITest*";
    //::testing::GTEST_FLAG(filter) = "HJPackerManagerTest*";
    //::testing::GTEST_FLAG(filter) = "HJM3U8ParserTest*";
    //::testing::GTEST_FLAG(filter) = "HJComplexDemuxerTest*";
    if (!has_cli_filter) {
        ::testing::GTEST_FLAG(filter) =
            "HJJsonReflectionTest*:HJPluginAudioMixerTest*:HJGraphAudioMixerTest*:HJLiveResourceFetcherTest*";
    }
    //  ::testing::GTEST_FLAG(filter) = "HJMediaDBManagerTest*";
    // ::testing::GTEST_FLAG(filter) = "HJXIOBlobFileTest*";
    // ::testing::GTEST_FLAG(filter) = "HJDownloadJobTest*";
    // ::testing::GTEST_FLAG(filter) = "HJBlobSourceTest*";
    // ::testing::GTEST_FLAG(filter) = "HJDataSourceTest*";
     ::testing::GTEST_FLAG(filter) = "HJFFDemuxerSeekPrecisionTest*";
    // Run the tests
    int test_result = RUN_ALL_TESTS();
    HJFLogi("Test run finished with result: {}", test_result);
}

void onTestOpenLocalIO()
{
    HJParams::Ptr params = HJCreates<HJParams>();

    HJCacheOptions cache_opts = {
        { "E:/movies/localio/medias", 100},
        { "E:/movies/localio/short", 200},
        { "E:/movies/localio/gift", 300 }
    };
    (*params)["cache_opts"] = cache_opts;
    (*params)["medias_dir"] = "E:/movies/localio";
    (*params)["medias_cache_max"] = 500;

    HJDataSourceKit::getInstance()->init(params, [&](const HJNotification::Ptr ntf) -> int {
        if (!ntf) {
            return HJ_OK;
        }
        switch (ntf->getID()) {
        };

        return HJ_OK;
    });

}
void onTestClseLocalIO()
{
    HJDataSourceKit::getInstance()->done();
}

bool shouldOpenLocalIOForRun(int argc, const char* argv[])
{
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i] ? argv[i] : "";
        if (arg.find("--gtest") == 0) {
            return false;
        }
    }
    return true;
}

int main(int argc, const char* argv[])
{
#if defined(HJ_OS_WIN32)
    _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
#elif defined(HJ_OS_MACOS)
//    @autoreleasepool
//    {
//        NSApp = [NSApplication sharedApplication];
//        AppDelegate* delegate = [[AppDelegate alloc] init];
//        [[NSApplication sharedApplication] setDelegate:delegate];
//    }
#endif
    HJContextConfig cfg;
    cfg.mIsLog = true;
    cfg.mLogMode = HJLOGMODE_FILE | HJLOGMODE_CONSOLE;
    cfg.mLogDir = HJUtilitys::logDir() + R"(xmediatools/)";
    cfg.mResDir = HJConcateDirectory(HJCurrentDirectory(), R"(resources)");
#if defined(HJ_OS_WIN32)
    cfg.mMediaDir = "E:/movies";
    //cfg.mResDir = HJUtils::exeDir() + "resources";
    //cfg.mMediaDir = "C:";
#elif defined(HJ_OS_MACOS)
    //cfg.mResDir = "/Users/zhiyongleng/works/MediaTools/SLMedia_OW/examples/Windows/XMediaTools/resources";
    cfg.mMediaDir = "/Users/zhiyongleng/works/movies";
#endif
    cfg.mThreadNum = 0;
    //HJFileUtil::removeFile((cfg.mLogDir + "HJLog.txt"));
    HJContext::Instance().init(cfg);

    //if (shouldOpenLocalIOForRun(argc, argv)) {
    //    onTestOpenLocalIO();
    //}

    runHJExecutorPoolTests(argc, argv);

     //HJContext::Instance().done();
#if defined(HJ_OS_MACOS)
    return NSApplicationMain(argc, argv);
#else
    return HJ_OK;
#endif
}
