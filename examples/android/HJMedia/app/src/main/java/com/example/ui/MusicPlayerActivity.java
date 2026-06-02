package com.example.ui;

import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.text.TextUtils;
import android.widget.Button;
import android.widget.EditText;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

import com.HJMediasdk.entry.HJCommonInterface;
import com.HJMediasdk.entry.player.HJMusicPlayerJni;
import com.HJMediasdk.entry.player.HJPlayerContextJni;
import com.HJMediasdk.utils.HJAudioListener;
import com.HJMediasdk.utils.HJBaseListener;
import com.HJMediasdk.utils.HJLog;

import java.io.File;
import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.Locale;

public class MusicPlayerActivity extends AppCompatActivity
{
    private static final String TAG = MusicPlayerActivity.class.getSimpleName();
    private static final String MUSIC_FILE_NAME = "c58733ac51124fe38cdc6540a7b8fa46.mkv";
    private static final long PROGRESS_POLL_INTERVAL_MS = 1000L;
    private static final int DEFAULT_PROGRESS_MAX_MS = 10 * 60 * 1000;
    private static final int VOLUME_SEEKBAR_MAX = 100;
    private static final float DEFAULT_VOLUME = 1.0f;

    private HJMusicPlayerJni mMusicPlayer;
    private EditText mEtUrl;
    private EditText mEtRepeats;
    private TextView mTvStatus;
    private TextView mTvTime;
    private TextView mTvVolume;
    private RadioGroup mRgAudioTracks;
    private SeekBar mSeekBar;
    private SeekBar mVolumeSeekBar;
    private final Handler mProgressHandler = new Handler(Looper.getMainLooper());
    private boolean mIsUserSeeking = false;
    private boolean mProgressTimerStarted = false;
    private boolean mUpdatingTrackGroup = false;
    private int mProgressMaxMs = DEFAULT_PROGRESS_MAX_MS;
    private float mCurrentVolume = DEFAULT_VOLUME;

    private final Runnable mProgressRunnable = new Runnable() {
        @Override
        public void run() {
            updateProgressFromNative();
            if (mProgressTimerStarted) {
                mProgressHandler.postDelayed(this, PROGRESS_POLL_INTERVAL_MS);
            }
        }
    };

