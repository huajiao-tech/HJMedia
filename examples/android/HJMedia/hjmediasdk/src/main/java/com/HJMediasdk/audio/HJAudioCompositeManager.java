package com.HJMediasdk.audio;

import android.Manifest;
import android.annotation.SuppressLint;

import androidx.annotation.RequiresPermission;

import com.HJMediasdk.audio.HJAudioCaptureCtrl;
import com.HJMediasdk.entry.player.HJAudioMixerConfig;
import com.HJMediasdk.entry.player.HJAudioMixerInputDesc;
import com.HJMediasdk.entry.player.HJAudioMixerJni;
import com.HJMediasdk.entry.player.HJMusicPlayerJni;
import com.HJMediasdk.utils.HJBaseListener;
import com.HJMediasdk.utils.HJLog;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;

public class HJAudioCompositeManager
{
    private static final class ReleaseTargets
    {
        private HJAudioCaptureCtrl mAudioCaptureCtrl;
        private HJAudioMixerJni mAudioMixer;
        private final List<HJMusicPlayerJni> mMusicPlayers = new ArrayList<>();
    }

    private static final class PlayerSession
    {
        private final String uniquePlayerName;
        private final String mixerInputId;
        private final PlayerListener listener;
        private HJBaseListener sdkPlayerListener;
        private com.HJMediasdk.utils.HJAudioListener.UI sdkPlayerAudioListener;
        private HJMusicPlayerJni musicPlayer;
        private float playerVolume = 1.0f;
        private float mixVolume = 1.0f;
        private boolean mute = false;
        private long durationMs = 0L;
        private int[] trackIds = new int[0];
        private boolean mixerInputAdded = false;

        private PlayerSession(String uniquePlayerName, PlayerListener listener)
        {
            this.uniquePlayerName = uniquePlayerName;
            this.mixerInputId = "player:" + uniquePlayerName;
            this.listener = listener;
        }
    }

    public interface CapturePcmListener
    {
        int onCapturePcm(ByteBuffer buffer, int samples, int sampleBytes, int channels);
    }

    public interface PlayerListener
    {
        void onTracksReady(String uniquePlayerName, int[] tracks);
        void onDurationReady(String uniquePlayerName, long durationMs);
        void onEof(String uniquePlayerName);
    }

    private static final String TAG = HJAudioCompositeManager.class.getSimpleName();
    private static final String MIX_INPUT_MIC = "mic";
    private static final int DEFAULT_BITRATE = 128000;
    private static final int DEFAULT_CAPTURE_SAMPLE_RATE = 44100;
    private static final int DEFAULT_CAPTURE_CHANNELS = 1;
    private static final int DEFAULT_PLAYER_SAMPLE_RATE = 48000;
    private static final int DEFAULT_PLAYER_CHANNELS = 2;
    private static final int DEFAULT_MIX_SAMPLE_RATE = 48000;
    private static final int DEFAULT_MIX_CHANNELS = 2;
    private static final int DEFAULT_FRAME_SAMPLES = 1024;
    private static final int DEFAULT_MAX_INPUTS = 3;

    private final Object mStateLock = new Object();
    private final Map<String, PlayerSession> mPlayerSessionMap = new LinkedHashMap<>();

    private HJAudioCaptureCtrl mAudioCaptureCtrl;
    private HJAudioMixerJni mAudioMixer;
    private volatile com.HJMediasdk.audio.HJAudioListener mAudioListener;
    private CapturePcmListener mCapturePcmListener;

    private int mCaptureSampleRate = DEFAULT_CAPTURE_SAMPLE_RATE;
    private int mCaptureChannels = DEFAULT_CAPTURE_CHANNELS;
    private int mPlayerSampleRate = DEFAULT_PLAYER_SAMPLE_RATE;
    private int mPlayerChannels = DEFAULT_PLAYER_CHANNELS;
    private int mMixSampleRate = DEFAULT_MIX_SAMPLE_RATE;
    private int mMixChannels = DEFAULT_MIX_CHANNELS;
    private int mBitrate = DEFAULT_BITRATE;
    private boolean mADTS = true;
    private boolean mEnableMixing = true;
    private boolean mMute = false;

