package com.example.ui;

import android.Manifest;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.media.AudioDeviceInfo;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.os.Build;
import android.os.Bundle;
import android.text.TextUtils;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.content.ContextCompat;

import com.HJMediasdk.audio.HJAudioCompositeManager;
import com.HJMediasdk.audio.HJAudioListener;
import com.HJMediasdk.entry.HJCommonInterface;
import com.HJMediasdk.entry.player.HJPlayerContextJni;
import com.HJMediasdk.utils.HJLog;
import com.example.audio.AudioEffectSimulate;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Locale;
import java.util.Map;

public class AudioCompositeActivity extends AppCompatActivity
{
    private static final String TAG = AudioCompositeActivity.class.getSimpleName();
    private static final String PLAYER0_NAME = "player0";
    private static final String PLAYER1_NAME = "player1";
    private static final String PLAYER0_FILE_NAME = "c58733ac51124fe38cdc6540a7b8fa46.mkv";
    private static final String PLAYER1_FILE_NAME = "momoda.mp3";
    private static final int REQUEST_RECORD_AUDIO = 1001;
    private static final long PROGRESS_POLL_INTERVAL_MS = 1000L;
    private static final int VOLUME_SEEKBAR_MAX = 100;
    private static final int DEFAULT_PROGRESS_MAX_MS = 10 * 60 * 1000;
    private static final float DEFAULT_PLAYER_VOLUME = 1.0f;
    private static final float DEFAULT_MIX_PLAYER_RATIO = 1.0f;
    private static final float DEFAULT_MIX_MIC_RATIO = 1.0f;

    private final android.os.Handler mMainHandler = new android.os.Handler(android.os.Looper.getMainLooper());
    private final Object mRecordLock = new Object();
    private final Map<String, PlayerPanel> mPlayerPanelMap = new HashMap<>();

    private Button mBtnInit;
    private Button mBtnRelease;
    private CheckBox mCbEnableMixing;
    private CheckBox mCbMute;
    private TextView mTvStatus;
    private TextView mTvMixMicRatio;
    private SeekBar mSeekMixMicRatio;

    private HJAudioCompositeManager mAudioCompositeManager;
    private PlayerPanel mPlayer0Panel;
    private PlayerPanel mPlayer1Panel;
    private boolean mIsRecordingAac = false;
    private float mMixMicRatio = DEFAULT_MIX_MIC_RATIO;
    private boolean mPendingInitAfterPermission = false;
    private byte[] mCachedAacExtra;
    private ByteArrayOutputStream mRecordedAdtsBuffer;
    private MediaPlayer mRecordedAdtsPlayer;
    private final AudioEffectSimulate mAudioEffectSimulate = new AudioEffectSimulate();
    private AudioManager mAudioManager;
    private boolean mHeadsetReceiverRegistered = false;

    private final class PlayerPanel
    {
        private final String uniquePlayerName;
        private final EditText etUrl;
        private final Button btnOpen;
        private final Button btnClose;
        private final Button btnPause;
        private final Button btnResume;
        private final TextView tvPlayerVolume;
        private final TextView tvMixVolume;
        private final TextView tvTime;
        private final TextView tvTracks;
        private final TextView tvStatus;
        private final RadioGroup rgTracks;
        private final SeekBar seekProgress;
        private final SeekBar seekPlayerVolume;
        private final SeekBar seekMixVolume;

        private boolean isUserSeeking = false;
        private boolean progressTimerStarted = false;
        private boolean updatingTrackGroup = false;
        private int progressMaxMs = DEFAULT_PROGRESS_MAX_MS;
        private float playerVolume = DEFAULT_PLAYER_VOLUME;
        private float mixVolume = DEFAULT_MIX_PLAYER_RATIO;

        private final Runnable progressRunnable = new Runnable()
        {
            @Override
            public void run()
            {
                priUpdateProgress(PlayerPanel.this);
                if (progressTimerStarted)
                {
                    mMainHandler.postDelayed(this, PROGRESS_POLL_INTERVAL_MS);
                }
            }
        };

