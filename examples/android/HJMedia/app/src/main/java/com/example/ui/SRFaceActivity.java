package com.example.ui;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.PixelFormat;
import android.opengl.GLES20;
import android.opengl.Matrix;
import android.os.Build;
import android.os.Bundle;
import android.view.View;
import android.widget.AdapterView;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.SeekBar;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;

import com.HJMediasdk.entry.HJCommonInterface;
import com.HJMediasdk.entry.inference.HJInferenceContextJni;
import com.HJMediasdk.entry.inference.HJInferenceEntryFaceSRJni;
import com.HJMediasdk.entry.inference.HJInferenceFaceDetectOption;
import com.HJMediasdk.entry.inference.HJInferenceFaceSRProcessOption;
import com.HJMediasdk.entry.inference.HJInferenceSROption;
import com.HJMediasdk.entry.inference.HJInferenceWrapperType;
import com.HJMediasdk.graphic.HJRenderEnv;
import com.HJMediasdk.graphic.draw.HJCopyShader;
import com.HJMediasdk.graphic.draw.HJFBOCtrl;
import com.HJMediasdk.graphic.draw.HJShaderCommon;
import com.HJMediasdk.utils.HJLog;
import com.HJMediasdk.utils.HJBitmap;
import com.example.utils.HJImgSeqAcquire;

import java.io.File;
import java.io.FileOutputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.concurrent.atomic.AtomicBoolean;

public class SRFaceActivity extends AppCompatActivity {

    private static final String TAG = "SRFaceActivity";
    private static final boolean s_bUseRandomSRDisable = false;
    private static final int FPS = 15;
    private static final int RET_WOULD_BLOCK = 3;
    private static final float FSR_DEFAULT_DENOISE_STRENGTH = 1.0f;
    private static final float FSR_DEFAULT_SHARPNESS = 1.0f;
    private static final float FSR_DEFAULT_EXTRA_SHARPEN_BOOST = 0.55f;
    private static final String MODEL_REALESRGAN = "realesrgan";
    private static final String MODEL_FSR = "fsr";
    private static final String MODEL_REALCUGAN = "realcugan";
    private static final String MODEL_PLAINUSR = "plainusr";
    private static final String[] ESRGAN_VARIANTS = new String[] {"realesr-general-x4v3", "realesr-animevideov3-x2"};
    private static final String[] CUGAN_VARIANTS = new String[] {"conservative", "no-denoise"};
    private static final Integer[] THREAD_OPTIONS = new Integer[] {1, 2, 4, 6};
    private static final String INPUT_SOURCE_IMAGESEQ = "imageseq";
    private static final String INPUT_SOURCE_IMAGE = "image";
    private static final String[] INPUT_SOURCE_OPTIONS = new String[] {INPUT_SOURCE_IMAGESEQ, INPUT_SOURCE_IMAGE};
    private static final String[] SR_MODE_OPTIONS = new String[] {"Full", "Face", "FaceScale", "FullScale"};
    private static final String[] PROC_POLICY_OPTIONS = new String[] {
            "Mipmap", "Bilinear", "Bicubic", "Lanczos3", "Lanczos4"
    };
    private static final String[] FACE_SCALE_PRESETS = new String[] {
            "80x100", "90x114", "100x126", "180x228", "220x278", "300x380", "360x456"
    };
    private static final int[][] FACE_SCALE_PRESET_SIZES = new int[][] {
            {80, 100}, {90, 114}, {100, 126}, {180, 228}, {220, 278}, {300, 380}, {360, 456}
    };
    private static final String[] FULL_SCALE_PRESETS = new String[] {
            "90x160", "180x320", "224x398", "300x534", "360x640"
    };
    private static final int[][] FULL_SCALE_PRESET_SIZES = new int[][] {
            {90, 160}, {180, 320}, {224, 398}, {300, 534}, {360, 640}
    };

    private SurfaceView mSurfaceView = null;
    private TextView mInfoText = null;
    private TextView mMixText = null;
    private TextView mMatchText = null;
    private TextView mFaceScaleText = null;
    private TextView mDisplayScaleWText = null;
    private TextView mDisplayScaleHText = null;
    private TextView mFsrDenoiseText = null;
    private TextView mFsrSharpnessText = null;
    private TextView mFsrBoostText = null;
    private TextView mSRStatusText = null;
    private TextView mSettingText = null;
    private Button mPauseButton = null;
    private Surface mSurface = null;
    private int mScreenWidth = 0;
    private int mScreenHeight = 0;

    private HJRenderEnv mRender = null;
    private HJCopyShader mComposeShader = null;
    private HJCopyShader mDisplayShader = null;
    private HJFBOCtrl mComposeFbo = null;
    private final int[] mOriginTextureIds = new int[1];
    private int mOriginTextureId = -1;
    private final int[] mSRTextureIds = new int[1];
    private int mSRTextureId = -1;
    private boolean mSRTextureOwnedByNative = false;
    private int mLastSRBitmapWidth = 0;
    private int mLastSRBitmapHeight = 0;
    private long mLastInfoUpdateMs = 0;
    private volatile int mRenderCostMs = 0;
    private int mDisplaySrcWidth = 0;
    private int mDisplaySrcHeight = 0;
    private int mDisplayTargetWidth = 0;
    private int mDisplayTargetHeight = 0;
    private final float[] mIdentityVertexMat = new float[16];
    private final float[] mScreenVertexMat = new float[16];

    private HJImgSeqAcquire mImgSeqAcquire = null;
    private Bitmap mSingleImageBitmap = null;
    private boolean mStarted = false;
    private final Object mPauseFrameLock = new Object();
    private volatile boolean mPaused = false;
    private HJBitmap mCachedPauseHb = null;
    private final AtomicBoolean mbSREnabled = new AtomicBoolean(true);
    private volatile int mSRMode = HJInferenceFaceSRProcessOption.HJFaceSRProcessMode_FaceScale;
    private volatile int mFaceScalePresetIndex = 1;
    private volatile int mFullScalePresetIndex = 1;
    private volatile int mPreScaleWidth = 90;
    private volatile int mPreScaleHeight = 114;
    private float mFaceSRMixRatio = 0.8f;
    private float mFaceSRMatchRatio = 1.0f;
    private volatile float mDisplayScaleW = 1.0f;
    private volatile float mDisplayScaleH = 1.0f;
    private HJInferenceEntryFaceSRJni mInferenceEntryFaceSRJni = null;
    private final SRSetting mSRSetting = new SRSetting();
    private final AtomicBoolean mSaveOnceTrigger = new AtomicBoolean(false);
    private int m_renderIdx = 0;
    private Spinner mSRModeSpinner;
    private Spinner mProcPolicySpinner;
    private Spinner mPreScaleSpinner;
    private SeekBar mMixSeekBar;
    private SeekBar mMatchSeekBar;
    private SeekBar mFsrDenoiseSeekBar;
    private SeekBar mFsrSharpnessSeekBar;
    private SeekBar mFsrBoostSeekBar;
    private CheckBox mFsrExtraSharpenCheckBox;
    private CheckBox mPostResizeCompareCheckBox;
    private volatile boolean mEnableNativePostSRDisplayResize = true;

    private interface SpinnerSelectionHandler {
        void onSelected(int position);
    }

    private static class SRSetting {
        boolean useGPU = false;
        String model = MODEL_FSR;
        String variant = "realesr-general-x4v3";
        float denoise = 0.5f;
        float fsrDenoiseStrength = FSR_DEFAULT_DENOISE_STRENGTH;
        float fsrSharpness = FSR_DEFAULT_SHARPNESS;
        boolean fsrEnableExtraSharpen = true;
        float fsrExtraSharpenBoost = FSR_DEFAULT_EXTRA_SHARPEN_BOOST;
        int cpuThreadNums = 4;
        int gpuThreadNums = 1;
        String inputSource = INPUT_SOURCE_IMAGESEQ;
        String inputFolder = "360x640";
        String imageFile = "";
    }

    private static int clampPercent(float value) {
        return Math.max(0, Math.min(100, Math.round(value * 100.0f)));
    }

    private static int findThreadOptionPos(int target, int fallback) {
        int fallbackPos = 0;
        for (int i = 0; i < THREAD_OPTIONS.length; i++) {
            if (THREAD_OPTIONS[i] == fallback) {
                fallbackPos = i;
            }
            if (THREAD_OPTIONS[i] == target) {
                return i;
            }
        }
        return fallbackPos;
    }

    private static int getProcPolicyByIndex(int procPolicyIndex) {
        switch (procPolicyIndex) {
            case 1:
                return 1;
            case 2:
                return 2;
            case 3:
                return 3;
            case 4:
                return 4;
            case 0:
                return 0;
            default:
                return 0;
        }
    }

    private static boolean isFaceMode(int srMode) {
        return srMode == HJInferenceFaceSRProcessOption.HJFaceSRProcessMode_FaceOrigin
                || srMode == HJInferenceFaceSRProcessOption.HJFaceSRProcessMode_FaceScale;
    }

    private static boolean usesPreScaleMode(int srMode) {
        return srMode == HJInferenceFaceSRProcessOption.HJFaceSRProcessMode_FaceScale
                || srMode == HJInferenceFaceSRProcessOption.HJFaceSRProcessMode_FullScale;
    }

    private boolean priIsTextureFSRModel(String model) {
        return MODEL_FSR.equals(model);
    }

    private boolean priIsTextureFSRSelected() {
        return priIsTextureFSRModel(mSRSetting.model);
    }

    private void priApplyTextureFSRModeConstraint() {
        if (!priIsTextureFSRSelected()) {
            return;
        }
        mSRMode = HJInferenceFaceSRProcessOption.HJFaceSRProcessMode_Full;
        if (mSRModeSpinner != null && mSRModeSpinner.getSelectedItemPosition() != HJInferenceFaceSRProcessOption.HJFaceSRProcessMode_Full) {
            mSRModeSpinner.setSelection(HJInferenceFaceSRProcessOption.HJFaceSRProcessMode_Full, false);
        }
    }

