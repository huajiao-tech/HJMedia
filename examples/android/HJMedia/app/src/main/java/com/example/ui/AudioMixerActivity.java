package com.example.ui;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.MediaCodec;
import android.media.MediaFormat;
import android.media.AudioTrack;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

import com.HJMediasdk.entry.HJCommonInterface;
import com.HJMediasdk.entry.player.HJAudioMixerConfig;
import com.HJMediasdk.entry.player.HJAudioMixerInputDesc;
import com.HJMediasdk.entry.player.HJAudioMixerJni;
import com.HJMediasdk.entry.player.HJMusicPlayerJni;
import com.HJMediasdk.entry.player.HJPlayerContextJni;
import com.HJMediasdk.utils.HJAudioListener;
import com.HJMediasdk.utils.HJBaseListener;
import com.HJMediasdk.utils.HJLog;

import java.io.File;
import java.nio.ByteBuffer;
import java.util.Locale;

public class AudioMixerActivity extends AppCompatActivity
{
    private static final String TAG = AudioMixerActivity.class.getSimpleName();
    private static final String PLAYER_A_FILE_NAME = "c58733ac51124fe38cdc6540a7b8fa46.mkv";
    private static final String PLAYER_B_FILE_NAME = "cdc6540a7b8fa46.mp3";
    private static final String INPUT_A = "player0";
    private static final String INPUT_B = "player1";
    private static final int OUTPUT_SAMPLE_RATE = 48000;
    private static final int OUTPUT_CHANNELS = 2;
    private static final int OUTPUT_SAMPLE_FMT = HJAudioMixerJni.AV_SAMPLE_FMT_S16;
    private static final int OUTPUT_FRAME_SAMPLES = 1024;
    private static final int SEEKBAR_MAX = 100;
    private static final long PCM_STATUS_INTERVAL = 10L;

    private final Handler mMainHandler = new Handler(Looper.getMainLooper());
    private final Object mAudioTrackLock = new Object();

    private volatile HJAudioMixerJni mMixer;
    private volatile HJMusicPlayerJni mPlayerA;
    private volatile HJMusicPlayerJni mPlayerB;
    private volatile long mMixedCallbackCount;
    private volatile long mMixedPlaybackWriteCount;

    private AudioTrack mAudioTrack;
    private int mTrackSampleRate;
    private int mTrackChannels;
    private int mTrackEncoding;
    private MixerAudioPlaybackRouter mPlaybackRouter;

    private TextView mTvSource;
    private TextView mTvRouteALabel;
    private TextView mTvRouteBLabel;
    private TextView mTvStatus;
    private TextView mTvStats;
    private TextView mTvInputState;
    private TextView mTvMixedInfo;
    private TextView mTvPlayerInfo;
    private SeekBar mSeekRouteA;
    private SeekBar mSeekRouteB;
    private CheckBox mCbMasterMute;

    private final HJBaseListener mMixerListener = (id, value, desc) -> {
        mMainHandler.post(() -> handleMixerNotify(id, value, desc));
        return 0;
    };

    private final HJAudioListener.UI mMixerUiListener = (sampleRate,
                                                         channels,
                                                         sampleFmt,
                                                         bytesPerSample,
                                                         buffer,
                                                         flag) -> {
        updateMixedInfo(sampleRate, channels, sampleFmt, buffer, flag);
        return ensurePlaybackRouter().handle(sampleRate,
                                             channels,
                                             sampleFmt,
                                             bytesPerSample,
                                             buffer,
                                             flag);
    };

    private final HJBaseListener mPlayerAListener = (id, value, desc) -> {
        postPlayerInfo("A event id=" + id + " value=" + value + " desc=" + desc);
        return 0;
    };

    private final HJBaseListener mPlayerBListener = (id, value, desc) -> {
        postPlayerInfo("B event id=" + id + " value=" + value + " desc=" + desc);
        return 0;
    };

    private final HJAudioListener.UI mPlayerAUiListener = (sampleRate,
                                                           channels,
                                                           sampleFmt,
                                                           bytesPerSample,
                                                           buffer,
                                                           flag) -> forwardPlayerPcm(INPUT_A,
                                                                                     sampleRate,
                                                                                     channels,
                                                                                     sampleFmt,
                                                                                     bytesPerSample,
                                                                                     buffer);

