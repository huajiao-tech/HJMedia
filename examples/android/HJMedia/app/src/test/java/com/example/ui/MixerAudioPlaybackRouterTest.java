package com.example.ui;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;

import com.HJMediasdk.entry.player.HJAudioMixerJni;

import org.junit.Test;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;

public class MixerAudioPlaybackRouterTest
{
    @Test
    public void aacDataShouldBeDroppedUntilCodecConfigArrives()
    {
        FakePcmPlayer pcmPlayer = new FakePcmPlayer();
        FakeAacDecoderFactory decoderFactory = new FakeAacDecoderFactory();
        MixerAudioPlaybackRouter router =
                new MixerAudioPlaybackRouter(pcmPlayer, decoderFactory, status -> {});

        int ret = router.handle(48000,
                                2,
                                HJAudioMixerJni.AV_SAMPLE_FMT_S16,
                                0,
                                ByteBuffer.wrap(new byte[]{0x11, 0x22}),
                                HJAudioMixerJni.HJAudioListenerFlag_AACDat);

        assertEquals(0, ret);
        assertEquals(0, decoderFactory.mCreatedCount);
        assertEquals(0, pcmPlayer.mPlayCount);
    }

    @Test
    public void pcmShouldBypassAacDecoder()
    {
        FakePcmPlayer pcmPlayer = new FakePcmPlayer();
        FakeAacDecoderFactory decoderFactory = new FakeAacDecoderFactory();
        MixerAudioPlaybackRouter router =
                new MixerAudioPlaybackRouter(pcmPlayer, decoderFactory, status -> {});
        ByteBuffer pcm = ByteBuffer.wrap(new byte[]{0x01, 0x02, 0x03, 0x04});

        int ret = router.handle(48000,
                                2,
                                HJAudioMixerJni.AV_SAMPLE_FMT_S16,
                                2,
                                pcm,
                                HJAudioMixerJni.HJAudioListenerFlag_PCM);

        assertEquals(0, ret);
        assertEquals(0, decoderFactory.mCreatedCount);
        assertEquals(1, pcmPlayer.mPlayCount);
        assertArrayEquals(new byte[]{0x01, 0x02, 0x03, 0x04}, pcmPlayer.mLastPayload);
    }

    @Test
    public void changedCodecConfigShouldRecreateDecoder()
    {
        FakePcmPlayer pcmPlayer = new FakePcmPlayer();
        FakeAacDecoderFactory decoderFactory = new FakeAacDecoderFactory();
        MixerAudioPlaybackRouter router =
                new MixerAudioPlaybackRouter(pcmPlayer, decoderFactory, status -> {});

        int headerRet = router.handle(48000,
                                      2,
                                      HJAudioMixerJni.AV_SAMPLE_FMT_S16,
                                      0,
                                      ByteBuffer.wrap(new byte[]{0x12, 0x10}),
                                      HJAudioMixerJni.HJAudioListenerFlag_AACHEADER);
        int dataRet = router.handle(48000,
                                    2,
                                    HJAudioMixerJni.AV_SAMPLE_FMT_S16,
                                    0,
                                    ByteBuffer.wrap(new byte[]{0x01, 0x02}),
                                    HJAudioMixerJni.HJAudioListenerFlag_AACDat);
        int newHeaderRet = router.handle(44100,
                                         1,
                                         HJAudioMixerJni.AV_SAMPLE_FMT_S16,
                                         0,
                                         ByteBuffer.wrap(new byte[]{0x11, (byte) 0x88}),
                                         HJAudioMixerJni.HJAudioListenerFlag_AACHEADER);
        int newDataRet = router.handle(44100,
                                       1,
                                       HJAudioMixerJni.AV_SAMPLE_FMT_S16,
                                       0,
                                       ByteBuffer.wrap(new byte[]{0x21, 0x22, 0x23}),
                                       HJAudioMixerJni.HJAudioListenerFlag_AACDat);

        assertEquals(0, headerRet);
        assertEquals(0, dataRet);
        assertEquals(0, newHeaderRet);
        assertEquals(0, newDataRet);
        assertEquals(2, decoderFactory.mCreatedCount);
        assertEquals(2, decoderFactory.mDecoders.size());
        assertNotNull(decoderFactory.mDecoders.get(0));
        assertNotNull(decoderFactory.mDecoders.get(1));
        assertEquals(1, decoderFactory.mDecoders.get(0).mReleaseCount);
        assertEquals(1, decoderFactory.mDecoders.get(0).mDecodeCount);
        assertEquals(1, decoderFactory.mDecoders.get(1).mDecodeCount);
        assertArrayEquals(new byte[]{0x11, (byte) 0x88}, decoderFactory.mDecoders.get(1).mConfiguredCodecConfig);
    }

    private static final class FakePcmPlayer implements MixerAudioPlaybackRouter.PcmPlayer
    {
        int mPlayCount;
        byte[] mLastPayload;

        @Override
        public int play(int sampleRate, int channels, int sampleFmt, ByteBuffer buffer)
        {
            mPlayCount += 1;
            ByteBuffer copy = buffer.duplicate();
            mLastPayload = new byte[copy.remaining()];
            copy.get(mLastPayload);
            return 0;
        }
    }

    private static final class FakeAacDecoderFactory implements MixerAudioPlaybackRouter.AacDecoderFactory
    {
        int mCreatedCount;
        final List<FakeAacDecoder> mDecoders = new ArrayList<>();

        @Override
        public MixerAudioPlaybackRouter.AacDecoder create(MixerAudioPlaybackRouter.PcmPlayer pcmPlayer,
                                                          MixerAudioPlaybackRouter.StatusListener statusListener)
        {
            mCreatedCount += 1;
            FakeAacDecoder decoder = new FakeAacDecoder(pcmPlayer);
            mDecoders.add(decoder);
            return decoder;
        }
    }

    private static final class FakeAacDecoder implements MixerAudioPlaybackRouter.AacDecoder
    {
        private final MixerAudioPlaybackRouter.PcmPlayer mPcmPlayer;
        int mDecodeCount;
        int mReleaseCount;
        byte[] mConfiguredCodecConfig;

        FakeAacDecoder(MixerAudioPlaybackRouter.PcmPlayer pcmPlayer)
        {
            mPcmPlayer = pcmPlayer;
        }

        @Override
        public int configure(int sampleRate, int channels, ByteBuffer codecConfig)
        {
            ByteBuffer copy = codecConfig.duplicate();
            mConfiguredCodecConfig = new byte[copy.remaining()];
            copy.get(mConfiguredCodecConfig);
            return 0;
        }

        @Override
        public int decode(ByteBuffer encodedData)
        {
            mDecodeCount += 1;
            return mPcmPlayer.play(48000,
                                   2,
                                   HJAudioMixerJni.AV_SAMPLE_FMT_S16,
                                   ByteBuffer.wrap(new byte[]{0x55, 0x66}));
        }

        @Override
        public void release()
        {
            mReleaseCount += 1;
        }
    }
}
