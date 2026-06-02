package com.example.exportui;

import android.graphics.PixelFormat;
import android.opengl.Matrix;
import android.os.Bundle;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

import com.HJMediasdk.entry.HJCommonInterface;
import com.HJMediasdk.entry.render.HJFacePointsMadeup;
import com.HJMediasdk.entry.render.HJRenderContextJni;
import com.HJMediasdk.entry.render.HJFacePointInfo;
import com.HJMediasdk.entry.render.HJRenderFaceuEntryJni;
import com.HJMediasdk.graphic.HJRenderEnv;
import com.HJMediasdk.graphic.draw.HJBitmapTexture;
import com.HJMediasdk.graphic.draw.HJCopyShader;
import com.HJMediasdk.graphic.draw.HJFBOCtrl;
import com.HJMediasdk.graphic.draw.HJShaderCommon;
import com.HJMediasdk.utils.HJBaseListener;
import com.HJMediasdk.utils.HJBaseToast;
import com.HJMediasdk.utils.HJLog;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.List;
import java.util.Map;
import android.widget.CheckBox;
import android.widget.CompoundButton;
public class FaceuActivity extends AppCompatActivity {

    private static final String TAG = "SecondActivityLog";
    private static final int s_imgSeqCount = 296;
    private static final String s_uniqueKey1 = "faceuUnique1";
    private static final String s_uniqueKey2 = "faceuUnique2";
    private static final String s_uniqueKey3 = "faceuUnique3";
    private static final int s_faceImgWidth = 720;
    private static final int s_faceImgHeight = 1280;