    private final HJBaseListener mMixerListener = (id, value, desc) -> {
        HJLog.i(TAG, String.format(Locale.US,
                "mixer event id=%d value=%d desc=%s",
                id,
                value,
                desc));
        return 0;
    };

    private final com.HJMediasdk.utils.HJAudioListener.UI mCaptureListener = (sampleRate,
                                                                              channels,
                                                                              sampleFmt,
                                                                              bytesPerSample,
                                                                              buffer,
                                                                              flag) -> {
        int samples = priCalcSamples(buffer, channels, bytesPerSample);
        int callbackRet = priNotifyCapturePcm(buffer, samples, bytesPerSample, channels);
        if (callbackRet < 0)
        {
            HJLog.e(TAG, "capture pcm listener failed ret=" + callbackRet);
            return callbackRet;
        }

        return priPushPcmToMixer(MIX_INPUT_MIC,
                                 sampleRate,
                                 channels,
                                 sampleFmt,
                                 bytesPerSample,
                                 buffer,
                                 System.currentTimeMillis());
    };

    private final com.HJMediasdk.utils.HJAudioListener.UI mMixerAudioListener = (sampleRate,
                                                                                 channels,
                                                                                 sampleFmt,
                                                                                 bytesPerSample,
                                                                                 buffer,
                                                                                 flag) -> priDispatchMixerAudio(sampleRate,
                                                                                                               channels,
                                                                                                               sampleFmt,
                                                                                                               bytesPerSample,
                                                                                                               buffer,
                                                                                                               flag);

    public HJAudioCompositeManager()
    {
    }

    public void setCaptureAudioConfig(int sampleRate, int channels)
    {
        synchronized (mStateLock)
        {
            mCaptureSampleRate = priClampSampleRate(sampleRate, DEFAULT_CAPTURE_SAMPLE_RATE);
            mCaptureChannels = priClampChannels(channels, DEFAULT_CAPTURE_CHANNELS);
        }
    }

    public void setPlayerAudioConfig(int sampleRate, int channels)
    {
        synchronized (mStateLock)
        {
            mPlayerSampleRate = priClampSampleRate(sampleRate, DEFAULT_PLAYER_SAMPLE_RATE);
            mPlayerChannels = priClampChannels(channels, DEFAULT_PLAYER_CHANNELS);
        }
    }

    public void setMixAudioConfig(int sampleRate, int channels)
    {
        synchronized (mStateLock)
        {
            mMixSampleRate = priClampSampleRate(sampleRate, DEFAULT_MIX_SAMPLE_RATE);
            mMixChannels = priClampChannels(channels, DEFAULT_MIX_CHANNELS);
        }
    }

    public void setBitrate(int bitrate)
    {
        mBitrate = Math.max(32_000, bitrate);
    }

    public void setIsADTS(boolean isADTS)
    {
        mADTS = isADTS;
    }

    public void setEnableMixing(boolean enableMixing)
    {
        synchronized (mStateLock)
        {
            mEnableMixing = enableMixing;
            if (mAudioMixer == null)
            {
                return;
            }

            for (PlayerSession session : mPlayerSessionMap.values())
            {
                priApplyPlayerMixLocked(session, enableMixing);
            }
        }
    }

    public int playerOpen(String uniquePlayerName, String url, PlayerListener listener)
    {
        String normalizedPlayerName = priNormalizePlayerName(uniquePlayerName);
        if (normalizedPlayerName == null)
        {
            HJLog.e(TAG, "openPlayer uniquePlayerName is empty");
            return -1;
        }

        HJMusicPlayerJni stalePlayer = null;
        synchronized (mStateLock)
        {
            PlayerSession staleSession = priRemovePlayerSessionLocked(normalizedPlayerName, true);
            if (staleSession != null)
            {
                stalePlayer = staleSession.musicPlayer;
            }
        }
        priReleasePlayer(stalePlayer);

        return priOpenPlayer(normalizedPlayerName, url, listener);
    }