    private void priUpdateSRControlUI() {
        boolean isTextureFSR = priIsTextureFSRSelected();
        boolean enablePreScale = !isTextureFSR && usesPreScaleMode(mSRMode);
        float disabledAlpha = 0.6f;
        View modeControl = findViewById(R.id.llSRModeControl);
        View procPolicyControl = findViewById(R.id.llSRProcPolicyControl);
        View scaleRatioPanel = findViewById(R.id.llSRFaceScaleRatio);
        View mixRatioPanel = findViewById(R.id.llSRFaceMixRatio);
        View matchRatioPanel = findViewById(R.id.llSRFaceMatchRatio);
        View textureFsrPanel = findViewById(R.id.llTextureFSRControls);
        View postResizeComparePanel = findViewById(R.id.cbSRPostResizeCompare);

        if (mSRModeSpinner != null) {
            mSRModeSpinner.setEnabled(!isTextureFSR);
        }
        if (modeControl != null) {
            modeControl.setAlpha(isTextureFSR ? disabledAlpha : 1.0f);
        }

        if (scaleRatioPanel != null) {
            scaleRatioPanel.setVisibility(enablePreScale ? View.VISIBLE : View.GONE);
        }
        if (mProcPolicySpinner != null) {
            mProcPolicySpinner.setEnabled(enablePreScale);
        }
        if (procPolicyControl != null) {
            procPolicyControl.setAlpha(enablePreScale ? 1.0f : disabledAlpha);
        }
        if (mPreScaleSpinner != null) {
            mPreScaleSpinner.setEnabled(enablePreScale);
        }

        if (mMixSeekBar != null) {
            mMixSeekBar.setEnabled(!isTextureFSR);
        }
        if (mixRatioPanel != null) {
            mixRatioPanel.setAlpha(isTextureFSR ? disabledAlpha : 1.0f);
        }
        if (mMatchSeekBar != null) {
            mMatchSeekBar.setEnabled(true);
        }
        if (matchRatioPanel != null) {
            matchRatioPanel.setAlpha(1.0f);
        }
        if (textureFsrPanel != null) {
            textureFsrPanel.setVisibility(isTextureFSR ? View.VISIBLE : View.GONE);
        }
        if (postResizeComparePanel != null) {
            postResizeComparePanel.setVisibility(isTextureFSR ? View.GONE : View.VISIBLE);
            postResizeComparePanel.setAlpha(isTextureFSR ? disabledAlpha : 1.0f);
            postResizeComparePanel.setEnabled(!isTextureFSR);
        }
        if (mPostResizeCompareCheckBox != null) {
            mPostResizeCompareCheckBox.setEnabled(!isTextureFSR);
        }
        if (mFsrBoostSeekBar != null) {
            mFsrBoostSeekBar.setEnabled(isTextureFSR && mSRSetting.fsrEnableExtraSharpen);
            mFsrBoostSeekBar.setAlpha((isTextureFSR && mSRSetting.fsrEnableExtraSharpen) ? 1.0f : disabledAlpha);
        }
    }

    private void priAdoptNativeSRTexture(int textureId) {
        if (textureId <= 0) {
            return;
        }
        if (!mSRTextureOwnedByNative && mSRTextureId > 0 && mSRTextureId != textureId) {
            HJShaderCommon.textureDestory(mSRTextureIds);
        }
        mSRTextureId = textureId;
        mSRTextureOwnedByNative = true;
        mLastSRBitmapWidth = 0;
        mLastSRBitmapHeight = 0;
    }

    private void priPrepareJavaOwnedSRTexture() {
        if (!mSRTextureOwnedByNative) {
            return;
        }
        mSRTextureId = -1;
        mSRTextureOwnedByNative = false;
    }

    private void priReleaseSRTexture() {
        if (mSRTextureId > 0 && !mSRTextureOwnedByNative) {
            HJShaderCommon.textureDestory(mSRTextureIds);
        }
        mSRTextureId = -1;
        mSRTextureOwnedByNative = false;
        mLastSRBitmapWidth = 0;
        mLastSRBitmapHeight = 0;
    }

    private void priDrawFullSRToComposeFbo() {
        if (mComposeShader == null || mComposeFbo == null || mSRTextureId <= 0) {
            return;
        }
        priDrawMatchedSRToComposeFbo(0, 0, mComposeFbo.width(), mComposeFbo.height());
    }

    private void priDrawMatchedSRToComposeFbo(int x, int y, int width, int height) {
        if (mComposeShader == null || mSRTextureId <= 0 || width <= 0 || height <= 0) {
            return;
        }

        final int drawX = Math.max(0, x);
        final int drawY = Math.max(0, y);
        final int drawWidth = width;
        final int drawHeight = height;
        final float matchRatio = Math.max(0.0f, Math.min(1.0f, mFaceSRMatchRatio));

        HJShaderCommon.viewport(drawX, drawY, drawWidth, drawHeight);
        if (matchRatio >= 0.999f) {
            mComposeShader.draw(mSRTextureId);
            return;
        }
        if (matchRatio <= 0.001f) {
            return;
        }

        final int scissorX = drawX + Math.round(drawWidth * (1.0f - matchRatio));
        final int scissorWidth = Math.max(0, drawX + drawWidth - scissorX);
        if (scissorWidth <= 0) {
            return;
        }

        GLES20.glEnable(GLES20.GL_SCISSOR_TEST);
        GLES20.glScissor(scissorX, drawY, scissorWidth, drawHeight);
        mComposeShader.draw(mSRTextureId);

        final int lineWidth = Math.max(1, Math.min(2, drawWidth));
        final int lineX = Math.max(drawX, Math.min(scissorX, drawX + drawWidth - lineWidth));
        GLES20.glScissor(lineX, drawY, lineWidth, drawHeight);
        GLES20.glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
        GLES20.glDisable(GLES20.GL_SCISSOR_TEST);
    }

    private String[] getPreScalePresetLabels() {
        if (mSRMode == HJInferenceFaceSRProcessOption.HJFaceSRProcessMode_FaceScale) {
            return FACE_SCALE_PRESETS;
        }
        if (mSRMode == HJInferenceFaceSRProcessOption.HJFaceSRProcessMode_FullScale) {
            return FULL_SCALE_PRESETS;
        }
        return new String[0];
    }

    private int[] resolveCurrentPreScale() {
        if (mSRMode == HJInferenceFaceSRProcessOption.HJFaceSRProcessMode_FaceScale) {
            int index = Math.max(0, Math.min(mFaceScalePresetIndex, FACE_SCALE_PRESET_SIZES.length - 1));
            return FACE_SCALE_PRESET_SIZES[index];
        }
        if (mSRMode == HJInferenceFaceSRProcessOption.HJFaceSRProcessMode_FullScale) {
            int index = Math.max(0, Math.min(mFullScalePresetIndex, FULL_SCALE_PRESET_SIZES.length - 1));
            return FULL_SCALE_PRESET_SIZES[index];
        }
        return new int[] {0, 0};
    }

    private void updateResolvedPreScale() {
        int[] size = resolveCurrentPreScale();
        mPreScaleWidth = size[0];
        mPreScaleHeight = size[1];
    }