    private final HJAudioListener.UI mPlayerBUiListener = (sampleRate,
                                                           channels,
                                                           sampleFmt,
                                                           bytesPerSample,
                                                           buffer,
                                                           flag) -> forwardPlayerPcm(INPUT_B,
                                                                                     sampleRate,
                                                                                     channels,
                                                                                     sampleFmt,
                                                                                     bytesPerSample,
                                                                                     buffer);

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_audio_mixer);
        HJLog.setEnable(true);

        String logDir = getFilesDir().getAbsolutePath() + File.separator + "player";
        HJPlayerContextJni.contextInit(
                logDir,
                HJCommonInterface.HJLOGLevel_INFO,
                HJCommonInterface.HJLogLMode_CONSOLE | HJCommonInterface.HJLLogMode_FILE,
                5 * 1024 * 1024,
                5);

        bindViews();
        bindControls();
        mTvSource.setText(String.format(Locale.US,
                "A: %s%nB: %s",
                getPlayerASourcePath(),
                getPlayerBSourcePath()));
        updateRouteLabel(mTvRouteALabel, getString(R.string.audio_mixer_route_a), mSeekRouteA);
        updateRouteLabel(mTvRouteBLabel, getString(R.string.audio_mixer_route_b), mSeekRouteB);
    }

    @Override
    protected void onDestroy()
    {
        stopMixerDemo();
        super.onDestroy();
    }

    private void bindViews()
    {
        mTvSource = findViewById(R.id.tvMixerSource);
        mTvRouteALabel = findViewById(R.id.tvRouteALabel);
        mTvRouteBLabel = findViewById(R.id.tvRouteBLabel);
        mTvStatus = findViewById(R.id.tvMixerStatus);
        mTvStats = findViewById(R.id.tvMixerStats);
        mTvInputState = findViewById(R.id.tvMixerInputState);
        mTvMixedInfo = findViewById(R.id.tvMixedInfo);
        mTvPlayerInfo = findViewById(R.id.tvPlayerInfo);
        mSeekRouteA = findViewById(R.id.seekRouteAVolume);
        mSeekRouteB = findViewById(R.id.seekRouteBVolume);
        mCbMasterMute = findViewById(R.id.cbMasterMute);
    }

    private void bindControls()
    {
        mSeekRouteA.setMax(SEEKBAR_MAX);
        mSeekRouteB.setMax(SEEKBAR_MAX);
        mSeekRouteA.setProgress(SEEKBAR_MAX);
        mSeekRouteB.setProgress(SEEKBAR_MAX);

        bindVolumeControl(mSeekRouteA, mTvRouteALabel, INPUT_A, getString(R.string.audio_mixer_route_a));
        bindVolumeControl(mSeekRouteB, mTvRouteBLabel, INPUT_B, getString(R.string.audio_mixer_route_b));

        mCbMasterMute.setOnCheckedChangeListener((buttonView, isChecked) -> applyMasterMute());

        Button btnInit = findViewById(R.id.btnMixerInit);
        Button btnStart = findViewById(R.id.btnMixerStart);
        Button btnDone = findViewById(R.id.btnMixerDone);

        btnInit.setOnClickListener(v -> initMixerOnly());
        btnStart.setOnClickListener(v -> startMixerDemo());
        btnDone.setOnClickListener(v -> {
            stopMixerDemo();
            showToast("done");
        });
    }

    private void bindVolumeControl(SeekBar seekBar,
                                   TextView labelView,
                                   String inputId,
                                   String routeName)
    {
        seekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser)
            {
                updateRouteLabel(labelView, routeName, seekBar);
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar)
            {
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar)
            {
                applyInputControl(inputId, currentVolume(seekBar));
            }
        });
    }

    private void initMixerOnly()
    {
        releaseMixer();
        releaseAudioTrack();

        HJAudioMixerJni mixer = new HJAudioMixerJni();
        int ret = mixer.init(buildMixerConfig(), mMixerListener, mMixerUiListener);
        if (ret < 0)
        {
            mixer.done();
            postStatus("mixer init failed ret=" + ret);
            return;
        }

        mMixer = mixer;
        applyInputControl(INPUT_A, currentVolume(mSeekRouteA));
        applyInputControl(INPUT_B, currentVolume(mSeekRouteB));
        applyMasterMute();
        postStatus("mixer init ok");
    }

    private void startMixerDemo()
    {
        if (mMixer == null)
        {
            initMixerOnly();
            if (mMixer == null)
            {
                return;
            }
        }

        releasePlayers();
        mMixedCallbackCount = 0L;
        mMixedPlaybackWriteCount = 0L;
        mTvMixedInfo.setText(getString(R.string.audio_mixer_mixed_default));

        HJMusicPlayerJni playerA = createPlayer(getPlayerASourcePath(),
                                               mPlayerAListener,
                                               mPlayerAUiListener,
                                               "A");
        if (playerA == null)
        {
            return;
        }

        HJMusicPlayerJni playerB = createPlayer(getPlayerBSourcePath(),
                                               mPlayerBListener,
                                               mPlayerBUiListener,
                                               "B");
        if (playerB == null)
        {
            playerA.done();
            return;
        }

        mPlayerA = playerA;
        mPlayerB = playerB;
        postStatus("mixer start ok, two players opened");
    }

    private HJMusicPlayerJni createPlayer(String sourcePath,
                                          HJBaseListener listener,
                                          HJAudioListener.UI audioListener,
                                          String label)
    {
        HJMusicPlayerJni player = new HJMusicPlayerJni();
        int ret = player.init(0, 120, 20, OUTPUT_SAMPLE_RATE, OUTPUT_CHANNELS, listener, audioListener);
        if (ret < 0)
        {
            postStatus("player " + label + " init failed ret=" + ret);
            player.done();
            return null;
        }

        File sourceFile = new File(sourcePath);
        if (!sourceFile.exists())
        {
            postStatus("player " + label + " source missing: " + sourcePath);
            player.done();
            return null;
        }

        ret = player.open(sourcePath);
        if (ret < 0)
        {
            postStatus("player " + label + " open failed ret=" + ret + " path=" + sourcePath);
            player.done();
            return null;
        }
        return player;
    }

    private void stopMixerDemo()
    {
        releasePlayers();
        releaseMixer();
        releaseAudioTrack();
        postStatus("mixer stopped");
    }

    private void releasePlayers()
    {
        HJMusicPlayerJni playerA = mPlayerA;
        HJMusicPlayerJni playerB = mPlayerB;
        mPlayerA = null;
        mPlayerB = null;
        if (playerA != null)
        {
            playerA.done();
        }
        if (playerB != null)
        {
            playerB.done();
        }
    }

    private void releaseMixer()
    {
        HJAudioMixerJni mixer = mMixer;
        mMixer = null;
        if (mixer != null)
        {
            Log.i(TAG, "mixer done entry");
            mixer.done();
            Log.i(TAG, "mixer done end");
        }
        releasePlaybackRouter();
    }

    private void releaseAudioTrack()
    {
        synchronized (mAudioTrackLock)
        {
            if (mAudioTrack != null)
            {
                try
                {
                    mAudioTrack.pause();
                    mAudioTrack.flush();
                }
                catch (Exception ignore)
                {
                }
                mAudioTrack.release();
                mAudioTrack = null;
            }
            mTrackSampleRate = 0;
            mTrackChannels = 0;
            mTrackEncoding = 0;
        }
    }

    private HJAudioMixerConfig buildMixerConfig()
    {
        HJAudioMixerConfig config = new HJAudioMixerConfig();
        config.outputSampleRate = OUTPUT_SAMPLE_RATE;
        config.outputChannels = OUTPUT_CHANNELS;
        config.outputSampleFmt = OUTPUT_SAMPLE_FMT;
        config.maxInputs = 2;
        config.frameSamples = OUTPUT_FRAME_SAMPLES;
        config.syncWindowMs = 42;
        config.lateThresholdMs = 150;
        config.enableCompand = true;
        config.enableLimiter = true;
        config.outType = HJAudioMixerConfig.OUT_TYPE_AAC;
        config.aacType = HJAudioMixerConfig.AAC_TYPE_RAW;
        config.inputs.add(buildInputDesc(INPUT_A, currentVolume(mSeekRouteA)));
        config.inputs.add(buildInputDesc(INPUT_B, currentVolume(mSeekRouteB)));
        return config;
    }

    private HJAudioMixerInputDesc buildInputDesc(String inputId, float volume)
    {
        HJAudioMixerInputDesc inputDesc = new HJAudioMixerInputDesc();
        inputDesc.inputId = inputId;
        inputDesc.sampleRate = OUTPUT_SAMPLE_RATE;
        inputDesc.channels = OUTPUT_CHANNELS;
        inputDesc.sampleFmt = OUTPUT_SAMPLE_FMT;
        inputDesc.volume = volume;
        return inputDesc;
    }

    private void updateMixedInfo(int sampleRate,
                                 int channels,
                                 int sampleFmt,
                                 ByteBuffer buffer,
                                 int flag)
    {
        mMixedCallbackCount += 1L;
        if ((mMixedCallbackCount % PCM_STATUS_INTERVAL) != 0)
        {
            return;
        }

        final int size = buffer == null ? 0 : buffer.remaining();
        final String info = String.format(Locale.US,
                "mixed callbacks=%d sr=%d ch=%d fmt=%d size=%d flag=%d",
                mMixedCallbackCount,
                sampleRate,
                channels,
                sampleFmt,
                size,
                flag);
        mMainHandler.post(() -> mTvMixedInfo.setText(info));
    }

    private int forwardPlayerPcm(String inputId,
                                 int sampleRate,
                                 int channels,
                                 int sampleFmt,
                                 int bytesPerSample,
                                 ByteBuffer buffer)
    {
        HJAudioMixerJni mixer = mMixer;
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
        long cur_time = System.currentTimeMillis();
        Log.i(TAG, "forwardPlayerPcm inputId:" + inputId + ", cur_time:" + cur_time);

        int ret = mixer.pushPcm(inputId,
                                buffer,
                                size,
                                samples,
                                channels,
                                sampleRate,
                                sampleFmt,
                                cur_time
                                );
        if (ret < 0 && (mMixedCallbackCount % PCM_STATUS_INTERVAL) == 0)
        {
            postStatus("push pcm failed input=" + inputId + " ret=" + ret);
        }
        return ret;
    }

    private int playMixedBuffer(int sampleRate, int channels, int sampleFmt, ByteBuffer buffer)
    {
        if (buffer == null)
        {
            return -1;
        }

        AudioTrack track = ensureAudioTrack(sampleRate, channels, sampleFmt);
        if (track == null)
        {
            return -1;
        }

        int size = buffer.remaining();
        if (size <= 0)
        {
            return 0;
        }

        synchronized (mAudioTrackLock)
        {
            if (mAudioTrack == null)
            {
                Log.w(TAG, "playMixedBuffer audio track is null");
                return -1;
            }
            int written = mAudioTrack.write(buffer, size, AudioTrack.WRITE_BLOCKING);
            if (written < 0)
            {
                postStatus("AudioTrack write failed ret=" + written);
                return written;
            }
            mMixedPlaybackWriteCount += 1L;
            if ((mMixedPlaybackWriteCount % PCM_STATUS_INTERVAL) == 0)
            {
                Log.i(TAG, "AudioTrack wrote pcm size=" + written
                        + " sr=" + sampleRate
                        + " ch=" + channels
                        + " fmt=" + sampleFmt
                        + " playState=" + mAudioTrack.getPlayState());
            }
        }
        return 0;
    }

    private AudioTrack ensureAudioTrack(int sampleRate, int channels, int sampleFmt)
    {
        int encoding = sampleFmtToEncoding(sampleFmt);
        if (encoding == AudioFormat.ENCODING_INVALID)
        {
            postStatus("unsupported sampleFmt=" + sampleFmt);
            return null;
        }

        synchronized (mAudioTrackLock)
        {
            if (mAudioTrack != null
                    && mTrackSampleRate == sampleRate
                    && mTrackChannels == channels
                    && mTrackEncoding == encoding)
            {
                if (mAudioTrack.getPlayState() != AudioTrack.PLAYSTATE_PLAYING)
                {
                    mAudioTrack.play();
                }
                return mAudioTrack;
            }

            releaseAudioTrack();

            int channelConfig = channels == 1
                    ? AudioFormat.CHANNEL_OUT_MONO
                    : AudioFormat.CHANNEL_OUT_STEREO;
            int minBufferSize = AudioTrack.getMinBufferSize(sampleRate, channelConfig, encoding);
            if (minBufferSize <= 0)
            {
                postStatus("AudioTrack minBuffer invalid=" + minBufferSize);
                return null;
            }

            int frameBytes = OUTPUT_FRAME_SAMPLES * channels * 2;
            int bufferSize = Math.max(minBufferSize, frameBytes * 4);
            AudioTrack track = new AudioTrack(
                    AudioManager.STREAM_MUSIC,
                    sampleRate,
                    channelConfig,
                    encoding,
                    bufferSize,
                    AudioTrack.MODE_STREAM);
            if (track.getState() != AudioTrack.STATE_INITIALIZED)
            {
                track.release();
                postStatus("AudioTrack init failed");
                return null;
            }

            track.play();
            Log.i(TAG, "AudioTrack init ok sr=" + sampleRate
                    + " ch=" + channels
                    + " encoding=" + encoding
                    + " bufferSize=" + bufferSize);
            mAudioTrack = track;
            mTrackSampleRate = sampleRate;
            mTrackChannels = channels;
            mTrackEncoding = encoding;
            return mAudioTrack;
        }
    }

    private void applyInputControl(String inputId, float volume)
    {
        HJAudioMixerJni mixer = mMixer;
        if (mixer == null)
        {
            return;
        }

        int volumeRet = mixer.setInputVolume(inputId, volume);
        if (volumeRet < 0)
        {
            postStatus("apply input control failed input=" + inputId + " volumeRet=" + volumeRet);
        }
    }

    private void applyMasterMute()
    {
        HJAudioMixerJni mixer = mMixer;
        if (mixer == null)
        {
            return;
        }

        int ret = mixer.setMasterMute(mCbMasterMute.isChecked());
        if (ret < 0)
        {
            postStatus("master mute failed ret=" + ret);
        }
    }

    private void handleMixerNotify(int id, long value, String desc)
    {
        String notifyText = "mixer event id=" + id + " value=" + value + " desc=" + desc;
        if (desc != null && desc.contains("\"inputId\""))
        {
            mTvInputState.setText(desc);
            return;
        }
        if (desc != null && desc.contains("\"mixPts\""))
        {
            mTvStats.setText(desc);
            return;
        }
        mTvStatus.setText(notifyText);
    }

    private void updateRouteLabel(TextView labelView,
                                  String routeName,
                                  SeekBar seekBar)
    {
        labelView.setText(String.format(Locale.US,
                "%s volume: %.2f",
                routeName,
                currentVolume(seekBar)));
    }

    private float currentVolume(SeekBar seekBar)
    {
        return Math.max(0f, Math.min(1f, seekBar.getProgress() / (float) SEEKBAR_MAX));
    }

    private int sampleFmtToEncoding(int sampleFmt)
    {
        if (sampleFmt == HJAudioMixerJni.AV_SAMPLE_FMT_S16)
        {
            return AudioFormat.ENCODING_PCM_16BIT;
        }
        return AudioFormat.ENCODING_INVALID;
    }

    private int audioEncodingToSampleFmt(int encoding)
    {
        if (encoding == AudioFormat.ENCODING_PCM_16BIT)
        {
            return HJAudioMixerJni.AV_SAMPLE_FMT_S16;
        }
        return HJAudioMixerJni.AV_SAMPLE_FMT_S16;
    }

    private MixerAudioPlaybackRouter ensurePlaybackRouter()
    {
        if (mPlaybackRouter == null)
        {
            mPlaybackRouter = new MixerAudioPlaybackRouter(this::playMixedBuffer,
                    (pcmPlayer, statusListener) -> new AndroidAacDecoder(pcmPlayer, statusListener, this),
                    this::postStatus);
        }
        return mPlaybackRouter;
    }

    private void releasePlaybackRouter()
    {
        if (mPlaybackRouter != null)
        {
            mPlaybackRouter.release();
            mPlaybackRouter = null;
        }
    }

    private String getDefaultMusicPath()
    {
        return new File(getFilesDir(),
                "HJResource" + File.separator + "audio" + File.separator + PLAYER_A_FILE_NAME).getAbsolutePath();
    }

    private String getPlayerASourcePath()
    {
        return getDefaultMusicPath();
    }

    private String getPlayerBSourcePath()
    {
        return new File(getFilesDir(),
                "HJResource" + File.separator + "audio" + File.separator + PLAYER_B_FILE_NAME).getAbsolutePath();
    }

    private void postStatus(String text)
    {
        mMainHandler.post(() -> mTvStatus.setText(text));
    }

    private void postPlayerInfo(String text)
    {
        mMainHandler.post(() -> mTvPlayerInfo.setText(text));
    }

    private void showToast(String text)
    {
        Toast.makeText(this, text, Toast.LENGTH_SHORT).show();
    }

    private static final class AndroidAacDecoder implements MixerAudioPlaybackRouter.AacDecoder
    {
        private static final String MIME_AAC = "audio/mp4a-latm";
        private static final long CODEC_TIMEOUT_US = 10_000L;

        private final MixerAudioPlaybackRouter.PcmPlayer mPcmPlayer;
        private final MixerAudioPlaybackRouter.StatusListener mStatusListener;
        private final AudioMixerActivity mActivity;
        private final MediaCodec.BufferInfo mBufferInfo = new MediaCodec.BufferInfo();

        private MediaCodec mDecoder;
        private int mOutputSampleRate;
        private int mOutputChannels;
        private int mOutputSampleFmt = HJAudioMixerJni.AV_SAMPLE_FMT_S16;

        AndroidAacDecoder(MixerAudioPlaybackRouter.PcmPlayer pcmPlayer,
                          MixerAudioPlaybackRouter.StatusListener statusListener,
                          AudioMixerActivity activity)
        {
            mPcmPlayer = pcmPlayer;
            mStatusListener = statusListener;
            mActivity = activity;
        }

        @Override
        public int configure(int sampleRate, int channels, ByteBuffer codecConfig)
        {
            release();
            try
            {
                MediaFormat format = MediaFormat.createAudioFormat(MIME_AAC, sampleRate, channels);
                format.setByteBuffer("csd-0", codecConfig.duplicate());
                MediaCodec decoder = MediaCodec.createDecoderByType(MIME_AAC);
                decoder.configure(format, null, null, 0);
                decoder.start();
                mDecoder = decoder;
                mOutputSampleRate = sampleRate;
                mOutputChannels = channels;
                mOutputSampleFmt = HJAudioMixerJni.AV_SAMPLE_FMT_S16;
                return 0;
            }
            catch (Exception e)
            {
                postStatus("aac decoder configure exception=" + e.getMessage());
                release();
                return -1;
            }
        }

        @Override
        public int decode(ByteBuffer encodedData)
        {
            if (mDecoder == null)
            {
                postStatus("aac decoder is not configured");
                return -1;
            }

            ByteBuffer data = encodedData.duplicate();
            int size = data.remaining();
            if (size <= 0)
            {
                return 0;
            }

            try
            {
                int inputIndex = mDecoder.dequeueInputBuffer(CODEC_TIMEOUT_US);
                if (inputIndex < 0)
                {
                    if (inputIndex != MediaCodec.INFO_TRY_AGAIN_LATER)
                    {
                        postStatus("aac decoder dequeue input ret=" + inputIndex);
                    }
                    return 0;
                }

                ByteBuffer inputBuffer = mDecoder.getInputBuffer(inputIndex);
                if (inputBuffer == null)
                {
                    mDecoder.queueInputBuffer(inputIndex, 0, 0, 0, 0);
                    postStatus("aac decoder input buffer is null");
                    return -1;
                }
                if (inputBuffer.capacity() < size)
                {
                    mDecoder.queueInputBuffer(inputIndex, 0, 0, 0, 0);
                    postStatus("aac decoder input buffer too small cap="
                            + inputBuffer.capacity() + " size=" + size);
                    return -1;
                }

                inputBuffer.clear();
                inputBuffer.put(data);
                mDecoder.queueInputBuffer(inputIndex,
                        0,
                        size,
                        System.nanoTime() / 1000L,
                        0);
                Log.i(TAG, "aac decoder queued size=" + size
                        + " sr=" + mOutputSampleRate
                        + " ch=" + mOutputChannels);
                return drainOutput();
            }
            catch (Exception e)
            {
                postStatus("aac decoder decode exception=" + e.getMessage());
                release();
                return -1;
            }
        }

        @Override
        public void release()
        {
            if (mDecoder != null)
            {
                try
                {
                    mDecoder.stop();
                }
                catch (Exception ignore)
                {
                }
                try
                {
                    mDecoder.release();
                }
                catch (Exception ignore)
                {
                }
                mDecoder = null;
            }
            mOutputSampleRate = 0;
            mOutputChannels = 0;
            mOutputSampleFmt = HJAudioMixerJni.AV_SAMPLE_FMT_S16;
        }

        private int drainOutput()
        {
            int ret = 0;
            long timeoutUs = CODEC_TIMEOUT_US;
            while (mDecoder != null)
            {
                int outputIndex = mDecoder.dequeueOutputBuffer(mBufferInfo, timeoutUs);
                timeoutUs = 0L;
                if (outputIndex >= 0)
                {
                    ByteBuffer outputBuffer = mDecoder.getOutputBuffer(outputIndex);
                    if (outputBuffer != null && mBufferInfo.size > 0)
                    {
                        outputBuffer.position(mBufferInfo.offset);
                        outputBuffer.limit(mBufferInfo.offset + mBufferInfo.size);
                        ByteBuffer pcmBuffer = outputBuffer.slice();
                        Log.i(TAG, "aac decoder output pcm size=" + mBufferInfo.size
                                + " sr=" + mOutputSampleRate
                                + " ch=" + mOutputChannels
                                + " fmt=" + mOutputSampleFmt);
                        int playRet = mPcmPlayer.play(mOutputSampleRate,
                                mOutputChannels,
                                mOutputSampleFmt,
                                pcmBuffer);
                        if (playRet < 0 && ret == 0)
                        {
                            ret = playRet;
                        }
                    }
                    mDecoder.releaseOutputBuffer(outputIndex, false);
                    continue;
                }
                if (outputIndex == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED)
                {
                    updateOutputFormat(mDecoder.getOutputFormat());
                    Log.i(TAG, "aac decoder output format changed sr=" + mOutputSampleRate
                            + " ch=" + mOutputChannels
                            + " fmt=" + mOutputSampleFmt);
                    continue;
                }
                if (outputIndex == MediaCodec.INFO_TRY_AGAIN_LATER)
                {
                    Log.i(TAG, "aac decoder output not ready");
                    break;
                }
                break;
            }
            return ret;
        }

        private void updateOutputFormat(MediaFormat format)
        {
            if (format.containsKey(MediaFormat.KEY_SAMPLE_RATE))
            {
                mOutputSampleRate = format.getInteger(MediaFormat.KEY_SAMPLE_RATE);
            }
            if (format.containsKey(MediaFormat.KEY_CHANNEL_COUNT))
            {
                mOutputChannels = format.getInteger(MediaFormat.KEY_CHANNEL_COUNT);
            }
            if (format.containsKey(MediaFormat.KEY_PCM_ENCODING))
            {
                mOutputSampleFmt = mActivity.audioEncodingToSampleFmt(
                        format.getInteger(MediaFormat.KEY_PCM_ENCODING));
            }
        }

        private void postStatus(String text)
        {
            if (mStatusListener != null)
            {
                mStatusListener.onStatus(text);
            }
        }
    }
}