    public int mixerSetPlayerVolume(String uniquePlayerName, float volume)
    {
        synchronized (mStateLock)
        {
            PlayerSession session = mPlayerSessionMap.get(priNormalizePlayerName(uniquePlayerName));
            if (session == null)
            {
                return -1;
            }

            session.mixVolume = priClampVolume(volume);
            if (mAudioMixer == null || !session.mixerInputAdded)
            {
                return 0;
            }
            return mAudioMixer.setInputVolume(session.mixerInputId, session.mixVolume);
        }
    }

    public int playerPause(String uniquePlayerName)
    {
        HJMusicPlayerJni musicPlayer = priGetPlayer(uniquePlayerName);
        if (musicPlayer == null)
        {
            return -1;
        }
        return musicPlayer.pause();
    }

    public int playerResume(String uniquePlayerName)
    {
        HJMusicPlayerJni musicPlayer = priGetPlayer(uniquePlayerName);
        if (musicPlayer == null)
        {
            return -1;
        }
        return musicPlayer.resume();
    }

    public int playerSeek(String uniquePlayerName, long timestamp)
    {
        HJMusicPlayerJni musicPlayer = priGetPlayer(uniquePlayerName);
        if (musicPlayer == null)
        {
            return -1;
        }
        return musicPlayer.seek(timestamp);
    }

    public int playerSetMute(String uniquePlayerName, boolean mute)
    {
        HJMusicPlayerJni musicPlayer;
        synchronized (mStateLock)
        {
            PlayerSession session = mPlayerSessionMap.get(priNormalizePlayerName(uniquePlayerName));
            if (session == null || session.musicPlayer == null)
            {
                return -1;
            }
            session.mute = mute;
            musicPlayer = session.musicPlayer;
        }
        return musicPlayer.setMute(mute);
    }

    public boolean playerIsMuted(String uniquePlayerName)
    {
        synchronized (mStateLock)
        {
            PlayerSession session = mPlayerSessionMap.get(priNormalizePlayerName(uniquePlayerName));
            if (session == null || session.musicPlayer == null)
            {
                return false;
            }
            return session.musicPlayer.isMuted();
        }
    }

    public int playerSetVolume(String uniquePlayerName, float volume)
    {
        HJMusicPlayerJni musicPlayer;
        float clampedVolume = priClampVolume(volume);
        synchronized (mStateLock)
        {
            PlayerSession session = mPlayerSessionMap.get(priNormalizePlayerName(uniquePlayerName));
            if (session == null || session.musicPlayer == null)
            {
                return -1;
            }
            session.playerVolume = clampedVolume;
            musicPlayer = session.musicPlayer;
        }
        return musicPlayer.setVolume(clampedVolume);
    }

    public float playerGetVolume(String uniquePlayerName)
    {
        HJMusicPlayerJni musicPlayer = priGetPlayer(uniquePlayerName);
        if (musicPlayer == null)
        {
            return 0.0f;
        }
        return musicPlayer.getVolume();
    }

    public long playerGetCurrentTimestamp(String uniquePlayerName)
    {
        HJMusicPlayerJni musicPlayer = priGetPlayer(uniquePlayerName);
        if (musicPlayer == null)
        {
            return 0L;
        }
        return musicPlayer.getCurrentTimestamp();
    }

    public long playerGetDuration(String uniquePlayerName)
    {
        synchronized (mStateLock)
        {
            PlayerSession session = mPlayerSessionMap.get(priNormalizePlayerName(uniquePlayerName));
            return session == null ? 0L : session.durationMs;
        }
    }

    public int playerSwitchAudioTrack(String uniquePlayerName, int trackId)
    {
        HJMusicPlayerJni musicPlayer = priGetPlayer(uniquePlayerName);
        if (musicPlayer == null)
        {
            return -1;
        }
        return musicPlayer.switchAudioTrack(trackId);
    }

    public int playeSwitchAudioTrack(String uniquePlayerName, int trackId)
    {
        return playerSwitchAudioTrack(uniquePlayerName, trackId);
    }