        private PlayerPanel(String uniquePlayerName,
                            int urlId,
                            int openId,
                            int closeId,
                            int pauseId,
                            int resumeId,
                            int playerVolumeTextId,
                            int mixVolumeTextId,
                            int timeId,
                            int tracksId,
                            int statusId,
                            int tracksGroupId,
                            int progressSeekId,
                            int playerVolumeSeekId,
                            int mixVolumeSeekId)
        {
            this.uniquePlayerName = uniquePlayerName;
            etUrl = findViewById(urlId);
            btnOpen = findViewById(openId);
            btnClose = findViewById(closeId);
            btnPause = findViewById(pauseId);
            btnResume = findViewById(resumeId);
            tvPlayerVolume = findViewById(playerVolumeTextId);
            tvMixVolume = findViewById(mixVolumeTextId);
            tvTime = findViewById(timeId);
            tvTracks = findViewById(tracksId);
            tvStatus = findViewById(statusId);
            rgTracks = findViewById(tracksGroupId);
            seekProgress = findViewById(progressSeekId);
            seekPlayerVolume = findViewById(playerVolumeSeekId);
            seekMixVolume = findViewById(mixVolumeSeekId);
        }

        private void bindControls()
        {
            btnOpen.setOnClickListener(v -> priOpenPlayer(this));
            btnClose.setOnClickListener(v -> priClosePlayer(this));
            btnPause.setOnClickListener(v -> priPausePlayer(this));
            btnResume.setOnClickListener(v -> priResumePlayer(this));

            seekProgress.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener()
            {
                @Override
                public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser)
                {
                    if (fromUser)
                    {
                        tvTime.setText(priFormatProgressText(progress, seekBar.getMax()));
                    }
                }

                @Override
                public void onStartTrackingTouch(SeekBar seekBar)
                {
                    isUserSeeking = true;
                }

                @Override
                public void onStopTrackingTouch(SeekBar seekBar)
                {
                    isUserSeeking = false;
                    if (mAudioCompositeManager == null)
                    {
                        priPostPlayerStatus(PlayerPanel.this, "seek failed: manager is null");
                        return;
                    }
                    int ret = mAudioCompositeManager.playerSeek(uniquePlayerName, seekBar.getProgress());
                    priPostPlayerStatus(PlayerPanel.this, "seek ret=" + ret);
                    priToastRet(uniquePlayerName + " seek", ret);
                }
            });

            seekPlayerVolume.setMax(VOLUME_SEEKBAR_MAX);
            seekMixVolume.setMax(VOLUME_SEEKBAR_MAX);
            seekPlayerVolume.setProgress(priVolumeToProgress(playerVolume));
            seekMixVolume.setProgress(priVolumeToProgress(mixVolume));

            seekPlayerVolume.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener()
            {
                @Override
                public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser)
                {
                    playerVolume = priProgressToVolume(progress);
                    refreshVolumeText();
                }

                @Override
                public void onStartTrackingTouch(SeekBar seekBar)
                {
                }