    private final HJBaseListener mPlayerListener = (id, value, desc) -> {
        runOnUiThread(() -> handlePlayerNotify(id, value, desc));
        return 0;
    };

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_music_player);
        HJLog.setEnable(true);
        mEtUrl = findViewById(R.id.etMusicUrl);
        mEtRepeats = findViewById(R.id.etMusicRepeats);
        mTvStatus = findViewById(R.id.tvMusicStatus);
        mTvTime = findViewById(R.id.tvMusicTime);
        mTvVolume = findViewById(R.id.tvMusicVolume);
        mRgAudioTracks = findViewById(R.id.rgAudioTracks);
        mSeekBar = findViewById(R.id.seekMusicProgress);
        mVolumeSeekBar = findViewById(R.id.seekMusicVolume);
        Button btnInit = findViewById(R.id.btnMusicInit);
        Button btnOpen = findViewById(R.id.btnMusicOpen);
        Button btnPause = findViewById(R.id.btnMusicPause);
        Button btnResume = findViewById(R.id.btnMusicResume);
        Button btnDone = findViewById(R.id.btnMusicDone);

        String logDir = getFilesDir().getAbsolutePath() + File.separator + "player";
        HJPlayerContextJni.contextInit(
                logDir,
                HJCommonInterface.HJLOGLevel_INFO,
                HJCommonInterface.HJLogLMode_CONSOLE | HJCommonInterface.HJLLogMode_FILE,
                5 * 1024 * 1024,
                5);

        mMusicPlayer = new HJMusicPlayerJni();
        mEtUrl.setText(getDefaultMusicPath());
        mSeekBar.setMax(mProgressMaxMs);
        mSeekBar.setProgress(0);
        mVolumeSeekBar.setMax(VOLUME_SEEKBAR_MAX);
        mVolumeSeekBar.setProgress(volumeToProgress(mCurrentVolume));
        mTvVolume.setText(formatVolumeText(mCurrentVolume));
        mSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                if (fromUser) {
                    mTvTime.setText(formatProgressText(progress, seekBar.getMax()));
                }
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
                mIsUserSeeking = true;
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                mIsUserSeeking = false;
                if (mMusicPlayer != null) {
                    int ret = mMusicPlayer.seek(seekBar.getProgress());
                    mTvStatus.setText("seek ret=" + ret);
                    toastRet("seek", ret);
                } else {
                    mTvStatus.setText("seek failed: player is null");
                }
            }
        });
        mVolumeSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                mCurrentVolume = progressToVolume(progress);
                mTvVolume.setText(formatVolumeText(mCurrentVolume));
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                applyCurrentVolume(true);
            }
        });

        mRgAudioTracks.setOnCheckedChangeListener((group, checkedId) -> {
            if (mUpdatingTrackGroup) {
                return;
            }
            if (checkedId == RadioGroup.NO_ID) {
                return;
            }
            if (mMusicPlayer == null) {
                mTvStatus.setText("switch track failed: player is null");
                return;
            }
            RadioButton rb = group.findViewById(checkedId);
            if (rb == null || rb.getTag() == null) {
                return;
            }
            int trackId = (int) rb.getTag();
            int ret = mMusicPlayer.switchAudioTrack(trackId);
            mTvStatus.setText("switch track(" + trackId + ") ret=" + ret);
            toastRet("switchTrack", ret);
        });

        btnInit.setOnClickListener(v -> {
            if (mMusicPlayer != null)
            {
                int repeats = parseRepeats();
                int ret = mMusicPlayer.init(repeats, 120, 20, 48000, 2, mPlayerListener, new HJAudioListener.UI() {
                    @Override
                    public int notify(int i_sampleRate, int i_channels, int i_sampleFmt, int i_bytesPerSample, ByteBuffer i_buffer, int i_nFlag) {
                        if (i_buffer != null)
                        {
                            HJLog.i(TAG, "notify sampleRate:" + i_sampleRate + " channels:" + i_channels + " sampleFmt:" + i_sampleFmt + " bytesPerSample:" + i_bytesPerSample + " pmc size " + i_buffer.capacity() + " flag " + i_nFlag);
                        }
                        return 0;
                    }
                });
                mTvStatus.setText("init ret=" + ret);
                toastRet("init", ret);
            }
            else
            {
                mTvStatus.setText("init failed: player is null");
            }
        });

        btnOpen.setOnClickListener(v -> {
            String url = getDefaultMusicPath();
            mEtUrl.setText(url);
            if (TextUtils.isEmpty(url))
            {
                Toast.makeText(this, "input url", Toast.LENGTH_SHORT).show();
                return;
            }
            if (mMusicPlayer != null)
            {
                int ret = mMusicPlayer.open(url, 0);
                mTvStatus.setText("open ret=" + ret);
                toastRet("open", ret);
                if (ret >= 0) {
                    mSeekBar.setProgress(0);
                    mTvTime.setText(formatProgressText(0, mSeekBar.getMax()));
                    applyCurrentVolume(false);
                }
            }
            else
            {
                mTvStatus.setText("open failed: player is null");
            }
        });

        btnPause.setOnClickListener(v -> {
            if (mMusicPlayer != null)
            {
                int ret = mMusicPlayer.pause();
                mTvStatus.setText("pause ret=" + ret);
                toastRet("pause", ret);
            }
            else
            {
                mTvStatus.setText("pause failed: player is null");
            }
        });

        btnResume.setOnClickListener(v -> {
            if (mMusicPlayer != null)
            {
                int ret = mMusicPlayer.resume();
                mTvStatus.setText("resume ret=" + ret);
                toastRet("resume", ret);
            }
            else
            {
                mTvStatus.setText("resume failed: player is null");
            }
        });

        btnDone.setOnClickListener(v -> {
            if (mMusicPlayer != null)
            {
                mMusicPlayer.done();
                mTvStatus.setText("done");
                Toast.makeText(this, "done", Toast.LENGTH_SHORT).show();
            }
            else
            {
                mTvStatus.setText("done skipped: player is null");
            }
        });
    }

    private void toastRet(String op, int ret)
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

    private String getDefaultMusicPath()
    {
        return new File(getFilesDir(),
                "HJResource" + File.separator + "audio" + File.separator + MUSIC_FILE_NAME).getAbsolutePath();
    }

    private int parseRepeats()
    {
        if (mEtRepeats == null || mEtRepeats.getText() == null) {
            return 0;
        }
        String raw = mEtRepeats.getText().toString().trim();
        if (raw.isEmpty()) {
            return 0;
        }
        try {
            int repeats = Integer.parseInt(raw);
            return Math.max(0, repeats);
        } catch (Exception ignore) {
            return 0;
        }
    }

    @Override
    protected void onResume()
    {
        super.onResume();
    }

    @Override
    protected void onPause()
    {
        stopProgressTimer();
        mProgressHandler.removeCallbacks(mProgressRunnable);
        super.onPause();
    }

    private void updateProgressFromNative()
    {
        if (mIsUserSeeking || mMusicPlayer == null) {
            return;
        }

        long ts = mMusicPlayer.getCurrentTimestamp();
        if (ts < 0) {
            ts = 0;
        }
        if (ts > Integer.MAX_VALUE) {
            ts = Integer.MAX_VALUE;
        }
        int progress = (int) ts;
/*
        if (progress > mProgressMaxMs) {
            mProgressMaxMs = (((progress / 60000) + 1) * 60000);
            mSeekBar.setMax(mProgressMaxMs);
        }
*/
        mSeekBar.setProgress(progress);
        mTvTime.setText(formatProgressText(progress, mSeekBar.getMax()));
    }

    private void handlePlayerNotify(int id, long value, String desc)
    {
        String event = desc == null ? "" : desc;
        HJLog.i(TAG, "onNotify id:" + id + " value:" + value + " desc:" + event);

        if (HJMusicPlayerJni.EVT_STREAM_OPENED.equals(event))
        {
            if (mMusicPlayer != null)
            {
                int[] trackIds = mMusicPlayer.getAudioTrackIds();
                refreshAudioTrackOptions(trackIds);
                mTvStatus.setText("tracks: " + Arrays.toString(trackIds));

                mProgressMaxMs = Math.toIntExact(mMusicPlayer.getDuration());
                mSeekBar.setMax(mProgressMaxMs);

				applyCurrentVolume(false);
                startProgressTimer();
            }

            return;
        }
        if (HJMusicPlayerJni.EVT_EOF.equals(event))
        {
//            stopProgressTimer();
            return;
        }

        mTvStatus.setText(String.format(Locale.US, "event id=%d value=%d desc=%s", id, value, event));
    }

    private void startProgressTimer()
    {
        if (mProgressTimerStarted) {
            return;
        }
        mProgressTimerStarted = true;
        mProgressHandler.removeCallbacks(mProgressRunnable);
        mProgressHandler.postDelayed(mProgressRunnable, PROGRESS_POLL_INTERVAL_MS);
    }

    private void stopProgressTimer()
    {
        mProgressTimerStarted = false;
        mProgressHandler.removeCallbacks(mProgressRunnable);
    }

    private String formatProgressText(int progressMs, int maxMs)
    {
        return "time: " + formatMs(progressMs) + " / " + formatMs(maxMs);
    }

    private String formatVolumeText(float volume)
    {
        return String.format(Locale.US, "volume: %.2f", volume);
    }

    private int volumeToProgress(float volume)
    {
        return Math.round(Math.max(0.0f, Math.min(1.0f, volume)) * VOLUME_SEEKBAR_MAX);
    }

    private float progressToVolume(int progress)
    {
        int clamped = Math.max(0, Math.min(VOLUME_SEEKBAR_MAX, progress));
        return clamped / (float) VOLUME_SEEKBAR_MAX;
    }

    private void applyCurrentVolume(boolean updateStatus)
    {
        if (mMusicPlayer == null)
        {
            if (updateStatus)
            {
                mTvStatus.setText("setVolume failed: player is null");
            }
            return;
        }

        int ret = mMusicPlayer.setVolume(mCurrentVolume);
        if (updateStatus || ret < 0)
        {
            mTvStatus.setText(String.format(Locale.US, "setVolume(%.2f) ret=%d", mCurrentVolume, ret));
        }
    }

    private String formatMs(int ms)
    {
        int totalSec = ms / 1000;
        int min = totalSec / 60;
        int sec = totalSec % 60;
        return String.format("%02d:%02d", min, sec);
    }

    private void refreshAudioTrackOptions(int[] trackIds)
    {
        mUpdatingTrackGroup = true;
        mRgAudioTracks.removeAllViews();
        if (trackIds == null || trackIds.length == 0)
        {
            mUpdatingTrackGroup = false;
            return;
        }

        for (int i = 0; i < trackIds.length; i++)
        {
            int trackId = trackIds[i];
            RadioButton rb = new RadioButton(this);
            rb.setId(android.view.View.generateViewId());
            rb.setTag(trackId);
            rb.setText("track " + i + " (id=" + trackId + ")");
            mRgAudioTracks.addView(rb);
            if (i == 0)
            {
                rb.setChecked(true);
            }
        }
        mUpdatingTrackGroup = false;
    }

    @Override
    protected void onDestroy()
    {
        stopProgressTimer();
        if (mMusicPlayer != null)
        {
            mMusicPlayer.done();
            mMusicPlayer = null;
        }
        HJPlayerContextJni.contextUnInit();
        super.onDestroy();
    }
}