    public int mixerSetMicVolume(float volume)
    {
        HJAudioMixerJni audioMixer = mAudioMixer;
        if (audioMixer == null)
        {
            return -1;
        }
        return audioMixer.setInputVolume(MIX_INPUT_MIC, priClampVolume(volume));
    }

    public void playerClose(String uniquePlayerName)
    {
        HJMusicPlayerJni musicPlayer = null;
        synchronized (mStateLock)
        {
            PlayerSession session = priRemovePlayerSessionLocked(priNormalizePlayerName(uniquePlayerName), true);
            if (session != null)
            {
                musicPlayer = session.musicPlayer;
            }
        }
        priReleasePlayer(musicPlayer);
    }

    public void mixerSetMute(boolean mute)
    {
        synchronized (mStateLock)
        {
            mMute = mute;
            if (mAudioMixer != null)
            {
                int ret = mAudioMixer.setMasterMute(mute);
                if (ret < 0)
                {
                    HJLog.e(TAG, "set mute failed ret=" + ret);
                }
            }
        }
    }

    public void setListener(com.HJMediasdk.audio.HJAudioListener listener)
    {
        synchronized (mStateLock)
        {
            mAudioListener = listener;
        }
    }

    public void setCapturePcmListener(CapturePcmListener listener)
    {
        synchronized (mStateLock)
        {
            mCapturePcmListener = listener;
        }
    }

    @SuppressLint("MissingPermission")
    public int init()
    {
        ReleaseTargets releaseTargets;
        synchronized (mStateLock)
        {
            releaseTargets = priDetachReleaseTargetsLocked();
        }
        priReleaseTargets(releaseTargets);

        int ret = 0;
        ReleaseTargets failedTargets = null;
        synchronized (mStateLock)
        {
            try
            {
                int mixerRet = priInitMixerLocked();
                if (mixerRet < 0)
                {
                    ret = mixerRet;
                    failedTargets = priDetachReleaseTargetsLocked();
                }
                else
                {
                    int captureRet = priInitCaptureLocked();
                    if (captureRet < 0)
                    {
                        ret = captureRet;
                        failedTargets = priDetachReleaseTargetsLocked();
                    }
                }
            }
            catch (Exception e)
            {
                HJLog.e(TAG, "init failed " + e.getMessage());
                ret = -1;
                failedTargets = priDetachReleaseTargetsLocked();
            }
        }
        priReleaseTargets(failedTargets);
        return ret;
    }

    public void release()
    {
        ReleaseTargets releaseTargets;
        synchronized (mStateLock)
        {
            releaseTargets = priDetachReleaseTargetsLocked();
        }
        priReleaseTargets(releaseTargets);
    }

    public void captureClose()
    {
        HJAudioCaptureCtrl audioCaptureCtrl;
        synchronized (mStateLock)
        {
            audioCaptureCtrl = mAudioCaptureCtrl;
            mAudioCaptureCtrl = null;
        }

        if (audioCaptureCtrl != null)
        {
            audioCaptureCtrl.release();
        }
    }

    public int pushCustomPCM(int channels, int sampleRate, ByteBuffer buffer, long timeMs)
    {
        return priPushPcmToMixer(MIX_INPUT_MIC,
                                 sampleRate,
                                 channels,
                                 HJAudioMixerJni.AV_SAMPLE_FMT_S16,
                                 2,
                                 buffer,
                                 timeMs);
    }

    @RequiresPermission(Manifest.permission.RECORD_AUDIO)
    private int priInitCaptureLocked()
    {
        HJAudioCaptureCtrl audioCaptureCtrl = new HJAudioCaptureCtrl();
        audioCaptureCtrl.registerAudioListener(mCaptureListener);
        int ret = audioCaptureCtrl.init(mCaptureSampleRate, mCaptureChannels);
        if (ret < 0)
        {
            audioCaptureCtrl.release();
            HJLog.e(TAG, "capture init failed ret=" + ret);
            return ret;
        }
        mAudioCaptureCtrl = audioCaptureCtrl;
        return 0;
    }