                @Override
                public void onStopTrackingTouch(SeekBar seekBar)
                {
                    if (mAudioCompositeManager == null)
                    {
                        return;
                    }
                    int ret = mAudioCompositeManager.playerSetVolume(uniquePlayerName, playerVolume);
                    priPostPlayerStatus(PlayerPanel.this,
                            String.format(Locale.US, "playerSetVolume(%.2f) ret=%d", playerVolume, ret));
                }
            });

            seekMixVolume.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener()
            {
                @Override
                public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser)
                {
                    mixVolume = priProgressToVolume(progress);
                    refreshVolumeText();
                }

                @Override
                public void onStartTrackingTouch(SeekBar seekBar)
                {
                }

                @Override
                public void onStopTrackingTouch(SeekBar seekBar)
                {
                    if (mAudioCompositeManager == null)
                    {
                        return;
                    }
                    int ret = mAudioCompositeManager.mixerSetPlayerVolume(uniquePlayerName, mixVolume);
                    priPostPlayerStatus(PlayerPanel.this,
                            String.format(Locale.US, "mixVolume(%.2f) ret=%d", mixVolume, ret));
                }
            });

            rgTracks.setOnCheckedChangeListener((group, checkedId) -> {
                if (updatingTrackGroup || checkedId == RadioGroup.NO_ID || mAudioCompositeManager == null)
                {
                    return;
                }
                RadioButton radioButton = group.findViewById(checkedId);
                if (radioButton == null || radioButton.getTag() == null)
                {
                    return;
                }
                int trackId = (int) radioButton.getTag();
                int ret = mAudioCompositeManager.playerSwitchAudioTrack(uniquePlayerName, trackId);
                priPostPlayerStatus(PlayerPanel.this, "switchTrack(" + trackId + ") ret=" + ret);
                priToastRet(uniquePlayerName + " switchTrack", ret);
            });

            refreshVolumeText();
            resetPlaybackUi();
        }

        private void setDefaultUrl(String url)
        {
            etUrl.setText(url);
        }

        private String getUrl()
        {
            return etUrl.getText() == null ? "" : etUrl.getText().toString().trim();
        }

        private void refreshVolumeText()
        {
            tvPlayerVolume.setText(String.format(Locale.US, "player volume: %.2f", playerVolume));
            tvMixVolume.setText(String.format(Locale.US, "mix volume: %.2f", mixVolume));
        }

        private void resetPlaybackUi()
        {
            stopProgressTimer();
            clearTracks();
            progressMaxMs = DEFAULT_PROGRESS_MAX_MS;
            seekProgress.setMax(progressMaxMs);
            seekProgress.setProgress(0);
            tvTime.setText(priFormatProgressText(0, progressMaxMs));
            tvStatus.setText(getString(R.string.audio_composite_status_default));
        }

        private void handleTracksReady(int[] tracks)
        {
            tvTracks.setText("tracks: " + Arrays.toString(tracks));
            refreshTrackOptions(tracks);
        }

        private void handleDurationReady(long durationMs)
        {
            long safeDuration = Math.max(0L, durationMs);
            progressMaxMs = safeDuration > Integer.MAX_VALUE ? Integer.MAX_VALUE : (int) safeDuration;
            seekProgress.setMax(Math.max(1, progressMaxMs));
            tvTime.setText(priFormatProgressText(0, seekProgress.getMax()));
            startProgressTimer();
        }

        private void handleEof()
        {
            priPostPlayerStatus(this, "player eof");
            stopProgressTimer();
        }

        private void updateProgress()
        {
            if (isUserSeeking || mAudioCompositeManager == null)
            {
                return;
            }

            long timestamp = mAudioCompositeManager.playerGetCurrentTimestamp(uniquePlayerName);
            if (timestamp < 0)
            {
                timestamp = 0;
            }
            if (timestamp > Integer.MAX_VALUE)
            {
                timestamp = Integer.MAX_VALUE;
            }

            int progress = (int) timestamp;
            seekProgress.setProgress(progress);
            tvTime.setText(priFormatProgressText(progress, seekProgress.getMax()));
        }

        private void startProgressTimer()
        {
            if (progressTimerStarted)
            {
                return;
            }
            progressTimerStarted = true;
            mMainHandler.removeCallbacks(progressRunnable);
            mMainHandler.postDelayed(progressRunnable, PROGRESS_POLL_INTERVAL_MS);
        }

        private void stopProgressTimer()
        {
            progressTimerStarted = false;
            mMainHandler.removeCallbacks(progressRunnable);
        }

        private void refreshTrackOptions(int[] tracks)
        {
            updatingTrackGroup = true;
            rgTracks.removeAllViews();
            if (tracks != null)
            {
                for (int i = 0; i < tracks.length; i++)
                {
                    int trackId = tracks[i];
                    RadioButton radioButton = new RadioButton(AudioCompositeActivity.this);
                    radioButton.setId(android.view.View.generateViewId());
                    radioButton.setTag(trackId);
                    radioButton.setText("track " + i + " (id=" + trackId + ")");
                    rgTracks.addView(radioButton);
                    if (i == 0)
                    {
                        radioButton.setChecked(true);
                    }
                }
            }
            updatingTrackGroup = false;
        }

        private void clearTracks()
        {
            updatingTrackGroup = true;
            rgTracks.removeAllViews();
            updatingTrackGroup = false;
            tvTracks.setText(getString(R.string.audio_composite_tracks_default));
        }
    }

    private final BroadcastReceiver mHeadsetPlugReceiver = new BroadcastReceiver()
    {
        @Override
        public void onReceive(Context context, Intent intent)
        {
            if (intent == null || !Intent.ACTION_HEADSET_PLUG.equals(intent.getAction()))
            {
                return;
            }
            int state = intent.getIntExtra("state", -1);
            boolean hasHeadsetConnected = priHasHeadsetConnected();
            HJLog.i(TAG, "headset plug broadcast state=" + state + ", connected=" + hasHeadsetConnected);
            priApplyHeadsetMixingState(hasHeadsetConnected);
        }
    };

    private final HJAudioCompositeManager.PlayerListener mPlayerListener =
            new HJAudioCompositeManager.PlayerListener()
            {
                @Override
                public void onTracksReady(String uniquePlayerName, int[] tracks)
                {
                    runOnUiThread(() -> {
                        PlayerPanel panel = priGetPlayerPanel(uniquePlayerName);
                        if (panel != null)
                        {
                            panel.handleTracksReady(tracks);
                        }
                    });
                }

                @Override
                public void onDurationReady(String uniquePlayerName, long durationMs)
                {
                    runOnUiThread(() -> {
                        PlayerPanel panel = priGetPlayerPanel(uniquePlayerName);
                        if (panel != null)
                        {
                            panel.handleDurationReady(durationMs);
                        }
                    });
                }

                @Override
                public void onEof(String uniquePlayerName)
                {
                    runOnUiThread(() -> {
                        PlayerPanel panel = priGetPlayerPanel(uniquePlayerName);
                        if (panel != null)
                        {
                            panel.handleEof();
                        }
                    });
                }
            };

    private final HJAudioListener mAacListener = new HJAudioListener()
    {
        @Override
        public int onExtraReady(ByteBuffer extra, int size)
        {
            synchronized (mRecordLock)
            {
                mCachedAacExtra = priCopyBufferBytes(extra, size);
            }
            return 0;
        }

        @Override
        public int onCapAAC(ByteBuffer outputFrame, int size, long sysTimeMs, int flag)
        {
            if (flag != HJAudioListener.AAC_MDAT)
            {
                return 0;
            }

            byte[] aacFrame = priCopyBufferBytes(outputFrame, size);
            if (aacFrame == null || aacFrame.length <= 0)
            {
                return 0;
            }

            synchronized (mRecordLock)
            {
                if (!mIsRecordingAac)
                {
                    return 0;
                }

                if (mRecordedAdtsBuffer == null)
                {
                    mRecordedAdtsBuffer = new ByteArrayOutputStream();
                }
                mRecordedAdtsBuffer.write(aacFrame, 0, aacFrame.length);
            }
            return 0;
        }

        @Override
        public int onErr(int extra)
        {
            runOnUiThread(() -> priPostStatus("aac callback err=" + extra));
            return 0;
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_audio_composite);

        HJLog.setEnable(true);

        String logDir = getFilesDir().getAbsolutePath() + File.separator + "audiocomposite";
        HJPlayerContextJni.contextInit(
                logDir,
                HJCommonInterface.HJLOGLevel_INFO,
                HJCommonInterface.HJLogLMode_CONSOLE | HJCommonInterface.HJLLogMode_FILE,
                5 * 1024 * 1024,
                5);

        priBindViews();
        priBindControls();
        priRefreshUrls();
        priRefreshMixMicText();
        priRefreshInitReleaseButtons(false);
        priRegisterHeadsetReceiver();
        priApplyHeadsetMixingState(priHasHeadsetConnected());
    }

    @Override
    protected void onDestroy()
    {
        priUnregisterHeadsetReceiver();
        priReleaseRecordedPlayer();
        priReleaseManager();
        mAudioEffectSimulate.release();
        super.onDestroy();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults)
    {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode != REQUEST_RECORD_AUDIO)
        {
            return;
        }

        boolean granted = grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED;
        if (!granted)
        {
            mPendingInitAfterPermission = false;
            priPostStatus("record audio permission denied");
            Toast.makeText(this, "record audio permission denied", Toast.LENGTH_SHORT).show();
            return;
        }

        priPostStatus("record audio permission granted");
        if (mPendingInitAfterPermission)
        {
            mPendingInitAfterPermission = false;
            priInitManager();
        }
    }

    private void priBindViews()
    {
        mBtnInit = findViewById(R.id.btnCompositeInit);
        mBtnRelease = findViewById(R.id.btnCompositeRelease);
        mCbEnableMixing = findViewById(R.id.cbCompositeEnableMixing);
        mCbMute = findViewById(R.id.cbCompositeMute);
        mTvStatus = findViewById(R.id.tvCompositeStatus);
        mTvMixMicRatio = findViewById(R.id.tvCompositeMixMicRatio);
        mSeekMixMicRatio = findViewById(R.id.seekCompositeMixMicRatio);

        mPlayer0Panel = new PlayerPanel(PLAYER0_NAME,
                                        R.id.etCompositePlayer0Url,
                                        R.id.btnCompositePlayer0Open,
                                        R.id.btnCompositePlayer0Close,
                                        R.id.btnCompositePlayer0Pause,
                                        R.id.btnCompositePlayer0Resume,
                                        R.id.tvCompositePlayer0Volume,
                                        R.id.tvCompositePlayer0MixVolume,
                                        R.id.tvCompositePlayer0Time,
                                        R.id.tvCompositePlayer0Tracks,
                                        R.id.tvCompositePlayer0Status,
                                        R.id.rgCompositePlayer0Tracks,
                                        R.id.seekCompositePlayer0Progress,
                                        R.id.seekCompositePlayer0Volume,
                                        R.id.seekCompositePlayer0MixVolume);
        mPlayer1Panel = new PlayerPanel(PLAYER1_NAME,
                                        R.id.etCompositePlayer1Url,
                                        R.id.btnCompositePlayer1Open,
                                        R.id.btnCompositePlayer1Close,
                                        R.id.btnCompositePlayer1Pause,
                                        R.id.btnCompositePlayer1Resume,
                                        R.id.tvCompositePlayer1Volume,
                                        R.id.tvCompositePlayer1MixVolume,
                                        R.id.tvCompositePlayer1Time,
                                        R.id.tvCompositePlayer1Tracks,
                                        R.id.tvCompositePlayer1Status,
                                        R.id.rgCompositePlayer1Tracks,
                                        R.id.seekCompositePlayer1Progress,
                                        R.id.seekCompositePlayer1Volume,
                                        R.id.seekCompositePlayer1MixVolume);

        mPlayerPanelMap.put(PLAYER0_NAME, mPlayer0Panel);
        mPlayerPanelMap.put(PLAYER1_NAME, mPlayer1Panel);
    }

    private void priBindControls()
    {
        mBtnInit.setOnClickListener(v -> priInitManager());
        mBtnRelease.setOnClickListener(v -> priReleaseManager());

        mCbEnableMixing.setOnCheckedChangeListener((buttonView, isChecked) -> {
            if (mAudioCompositeManager != null)
            {
                mAudioCompositeManager.setEnableMixing(isChecked);
            }
            priPostStatus("enableMixing=" + isChecked);
        });

        mCbMute.setOnCheckedChangeListener((buttonView, isChecked) -> {
            if (mAudioCompositeManager != null)
            {
                mAudioCompositeManager.mixerSetMute(isChecked);
            }
            priPostStatus("mute=" + isChecked);
        });

        Button btnRecordBegin = findViewById(R.id.btnCompositeRecordBegin);
        Button btnRecordEnd = findViewById(R.id.btnCompositeRecordEnd);
        btnRecordBegin.setOnClickListener(v -> priRecordBegin());
        btnRecordEnd.setOnClickListener(v -> priRecordEnd());

        mSeekMixMicRatio.setMax(VOLUME_SEEKBAR_MAX);
        mSeekMixMicRatio.setProgress(priVolumeToProgress(mMixMicRatio));
        mSeekMixMicRatio.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener()
        {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser)
            {
                mMixMicRatio = priProgressToVolume(progress);
                priRefreshMixMicText();
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar)
            {
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar)
            {
                if (mAudioCompositeManager == null)
                {
                    return;
                }
                int ret = mAudioCompositeManager.mixerSetMicVolume(mMixMicRatio);
                priPostStatus(String.format(Locale.US, "mixMicRatio(%.2f) ret=%d", mMixMicRatio, ret));
            }
        });

        mPlayer0Panel.bindControls();
        mPlayer1Panel.bindControls();
    }

    private void priInitManager()
    {
        if (!priHasRecordAudioPermission())
        {
            mPendingInitAfterPermission = true;
            requestPermissions(new String[]{Manifest.permission.RECORD_AUDIO}, REQUEST_RECORD_AUDIO);
            priPostStatus("requesting record audio permission");
            return;
        }
        mPendingInitAfterPermission = false;
        priReleaseManager();

        HJAudioCompositeManager audioCompositeManager = new HJAudioCompositeManager();
        audioCompositeManager.setCaptureAudioConfig(44100, 1);
        audioCompositeManager.setPlayerAudioConfig(48000, 2);
        audioCompositeManager.setMixAudioConfig(48000, 2);
        audioCompositeManager.setBitrate(128000);
        audioCompositeManager.setIsADTS(true);
        audioCompositeManager.setEnableMixing(mCbEnableMixing.isChecked());
        audioCompositeManager.mixerSetMute(mCbMute.isChecked());
        audioCompositeManager.setCapturePcmListener((buffer, samples, sampleBytes, channels) ->
                mAudioEffectSimulate.doEffect(buffer, samples, sampleBytes, channels));
        audioCompositeManager.setListener(mAacListener);

        int ret = audioCompositeManager.init();
        if (ret < 0)
        {
            audioCompositeManager.release();
            priPostStatus("init failed ret=" + ret);
            priToastRet("init", ret);
            return;
        }

        mAudioCompositeManager = audioCompositeManager;
        priApplyHeadsetMixingState(priHasHeadsetConnected());
        mAudioCompositeManager.mixerSetMicVolume(mMixMicRatio);
        priRefreshInitReleaseButtons(true);
        priPostStatus("init ok");
        priToastRet("init", ret);
    }

    private void priReleaseManager()
    {
        synchronized (mRecordLock)
        {
            mIsRecordingAac = false;
            mRecordedAdtsBuffer = null;
        }

        if (mPlayer0Panel != null)
        {
            mPlayer0Panel.resetPlaybackUi();
        }
        if (mPlayer1Panel != null)
        {
            mPlayer1Panel.resetPlaybackUi();
        }

        if (mAudioCompositeManager != null)
        {
            mAudioCompositeManager.release();
            mAudioCompositeManager = null;
        }

        priRefreshInitReleaseButtons(false);
        priPostStatus("release");
    }

    private void priRegisterHeadsetReceiver()
    {
        if (mHeadsetReceiverRegistered)
        {
            return;
        }
        IntentFilter filter = new IntentFilter(Intent.ACTION_HEADSET_PLUG);
        ContextCompat.registerReceiver(this, mHeadsetPlugReceiver, filter, ContextCompat.RECEIVER_NOT_EXPORTED);
        mHeadsetReceiverRegistered = true;
    }

    private void priUnregisterHeadsetReceiver()
    {
        if (!mHeadsetReceiverRegistered)
        {
            return;
        }
        unregisterReceiver(mHeadsetPlugReceiver);
        mHeadsetReceiverRegistered = false;
    }

    private void priApplyHeadsetMixingState(boolean enableMixing)
    {
        HJLog.i(TAG, "apply headset mixing enable=" + enableMixing);
        if (mCbEnableMixing.isChecked() != enableMixing)
        {
            mCbEnableMixing.setChecked(enableMixing);
        }

        if (mAudioCompositeManager != null)
        {
            HJLog.i(TAG, "setEnableMixing direct enable=" + enableMixing);
            mAudioCompositeManager.setEnableMixing(enableMixing);
        }
        priPostStatus("headset mixing=" + enableMixing);
    }

    private boolean priHasHeadsetConnected()
    {
        if (mAudioManager == null)
        {
            mAudioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
        }
        if (mAudioManager == null)
        {
            return false;
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M)
        {
            AudioDeviceInfo[] devices = mAudioManager.getDevices(AudioManager.GET_DEVICES_OUTPUTS);
            if (devices != null)
            {
                for (AudioDeviceInfo device : devices)
                {
                    if (device == null)
                    {
                        continue;
                    }
                    int type = device.getType();
                    if (type == AudioDeviceInfo.TYPE_WIRED_HEADSET
                            || type == AudioDeviceInfo.TYPE_WIRED_HEADPHONES
                            || type == AudioDeviceInfo.TYPE_USB_HEADSET
                            || type == AudioDeviceInfo.TYPE_BLUETOOTH_A2DP
                            || type == AudioDeviceInfo.TYPE_BLUETOOTH_SCO)
                    {
                        HJLog.i(TAG, "headset detected output type=" + type);
                        return true;
                    }
                }
            }
            HJLog.i(TAG, "headset detected output type=none");
            return false;
        }
        boolean hasHeadsetConnected = mAudioManager.isWiredHeadsetOn()
                || mAudioManager.isBluetoothA2dpOn()
                || mAudioManager.isBluetoothScoOn();
        HJLog.i(TAG, "headset legacy detect connected=" + hasHeadsetConnected);
        return hasHeadsetConnected;
    }

    private void priRecordBegin()
    {
        if (mAudioCompositeManager == null)
        {
            priPostStatus("recordBegin failed: manager is null");
            return;
        }

        synchronized (mRecordLock)
        {
            mIsRecordingAac = true;
            mRecordedAdtsBuffer = new ByteArrayOutputStream();
        }
        priReleaseRecordedPlayer();
        priPostStatus("recordBegin");
    }

    private void priRecordEnd()
    {
        byte[] adtsData;
        int extraSize;
        synchronized (mRecordLock)
        {
            mIsRecordingAac = false;
            adtsData = mRecordedAdtsBuffer == null ? null : mRecordedAdtsBuffer.toByteArray();
            mRecordedAdtsBuffer = null;
            extraSize = mCachedAacExtra == null ? 0 : mCachedAacExtra.length;
        }

        priReleaseManager();

        if (adtsData == null || adtsData.length <= 0)
        {
            priPostStatus("recordEnd no adts data");
            return;
        }

        try
        {
            File outputFile = priGetRecordedAdtsFile();
            try (FileOutputStream outputStream = new FileOutputStream(outputFile, false))
            {
                outputStream.write(adtsData);
                outputStream.flush();
            }
            priPlayRecordedAdts(outputFile);
            priPostStatus(String.format(Locale.US,
                    "recordEnd play adts size=%d extra=%d",
                    adtsData.length,
                    extraSize));
        }
        catch (Exception e)
        {
            HJLog.e(TAG, "recordEnd failed " + e.getMessage());
            priPostStatus("recordEnd failed: " + e.getMessage());
        }
    }

    private void priOpenPlayer(PlayerPanel panel)
    {
        if (mAudioCompositeManager == null)
        {
            priPostPlayerStatus(panel, "open failed: manager is null");
            return;
        }

        String url = panel.getUrl();
        if (TextUtils.isEmpty(url))
        {
            Toast.makeText(this, "input url", Toast.LENGTH_SHORT).show();
            return;
        }

        int ret = mAudioCompositeManager.playerOpen(panel.uniquePlayerName, url, mPlayerListener);
        priPostPlayerStatus(panel, "playerOpen ret=" + ret);
        priToastRet(panel.uniquePlayerName + " playerOpen", ret);
        if (ret >= 0)
        {
            panel.resetPlaybackUi();
            panel.startProgressTimer();
            mAudioCompositeManager.playerSetVolume(panel.uniquePlayerName, panel.playerVolume);
            mAudioCompositeManager.mixerSetPlayerVolume(panel.uniquePlayerName, panel.mixVolume);
            mAudioCompositeManager.mixerSetMicVolume(mMixMicRatio);
        }
    }

    private void priClosePlayer(PlayerPanel panel)
    {
        if (mAudioCompositeManager == null)
        {
            priPostPlayerStatus(panel, "playerClose skipped: manager is null");
            return;
        }
        mAudioCompositeManager.playerClose(panel.uniquePlayerName);
        panel.resetPlaybackUi();
        priPostPlayerStatus(panel, "playerClose");
    }

    private void priPausePlayer(PlayerPanel panel)
    {
        if (mAudioCompositeManager == null)
        {
            priPostPlayerStatus(panel, "pause failed: manager is null");
            return;
        }
        int ret = mAudioCompositeManager.playerPause(panel.uniquePlayerName);
        priPostPlayerStatus(panel, "pause ret=" + ret);
        priToastRet(panel.uniquePlayerName + " pause", ret);
    }

    private void priResumePlayer(PlayerPanel panel)
    {
        if (mAudioCompositeManager == null)
        {
            priPostPlayerStatus(panel, "resume failed: manager is null");
            return;
        }
        int ret = mAudioCompositeManager.playerResume(panel.uniquePlayerName);
        priPostPlayerStatus(panel, "resume ret=" + ret);
        priToastRet(panel.uniquePlayerName + " resume", ret);
    }

    private void priUpdateProgress(PlayerPanel panel)
    {
        if (panel != null)
        {
            panel.updateProgress();
        }
    }

    private void priRefreshUrls()
    {
        mPlayer0Panel.setDefaultUrl(priGetDefaultMusicPath(PLAYER0_FILE_NAME));
        mPlayer1Panel.setDefaultUrl(priGetDefaultMusicPath(PLAYER1_FILE_NAME));
    }

    private void priRefreshMixMicText()
    {
        mTvMixMicRatio.setText(String.format(Locale.US, "mix_mic_ratio: %.2f", mMixMicRatio));
    }

    private String priGetDefaultMusicPath(String fileName)
    {
        return new File(getFilesDir(),
                "HJResource" + File.separator + "audio" + File.separator + fileName).getAbsolutePath();
    }

    private String priFormatProgressText(int progressMs, int maxMs)
    {
        return "time: " + priFormatMs(progressMs) + " / " + priFormatMs(maxMs);
    }

    private String priFormatMs(int ms)
    {
        int totalSec = ms / 1000;
        int min = totalSec / 60;
        int sec = totalSec % 60;
        return String.format(Locale.US, "%02d:%02d", min, sec);
    }

    private int priVolumeToProgress(float volume)
    {
        return Math.round(Math.max(0.0f, Math.min(1.0f, volume)) * VOLUME_SEEKBAR_MAX);
    }

    private float priProgressToVolume(int progress)
    {
        int clamped = Math.max(0, Math.min(VOLUME_SEEKBAR_MAX, progress));
        return clamped / (float) VOLUME_SEEKBAR_MAX;
    }

    private void priPostPlayerStatus(PlayerPanel panel, String text)
    {
        if (panel != null)
        {
            panel.tvStatus.setText(text);
            priPostStatus(panel.uniquePlayerName + ": " + text);
        }
    }

    private void priPostStatus(String text)
    {
        mTvStatus.setText(text);
    }

    private PlayerPanel priGetPlayerPanel(String uniquePlayerName)
    {
        return mPlayerPanelMap.get(uniquePlayerName);
    }

    private void priToastRet(String op, int ret)
    {
        if (ret < 0)
        {
            Toast.makeText(this, op + " failed: " + ret, Toast.LENGTH_SHORT).show();
        }
        else
        {
            Toast.makeText(this, op + " ok", Toast.LENGTH_SHORT).show();
        }
    }

    private void priRefreshInitReleaseButtons(boolean inited)
    {
        if (mBtnInit != null)
        {
            mBtnInit.setEnabled(!inited);
        }
        if (mBtnRelease != null)
        {
            mBtnRelease.setEnabled(inited);
        }
    }

    private boolean priHasRecordAudioPermission()
    {
        return ContextCompat.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO)
                == PackageManager.PERMISSION_GRANTED;
    }

    private byte[] priCopyBufferBytes(ByteBuffer buffer, int size)
    {
        if (buffer == null || size <= 0)
        {
            return null;
        }

        ByteBuffer duplicate = buffer.duplicate();
        int copySize = Math.min(size, duplicate.remaining());
        if (copySize <= 0)
        {
            return null;
        }

        byte[] outData = new byte[copySize];
        duplicate.get(outData, 0, copySize);
        return outData;
    }

    private File priGetRecordedAdtsFile()
    {
        return new File(getCacheDir(), "audio_composite_record.aac");
    }

    private void priPlayRecordedAdts(File audioFile) throws Exception
    {
        if (audioFile == null || !audioFile.exists())
        {
            throw new IllegalArgumentException("aac file missing");
        }

        priReleaseRecordedPlayer();

        MediaPlayer mediaPlayer = new MediaPlayer();
        mediaPlayer.setDataSource(audioFile.getAbsolutePath());
        mediaPlayer.setOnCompletionListener(mp -> priReleaseRecordedPlayer());
        mediaPlayer.setOnErrorListener((mp, what, extra) -> {
            priPostStatus("play adts failed what=" + what + " extra=" + extra);
            priReleaseRecordedPlayer();
            return true;
        });
        mediaPlayer.prepare();
        mediaPlayer.start();
        mRecordedAdtsPlayer = mediaPlayer;
    }

    private void priReleaseRecordedPlayer()
    {
        if (mRecordedAdtsPlayer == null)
        {
            return;
        }

        try
        {
            if (mRecordedAdtsPlayer.isPlaying())
            {
                mRecordedAdtsPlayer.stop();
            }
        }
        catch (Exception ignore)
        {
        }

        mRecordedAdtsPlayer.release();
        mRecordedAdtsPlayer = null;
    }
}