    private void setupSpinner(Spinner spinner, String[] options, int selection, SpinnerSelectionHandler handler) {
        if (spinner == null) {
            return;
        }
        ArrayAdapter<String> adapter = new ArrayAdapter<>(this, R.layout.spinner_item_green, options);
        adapter.setDropDownViewResource(R.layout.spinner_dropdown_item_green);
        spinner.setAdapter(adapter);
        spinner.setSelection(Math.max(0, Math.min(selection, options.length - 1)), false);
        spinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                if (handler != null) {
                    handler.onSelected(position);
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
            }
        });
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_sr_face);

        HJLog.setEnable(true);

        final String logDir = getFilesDir().getAbsolutePath() + File.separator + "inference";
        int initErr = HJInferenceContextJni.contextInit(
                logDir,
                HJCommonInterface.HJLOGLevel_INFO,
                HJCommonInterface.HJLogLMode_CONSOLE | HJCommonInterface.HJLLogMode_FILE,
                5 * 1024 * 1024,
                5);
        HJLog.i(TAG, "contextInit err=" + initErr + ", logDir=" + logDir);

        mInferenceEntryFaceSRJni = new HJInferenceEntryFaceSRJni();

        mImgSeqAcquire = new HJImgSeqAcquire();
        mSurfaceView = findViewById(R.id.srFaceSurfaceView);
        mInfoText = findViewById(R.id.tvSRFaceInfo);
        mMixText = findViewById(R.id.tvSRFaceMix);
        mMatchText = findViewById(R.id.tvSRFaceMatch);
        mFaceScaleText = findViewById(R.id.tvSRFaceScaleValue);
        mDisplayScaleWText = findViewById(R.id.tvSRDisplayScaleW);
        mDisplayScaleHText = findViewById(R.id.tvSRDisplayScaleH);
        mFsrDenoiseText = findViewById(R.id.tvTextureFSRDenoise);
        mFsrSharpnessText = findViewById(R.id.tvTextureFSRSharpness);
        mFsrBoostText = findViewById(R.id.tvTextureFSRBoost);
        mSRStatusText = findViewById(R.id.tvSRStatus);
        mSettingText = findViewById(R.id.tvSRFaceSettingInfo);
        mSRModeSpinner = findViewById(R.id.spSRMode);
        mProcPolicySpinner = findViewById(R.id.spSRProcPolicy);
        mPreScaleSpinner = findViewById(R.id.spSRFaceScalePreset);
        mFsrDenoiseSeekBar = findViewById(R.id.sbTextureFSRDenoise);
        mFsrSharpnessSeekBar = findViewById(R.id.sbTextureFSRSharpness);
        mFsrBoostSeekBar = findViewById(R.id.sbTextureFSRBoost);
        mFsrExtraSharpenCheckBox = findViewById(R.id.cbTextureFSRExtraSharpen);
        mPostResizeCompareCheckBox = findViewById(R.id.cbSRPostResizeCompare);
        
        mSurfaceView.setZOrderOnTop(false);
        mSurfaceView.getHolder().setFormat(PixelFormat.RGBA_8888);
        mSurfaceView.getHolder().addCallback(mSurfaceHolderCallback);

        Button settingButton = findViewById(R.id.btnSRFaceSetting);
        settingButton.setOnClickListener(v -> showSRSettingDialog());
        mPauseButton = findViewById(R.id.btnSRFacePause);
        updatePauseButtonText();
        mPauseButton.setOnClickListener(v -> {
            if (mPaused) {
                releaseCachedPauseFrame();
                mPaused = false;
            } else {
                if (cachePauseFrame()) {
                    mPaused = true;
                } else {
                    HJLog.w(TAG, "pause ignored, no frame available to cache");
                }
            }
            updatePauseButtonText();
        });

        setupSpinner(mSRModeSpinner, SR_MODE_OPTIONS, 2, position -> {
            mSRMode = position;
            refreshPreScaleSpinner();
            updateResolvedPreScale();
            updateFaceScaleUI();
            updateFaceScaleText();
        });
        setupSpinner(mProcPolicySpinner, PROC_POLICY_OPTIONS, 3, null);
        refreshPreScaleSpinner();
        updateResolvedPreScale();
        updateFaceScaleUI();
        updateFaceScaleText();

        CheckBox useSRCheckBox = findViewById(R.id.cbUseSR);
        useSRCheckBox.setChecked(true);
        priUpdateSRStatusText(true);
        useSRCheckBox.setOnCheckedChangeListener((buttonView, isChecked) -> {
            mbSREnabled.set(isChecked);
            priUpdateSRStatusText(isChecked);
            if (!isChecked) {
                mLastInfoUpdateMs = 0;
                maybeUpdateInfoTextDisabled();
            }
        });
        if (mPostResizeCompareCheckBox != null) {
            mPostResizeCompareCheckBox.setChecked(true);
            mPostResizeCompareCheckBox.setOnCheckedChangeListener((buttonView, isChecked) -> {
                mEnableNativePostSRDisplayResize = isChecked;
                updateSettingText();
            });
        }

        mMixSeekBar = findViewById(R.id.sbSRFaceMix);
        mMixSeekBar.setMax(100);
        mMixSeekBar.setProgress(80);
        mMixSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                mFaceSRMixRatio = Math.max(0f, Math.min(1f, progress / 100.f));
                updateMixText();
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
            }
        });
        updateMixText();

        mMatchSeekBar = findViewById(R.id.sbSRFaceMatch);
        mMatchSeekBar.setMax(100);
        mMatchSeekBar.setProgress(100);
        mMatchSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                mFaceSRMatchRatio = Math.max(0f, Math.min(1f, progress / 100.f));
                updateMatchText();
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
            }
        });
        updateMatchText();
        priApplyTextureFSRModeConstraint();
        priUpdateSRControlUI();

        if (mFsrDenoiseSeekBar != null) {
            mFsrDenoiseSeekBar.setMax(100);
            mFsrDenoiseSeekBar.setProgress(clampPercent(mSRSetting.fsrDenoiseStrength));
            mFsrDenoiseSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
                @Override
                public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                    mSRSetting.fsrDenoiseStrength = Math.max(0f, Math.min(1f, progress / 100.f));
                    updateFsrText();
                    updateSettingText();
                }

                @Override
                public void onStartTrackingTouch(SeekBar seekBar) {
                }

                @Override
                public void onStopTrackingTouch(SeekBar seekBar) {
                    applyRealtimeTextureFsrSetting();
                }
            });
        }
        if (mFsrSharpnessSeekBar != null) {
            mFsrSharpnessSeekBar.setMax(100);
            mFsrSharpnessSeekBar.setProgress(clampPercent(mSRSetting.fsrSharpness));
            mFsrSharpnessSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
                @Override
                public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                    mSRSetting.fsrSharpness = Math.max(0f, Math.min(1f, progress / 100.f));
                    updateFsrText();
                    updateSettingText();
                }

                @Override
                public void onStartTrackingTouch(SeekBar seekBar) {
                }

                @Override
                public void onStopTrackingTouch(SeekBar seekBar) {
                    applyRealtimeTextureFsrSetting();
                }
            });
        }
        if (mFsrBoostSeekBar != null) {
            mFsrBoostSeekBar.setMax(100);
            mFsrBoostSeekBar.setProgress(clampPercent(mSRSetting.fsrExtraSharpenBoost));
            mFsrBoostSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
                @Override
                public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                    mSRSetting.fsrExtraSharpenBoost = Math.max(0f, Math.min(1f, progress / 100.f));
                    updateFsrText();
                    updateSettingText();
                }

                @Override
                public void onStartTrackingTouch(SeekBar seekBar) {
                }

                @Override
                public void onStopTrackingTouch(SeekBar seekBar) {
                    applyRealtimeTextureFsrSetting();
                }
            });
        }
        if (mFsrExtraSharpenCheckBox != null) {
            mFsrExtraSharpenCheckBox.setChecked(mSRSetting.fsrEnableExtraSharpen);
            mFsrExtraSharpenCheckBox.setOnCheckedChangeListener((buttonView, isChecked) -> {
                mSRSetting.fsrEnableExtraSharpen = isChecked;
                updateFsrText();
                updateSettingText();
                applyRealtimeTextureFsrSetting();
            });
        }
        updateFsrText();

        SeekBar displayScaleWSeekBar = findViewById(R.id.sbSRDisplayScaleW);
        displayScaleWSeekBar.setMax(100);
        displayScaleWSeekBar.setProgress(100);
        displayScaleWSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                float value = progress / 100.f;
                mDisplayScaleW = Math.max(0.0f, Math.min(1.0f, value));
                updateDisplayScaleText();
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
            }
        });

        SeekBar displayScaleHSeekBar = findViewById(R.id.sbSRDisplayScaleH);
        displayScaleHSeekBar.setMax(100);
        displayScaleHSeekBar.setProgress(100);
        displayScaleHSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                float value = progress / 100.f;
                mDisplayScaleH = Math.max(0.0f, Math.min(1.0f, value));
                updateDisplayScaleText();
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
            }
        });
        updateDisplayScaleText();
        updateSettingText();
    }

    @Override
    protected void onDestroy() {
        stopProcess();
        if (mInferenceEntryFaceSRJni != null) {
            mInferenceEntryFaceSRJni.done();
            mInferenceEntryFaceSRJni = null;
        }
        HJInferenceContextJni.contextUnInit();
        super.onDestroy();
    }

    private final SurfaceHolder.Callback mSurfaceHolderCallback = new SurfaceHolder.Callback() {
        @Override
        public void surfaceCreated(SurfaceHolder holder) {
            mSurface = holder.getSurface();
            HJLog.i(TAG, "surfaceCreated");
        }

        @Override
        public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
            mScreenWidth = width;
            mScreenHeight = height;
            mDisplaySrcWidth = 0;
            mDisplaySrcHeight = 0;
            mDisplayTargetWidth = 0;
            mDisplayTargetHeight = 0;
            mSurface = holder.getSurface();
            HJLog.i(TAG, "surfaceChanged w=" + width + " h=" + height);

            if (mRender != null) {
                mRender.setSurface(mSurface);
            } else if (mStarted) {
                startRenderIfPossible();
            }
        }

        @Override
        public void surfaceDestroyed(SurfaceHolder holder) {
            mSurface = null;
            HJLog.i(TAG, "surfaceDestroyed");
            if (mRender != null) {
                mRender.setSurface(null);
            }
        }
    };

    private void startProcess() {
        int ret = startInputBySetting();
        if (ret < 0) {
            Toast.makeText(this, "input start failed: " + ret, Toast.LENGTH_SHORT).show();
            return;
        }
        mStarted = true;
        startRenderIfPossible();
    }

    private int startInputBySetting() {
        releaseCachedPauseFrame();
        mPaused = false;
        updatePauseButtonText();
        if (INPUT_SOURCE_IMAGE.equals(mSRSetting.inputSource)) {
            if (mImgSeqAcquire != null) {
                mImgSeqAcquire.stop();
            }
            if (mSingleImageBitmap != null && !mSingleImageBitmap.isRecycled()) {
                mSingleImageBitmap.recycle();
            }
            mSingleImageBitmap = null;

            if (mSRSetting.imageFile == null || mSRSetting.imageFile.isEmpty()) {
                return -21;
            }

            final String imagePath = getFilesDir().getAbsolutePath() + File.separator + "HJResource/image/" + mSRSetting.imageFile;
            Bitmap bmp = BitmapFactory.decodeFile(imagePath);
            if (bmp == null) {
                return -22;
            }
            mSingleImageBitmap = bmp;
            return 0;
        }

        if (mSingleImageBitmap != null && !mSingleImageBitmap.isRecycled()) {
            mSingleImageBitmap.recycle();
        }
        mSingleImageBitmap = null;
        final String imgDir = getFilesDir().getAbsolutePath() + File.separator + "HJResource/imgseq/" + mSRSetting.inputFolder;
        if (mImgSeqAcquire != null) {
            mImgSeqAcquire.stop();
        }
        return mImgSeqAcquire.start(imgDir, 1000 / FPS, true);
    }

    private void reInitFaceSROnRenderThread() {
        if (mInferenceEntryFaceSRJni == null) {
            Toast.makeText(this, "inference entry not ready", Toast.LENGTH_SHORT).show();
            return;
        }

        final SRSetting snapshot = new SRSetting();
        snapshot.useGPU = mSRSetting.useGPU;
        snapshot.model = mSRSetting.model;
        snapshot.variant = mSRSetting.variant;
        snapshot.denoise = mSRSetting.denoise;
        snapshot.fsrDenoiseStrength = mSRSetting.fsrDenoiseStrength;
        snapshot.fsrSharpness = mSRSetting.fsrSharpness;
        snapshot.fsrEnableExtraSharpen = mSRSetting.fsrEnableExtraSharpen;
        snapshot.fsrExtraSharpenBoost = mSRSetting.fsrExtraSharpenBoost;
        snapshot.cpuThreadNums = mSRSetting.cpuThreadNums;
        snapshot.gpuThreadNums = mSRSetting.gpuThreadNums;

        Runnable task = () -> {
            int ret = initFaceSRInternal(snapshot);
            if (ret < 0)
            {
                runOnUiThread(() -> Toast.makeText(SRFaceActivity.this, ret < 0 ? "initFaceSR failed: " + ret : "initFaceSR success", Toast.LENGTH_SHORT).show());
            }
        };

        if (mRender != null) {
            mRender.AsyncQueueEvent(0, task);
        } else {
            new Thread(task, "SRFaceReInit").start();
        }
    }

    private int initFaceSRInternal(SRSetting setting) {
        if (mInferenceEntryFaceSRJni == null) {
            return -1;
        }

        final String modelPath = new File(getFilesDir(), "HJResource/model").getAbsolutePath();

        HJInferenceFaceDetectOption faceOption = new HJInferenceFaceDetectOption();
        faceOption.ncnnRetinaFaceThreadNums = 1;
        faceOption.retinaFaceTargetSize = 200;
        faceOption.ncnnRetinaFaceUseGPU = false;
        faceOption.ncnnScrfdUseGPU = false;
        faceOption.ncnnScrfdThreadNums = 1;

        HJInferenceSROption srOption = new HJInferenceSROption();
        srOption.ncnnUseGPU = setting.useGPU;
        srOption.ncnnThreadNums = setting.useGPU ? setting.gpuThreadNums : setting.cpuThreadNums;
        srOption.ncnnRealESRGANDenose = setting.denoise;
        srOption.textureFsrDenoiseStrength = setting.fsrDenoiseStrength;
        srOption.textureFsrSharpness = setting.fsrSharpness;
        srOption.textureFsrEnableExtraSharpen = setting.fsrEnableExtraSharpen;
        srOption.textureFsrExtraSharpenBoost = setting.fsrExtraSharpenBoost;
        srOption.textureFsrScale = 2;

        int srType;
        if (priIsTextureFSRModel(setting.model)) {
            srType = HJInferenceWrapperType.VideoSR.HJVideoSRWrapperType_TEXTUREFSR;
            srOption.ncnnScale = 2;
        } else if (MODEL_REALCUGAN.equals(setting.model)) {
            srType = HJInferenceWrapperType.VideoSR.HJVideoSRWrapperType_NCNNREALCUGAN;
            srOption.ncnnRealCUGANType = setting.variant;
            srOption.ncnnScale = 2;
        } else if (MODEL_PLAINUSR.equals(setting.model)) {
            srType = HJInferenceWrapperType.VideoSR.HJVideoSRWrapperType_NCNNPLAINUSR;
            srOption.ncnnScale = 2;
        } else {
            srType = HJInferenceWrapperType.VideoSR.HJVideoSRWrapperType_NCNNREALESRGAN;
            srOption.ncnnRealESRGANType = setting.variant;
            if (srOption.ncnnRealESRGANType.equals("realesr-general-x4v3"))
            {
                srOption.ncnnScale = 4;
            }
            else
            {
                srOption.ncnnScale = 2;
            }
        }

        mInferenceEntryFaceSRJni.done();
        int ret = mInferenceEntryFaceSRJni.initFaceSR(
                modelPath,
                HJInferenceWrapperType.FaceDetect.HJFaceDetectWrapperType_NCNNRETINAFACE,
                faceOption,
                srType,
                srOption);

        HJLog.i(TAG, "initFaceSR ret=" + ret
                + ", gpu=" + setting.useGPU
                + ", model=" + setting.model
                + ", variant=" + setting.variant
                + ", threads=" + srOption.ncnnThreadNums
                + ", denoise=" + srOption.ncnnRealESRGANDenose
                + ", fsrDenoise=" + srOption.textureFsrDenoiseStrength
                + ", fsrSharpness=" + srOption.textureFsrSharpness
                + ", fsrExtra=" + srOption.textureFsrEnableExtraSharpen
                + ", fsrBoost=" + srOption.textureFsrExtraSharpenBoost
                + (priIsTextureFSRModel(setting.model) ? " (fsr)" : " (ncnn)"));
        return ret;
    }

    private void showSRSettingDialog() {
        View view = getLayoutInflater().inflate(R.layout.dialog_sr_face_setting, null, false);
        RadioGroup rgDevice = view.findViewById(R.id.rgSRDevice);
        RadioButton rbCPU = view.findViewById(R.id.rbSRCPU);
        RadioButton rbGPU = view.findViewById(R.id.rbSRGPU);
        Spinner spModel = view.findViewById(R.id.spSRModel);
        Spinner spVariant = view.findViewById(R.id.spSRRatioVariant);
        Spinner spThreads = view.findViewById(R.id.spSRThreads);
        Spinner spInputSource = view.findViewById(R.id.spSRInputSource);
        Spinner spInputFolder = view.findViewById(R.id.spSRInputFolder);
        Spinner spImageFile = view.findViewById(R.id.spSRImageFile);
        View tvInputFolderTitle = view.findViewById(R.id.tvSRInputFolderTitle);
        View tvImageFileTitle = view.findViewById(R.id.tvSRImageFileTitle);
        View llDenoise = view.findViewById(R.id.llSRDenoise);
        Spinner spDenoise = view.findViewById(R.id.spSRDenoise);

        if (mSRSetting.useGPU) {
            rbGPU.setChecked(true);
        } else {
            rbCPU.setChecked(true);
        }

        // Setup Model Spinner
        String[] modelOptions = {MODEL_REALESRGAN, MODEL_FSR, MODEL_REALCUGAN, MODEL_PLAINUSR};
        ArrayAdapter<String> modelAdapter = new ArrayAdapter<>(this, android.R.layout.simple_spinner_dropdown_item, modelOptions);
        spModel.setAdapter(modelAdapter);
        int modelPos = 0;
        for (int i = 0; i < modelOptions.length; i++) {
            if (modelOptions[i].equals(mSRSetting.model)) {
                modelPos = i;
                break;
            }
        }
        spModel.setSelection(modelPos);

        ArrayAdapter<Integer> threadAdapter = new ArrayAdapter<>(this, android.R.layout.simple_spinner_dropdown_item, THREAD_OPTIONS);
        spThreads.setAdapter(threadAdapter);
        final int[] cpuThreadNums = new int[] {mSRSetting.cpuThreadNums};
        final int[] gpuThreadNums = new int[] {mSRSetting.gpuThreadNums};

        int threadPos = mSRSetting.useGPU
                ? findThreadOptionPos(gpuThreadNums[0], 1)
                : findThreadOptionPos(cpuThreadNums[0], 4);
        spThreads.setSelection(threadPos);

        spThreads.setOnItemSelectedListener(new android.widget.AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(android.widget.AdapterView<?> parent, View view, int position, long id) {
                Object threadObj = parent.getItemAtPosition(position);
                int selected = threadObj instanceof Integer ? (Integer) threadObj : 4;
                boolean useGPU = rgDevice.getCheckedRadioButtonId() == R.id.rbSRGPU;
                if (useGPU) {
                    gpuThreadNums[0] = selected;
                } else {
                    cpuThreadNums[0] = selected;
                }
            }

            @Override
            public void onNothingSelected(android.widget.AdapterView<?> parent) {
            }
        });

        Runnable refreshThreadByDevice = () -> {
            boolean useGPU = rgDevice.getCheckedRadioButtonId() == R.id.rbSRGPU;
            int pos = useGPU
                    ? findThreadOptionPos(gpuThreadNums[0], 1)
                    : findThreadOptionPos(cpuThreadNums[0], 4);
            spThreads.setSelection(pos);
        };
        rgDevice.setOnCheckedChangeListener((group, checkedId) -> refreshThreadByDevice.run());

        // Setup Denoise Spinner
        String[] denoiseOptions = new String[] {"0.0", "0.2", "0.5", "0.8", "1.0"};
        ArrayAdapter<String> denoiseAdapter = new ArrayAdapter<>(this, android.R.layout.simple_spinner_dropdown_item, denoiseOptions);
        spDenoise.setAdapter(denoiseAdapter);
        int denoisePos = 0;
        for (int i = 0; i < denoiseOptions.length; i++) {
            if (Float.parseFloat(denoiseOptions[i]) == mSRSetting.denoise) {
                denoisePos = i;
                break;
            }
        }
        spDenoise.setSelection(denoisePos);
        
        final float[] selectedDenoise = new float[] {mSRSetting.denoise};
        spDenoise.setOnItemSelectedListener(new android.widget.AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(android.widget.AdapterView<?> parent, View view, int position, long id) {
                Object denoiseObj = parent.getItemAtPosition(position);
                if (denoiseObj != null) {
                    selectedDenoise[0] = Float.parseFloat(denoiseObj.toString());
                    HJLog.i(TAG, "Denoise spinner changed to: " + selectedDenoise[0]);
                }
            }

            @Override
            public void onNothingSelected(android.widget.AdapterView<?> parent) {
            }
        });

        List<String> inputFolders = listInputFolders();
        if (inputFolders.isEmpty()) {
            inputFolders.add(mSRSetting.inputFolder == null || mSRSetting.inputFolder.isEmpty() ? "360x640" : mSRSetting.inputFolder);
        }
        ArrayAdapter<String> inputFolderAdapter = new ArrayAdapter<>(this, android.R.layout.simple_spinner_dropdown_item, inputFolders);
        spInputFolder.setAdapter(inputFolderAdapter);
        int folderPos = 0;
        for (int i = 0; i < inputFolders.size(); i++) {
            if (inputFolders.get(i).equals(mSRSetting.inputFolder)) {
                folderPos = i;
                break;
            }
        }
        spInputFolder.setSelection(folderPos);

        ArrayAdapter<String> inputSourceAdapter = new ArrayAdapter<>(this, android.R.layout.simple_spinner_dropdown_item, INPUT_SOURCE_OPTIONS);
        spInputSource.setAdapter(inputSourceAdapter);
        int sourcePos = INPUT_SOURCE_IMAGE.equals(mSRSetting.inputSource) ? 1 : 0;
        spInputSource.setSelection(sourcePos);

        List<String> imageFiles = listImageJpgFiles();
        if (imageFiles.isEmpty()) {
            imageFiles.add("");
        }
        ArrayAdapter<String> imageAdapter = new ArrayAdapter<>(this, android.R.layout.simple_spinner_dropdown_item, imageFiles);
        spImageFile.setAdapter(imageAdapter);
        int imagePos = 0;
        for (int i = 0; i < imageFiles.size(); i++) {
            if (imageFiles.get(i).equals(mSRSetting.imageFile)) {
                imagePos = i;
                break;
            }
        }
        spImageFile.setSelection(imagePos);

        Runnable refreshInputSource = () -> {
            Object srcObj = spInputSource.getSelectedItem();
            boolean isImage = srcObj != null && INPUT_SOURCE_IMAGE.equals(srcObj.toString());
            int imageVisibility = isImage ? View.VISIBLE : View.GONE;
            int seqVisibility = isImage ? View.GONE : View.VISIBLE;
            if (tvImageFileTitle != null) {
                tvImageFileTitle.setVisibility(imageVisibility);
            }
            if (spImageFile != null) {
                spImageFile.setVisibility(imageVisibility);
            }
            if (tvInputFolderTitle != null) {
                tvInputFolderTitle.setVisibility(seqVisibility);
            }
            if (spInputFolder != null) {
                spInputFolder.setVisibility(seqVisibility);
            }
        };
        spInputSource.setOnItemSelectedListener(new android.widget.AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(android.widget.AdapterView<?> parent, View view, int position, long id) {
                refreshInputSource.run();
            }

            @Override
            public void onNothingSelected(android.widget.AdapterView<?> parent) {
            }
        });
        refreshInputSource.run();

        Runnable refreshVariant = () -> {
            Object selectedModelObj = spModel.getSelectedItem();
            String selectedModel = selectedModelObj == null ? MODEL_FSR : selectedModelObj.toString();
            
            if (priIsTextureFSRModel(selectedModel)) {
                spVariant.setAdapter(new ArrayAdapter<>(this, android.R.layout.simple_spinner_dropdown_item, new String[]{"N/A"}));
                spVariant.setEnabled(false);
                llDenoise.setVisibility(View.GONE);
            } else if (selectedModel.equals(MODEL_PLAINUSR)) {
                spVariant.setAdapter(new ArrayAdapter<>(this, android.R.layout.simple_spinner_dropdown_item, new String[]{"N/A"}));
                spVariant.setEnabled(false);
                llDenoise.setVisibility(View.GONE);
            } else {
                spVariant.setEnabled(true);
                boolean isRealCugan = selectedModel.equals(MODEL_REALCUGAN);
                String[] items = isRealCugan ? CUGAN_VARIANTS : ESRGAN_VARIANTS;
                ArrayAdapter<String> variantAdapter = new ArrayAdapter<>(this, android.R.layout.simple_spinner_dropdown_item, items);
                spVariant.setAdapter(variantAdapter);
                
                String current = mSRSetting.variant;
                int pos = 0;
                for (int i = 0; i < items.length; i++) {
                    if (items[i].equals(current)) {
                        pos = i;
                        break;
                    }
                }
                spVariant.setSelection(pos);
                
                // Show denoise option only for realesr-general-x4v3
                boolean showDenoise = !isRealCugan && "realesr-general-x4v3".equals(current);
                llDenoise.setVisibility(showDenoise ? View.VISIBLE : View.GONE);
            }
        };

        spModel.setOnItemSelectedListener(new android.widget.AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(android.widget.AdapterView<?> parent, View view, int position, long id) {
                refreshVariant.run();
            }

            @Override
            public void onNothingSelected(android.widget.AdapterView<?> parent) {
            }
        });
        
        spVariant.setOnItemSelectedListener(new android.widget.AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(android.widget.AdapterView<?> parent, View view, int position, long id) {
                // Update denoise visibility based on selected variant
                Object selectedModelObj = spModel.getSelectedItem();
                String selectedModel = selectedModelObj == null ? MODEL_FSR : selectedModelObj.toString();
                boolean isRealCugan = selectedModel.equals(MODEL_REALCUGAN);
                Object variantObj = spVariant.getSelectedItem();
                String selectedVariant = variantObj == null ? "" : variantObj.toString();
                boolean showDenoise = !priIsTextureFSRModel(selectedModel)
                        && (!isRealCugan && "realesr-general-x4v3".equals(selectedVariant));
                llDenoise.setVisibility(showDenoise ? View.VISIBLE : View.GONE);
            }

            @Override
            public void onNothingSelected(android.widget.AdapterView<?> parent) {
            }
        });
        
        refreshVariant.run();

        new AlertDialog.Builder(this)
                .setTitle("SR Setting")
                .setView(view)
                .setNegativeButton("Cancel", null)
                .setPositiveButton("Apply", (dialog, which) -> {
                    mSRSetting.useGPU = rgDevice.getCheckedRadioButtonId() == R.id.rbSRGPU;
                    Object modelObj = spModel.getSelectedItem();
                    mSRSetting.model = modelObj == null ? MODEL_FSR : modelObj.toString();

                    if (priIsTextureFSRModel(mSRSetting.model) || mSRSetting.model.equals(MODEL_PLAINUSR)) {
                        mSRSetting.variant = "";
                    } else {
                        Object variantObj = spVariant.getSelectedItem();
                        mSRSetting.variant = variantObj == null ? (MODEL_REALCUGAN.equals(mSRSetting.model) ? CUGAN_VARIANTS[0] : ESRGAN_VARIANTS[0]) : variantObj.toString();
                    }

                    // Update denoise from spinner
                    mSRSetting.denoise = selectedDenoise[0];
                    HJLog.i(TAG, "Apply denoise selected: " + selectedDenoise[0] + ", saved to mSRSetting.denoise: " + mSRSetting.denoise);

                    mSRSetting.cpuThreadNums = cpuThreadNums[0];
                    mSRSetting.gpuThreadNums = gpuThreadNums[0];
                    Object folderObj = spInputFolder.getSelectedItem();
                    mSRSetting.inputFolder = folderObj == null
                            ? (inputFolders.isEmpty() ? "360x640" : inputFolders.get(0))
                            : folderObj.toString();
                    Object sourceObj = spInputSource.getSelectedItem();
                    mSRSetting.inputSource = sourceObj == null ? INPUT_SOURCE_IMAGESEQ : sourceObj.toString();
                    Object imageObj = spImageFile.getSelectedItem();
                    mSRSetting.imageFile = imageObj == null ? "" : imageObj.toString();
                    priApplyTextureFSRModeConstraint();
                    priUpdateSRControlUI();
                    updateFaceScaleText();
                    if (mStarted) {
                        int startRet = startInputBySetting();
                        if (startRet < 0) {
                            Toast.makeText(SRFaceActivity.this, "input switch failed: " + startRet, Toast.LENGTH_SHORT).show();
                        }
                    }
                    startProcess();
                    updateSettingText();
                    reInitFaceSROnRenderThread();
                })
                .show();
    }

    private void startRenderIfPossible() {
        if (mSurface == null || mScreenWidth <= 0 || mScreenHeight <= 0) {
            Toast.makeText(this, "surface not ready", Toast.LENGTH_SHORT).show();
            return;
        }
        if (mRender != null) {
            return;
        }

        mRender = new HJRenderEnv();
        mRenderCostMs = 0;
        mRender.setOnRenderFrameStatListener(costMs -> mRenderCostMs = (int) costMs);
        int err = mRender.init(true, mScreenWidth, mScreenHeight, mSurface, FPS);
        if (err < 0) {
            mRender = null;
            Toast.makeText(this, "render init failed: " + err, Toast.LENGTH_SHORT).show();
            return;
        }

        mRender.addCallback(new HJRenderEnv.glDrawCallback() {
            @Override
            public int init() {
                mComposeShader = new HJCopyShader(HJShaderCommon.COORD_VERTEX_TEXTURE_FLIP);
                int ret = mComposeShader.init(HJShaderCommon.SHADER_2D, HJCopyShader.COPY_BLEND | HJCopyShader.COPY_BLEND_SRC_ONEMINUSSRC); //alpha Feathering operation
                //int ret = mComposeShader.init(HJShaderCommon.SHADER_2D, HJCopyShader.COPY_DIRECT);
                if (ret < 0) {
                    return ret;
                }
                mDisplayShader = new HJCopyShader(HJShaderCommon.COORD_VERTEX_TEXTURE_MAP);
                ret = mDisplayShader.init(HJShaderCommon.SHADER_2D, HJCopyShader.COPY_DIRECT);
                Matrix.setIdentityM(mIdentityVertexMat, 0);
                Matrix.setIdentityM(mScreenVertexMat, 0);
                mDisplaySrcWidth = 0;
                mDisplaySrcHeight = 0;
                return ret;
            }

            @Override
            public int draw()
            {
                m_renderIdx++;
                HJBitmap hb = null;
                Bitmap bmp = null;
                final boolean isImageSource = INPUT_SOURCE_IMAGE.equals(mSRSetting.inputSource);
                final boolean paused = mPaused;
                if (isImageSource)
                {
                    bmp = mSingleImageBitmap;
                }
                else if (paused)
                {
                    synchronized (mPauseFrameLock) {
                        hb = mCachedPauseHb;
                    }
                    if (hb != null) {
                        bmp = hb.get();
                    }
                }
                else if (mImgSeqAcquire != null)
                {
                    hb = mImgSeqAcquire.acquire();
                    if (hb != null)
                    {
                        bmp = hb.get();
                    }
                }
                if (bmp != null && !bmp.isRecycled()) {

                    int imgSrcWidth = bmp.getWidth();
                    int imgSrcHeight = bmp.getHeight();

                    float displayScaleW = Math.max(0.0f, Math.min(1.0f, mDisplayScaleW));
                    float displayScaleH = Math.max(0.0f, Math.min(1.0f, mDisplayScaleH));
                    int displayW = Math.max(1, Math.round(mScreenWidth * displayScaleW));
                    int displayH = Math.max(1, Math.round(mScreenHeight * displayScaleH));
                    if (displayW > mScreenWidth) {
                        displayW = mScreenWidth;
                    }
                    if (displayH > mScreenHeight) {
                        displayH = mScreenHeight;
                    }
                    int displayX = 0;
                    int displayY = Math.max(0, mScreenHeight - displayH);

                    ensureScreenDisplayMatrix(imgSrcWidth, imgSrcHeight, displayW, displayH);

                    int fboW = mRender.getOutWidth();
                    int fboH = mRender.getOutHeight();

//                    if (fboW > imgSrcWidth)
//                    {
//                        int ratio = (fboW + imgSrcWidth / 2) / imgSrcWidth;
//                        fboW = ratio * imgSrcWidth;
//                        fboH = ratio * imgSrcHeight;
//                    }
//                    else
//                    {
//                        int ratio = (imgSrcWidth + fboW / 2) / fboW;
//                        fboW = imgSrcWidth / ratio;
//                        fboH = imgSrcHeight / ratio;
//                    }

//                    float valRatio = 0.5f;
//                    fboW = (int)(fboW * valRatio);
//                    fboH = (int)(fboH * valRatio);
//                    fboW = imgSrcWidth;
//                    fboH = imgSrcHeight;

                    ensureComposeFbo(fboW, fboH);
                    uploadBitmapTexture(bmp);

                    boolean bSR = false;
                    boolean bTextureFSR = priIsTextureFSRSelected();

                    boolean bSREnable = mbSREnabled.get();
                    if (s_bUseRandomSRDisable && ((m_renderIdx % FPS) == 0))
                    {
                        bSREnable = false;
                    }

                    if (bSREnable)
                    {
                        int ret;
                        int procPolicy = 0;
                        if (bTextureFSR) {
                            ret = mInferenceEntryFaceSRJni.processTexture(mOriginTextureId, imgSrcWidth, imgSrcHeight);
                        } else {
                            HJInferenceFaceSRProcessOption processOption = buildProcessOption();
                            procPolicy = processOption.policy;
                            ret = mInferenceEntryFaceSRJni.process(bmp, processOption);
                        }
                        // Compatible with both return semantics:
                        // new: 0=success, 3=would-block; old: >0 means face count.
                        boolean canFetchSR = (ret == 0);
                        if (canFetchSR)
                        {
                            if (bTextureFSR) {
                                int srTextureId = mInferenceEntryFaceSRJni.getLastSRTextureId();
                                int srTextureWidth = mInferenceEntryFaceSRJni.getLastSRTextureWidth();
                                int srTextureHeight = mInferenceEntryFaceSRJni.getLastSRTextureHeight();
                                if (srTextureId > 0 && srTextureWidth > 0 && srTextureHeight > 0) {
                                    HJLog.i(TAG, "SR texture ready wh: " + srTextureWidth + " x " + srTextureHeight + ", ret=" + ret + " fbo wh: " + fboW + " x " + fboH);
                                    priAdoptNativeSRTexture(srTextureId);
                                    bSR = true;
                                } else {
                                    HJLog.w(TAG, "processTexture ret=" + ret + " but last SR texture is invalid");
                                }
                            } else {
                                Bitmap srBitmap = mInferenceEntryFaceSRJni.getLastSRBitmap();
                                if (srBitmap != null && !srBitmap.isRecycled())
                                {
                                    HJLog.i(TAG, "SR bitmap ready wh: " + srBitmap.getWidth() + " x " + srBitmap.getHeight() + ", ret=" + ret + " fbo wh: " + fboW + " x " + fboH + " procPolicy:" + procPolicy);
                                    uploadSRBitmapTexture(srBitmap);
                                    bSR = true;

                                    // Save bitmaps for comparison if SR succeeded and it's the first time
                                    if (mSaveOnceTrigger.compareAndSet(true, false)) {
                                        saveBitmap(bmp, "origin");
                                        // Use captureFbo after it's drawn, but we'll capture it here just for full image SR
                                        // Since it's FullSR, srBitmap IS the output.
                                        saveBitmap(srBitmap, "sr_result");
                                    }

                                    srBitmap.recycle();
                                }
                                else
                                {
                                    HJLog.w(TAG, "process ret=" + ret + " but getLastSRBitmap is null");
                                }
                            }
                        }
                        else if (ret == RET_WOULD_BLOCK)
                        {
                            HJLog.i(TAG, "RET_WOULD_BLOCK");
                        }
                        else if (ret < 0)
                        {
                            HJLog.e(TAG, "process failed ret=" + ret);
                        }
                        maybeUpdateInfoText();
                    }
                    else
                    {
                        maybeUpdateInfoTextDisabled();
                    }

                    if (!isImageSource && hb != null && mImgSeqAcquire != null && !paused)
                    {
                        boolean bRecovery = mImgSeqAcquire.recovery(hb);
                        if (!bRecovery)
                        {
                            hb.close();
                        }
                    }

                    mComposeFbo.attach(0);
                    mComposeShader.setVertexMat(mIdentityVertexMat);
                    mComposeShader.draw(mOriginTextureId);
                    if (bSR)
                    {
                        if (bTextureFSR) {
                            priDrawFullSRToComposeFbo();
                        } else {
                            priDrawSRToComposeFbo(imgSrcWidth, imgSrcHeight, displayW, displayH);
                        }
                    }

//                    // Save bitmaps for comparison if SR succeeded and it's the first time
//                    if (bSR && mSaveOnceTrigger.compareAndSet(true, false)) {
//                        saveBitmap(bmp, "origin");
//                        Bitmap capturedSr = captureFbo(mFrameWidth, mFrameHeight);
//                        saveBitmap(capturedSr, "composed_sr");
//                    }

                    mComposeFbo.detach();


                    HJShaderCommon.viewport(displayX, displayY, displayW, displayH);
                    mDisplayShader.setVertexMat(mScreenVertexMat);
                    mDisplayShader.draw(mComposeFbo.get_tex_id(0));
                }
                else {
                    HJLog.i(TAG, isImageSource ? "single image not loaded" : "bitmap no acquire");
                }
                return 0;
            }

            @Override
            public int nodraw() {
                return 0;
            }

            @Override
            public int release() {
                if (mComposeShader != null) {
                    mComposeShader.release();
                    mComposeShader = null;
                }
                if (mDisplayShader != null) {
                    mDisplayShader.release();
                    mDisplayShader = null;
                }
                if (mOriginTextureId > 0) {
                    HJShaderCommon.textureDestory(mOriginTextureIds);
                    mOriginTextureId = -1;
                }
                priReleaseSRTexture();
                if (mComposeFbo != null) {
                    mComposeFbo.release();
                    mComposeFbo = null;
                }
//                mFrameWidth = 0;
//                mFrameHeight = 0;
                mDisplaySrcWidth = 0;
                mDisplaySrcHeight = 0;
                mDisplayTargetWidth = 0;
                mDisplayTargetHeight = 0;
                return 0;
            }
        });
        //Toast.makeText(this, "process started", Toast.LENGTH_SHORT).show();
    }

    private void ensureScreenDisplayMatrix(int srcWidth, int srcHeight, int targetWidth, int targetHeight) {
        if (mRender == null || srcWidth <= 0 || srcHeight <= 0 || targetWidth <= 0 || targetHeight <= 0) {
            return;
        }
        if (mDisplaySrcWidth == srcWidth
                && mDisplaySrcHeight == srcHeight
                && mDisplayTargetWidth == targetWidth
                && mDisplayTargetHeight == targetHeight) {
            return;
        }
        mRender.ScreenDisplayRatio(HJRenderEnv.SCREEN_SCALE_CLIP, targetWidth, targetHeight, srcWidth, srcHeight);
        Matrix.setIdentityM(mScreenVertexMat, 0);
        Matrix.scaleM(mScreenVertexMat, 0, mRender.getVertexMatScaleX(), mRender.getVertexMatScaleY(), 1.f);
        mDisplaySrcWidth = srcWidth;
        mDisplaySrcHeight = srcHeight;
        mDisplayTargetWidth = targetWidth;
        mDisplayTargetHeight = targetHeight;
    }

    private void uploadBitmapTexture(Bitmap bmp) {
        if (bmp == null || bmp.isRecycled()) {
            return;
        }
        if (mOriginTextureId <= 0) {
            mOriginTextureId = HJShaderCommon.textureCreate(HJShaderCommon.SHADER_2D, mOriginTextureIds);
        }
        if (mOriginTextureId > 0) {
            HJShaderCommon.textureUpload(HJShaderCommon.SHADER_2D, mOriginTextureId, bmp);
        }
    }

    private void uploadSRBitmapTexture(Bitmap bmp) {
        if (bmp == null || bmp.isRecycled()) {
            return;
        }
        mLastSRBitmapWidth = bmp.getWidth();
        mLastSRBitmapHeight = bmp.getHeight();
        priPrepareJavaOwnedSRTexture();
        if (mSRTextureId <= 0) {
            mSRTextureId = HJShaderCommon.textureCreate(HJShaderCommon.SHADER_2D, mSRTextureIds);
        }
        if (mSRTextureId > 0) {
            HJShaderCommon.textureUpload(HJShaderCommon.SHADER_2D, mSRTextureId, bmp);
        }
    }

    private void ensureComposeFbo(int width, int height) {
        if (width <= 0 || height <= 0) {
            return;
        }
        if (mComposeFbo != null && mComposeFbo.width() == width && mComposeFbo.height() == height) {
            return;
        }
        if (mComposeFbo != null) {
            mComposeFbo.release();
            mComposeFbo = null;
        }
        mComposeFbo = new HJFBOCtrl();
        int ret = mComposeFbo.init(1, width, height);
        if (ret < 0) {
            HJLog.e(TAG, "compose fbo init failed ret=" + ret + " width=" + width + " height=" + height);
            mComposeFbo = null;
        }
    }

    private void priDrawSRToComposeFbo(int i_imgSrcWidth, int i_imgSrcHeight, int i_displayW, int i_displayH)
    {
        if (mComposeShader == null || mComposeFbo == null || mSRTextureId <= 0 || mInferenceEntryFaceSRJni == null) {
            return;
        }
        int fboWidth = mComposeFbo.width();
        int fboHeight = mComposeFbo.height();

        int x = mInferenceEntryFaceSRJni.getSROriginX();
        int y = mInferenceEntryFaceSRJni.getSROriginY();
        int w = mInferenceEntryFaceSRJni.getSROriginW();
        int h = mInferenceEntryFaceSRJni.getSROriginH();

        //ratio = fboWidth / i_imgSrcWidth
//        int realX = (x * fboWidth) / i_imgSrcWidth;
//        int realY = ((i_imgSrcHeight - h - y) * fboWidth) / i_imgSrcWidth;
//        int realW = (w * fboWidth) / i_imgSrcWidth;
//        int realH = (h * fboWidth) / i_imgSrcWidth;

        int realX = (x * fboWidth + (i_imgSrcWidth / 2)) / i_imgSrcWidth;
        int realY = ((i_imgSrcHeight - h - y) * fboWidth + (i_imgSrcWidth / 2)) / i_imgSrcWidth;
        int realW = mInferenceEntryFaceSRJni.getSRTargetDisplayW();
        int realH = mInferenceEntryFaceSRJni.getSRTargetDisplayH();
        if (realW <= 0 || realH <= 0) {
            realW = (w * fboWidth + (i_imgSrcWidth / 2)) / i_imgSrcWidth;
            realH = (h * fboWidth + (i_imgSrcWidth / 2)) / i_imgSrcWidth;
        }

        HJLog.i(TAG, "priDrawSRToComposeFbo"
                + " srcRect=(" + x + "," + y + "," + w + "," + h + ")"
                + " drawRect=(" + realX + "," + realY + "," + realW + "," + realH + ")"
                + " bitmapWH=(" + mLastSRBitmapWidth + "," + mLastSRBitmapHeight + ")"
                + " img=(" + i_imgSrcWidth + "," + i_imgSrcHeight + ")"
                + " display=(" + i_displayW + "," + i_displayH + ")"
                + " fbo=(" + fboWidth + "," + fboHeight + ")"
                + " targetWH=(" + mInferenceEntryFaceSRJni.getSRTargetDisplayW() + "," + mInferenceEntryFaceSRJni.getSRTargetDisplayH() + ")"
                + " postResize=" + buildPostResizeModeText());

//        int rx = mRender.ScreenDisplayRatioMap(HJRenderEnv.SCREEN_SCALE_CLIP, i_displayW, i_displayH, i_imgSrcWidth, i_imgSrcHeight, x);
//        int ry = mRender.ScreenDisplayRatioMap(HJRenderEnv.SCREEN_SCALE_CLIP, i_displayW, i_displayH, i_imgSrcWidth, i_imgSrcHeight, i_imgSrcHeight - h - y);
//        int rw = mRender.ScreenDisplayRatioMap(HJRenderEnv.SCREEN_SCALE_CLIP, i_displayW, i_displayH, i_imgSrcWidth, i_imgSrcHeight, w);
//        int rh = mRender.ScreenDisplayRatioMap(HJRenderEnv.SCREEN_SCALE_CLIP, i_displayW, i_displayH, i_imgSrcWidth, i_imgSrcHeight, h);
//
//        HJLog.i(TAG, "mapval" + " img: ( " + i_imgSrcWidth + "," + i_imgSrcHeight + ")"  + " dis: ( " + i_displayW + "," + i_displayH + ")"  + " fbo: ( " + fboWidth + "," + fboHeight + ")" + " o: ( " + realX + "," + realY + "," + realW + "," + realH + ")" + " new: (" + rx + "," + ry + "," + rw + "," + rh + ")");
        priDrawMatchedSRToComposeFbo(realX, realY, realW, realH);

//        if (mComposeShader == null || mSRTextureId <= 0 || mFrameWidth <= 0 || mFrameHeight <= 0 || mInferenceEntryFaceSRJni == null) {
//            return;
//        }
//        float ratio = mFaceSRMixRatio;
//        if (ratio <= 0f) {
//            return;
//        }
//        int x = mInferenceEntryFaceSRJni.getSROriginX();
//        int y = mInferenceEntryFaceSRJni.getSROriginY();
//        int w = mInferenceEntryFaceSRJni.getSROriginW();
//        int h = mInferenceEntryFaceSRJni.getSROriginH();
//        if (w <= 0 || h <= 0) {
//            return;
//        }
//        if (x < 0) {
//            w += x;
//            x = 0;
//        }
//        if (y < 0) {
//            h += y;
//            y = 0;
//        }
//        if (x >= mFrameWidth || y >= mFrameHeight || w <= 0 || h <= 0) {
//            return;
//        }
//        if (x + w > mFrameWidth) {
//            w = mFrameWidth - x;
//        }
//        if (y + h > mFrameHeight) {
//            h = mFrameHeight - y;
//        }
//        if (w <= 0 || h <= 0) {
//            return;
//        }
//
//        // face rect is top-left origin, glViewport is bottom-left origin.
//        int glY = mFrameHeight - (y + h);
//        if (glY < 0) {
//            glY = 0;
//        }
//
//        if (mFullSREnabled.get())
//        {
//            HJShaderCommon.viewport(0, 0, mFrameWidth, mFrameHeight);
//        }
//        else
//        {
//            HJShaderCommon.viewport(x, glY, w, h);
//        }
//
//        if (ratio <= 0.001f) {
//            return;
//        }
//        if (ratio >= 0.999f) {
//            mComposeShader.draw(mSRTextureId);
//            return;
//        }
//
//        int srW = Math.round(w * ratio);
//        if (srW <= 0) {
//            return;
//        }
//        if (srW > w) {
//            srW = w;
//        }
//        int srX = x + (w - srW);
//
//        GLES20.glEnable(GLES20.GL_SCISSOR_TEST);
//        GLES20.glScissor(srX, glY, srW, h);
//        mComposeShader.draw(mSRTextureId);
//
//        // Draw a visible split line for SR mix comparison (green vertical line).
//        int lineW = Math.max(1, Math.min(2, w));
//        int lineX = Math.max(x, Math.min(srX, x + w - lineW));
//        GLES20.glScissor(lineX, glY, lineW, h);
//        GLES20.glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
//        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
//
//        GLES20.glDisable(GLES20.GL_SCISSOR_TEST);
    }

    private void maybeUpdateInfoText() {
        if (mInfoText == null || mInferenceEntryFaceSRJni == null) {
            return;
        }
        long now = System.currentTimeMillis();
        if (now - mLastInfoUpdateMs < 120) {
            return;
        }
        mLastInfoUpdateMs = now;
        final float fps = mRenderCostMs > 0 ? (1000.0f / (float) mRenderCostMs) : 0.0f;
        final String text = "render:" + mRenderCostMs + " ms"
                + "  fps:" + String.format(Locale.US, "%.2f", fps)
                + "  fd:" + mInferenceEntryFaceSRJni.getFaceDetectTime() + " ms"
                + "  sr:" + mInferenceEntryFaceSRJni.getSRTime() + " ms"
                + "  rect(" + mInferenceEntryFaceSRJni.getSROriginX() + "," + mInferenceEntryFaceSRJni.getSROriginY()
                + "," + mInferenceEntryFaceSRJni.getSROriginW() + "," + mInferenceEntryFaceSRJni.getSROriginH() + ")"
                + "->scaleWH(" + mInferenceEntryFaceSRJni.getSROriginScaleW() + "," + mInferenceEntryFaceSRJni.getSROriginScaleH() + ")"
                + " target(" + mInferenceEntryFaceSRJni.getSRTargetDisplayW() + "," + mInferenceEntryFaceSRJni.getSRTargetDisplayH() + ")"
                + " pad(" + buildPadInfoText() + ")"
                + " post(" + buildPostResizeModeText() + ")";
        runOnUiThread(() -> mInfoText.setText(text));
    }

    private void maybeUpdateInfoTextDisabled() {
        if (mInfoText == null) {
            return;
        }
        long now = System.currentTimeMillis();
        if (now - mLastInfoUpdateMs < 120) {
            return;
        }
        mLastInfoUpdateMs = now;
        runOnUiThread(() -> mInfoText.setText(getString(R.string.sr_face_info_default)));
    }

    private void updateMixText() {
        if (mMixText == null) {
            return;
        }
        final String text = String.format(Locale.US, "FaceSR Mix: %.2f", mFaceSRMixRatio);
        mMixText.setText(text);
    }

    private void priUpdateSRStatusText(boolean enabled) {
        if (mSRStatusText == null) {
            return;
        }
        mSRStatusText.setText(enabled ? "ON" : "OFF");
        mSRStatusText.setTextColor(0xFF00FF66);
    }

    private void updateMatchText() {
        if (mMatchText == null) {
            return;
        }
        final String text = String.format(Locale.US, "FaceSR Match: %.2f", mFaceSRMatchRatio);
        mMatchText.setText(text);
    }

    private void updateDisplayScaleText() {
        if (mDisplayScaleWText != null) {
            mDisplayScaleWText.setText(String.format(Locale.US, "Display W: %.2f", mDisplayScaleW));
        }
        if (mDisplayScaleHText != null) {
            mDisplayScaleHText.setText(String.format(Locale.US, "Display H: %.2f", mDisplayScaleH));
        }
    }

    private void updateFsrText() {
        if (mFsrDenoiseText != null) {
            mFsrDenoiseText.setText(String.format(Locale.US, "FSR Denoise: %.2f", mSRSetting.fsrDenoiseStrength));
        }
        if (mFsrSharpnessText != null) {
            mFsrSharpnessText.setText(String.format(Locale.US, "FSR Sharpness: %.2f", mSRSetting.fsrSharpness));
        }
        if (mFsrBoostText != null) {
            mFsrBoostText.setText(String.format(Locale.US, "FSR Extra Boost: %.2f", mSRSetting.fsrExtraSharpenBoost));
            mFsrBoostText.setEnabled(mSRSetting.fsrEnableExtraSharpen);
            mFsrBoostText.setAlpha(mSRSetting.fsrEnableExtraSharpen ? 1.0f : 0.6f);
        }
        priUpdateSRControlUI();
    }

    private void applyRealtimeTextureFsrSetting() {
        if (!priIsTextureFSRSelected()) {
            return;
        }
        updateSettingText();
        if (mInferenceEntryFaceSRJni != null) {
            reInitFaceSROnRenderThread();
        }
    }

    private void refreshPreScaleSpinner() {
        if (mPreScaleSpinner == null) {
            return;
        }
        String[] labels = getPreScalePresetLabels();
        ArrayAdapter<String> adapter = new ArrayAdapter<>(this, R.layout.spinner_item_green, labels);
        adapter.setDropDownViewResource(R.layout.spinner_dropdown_item_green);
        mPreScaleSpinner.setAdapter(adapter);
        if (labels.length <= 0) {
            mPreScaleSpinner.setEnabled(false);
            return;
        }
        mPreScaleSpinner.setEnabled(true);
        int selection = mSRMode == HJInferenceFaceSRProcessOption.HJFaceSRProcessMode_FaceScale
                ? mFaceScalePresetIndex
                : mFullScalePresetIndex;
        selection = Math.max(0, Math.min(selection, labels.length - 1));
        mPreScaleSpinner.setSelection(selection, false);
        mPreScaleSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                if (mSRMode == HJInferenceFaceSRProcessOption.HJFaceSRProcessMode_FaceScale) {
                    mFaceScalePresetIndex = position;
                } else if (mSRMode == HJInferenceFaceSRProcessOption.HJFaceSRProcessMode_FullScale) {
                    mFullScalePresetIndex = position;
                }
                updateResolvedPreScale();
                updateFaceScaleText();
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
            }
        });
    }

    private String buildPadInfoText() {
        if (mInferenceEntryFaceSRJni == null) {
            return "0x0";
        }
        int scaleW = mInferenceEntryFaceSRJni.getSROriginScaleW();
        int scaleH = mInferenceEntryFaceSRJni.getSROriginScaleH();
        int padW = mInferenceEntryFaceSRJni.getSRPadLeft() + mInferenceEntryFaceSRJni.getSRPadRight();
        int padH = mInferenceEntryFaceSRJni.getSRPadTop() + mInferenceEntryFaceSRJni.getSRPadBottom();
        if (padW > 0) {
            return padW + "x" + Math.max(0, scaleH);
        }
        if (padH > 0) {
            return Math.max(0, scaleW) + "x" + padH;
        }
        return "0x0";
    }

    private int getSelectedProcPolicy() {
        if (!usesPreScaleMode(mSRMode)) {
            return 1;
        }
        if (mProcPolicySpinner != null) {
            int idx = mProcPolicySpinner.getSelectedItemPosition();
            if (idx >= 0) {
                return getProcPolicyByIndex(idx);
            }
        }
        return getProcPolicyByIndex(0);
    }

    private HJInferenceFaceSRProcessOption buildProcessOption() {
        priApplyTextureFSRModeConstraint();
        updateResolvedPreScale();
        HJInferenceFaceSRProcessOption option = new HJInferenceFaceSRProcessOption();
        option.mode = mSRMode;
        option.preScaleWidth = usesPreScaleMode(mSRMode) ? mPreScaleWidth : 0;
        option.preScaleHeight = usesPreScaleMode(mSRMode) ? mPreScaleHeight : 0;
        option.composeCanvasWidth = mComposeFbo != null ? mComposeFbo.width() : 0;
        option.composeCanvasHeight = mComposeFbo != null ? mComposeFbo.height() : 0;
        option.bFeather = isFaceMode(mSRMode);
        option.bEnablePostSRDisplayResize = mEnableNativePostSRDisplayResize;
        option.mixAlphaRatio = mFaceSRMixRatio;
        option.policy = getSelectedProcPolicy();
        return option;
    }

    private void updateFaceScaleUI() {
        priApplyTextureFSRModeConstraint();
        priUpdateSRControlUI();
    }

    private void updateFaceScaleText() {
        if (mFaceScaleText == null) {
            return;
        }
        if (priIsTextureFSRSelected()) {
            mFaceScaleText.setText("PreScale: FSR Full Only");
            return;
        }
        if (!usesPreScaleMode(mSRMode)) {
            mFaceScaleText.setText("PreScale: Off");
            return;
        }
        mFaceScaleText.setText(String.format(Locale.US, "PreScale: %dx%d", mPreScaleWidth, mPreScaleHeight));
    }

    private void updateSettingText() {
        if (mSettingText == null) {
            return;
        }
        final String device = mSRSetting.useGPU ? "GPU" : "CPU";
        final String inputDesc = INPUT_SOURCE_IMAGE.equals(mSRSetting.inputSource)
                ? ("image:" + (mSRSetting.imageFile == null ? "" : mSRSetting.imageFile))
                : ("imgseq:" + mSRSetting.inputFolder);
        final int threadNums = mSRSetting.useGPU ? mSRSetting.gpuThreadNums : mSRSetting.cpuThreadNums;

        StringBuilder textBuilder = new StringBuilder("SR: " + device + " / " + mSRSetting.model);
        if (!priIsTextureFSRSelected() && mSRSetting.variant != null && !mSRSetting.variant.isEmpty()) {
            textBuilder.append(" / ").append(mSRSetting.variant);
        }
        textBuilder.append(" / t").append(threadNums);

        if (priIsTextureFSRSelected()) {
            textBuilder.append(" / fsrDenoise=").append(String.format(Locale.US, "%.1f", mSRSetting.fsrDenoiseStrength));
            textBuilder.append(" / fsrSharp=").append(String.format(Locale.US, "%.1f", mSRSetting.fsrSharpness));
            if (mSRSetting.fsrEnableExtraSharpen) {
                textBuilder.append(" / fsrBoost=").append(String.format(Locale.US, "%.2f", mSRSetting.fsrExtraSharpenBoost));
            } else {
                textBuilder.append(" / fsrExtra=off");
            }
        } else if ("realesr-general-x4v3".equals(mSRSetting.variant)) {
            textBuilder.append(" / ncnnDenoise=").append(String.format(Locale.US, "%.1f", mSRSetting.denoise));
        }
        if (!priIsTextureFSRSelected()) {
            textBuilder.append(" / post=").append(buildPostResizeModeText());
        }

        textBuilder.append(" / ").append(inputDesc);
        mSettingText.setText(textBuilder.toString());
    }

    private String buildPostResizeModeText() {
        return mEnableNativePostSRDisplayResize ? "nativeLanczos" : "glLinear";
    }

    private List<String> listImageJpgFiles() {
        final String dirPath = getFilesDir().getAbsolutePath() + File.separator + "HJResource/image";
        File dir = new File(dirPath);
        List<String> out = new ArrayList<>();
        if (!dir.exists() || !dir.isDirectory()) {
            return out;
        }
        File[] files = dir.listFiles();
        if (files == null) {
            return out;
        }
        for (File file : files) {
            if (file == null || !file.isFile()) {
                continue;
            }
            String name = file.getName().toLowerCase(Locale.US);
            if (name.endsWith(".jpg") || name.endsWith(".jpeg")) {
                out.add(file.getName());
            }
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
            out.sort(String::compareToIgnoreCase);
        }
        return out;
    }

    private List<String> listInputFolders() {
        final String dirPath = getFilesDir().getAbsolutePath() + File.separator + "HJResource/imgseq";
        File dir = new File(dirPath);
        List<String> out = new ArrayList<>();
        if (!dir.exists() || !dir.isDirectory()) {
            return out;
        }
        File[] files = dir.listFiles();
        if (files == null) {
            return out;
        }
        for (File file : files) {
            if (file != null && file.isDirectory()) {
                out.add(file.getName());
            }
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
            out.sort(String::compareToIgnoreCase);
        }
        return out;
    }

    private void stopProcess() {
        mStarted = false;
        releaseCachedPauseFrame();
        mPaused = false;
        updatePauseButtonText();

        if (mImgSeqAcquire != null) {
            mImgSeqAcquire.stop();
        }
        if (mSingleImageBitmap != null && !mSingleImageBitmap.isRecycled()) {
            mSingleImageBitmap.recycle();
        }
        mSingleImageBitmap = null;

        if (mRender != null) {
            mRender.release();
            mRender = null;
        }

    }

    private boolean cachePauseFrame() {
        if (INPUT_SOURCE_IMAGE.equals(mSRSetting.inputSource) || mImgSeqAcquire == null) {
            return true;
        }
        synchronized (mPauseFrameLock) {
            if (mCachedPauseHb != null) {
                return true;
            }
            mCachedPauseHb = mImgSeqAcquire.acquire();
            return mCachedPauseHb != null;
        }
    }

    private void releaseCachedPauseFrame() {
        HJBitmap cached = null;
        synchronized (mPauseFrameLock) {
            if (mCachedPauseHb == null) {
                return;
            }
            cached = mCachedPauseHb;
            mCachedPauseHb = null;
        }
        if (mImgSeqAcquire != null) {
            boolean recovered = mImgSeqAcquire.recovery(cached);
            if (!recovered) {
                cached.close();
            }
        } else {
            cached.close();
        }
    }

    private void updatePauseButtonText() {
        if (mPauseButton == null) {
            return;
        }
        mPauseButton.setText(mPaused ? R.string.sr_face_resume : R.string.sr_face_pause);
    }

    private void saveBitmap(Bitmap bmp, String prefix) {
        if (bmp == null || bmp.isRecycled()) return;
        // Save to files/HJResource/debug_output
        File outputDir = new File(getFilesDir(), "HJResource/debug_output");
        if (!outputDir.exists()) {
            outputDir.mkdirs();
        }
        File file = new File(outputDir, prefix + "_" + System.currentTimeMillis() + ".png");
        try (FileOutputStream out = new FileOutputStream(file)) {
            bmp.compress(Bitmap.CompressFormat.PNG, 100, out);
            HJLog.i(TAG, "Successfully saved bitmap to: " + file.getAbsolutePath());
        } catch (Exception e) {
            HJLog.e(TAG, "Failed to save bitmap " + prefix);
        }
    }

    private Bitmap captureFbo(int w, int h) {
        ByteBuffer buffer = ByteBuffer.allocateDirect(w * h * 4);
        buffer.order(ByteOrder.nativeOrder());
        GLES20.glReadPixels(0, 0, w, h, GLES20.GL_RGBA, GLES20.GL_UNSIGNED_BYTE, buffer);
        Bitmap bmp = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
        buffer.rewind();
        bmp.copyPixelsFromBuffer(buffer);
        // Vertical flip needed because OpenGL origin is bottom-left
        android.graphics.Matrix matrix = new android.graphics.Matrix();
        matrix.postScale(1, -1);
        return Bitmap.createBitmap(bmp, 0, 0, w, h, matrix, true);
    }
}