    private int priInitMixerLocked()
    {
        HJAudioMixerJni mixer = new HJAudioMixerJni();
        int ret = mixer.init(priBuildMixerConfig(), mMixerListener, mMixerAudioListener);
        if (ret < 0)
        {
            mixer.done();
            HJLog.e(TAG, "mixer init failed ret=" + ret);
            return ret;
        }
        mAudioMixer = mixer;
        ret = mixer.setMasterMute(mMute);
        if (ret < 0)
        {
            HJLog.e(TAG, "apply mute failed ret=" + ret);
        }
        return 0;
    }

    private int priOpenPlayer(String uniquePlayerName, String url, PlayerListener listener)
    {
        if (url == null || url.trim().isEmpty())
        {
            HJLog.e(TAG, "openPlayer url is empty");
            return -1;
        }

        int playerSampleRate;
        int playerChannels;
        synchronized (mStateLock)
        {
            if (mAudioMixer == null)
            {
                HJLog.e(TAG, "openPlayer failed: mixer is null, call init first");
                return -1;
            }
            playerSampleRate = mPlayerSampleRate;
            playerChannels = mPlayerChannels;
        }

        HJMusicPlayerJni player = new HJMusicPlayerJni();
        PlayerSession session = new PlayerSession(uniquePlayerName, listener);
        session.musicPlayer = player;
        session.sdkPlayerListener = priBuildPlayerListener(uniquePlayerName, player);
        session.sdkPlayerAudioListener = priBuildPlayerAudioListener(uniquePlayerName, player);
        int initRet = player.init(1,
                                  120,
                                  20,
                                  playerSampleRate,
                                  playerChannels,
                                  session.sdkPlayerListener,
                                  session.sdkPlayerAudioListener);
        if (initRet < 0)
        {
            player.done();
            HJLog.e(TAG, "music player init failed ret=" + initRet + " player=" + uniquePlayerName);
            return initRet;
        }

        synchronized (mStateLock)
        {
            if (mAudioMixer == null)
            {
                player.done();
                HJLog.e(TAG, "openPlayer failed after init: mixer is null");
                return -1;
            }

            mPlayerSessionMap.put(uniquePlayerName, session);
            if (mEnableMixing)
            {
                int mixRet = priApplyPlayerMixLocked(session, true);
                if (mixRet < 0)
                {
                    mPlayerSessionMap.remove(uniquePlayerName);
                    player.done();
                    HJLog.e(TAG, "add mixer input failed ret=" + mixRet + " player=" + uniquePlayerName);
                    return mixRet;
                }
            }
        }

        int openRet = player.open(url);
        if (openRet < 0)
        {
            synchronized (mStateLock)
            {
                priRemovePlayerSessionLocked(uniquePlayerName, true);
            }
            player.done();
            HJLog.e(TAG, "music player open failed ret=" + openRet
                    + " player=" + uniquePlayerName
                    + " url=" + url);
            return openRet;
        }

        return 0;
    }

    private PlayerSession priRemovePlayerSessionLocked(String uniquePlayerName, boolean removeMixerInput)
    {
        if (uniquePlayerName == null)
        {
            return null;
        }

        PlayerSession session = mPlayerSessionMap.remove(uniquePlayerName);
        if (session == null)
        {
            return null;
        }

        if (removeMixerInput && mAudioMixer != null && session.mixerInputAdded)
        {
            int ret = mAudioMixer.removeInput(session.mixerInputId, true);
            if (ret < 0)
            {
                HJLog.e(TAG, "remove mixer input failed input="
                        + session.mixerInputId
                        + " ret="
                        + ret);
            }
            session.mixerInputAdded = false;
        }
        return session;
    }

