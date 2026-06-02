#include <gtest/gtest.h>

extern "C" {
#include <libavutil/samplefmt.h>
}

#include "../HJAudioMixer.h"

using namespace HJ;

namespace {

HJAudioMixerConfig makeMixerConfig(int i_channels, AVSampleFormat i_sample_fmt)
{
    HJAudioMixerConfig config{};
    config.m_max_inputs = 1;
    config.m_frame_samples = HJ_AUDIO_MIXER_OUTPUT_SAMPLES;
    config.m_enable_compand = false;
    config.m_enable_limiter = false;
    config.m_output_info = HJAudioInfo::makeAudioInfo(
        i_channels,
        48000,
        static_cast<int>(i_sample_fmt),
        config.m_frame_samples);
    return config;
}

HJMediaFrame::Ptr makeInputFrame(const HJAudioInfo::Ptr& i_info, int64_t i_pts)
{
    auto frame = HJMediaFrame::makeSilenceAudioFrame(i_info);
    if (frame != nullptr) {
        frame->setPTSDTS(i_pts, i_pts, HJTimeBase{ 1, i_info->m_samplesRate });
        frame->setDuration(i_info->m_sampleCnt, HJTimeBase{ 1, i_info->m_samplesRate });
    }
    return frame;
}

}  // namespace

TEST(HJAudioMixerTest, MixFramesHandlesMonoInputWithoutPeakOverflow)
{
    HJAudioMixerConfig config = makeMixerConfig(1, AV_SAMPLE_FMT_FLTP);
    HJAudioMixer mixer;
    ASSERT_EQ(mixer.init(config), HJ_OK);

    HJAudioMixerInputDesc input_desc{};
    input_desc.m_input_id = "mono";
    input_desc.m_input_info = std::dynamic_pointer_cast<HJAudioInfo>(config.m_output_info->dup());
    ASSERT_EQ(mixer.configureInput(0, input_desc), HJ_OK);

    auto frame = makeInputFrame(input_desc.m_input_info, 0);
    ASSERT_NE(frame, nullptr);

    auto mixed = mixer.mixFrames({ frame });
    ASSERT_NE(mixed, nullptr);
    ASSERT_NE(mixed->getAudioInfo(), nullptr);
    EXPECT_EQ(mixed->getAudioInfo()->m_channels, 1);
}

TEST(HJAudioMixerTest, MixFramesFallsBackToSilenceForIncompatibleInput)
{
    HJAudioMixerConfig config = makeMixerConfig(2, AV_SAMPLE_FMT_FLTP);
    HJAudioMixer mixer;
    ASSERT_EQ(mixer.init(config), HJ_OK);

    HJAudioMixerInputDesc input_desc{};
    input_desc.m_input_id = "stereo";
    input_desc.m_input_info = std::dynamic_pointer_cast<HJAudioInfo>(config.m_output_info->dup());
    ASSERT_EQ(mixer.configureInput(0, input_desc), HJ_OK);

    auto incompatible_info = HJAudioInfo::makeAudioInfo(1, 44100, AV_SAMPLE_FMT_S16, config.m_frame_samples);
    auto frame = makeInputFrame(incompatible_info, 0);
    ASSERT_NE(frame, nullptr);

    auto mixed = mixer.mixFrames({ frame });
    ASSERT_NE(mixed, nullptr);
    ASSERT_NE(mixed->getAudioInfo(), nullptr);
    EXPECT_EQ(mixed->getAudioInfo()->m_channels, config.m_output_info->m_channels);
    EXPECT_EQ(mixed->getAudioInfo()->m_sampleFmt, config.m_output_info->m_sampleFmt);
    EXPECT_EQ(mixed->getAudioInfo()->m_samplesRate, config.m_output_info->m_samplesRate);
}