    // UI Components
    private Button btnOpen;
    private EditText etFileName1, etFileName2, etFileName3;
    private Button btnAdd1, btnAdd2, btnAdd3;
    private Button btnRemove1, btnRemove2, btnRemove3;
    private Button btnRemoveAll;
    private CheckBox chkToggleImgSeq;
    private SurfaceView m_view = null;
    private Surface m_surface = null;
    private int m_screenWidth = 0;
    private int m_screenHeight = 0;
    private HJRenderEnv m_render;
    private HJRenderFaceuEntryJni m_faceu = null;
    private HJFacePointInfo m_facePointInfo = new HJFacePointInfo();
    private HJFacePointsMadeup m_faceMadeup = new HJFacePointsMadeup();
    private HJBaseListener m_faceuListener;
    private boolean m_isImgSeqEnabled = false;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_faceu);

        initViews();

        setupClickListeners();
        HJLog.setEnable(true);
        HJRenderContextJni.contextInit(getFilesDir().getAbsolutePath() + File.separator + "render", HJCommonInterface.HJLOGLevel_INFO, HJCommonInterface.HJLogLMode_CONSOLE | HJCommonInterface.HJLLogMode_FILE,  5 * 1024 * 1024, 5);
    }
    
    private void initViews() {
        m_view = (SurfaceView)findViewById(R.id.faceuView);
        m_view.getHolder().setFormat(PixelFormat.RGBA_8888);
        m_view.getHolder().addCallback(mSurfaceHolderCallback);

        btnOpen = findViewById(R.id.btnOpen);
        etFileName1 = findViewById(R.id.etFileName1);
        etFileName2 = findViewById(R.id.etFileName2);
        etFileName3 = findViewById(R.id.etFileName3);
        btnAdd1 = findViewById(R.id.btnAdd1);
        btnAdd2 = findViewById(R.id.btnAdd2);
        btnAdd3 = findViewById(R.id.btnAdd3);
        btnRemove1 = findViewById(R.id.btnRemove1);
        btnRemove2 = findViewById(R.id.btnRemove2);
        btnRemove3 = findViewById(R.id.btnRemove3);
        btnRemoveAll = findViewById(R.id.btnRemoveAll);
        chkToggleImgSeq = findViewById(R.id.chkToggleImgSeq);
    }
    @Override
    protected void onDestroy()
    {
        HJLog.i(TAG, "closePlayerenter ");
        if (m_render != null)
        {
            m_render.SyncQueueEvent(0, new Runnable() {
                @Override
                public void run() {
                    if (m_faceu != null) {
                        HJLog.i(TAG, "faceu done enter");
                        m_faceu.faceuDone();
                        HJLog.i(TAG, "faceu done end");
                        m_faceu = null;
                    }
                }
            });
        }

        if (m_render != null)
        {
            HJLog.i(TAG, "render release enter");
            m_render.release();
            HJLog.i(TAG, "render release end");
            m_render = null;
        }
        HJLog.i(TAG, "contextUnInit enter");
        HJRenderContextJni.contextUnInit();
        HJLog.i(TAG, "contextUnInit end");
        super.onDestroy();
    }
    private final SurfaceHolder.Callback mSurfaceHolderCallback = new SurfaceHolder.Callback() {
        @Override
        public void surfaceCreated(SurfaceHolder holder) {
            m_surface = holder.getSurface();
            HJLog.i(TAG, "surfaceTest ANativeWindow_ surfaceCreated");
        }

        @Override
        public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
            m_screenWidth = w;
            m_screenHeight = h;
            HJLog.i(TAG, "surfaceTest ANativeWindow_ surfaceChanged");
            if (m_render != null)
            {
                m_render.setSurface(holder.getSurface());
            }
        }

        @Override
        public void surfaceDestroyed(SurfaceHolder holder) {
            m_surface = null;
            HJLog.i(TAG, "surfaceTest ANativeWindow_ surfaceDetroyed enter");

            if (m_render != null)
            {
                m_render.setSurface(m_surface);
            }
            HJLog.i(TAG, "surfaceTest ANativeWindow_ java surfaceDetroyed end");
        }
    };


    private HJCopyShader m_faceuCopyShader = null;
    private HJCopyShader m_imgSeqCopyShader = null;
    private HJFBOCtrl m_fbo = null;
    private final float[] m_vertexDstMatrix = new float[16];
    private void priRenderProc()
    {
        m_render.addCallback(new HJRenderEnv.glDrawCallback()
        {
            @Override
            public int init()
            {
                m_imgSeqCopyShader = new HJCopyShader(HJShaderCommon.COORD_VERTEX_TEXTURE_FLIP);
                m_imgSeqCopyShader.init(HJShaderCommon.SHADER_2D, HJCopyShader.COPY_DIRECT);

                m_faceuCopyShader = new HJCopyShader(HJShaderCommon.COORD_VERTEX_TEXTURE_MAP);
                m_faceuCopyShader.init(HJShaderCommon.SHADER_2D, HJCopyShader.COPY_BLEND | HJCopyShader.COPY_BLEND_ONE_ONEMINUSSRC);
                m_fbo = new HJFBOCtrl();
                m_fbo.init(1, s_faceImgWidth, s_faceImgHeight);
                m_render.ScreenDisplay(HJRenderEnv.SCREEN_SCALE_CLIP, m_screenWidth, m_screenHeight, s_faceImgWidth, s_faceImgHeight);
                Matrix.setIdentityM(m_vertexDstMatrix, 0);
                Matrix.scaleM(m_vertexDstMatrix, 0, m_render.getVertexMatScaleX(), m_render.getVertexMatScaleY(), 1.f);
                return 0;
            }

            @Override
            public int nodraw()
            {
                return 0;
            }

            @Override
            public int draw() {
                if (m_faceu == null)
                {
                    return 0;
                }

                long t0 = System.currentTimeMillis();

                int[] outIdx = new int[1];

                List<HJFacePointsMadeup.FacePoint> makeupPoint = m_faceMadeup.getFacePoints(-1, outIdx);

                m_facePointInfo.clear();

                int faceCount = 1;
                m_facePointInfo.setSize(s_faceImgWidth, s_faceImgHeight);

                for (int i = 0; i < faceCount; i++)
                {
                    HJFacePointInfo.HJSingleFaceInfo singleFaceInfo = new HJFacePointInfo.HJSingleFaceInfo();
                    singleFaceInfo.setRect(makeupPoint.get(5).x, makeupPoint.get(5).y, makeupPoint.get(8).x - makeupPoint.get(5).x, makeupPoint.get(8).y - makeupPoint.get(5).y);
                    for (int j = 0; j < 5; j++)
                    {
                        singleFaceInfo.add(makeupPoint.get(j).x, makeupPoint.get(j).y);
                    }
                    m_facePointInfo.add(singleFaceInfo);
                }


//                m_points.setContainFace(true);
//                m_pointsOffset.setContainFace(true);
//                for (HJFacePointsMadeup.FacePoint mpt : makeupPoint) {
//                    m_points.addPoint(mpt.x, mpt.y);
//                }

                m_fbo.attach(0);
                m_faceu.faceuRender(m_facePointInfo);
                m_fbo.detach();


                HJShaderCommon.viewport(0, 0, m_screenWidth, m_screenHeight);
                //HJShaderCommon.clear(0.f, 0.f, 0.f, 0.f);
                if (m_isImgSeqEnabled)
                {
                    int imgIdx = outIdx[0];
                    if (imgIdx >= 0 && imgIdx < s_imgSeqCount)
                    {
                        long tt0 = System.currentTimeMillis();
                        String imgUrl = getFilesDir().getAbsolutePath() + File.separator + "HJResource/imgseq/sing/sing_" + imgIdx + ".jpg";
                        HJBitmapTexture bitmapTexture = new HJBitmapTexture();
                        bitmapTexture.init(imgUrl);
                        long tt1 = System.currentTimeMillis();
                        m_imgSeqCopyShader.draw(bitmapTexture.getTextureId());
                        bitmapTexture.release();
                        long tt2 = System.currentTimeMillis();
                        HJLog.i(TAG, "draw time: " + (tt2 - tt1) + "ms" + " init time: " + (tt1 - tt0) + "ms");
                    }
                }

                m_faceuCopyShader.setVertexMat(m_vertexDstMatrix);
                m_faceuCopyShader.draw(m_fbo.get_tex_id(0));

                long t1 = System.currentTimeMillis();
                HJLog.i(TAG, "draw time: " + (t1 - t0) + " ms");
                return 0;
            }

            @Override
            public int release()
            {
                if (m_faceuCopyShader != null)
                {
                    m_faceuCopyShader.release();
                    m_faceuCopyShader = null;
                }
                if (m_imgSeqCopyShader != null)
                {
                    m_imgSeqCopyShader.release();
                    m_imgSeqCopyShader = null;
                }
                if (m_fbo != null)
                {
                    m_fbo.release();
                    m_fbo = null;
                }
                return 0;
            }
        });
    }

    private void setupClickListeners() {
        // Open button click listener
        btnOpen.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                //Toast.makeText(FaceuActivity.this, "Open button clicked", Toast.LENGTH_SHORT).show();
                HJLog.i(TAG, "Open button clicked");
                if (m_render == null)
                {
                    m_render = new HJRenderEnv();
                    int i_err = m_render.init(true, m_screenWidth, m_screenHeight, m_surface);
                    if (i_err < 0)
                    {
                        HJLog.e(TAG, "Render init failed with error code: " + i_err);
                    }
                    priRenderProc();
                }

                if (m_render != null)
                {
                    m_render.AsyncQueueEvent(0, new Runnable() {
                        @Override
                        public void run()
                        {
                            if (m_faceu == null)
                            {
                                m_faceuListener = new HJBaseListener()
                                {
                                    @Override
                                    public int onNotify(int id, int value, String uniqueKey) {
                                        // Handle any callbacks from the native layer here
                                        HJLog.i(TAG, "Faceu notification: id=" + id + ", value=" + value + ", desc="
                                                + uniqueKey);
                                        if (id == HJRenderFaceuEntryJni.HJ_FACEU_NOTIFY_COMPLETE) {
                                            HJLog.i(TAG, "faceu complete " + uniqueKey);
                                        }
                                        return 0;
                                    }
                                };
                                m_faceu = new HJRenderFaceuEntryJni();
                                int i_err = m_faceu.faceuInit(m_faceuListener);
                                if (i_err < 0) {
                                    HJLog.e(TAG, "faceu init failed with error code: " + i_err);
                                    HJBaseToast.toast(FaceuActivity.this, "faceuInit error", 1000);
                                    return;
                                }

                            }

                        }
                    });
                }

            }
        });
        chkToggleImgSeq.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                m_isImgSeqEnabled = isChecked;
                HJLog.i(TAG, "Image sequence rendering is now: " + (isChecked ? "Enabled" : "Disabled"));
            }
        });
        // Add button click listeners
        btnAdd1.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String fileName = etFileName1.getText().toString();
                if (m_render != null)
                {
                    m_render.AsyncQueueEvent(0, new Runnable() {
                        @Override
                        public void run() {
                            int i_err = m_faceu.faceuAdd(s_uniqueKey1, getFilesDir().getAbsolutePath() + File.separator + "HJResource/faceu/" + fileName, false);
                            if (i_err < 0)
                            {
                                HJBaseToast.toast(FaceuActivity.this, "faceuAdd error", 1000);
                            }
                        }
                    });
                }
                else {
                    HJBaseToast.toast(FaceuActivity.this, "please open first", 400);
                }
            }
        });
        
        btnAdd2.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String fileName = etFileName2.getText().toString();
                if (m_render != null) {
                    m_render.AsyncQueueEvent(0, new Runnable() {
                        @Override
                        public void run() {
                            int i_err = m_faceu.faceuAdd(s_uniqueKey2, getFilesDir().getAbsolutePath() + File.separator + "HJResource/faceu/" + fileName, false);
                            if (i_err < 0)
                            {
                                HJBaseToast.toast(FaceuActivity.this, "faceuAdd error", 1000);
                            }
                        }
                    });
                }
                else {
                    HJBaseToast.toast(FaceuActivity.this, "please open first", 400);
                }
            }
        });
        
        btnAdd3.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String fileName = etFileName3.getText().toString();
                if (m_render != null) {
                    m_render.AsyncQueueEvent(0, new Runnable() {
                        @Override
                        public void run() {
                            int i_err = m_faceu.faceuAdd(s_uniqueKey3, getFilesDir().getAbsolutePath() + File.separator + "HJResource/faceu/" + fileName, false);
                            if (i_err < 0)
                            {
                                HJBaseToast.toast(FaceuActivity.this, "faceuAdd error", 1000);
                            }
                        }
                    });
                }
                else {
                    HJBaseToast.toast(FaceuActivity.this, "please open first", 400);
                }
            }
        });
        
        // Remove button click listeners
        btnRemove1.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (m_render != null) {
                    m_render.AsyncQueueEvent(0, new Runnable() {
                        @Override
                        public void run() {
                            int i_err = m_faceu.faceuRemove(s_uniqueKey1);
                            if (i_err < 0)
                            {
                                HJBaseToast.toast(FaceuActivity.this, "faceuRemove error", 1000);
                            }
                        }
                    });
                }
                else {
                    HJBaseToast.toast(FaceuActivity.this, "please open first", 400);
                }
            }
        });
        
        btnRemove2.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (m_render != null) {
                    m_render.AsyncQueueEvent(0, new Runnable() {
                        @Override
                        public void run() {
                            int i_err = m_faceu.faceuRemove(s_uniqueKey2);
                            if (i_err < 0)
                            {
                                HJBaseToast.toast(FaceuActivity.this, "faceuRemove error", 1000);
                            }
                        }
                    });
                }
                else {
                    HJBaseToast.toast(FaceuActivity.this, "please open first", 400);
                }
            }
        });
        
        btnRemove3.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (m_render != null) {
                    m_render.AsyncQueueEvent(0, new Runnable() {
                        @Override
                        public void run() {
                            int i_err = m_faceu.faceuRemove(s_uniqueKey3);
                            if (i_err < 0)
                            {
                                HJBaseToast.toast(FaceuActivity.this, "faceuRemove error", 1000);
                            }
                        }
                    });
                }
                else {
                    HJBaseToast.toast(FaceuActivity.this, "please open first", 400);
                }
            }
        });
        
        // Remove All button click listener
        btnRemoveAll.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (m_render != null) {
                    m_render.AsyncQueueEvent(0, new Runnable() {
                        @Override
                        public void run() {
                            int i_err = m_faceu.faceuRemoveAll();
                            if (i_err < 0)
                            {
                                HJBaseToast.toast(FaceuActivity.this, "faceuRemove error", 1000);
                            }
                        }
                    });
                }
                else {
                    HJBaseToast.toast(FaceuActivity.this, "please open first", 400);
                }
            }
        });
    }
}