    private int priApplyPlayerMixLocked(PlayerSession session, boolean enableMixer)
    {
        if (session == null || mAudioMixer == null)
        {
            return -1;
        }

        if (enableMixer)
        {
            if (!session.mixerInputAdded)
            {
                int addRet = mAudioMixer.addInput(priBuildInputDesc(session.mixerInputId,
                                                                    mPlayerSampleRate,
                                                                    mPlayerChannels,
                                                                    session.mixVolume));
                if (addRet < 0)
                {
                    HJLog.e(TAG, "add input failed input="
                            + session.mixerInputId
                            + " ret="
                            + addRet);
                    return addRet;
                }
                session.mixerInputAdded = true;
                return addRet;
            }
            return mAudioMixer.setInputVolume(session.mixerInputId, session.mixVolume);
        }

        if (!session.mixerInputAdded)
        {
            return 0;
        }

        int removeRet = mAudioMixer.removeInput(session.mixerInputId, true);
        if (removeRet < 0)
        {
            HJLog.e(TAG, "remove input failed input="
                    + session.mixerInputId
                    + " ret="
                    + removeRet);
            return removeRet;
        }
        session.mixerInputAdded = false;
        return 0;
    }

    private ReleaseTargets priDetachReleaseTargetsLocked()
    {
        ReleaseTargets releaseTargets = new ReleaseTargets();

        if (mAudioCaptureCtrl != null)
        {
            releaseTargets.mAudioCaptureCtrl = mAudioCaptureCtrl;
            mAudioCaptureCtrl = null;
        }

        for (PlayerSession session : mPlayerSessionMap.values())
        {
            if (session.musicPlayer != null)
            {
                releaseTargets.mMusicPlayers.add(session.musicPlayer);
                session.musicPlayer = null;
            }
        }
        mPlayerSessionMap.clear();

        if (mAudioMixer != null)
        {
            releaseTargets.mAudioMixer = mAudioMixer;
            mAudioMixer = null;
        }
        return releaseTargets;
    }

    private void priReleaseTargets(ReleaseTargets releaseTargets)
    {
        if (releaseTargets == null)
        {
            return;
        }

        if (releaseTargets.mAudioCaptureCtrl != null)
        {
            releaseTargets.mAudioCaptureCtrl.release();
        }

        for (HJMusicPlayerJni musicPlayer : releaseTargets.mMusicPlayers)
        {
            priReleasePlayer(musicPlayer);
        }

        if (releaseTargets.mAudioMixer != null)
        {
            releaseTargets.mAudioMixer.done();
        }
    }

    private HJBaseListener priBuildPlayerListener(String uniquePlayerName, HJMusicPlayerJni player)
    {
        return (id, value, desc) -> priHandlePlayerNotify(uniquePlayerName, player, id, value, desc);
    }

    private com.HJMediasdk.utils.HJAudioListener.UI priBuildPlayerAudioListener(String uniquePlayerName,
                                                                                 HJMusicPlayerJni player)
    {
        return (sampleRate,
                channels,
                sampleFmt,
                bytesPerSample,
                buffer,
                flag) -> priHandlePlayerAudio(uniquePlayerName,
                                              player,
                                              sampleRate,
                                              channels,
                                              sampleFmt,
                                              bytesPerSample,
                                              buffer,
                                              flag);
    }

    private int priHandlePlayerNotify(String uniquePlayerName,
                                      HJMusicPlayerJni player,
                                      int id,
                                      long value,
                                      String desc)
    {
        HJLog.i(TAG, String.format(Locale.US,
                "player=%s event id=%d value=%d desc=%s",
                uniquePlayerName,
                id,
                value,
                desc));

        PlayerListener listener = null;
        int[] tracks = null;
        long durationMs = 0L;
        boolean notifyTracksReady = false;
        boolean notifyDurationReady = false;
        boolean notifyEof = false;
        String event = desc == null ? "" : desc;

        synchronized (mStateLock)
        {
            PlayerSession session = mPlayerSessionMap.get(uniquePlayerName);
            if (session == null || session.musicPlayer != player)
            {
                return 0;
            }

            listener = session.listener;
            if (HJMusicPlayerJni.EVT_STREAM_OPENED.equals(event))
            {
                session.trackIds = priCopyTrackIds(player.getAudioTrackIds());
                session.durationMs = player.getDuration();
                tracks = priCopyTrackIds(session.trackIds);
                durationMs = session.durationMs;
                notifyTracksReady = true;
                notifyDurationReady = true;
            }
            else if (HJMusicPlayerJni.EVT_EOF.equals(event))
            {
                notifyEof = true;
            }
        }

        if (listener == null)
        {
            return 0;
        }

        if (notifyTracksReady)
        {
            listener.onTracksReady(uniquePlayerName, tracks);
        }
        if (notifyDurationReady)
        {
            listener.onDurationReady(uniquePlayerName, durationMs);
        }
        if (notifyEof)
        {
            listener.onEof(uniquePlayerName);
        }
        return 0;
    }

