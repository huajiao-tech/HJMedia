package com.example.ui;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.util.Log;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

import com.HJMediasdk.entry.HJCommonInterface;
import com.HJMediasdk.entry.inference.HJInferenceContextJni;
import com.HJMediasdk.entry.inference.HJInferenceEntryJni;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class SRActivity extends AppCompatActivity {

    private static final String TAG = "SRActivity";

    private HJInferenceEntryJni mInferenceEntryJni;
    private ExecutorService mProcessExecutor;

    private RadioGroup mRgImage;
    private RadioGroup mRgSRType;
    private RadioGroup mRgModel;
    private RadioGroup mRgDevice;
    private RadioGroup mRgThread;
    private RadioGroup mRgScale;
    private ImageView mSrView;
    private TextView mTvElapse;
    private TextView mTvStatus;
    private Button mBtnInit;
    private Button mBtnProcess;

    private File mResourceRoot;
    private int mOriginalWidth = 0;
    private int mOriginalHeight = 0;
    private int mOutIndex = 0;
    private Bitmap mDisplayedBitmap;
    private boolean mSrInited = false;
    private String mInitConfigKey = "";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_sr);

        final String logDir = getFilesDir().getAbsolutePath() + File.separator + "inference";
        int initErr = HJInferenceContextJni.contextInit(
                logDir,
                HJCommonInterface.HJLOGLevel_INFO,
                HJCommonInterface.HJLogLMode_CONSOLE | HJCommonInterface.HJLLogMode_FILE,
                5 * 1024 * 1024,
                5);
        Log.i(TAG, "contextInit err=" + initErr + ", logDir=" + logDir);

        mInferenceEntryJni = new HJInferenceEntryJni();
        mProcessExecutor = Executors.newSingleThreadExecutor();
        mResourceRoot = new File(getFilesDir(), "HJResource");
        ensureImageResource("180x200.jpg");
        ensureImageResource("128x128.jpg");
        ensureImageResource("360x640.jpg");

        mRgImage = findViewById(R.id.rgImageRatio);
        mRgSRType = findViewById(R.id.rgSRType);
        mRgModel = findViewById(R.id.rgModelType);
        mRgDevice = findViewById(R.id.rgDeviceType);
        mRgThread = findViewById(R.id.rgThreadNum);
        mRgScale = findViewById(R.id.rgScaleNum);
        mSrView = findViewById(R.id.srImageView);
        mTvElapse = findViewById(R.id.tvElapse);
        mTvStatus = findViewById(R.id.tvStatus);
        mBtnInit = findViewById(R.id.btnInit);
        mBtnProcess = findViewById(R.id.btnProcess);

        mRgImage.setOnCheckedChangeListener((group, checkedId) -> drawImageScaled4(getSelectedImagePath(), true));
        mRgModel.setOnCheckedChangeListener((group, checkedId) -> markInitDirty());
        mRgSRType.setOnCheckedChangeListener((group, checkedId) -> markInitDirty());
        mRgDevice.setOnCheckedChangeListener((group, checkedId) -> markInitDirty());
        mRgThread.setOnCheckedChangeListener((group, checkedId) -> markInitDirty());
        mRgScale.setOnCheckedChangeListener((group, checkedId) -> markInitDirty());
        mBtnInit.setOnClickListener(v -> runInit());
        mBtnProcess.setOnClickListener(v -> runProcess());
    }

    @Override
    protected void onResume() {
        super.onResume();
        drawImageScaled4(getSelectedImagePath(), true);
    }

    @Override
    protected void onDestroy() {
        if (mProcessExecutor != null) {
            mProcessExecutor.shutdownNow();
            mProcessExecutor = null;
        }
        releaseDisplayedBitmap();
        HJInferenceContextJni.contextUnInit();
        super.onDestroy();
    }

    private void runProcess() {
        final String configKey = buildConfigKey();
        if (!mSrInited || !configKey.equals(mInitConfigKey)) {
            mTvStatus.setText("not inited, click init first");
            Toast.makeText(this, "please click init first", Toast.LENGTH_SHORT).show();
            return;
        }

        final String inputPath = getSelectedImagePath();
        final File outDir = new File(mResourceRoot, "image");
        if (!outDir.exists()) {
            outDir.mkdirs();
        }
        final String outPath = new File(outDir, "out_" + (mOutIndex++) + ".jpg").getAbsolutePath();

        if (!new File(inputPath).exists()) {
            Toast.makeText(this, "input not found: " + inputPath, Toast.LENGTH_SHORT).show();
            return;
        }

        drawImageScaled4(inputPath, true);
        mTvElapse.setText("elapse(ms): running...");
        mTvStatus.setText("processing...");
        Log.i(TAG, "runProcess in=" + inputPath + ", out=" + outPath);
        mBtnInit.setEnabled(false);
        mBtnProcess.setEnabled(false);

        mProcessExecutor.execute(() -> {
            final int ret = mInferenceEntryJni.processSR(inputPath, outPath);
            runOnUiThread(() -> {
                mBtnInit.setEnabled(true);
                mBtnProcess.setEnabled(true);
                if (ret < 0) {
                    mTvElapse.setText("elapse(ms): --");
                    mTvStatus.setText("process failed: " + ret);
                    Toast.makeText(SRActivity.this, "process failed: " + ret, Toast.LENGTH_SHORT).show();
                    return;
                }
                drawImageScaled4(outPath, false);
                mTvElapse.setText("elapse(ms): " + ret);
                //mTvStatus.setText("done, elapse=" + ret + "ms, out=" + outPath);
                mTvStatus.setText("done, elapse=" + ret + "ms");
                Log.i(TAG, "process done ret(ms)=" + ret + ", out=" + outPath);
            });
        });
    }

    private void runInit() {
        final String modelPath = new File(mResourceRoot, "model").getAbsolutePath();
        final int srType = getSelectedSRType();
        final String modeType = getSelectedModelType();
        final boolean isCpu = mRgDevice.getCheckedRadioButtonId() == R.id.rbCpu;
        final int threadNum = getSelectedThreadNum();
        final int scaleNum = getSelectedScale();
        final String configKey = buildConfigKey();

        mTvElapse.setText("elapse(ms): --");
        mTvStatus.setText("init running... type=" + (srType == 1 ? "REALCUGAN" : "REALESRGAN") + ", device=" + (isCpu ? "cpu" : "gpu")
                + ", thread=" + threadNum + ", scale=" + scaleNum + ", model=" + modeType);
        Log.i(TAG, "runInit modelPath=" + modelPath + ", srType=" + srType + ", modeType=" + modeType + ", device="
                + (isCpu ? "cpu" : "gpu") + ", thread=" + threadNum + ", scale=" + scaleNum);

        mBtnInit.setEnabled(false);
        mBtnProcess.setEnabled(false);
        mProcessExecutor.execute(() -> {
            final int ret = mInferenceEntryJni.initSR(modelPath, srType, modeType, isCpu, threadNum, scaleNum);
            runOnUiThread(() -> {
                mBtnInit.setEnabled(true);
                mBtnProcess.setEnabled(true);
                if (ret < 0) {
                    mSrInited = false;
                    mTvStatus.setText("init failed: " + ret);
                    Toast.makeText(SRActivity.this, "init failed: " + ret, Toast.LENGTH_SHORT).show();
                    return;
                }
                mSrInited = true;
                mInitConfigKey = configKey;
                mTvStatus.setText("init done");
                Toast.makeText(SRActivity.this, "init done", Toast.LENGTH_SHORT).show();
            });
        });
    }

    private String buildConfigKey() {
        return getSelectedSRType() + "|" + getSelectedModelType() + "|" + (mRgDevice.getCheckedRadioButtonId() == R.id.rbCpu ? "cpu" : "gpu")
                + "|" + getSelectedThreadNum() + "|" + getSelectedScale();
    }

    private void markInitDirty() {
        mSrInited = false;
        mTvStatus.setText("config changed, please init again");
    }

    private String getSelectedImagePath() {
        String name = "180x200.jpg";
        int checkedId = mRgImage.getCheckedRadioButtonId();
        if (checkedId == R.id.rbRatio128x128) {
            name = "128x128.jpg";
        } else if (checkedId == R.id.rbRatio360x640) {
            name = "360x640.jpg";
        }
        return new File(mResourceRoot, "image" + File.separator + name).getAbsolutePath();
    }

    private String getSelectedModelType() {
        int checkedId = mRgModel.getCheckedRadioButtonId();
        if (checkedId == R.id.rbModelAnimex2) {
            return "realesr-animevideov3-x2";
        }
        if (checkedId == R.id.rbModelX2plus) {
            return "realesrgan-x2plus";
        }
        return "realesr-general-x4v3";
    }

    private int getSelectedSRType() {
        if (mRgSRType.getCheckedRadioButtonId() == R.id.rbRealESRGAN) {
            return 0;
        }
        return 1;
    }

    private int getSelectedThreadNum() {
        int checkedId = mRgThread.getCheckedRadioButtonId();
        if (checkedId != -1) {
            RadioButton rb = findViewById(checkedId);
            if (rb != null) {
                try {
                    int v = Integer.parseInt(rb.getText().toString().trim());
                    if (v == 1 || v == 2 || v == 3 || v == 4 || v == 6) {
                        return v;
                    }
                } catch (Exception ignore) {
                }
            }
        }
        return 4;
    }

    private int getSelectedScale() {
        int checkedId = mRgScale.getCheckedRadioButtonId();
        if (checkedId != -1) {
            RadioButton rb = findViewById(checkedId);
            if (rb != null) {
                try {
                    int v = Integer.parseInt(rb.getText().toString().trim());
                    if (v == 1 || v == 2 || v == 4) {
                        return v;
                    }
                } catch (Exception ignore) {
                }
            }
        }
        return 2;
    }

    private void drawImageScaled4(String imagePath, boolean useAsOriginalSize) {
        Bitmap src = BitmapFactory.decodeFile(imagePath);
        if (src == null) {
            mTvStatus.setText("image decode failed: " + imagePath);
            Log.w(TAG, "decode failed: " + imagePath);
            return;
        }

        if (useAsOriginalSize || mOriginalWidth <= 0 || mOriginalHeight <= 0) {
            mOriginalWidth = src.getWidth();
            mOriginalHeight = src.getHeight();
        }

        int targetW = Math.max(1, mOriginalWidth * 4);
        int targetH = Math.max(1, mOriginalHeight * 4);
        Bitmap scaled = Bitmap.createScaledBitmap(src, targetW, targetH, true);
        if (scaled != src) {
            src.recycle();
        }

        releaseDisplayedBitmap();
        mDisplayedBitmap = scaled;
        mSrView.setImageBitmap(mDisplayedBitmap);
        mTvStatus.setText("showing: " + new File(imagePath).getName());
    }

    private void releaseDisplayedBitmap() {
        if (mDisplayedBitmap != null && !mDisplayedBitmap.isRecycled()) {
            mDisplayedBitmap.recycle();
        }
        mDisplayedBitmap = null;
        if (mSrView != null) {
            mSrView.setImageBitmap(null);
        }
    }

    private void ensureImageResource(String fileName) {
        File target = new File(mResourceRoot, "image" + File.separator + fileName);
        if (target.exists()) {
            return;
        }
        File parent = target.getParentFile();
        if (parent != null && !parent.exists()) {
            parent.mkdirs();
        }
        try (InputStream is = getAssets().open("image/" + fileName);
             FileOutputStream fos = new FileOutputStream(target)) {
            byte[] buffer = new byte[8 * 1024];
            int len;
            while ((len = is.read(buffer)) > 0) {
                fos.write(buffer, 0, len);
            }
        } catch (Exception e) {
            Log.w(TAG, "ensureImageResource failed for " + fileName, e);
        }
    }
}