    private int priHandlePlayerAudio(String uniquePlayerName,
                                     HJMusicPlayerJni player,
                                     int sampleRate,
                                     int channels,
                                     int sampleFmt,
                                     int bytesPerSample,
                                     ByteBuffer buffer,
                                     int flag)
    {
        String inputId;
        synchronized (mStateLock)
        {
            PlayerSession session = mPlayerSessionMap.get(uniquePlayerName);
            if (session == null || session.musicPlayer != player || !session.mixerInputAdded)
            {
                return 0;
            }
            inputId = session.mixerInputId;
        }

        return priPushPcmToMixer(inputId,
                                 sampleRate,
                                 channels,
                                 sampleFmt,
                                 bytesPerSample,
                                 buffer,
                                 System.currentTimeMillis());
    }

    private HJAudioMixerConfig priBuildMixerConfig()
    {
        HJAudioMixerConfig config = new HJAudioMixerConfig();
        config.outputSampleRate = mMixSampleRate;
        config.outputChannels = mMixChannels;
        config.outputSampleFmt = HJAudioMixerJni.AV_SAMPLE_FMT_S16;
        config.maxInputs = DEFAULT_MAX_INPUTS;
        config.frameSamples = DEFAULT_FRAME_SAMPLES;
        config.syncWindowMs = 42;
        config.lateThresholdMs = 150;
        config.enableCompand = true;
        config.enableLimiter = true;
        config.outType = HJAudioMixerConfig.OUT_TYPE_AAC;
        config.aacType = mADTS ? HJAudioMixerConfig.AAC_TYPE_ADTS
                               : HJAudioMixerConfig.AAC_TYPE_RAW;
        config.inputs.add(priBuildInputDesc(MIX_INPUT_MIC, mCaptureSampleRate, mCaptureChannels, 1.0f));
        return config;
    }

    private HJAudioMixerInputDesc priBuildInputDesc(String inputId, int sampleRate, int channels, float volume)
    {
        HJAudioMixerInputDesc inputDesc = new HJAudioMixerInputDesc();
        inputDesc.inputId = inputId;
        inputDesc.sampleRate = sampleRate;
        inputDesc.channels = channels;
        inputDesc.sampleFmt = HJAudioMixerJni.AV_SAMPLE_FMT_S16;
        inputDesc.volume = priClampVolume(volume);
        return inputDesc;
    }

    private int priClampSampleRate(int sampleRate, int defaultSampleRate)
    {
        return sampleRate > 0 ? sampleRate : defaultSampleRate;
    }

    private int priClampChannels(int channels, int defaultChannels)
    {
        if (channels <= 0)
        {
            return defaultChannels;
        }
        return channels >= 2 ? 2 : 1;
    }

    private float priClampVolume(float volume)
    {
        if (Float.isNaN(volume) || Float.isInfinite(volume))
        {
            return 1.0f;
        }
        return Math.max(0.0f, Math.min(1.0f, volume));
    }

    private int priPushPcmToMixer(String inputId,
                                  int sampleRate,
                                  int channels,
                                  int sampleFmt,
                                  int bytesPerSample,
                                  ByteBuffer buffer,
                                  long ptsMs)
    {
        HJAudioMixerJni mixer = mAudioMixer;
        if (mixer == null || buffer == null || channels <= 0 || bytesPerSample <= 0)
        {
            return 0;
        }

        int size = buffer.remaining();
        if (size <= 0)
        {
            return 0;
        }

        int samples = size / (channels * bytesPerSample);
        if (samples <= 0)
        {
            return 0;
        }

        int ret = mixer.pushPcm(inputId,
                                buffer,
                                size,
                                samples,
                                channels,
                                sampleRate,
                                sampleFmt,
                                ptsMs);
        if (ret < 0)
        {
            HJLog.e(TAG, "push pcm to mixer failed input=" + inputId + " ret=" + ret);
        }
        return ret;
    }

    private int priDispatchMixerAudio(int sampleRate,
                                      int channels,
                                      int sampleFmt,
                                      int bytesPerSample,
                                      ByteBuffer buffer,
                                      int flag)
    {
        if (buffer == null || !buffer.hasRemaining())
        {
            return 0;
        }

        com.HJMediasdk.audio.HJAudioListener audioListener = priGetAudioListener();
        if (audioListener == null)
        {
            return 0;
        }

        if ((flag & HJAudioMixerJni.HJAudioListenerFlag_PCM) != 0)
        {
            HJLog.e(TAG, String.format(Locale.US,
                    "unexpected pcm output from mixer sr=%d ch=%d fmt=%d bytes=%d flag=%d",
                    sampleRate,
                    channels,
                    sampleFmt,
                    bytesPerSample,
                    flag));
            return 0;
        }

        int ret = 0;

        if ((flag & HJAudioMixerJni.HJAudioListenerFlag_AACHEADER) != 0)
        {
            ret = audioListener.onExtraReady(buffer.duplicate(), buffer.remaining());
            if (ret < 0)
            {
                HJLog.e(TAG, "dispatch aac header failed ret=" + ret);
                return ret;
            }
        }

        if ((flag & HJAudioMixerJni.HJAudioListenerFlag_AACDat) != 0)
        {
            ret = audioListener.onCapAAC(buffer.duplicate(),
                                         buffer.remaining(),
                                         System.currentTimeMillis(),
                                         com.HJMediasdk.audio.HJAudioListener.AAC_MDAT);
            if (ret < 0)
            {
                HJLog.e(TAG, "dispatch aac data failed ret=" + ret);
            }
        }
        return ret;
    }

    private com.HJMediasdk.audio.HJAudioListener priGetAudioListener()
    {
        return mAudioListener;
    }

    private int priNotifyCapturePcm(ByteBuffer buffer, int samples, int sampleBytes, int channels)
    {
        CapturePcmListener capturePcmListener = mCapturePcmListener;
        if (capturePcmListener == null || buffer == null)
        {
            return 0;
        }
        return capturePcmListener.onCapturePcm(buffer, samples, sampleBytes, channels);
    }

    private int priCalcSamples(ByteBuffer buffer, int channels, int bytesPerSample)
    {
        if (buffer == null || channels <= 0 || bytesPerSample <= 0)
        {
            return 0;
        }

        int size = buffer.remaining();
        if (size <= 0)
        {
            return 0;
        }
        return size / (channels * bytesPerSample);
    }

    private HJMusicPlayerJni priGetPlayer(String uniquePlayerName)
    {
        synchronized (mStateLock)
        {
            PlayerSession session = mPlayerSessionMap.get(priNormalizePlayerName(uniquePlayerName));
            return session == null ? null : session.musicPlayer;
        }
    }

    private String priNormalizePlayerName(String uniquePlayerName)
    {
        if (uniquePlayerName == null)
        {
            return null;
        }

        String normalizedPlayerName = uniquePlayerName.trim();
        return normalizedPlayerName.isEmpty() ? null : normalizedPlayerName;
    }

    private int[] priCopyTrackIds(int[] trackIds)
    {
        if (trackIds == null || trackIds.length == 0)
        {
            return new int[0];
        }

        int[] outTrackIds = new int[trackIds.length];
        System.arraycopy(trackIds, 0, outTrackIds, 0, trackIds.length);
        return outTrackIds;
    }

    private void priReleasePlayer(HJMusicPlayerJni musicPlayer)
    {
        if (musicPlayer != null)
        {
            musicPlayer.done();
        }
    }
}

