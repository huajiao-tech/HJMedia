#pragma once

#include <array>

#include "HJRteCom.h"
#include "HJMediaUtils.h"

#if defined (HarmonyOS)
    #include "HJOGBaseShader.h"
    #include <GLES3/gl3.h>
#endif

NS_HJ_BEGIN

class HJOGEGLSurface;

class HJRteComDraw : public HJRteCom
{
public:
    //using customDrawFun = std::function<int(HJRteDriftInfo::Ptr i_driftInfo, HJRteDriftInfo::Ptr& o_driftInfo)>;
    
    HJ_DEFINE_CREATE(HJRteComDraw);
    HJRteComDraw();
    virtual ~HJRteComDraw();
        
	virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void done() override;


    virtual HJSizei getTargetWH();    
        
    // int render(HJRteDriftInfo::Ptr& o_param);
    
    // void setCustomDraw(customDrawFun i_func)
    // {
    //     m_customDraw = i_func;
    // }

    void setFps(int i_fps)
    {
        m_fps = i_fps;
    }
    int getFps() const
    {
        return m_fps;       
    }
    
// protected:
//     virtual int attach();
//     virtual int detach();
private:
    //customDrawFun m_customDraw = nullptr;
    int m_fps = 30;  
};
class HJRteComDrawEGL : public HJRteComDraw
{
public:
    HJ_DEFINE_CREATE(HJRteComDrawEGL);
    HJRteComDrawEGL();
    virtual ~HJRteComDrawEGL() = default;
        
	virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void done() override;
        
    //virtual void reset() override;  
    virtual HJSizei getTargetWH() override;   

    void setSurface(const std::shared_ptr<HJOGEGLSurface> & i_eglSurface);
    std::shared_ptr<HJOGEGLSurface> getSurface(); 

    virtual int bind() override;
    virtual int unbind(bool i_bDraw) override;
    virtual int render(const std::shared_ptr<HJRteComLink>&i_link, const HJRteDriftInfo::Ptr& i_drift) override;
    
protected:
    // virtual int attach() override;
    // virtual int detach() override;
    std::weak_ptr<HJOGEGLSurface> m_eglSurfaceWtr;
private:
    //int m_curRenderIdx = 0;
};

class HJRteComDrawEGLUI_0 : public HJRteComDrawEGL
{
public:
    HJ_DEFINE_CREATE(HJRteComDrawEGLUI_0);
    HJRteComDrawEGLUI_0();
    virtual ~HJRteComDrawEGLUI_0() = default;
};
class HJRteComDrawEGLUI_1 : public HJRteComDrawEGL
{
public:
    HJ_DEFINE_CREATE(HJRteComDrawEGLUI_1);
    HJRteComDrawEGLUI_1();
    virtual ~HJRteComDrawEGLUI_1() = default;
};
class HJRteComDrawEGLUI_2 : public HJRteComDrawEGL
{
public:
    HJ_DEFINE_CREATE(HJRteComDrawEGLUI_2);
    HJRteComDrawEGLUI_2();
    virtual ~HJRteComDrawEGLUI_2() = default;
};
class HJRteComDrawEGLUI_3 : public HJRteComDrawEGL
{
public:
    HJ_DEFINE_CREATE(HJRteComDrawEGLUI_3);
    HJRteComDrawEGLUI_3();
    virtual ~HJRteComDrawEGLUI_3() = default;
};

class HJRteComDrawEGLEncoder : public HJRteComDrawEGL
{
public:
    HJ_DEFINE_CREATE(HJRteComDrawEGLEncoder);
    HJRteComDrawEGLEncoder() = default;
    virtual ~HJRteComDrawEGLEncoder() = default;
};

class HJRteComDrawEGLImgReceiver : public HJRteComDrawEGL
{
public:
    HJ_DEFINE_CREATE(HJRteComDrawEGLImgReceiver);
    HJRteComDrawEGLImgReceiver() = default;
    virtual ~HJRteComDrawEGLImgReceiver() = default;

    virtual int adjustResolution(int i_width, int i_height) override;
};

class HJOGFBOCtrl;
class HJRteComDrawFBO : public HJRteComDraw
{
public:
    HJ_DEFINE_CREATE(HJRteComDrawFBO);
    HJRteComDrawFBO();
    virtual ~HJRteComDrawFBO() = default;;

    virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void done() override;
    virtual HJSizei getTargetWH() override; 
    
    virtual int bind() override;
    virtual int unbind(bool i_bDraw) override;
    virtual int render(const std::shared_ptr<HJRteComLink>&i_link, const HJRteDriftInfo::Ptr& i_drift) override;
    
    virtual int draw(GLuint textureId, int i_fitMode, int srcw, int srch, int dstw, int dsth, float* texMat = nullptr, bool i_bYFlip = false, bool i_bXFlip = false)
    {
        return HJ_OK;
    }

    virtual int getWidth() override;
    virtual int getHeight() override;
    virtual float *getTexMatrix() override
    {
        return HJRteCom::getTexMatrix();
    }
    virtual GLuint getTextureId() override;
    virtual HJRteTextureType getTextureType() override
    {
        return HJRteTextureType_2D;
    }

    virtual int adjustResolution(int i_width, int i_height) override;
    
    //int check(int i_width, int i_height, bool i_bTransparency = true);
    //int tryRestart(int i_width, int i_height, bool i_bTransparency = true);

    void setAdjustResolution(bool i_bAdjustResolution)
    {
        m_isAdjustResolution = i_bAdjustResolution;
    }
    bool isAdjustResolution() const
    {
        return m_isAdjustResolution;
    }

    const std::shared_ptr<HJOGBaseShader> & getShader() const 
    {
        return m_endShader;
    }

    static int tryRestartFbo(std::shared_ptr<HJOGFBOCtrl>& o_fbo, int i_width, int i_height, bool i_bTranspareny = true);

    std::shared_ptr<HJOGFBOCtrl> takeFbo();
    
protected:

    std::shared_ptr<HJOGBaseShader> m_endShader = nullptr;
    std::shared_ptr<HJOGFBOCtrl> m_dynamicFbo = nullptr;
    bool m_forceXMirror = false;
    bool m_forceYMirror = false;

    int m_adjustWidth = 0;
    int m_adjustHeight = 0;
    bool m_adjustTransparency = true;
private:
    bool m_isBinded = false;
    bool m_isAdjustResolution = true;


};


class HJRteComDrawCopy2DFBO : public HJRteComDrawFBO
{
public:
    HJ_DEFINE_CREATE(HJRteComDrawCopy2DFBO);
    HJRteComDrawCopy2DFBO();
    virtual ~HJRteComDrawCopy2DFBO() = default;
    virtual int init(HJBaseParam::Ptr i_param) override;
};
class HJRteComDrawCopyOESFBO : public HJRteComDrawFBO
{
public:
    HJ_DEFINE_CREATE(HJRteComDrawCopyOESFBO);
    HJRteComDrawCopyOESFBO();
    virtual ~HJRteComDrawCopyOESFBO() = default;
    virtual int init(HJBaseParam::Ptr i_param) override;
};
class HJRteComDrawGrayFBO : public HJRteComDrawFBO
{
public:
    HJ_DEFINE_CREATE(HJRteComDrawGrayFBO);
    HJRteComDrawGrayFBO();
    virtual ~HJRteComDrawGrayFBO() = default;
    
    virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void done() override;
};

class HJRteComDrawDenoiseFilter : public HJRteComDrawFBO
{
public:
    HJ_DEFINE_CREATE(HJRteComDrawDenoiseFilter);
    HJRteComDrawDenoiseFilter();
    virtual ~HJRteComDrawDenoiseFilter() = default;

    virtual int init(HJBaseParam::Ptr i_param) override;
    virtual void done() override;
    virtual int setParam(HJBaseParam::Ptr i_param) override;
    virtual int draw(GLuint textureId, int i_fitMode, int srcw, int srch, int dstw, int dsth, float* texMat = nullptr, bool i_bYFlip = false, bool i_bXFlip = false) override;

private:
    float priClamp01(float i_value) const;
    void priUpdateDenoiseParameters();

    std::shared_ptr<HJOGBaseShader> m_denoiseShader = nullptr;
    std::array<float, 18> m_denoiseOffsets = {};
    std::array<float, 9> m_denoiseSpatialWeights = {};
    float m_denoiseRangeFactor = 0.0f;
    float m_denoiseStrength = 1.0f;
};

class HJRteComDrawSRFilter : public HJRteComDrawFBO
{
public:
    HJ_DEFINE_CREATE(HJRteComDrawSRFilter);
    HJRteComDrawSRFilter();
    virtual ~HJRteComDrawSRFilter() = default;

    virtual int init(HJBaseParam::Ptr i_param) override;
    virtual void done() override;
    virtual int setParam(HJBaseParam::Ptr i_param) override;
    virtual int adjustResolution(int i_width, int i_height) override;
    virtual int bind() override;
    virtual int draw(GLuint textureId, int i_fitMode, int srcw, int srch, int dstw, int dsth, float* texMat = nullptr, bool i_bYFlip = false, bool i_bXFlip = false) override;
    void setScaleFactor(int i_scaleFactor);
    void setDynamicScaleRatio(bool i_bDynamicScaleRatio);

private:
    float priClamp01(float i_value) const;
    float priGetExtraSharpness() const;
    int priGetEffectiveScaleFactor() const;
    HJSizei priGetFallbackOutputSize(int i_srcWidth, int i_srcHeight) const;
    HJSizei priComputeTargetOutputSize(int i_srcWidth, int i_srcHeight, int i_viewportWidth, int i_viewportHeight, int i_renderMode) const;
    HJSizei priResolveOutputSizeFromTargets(int i_srcWidth, int i_srcHeight, bool& o_bFromTarget);
    int priEnsureIntermediateFbos(int i_dstWidth, int i_dstHeight);
    void priBuildFsrOriginConstants(unsigned int i_inputWidth, unsigned int i_inputHeight, unsigned int i_outputWidth, unsigned int i_outputHeight,
        std::array<uint32_t, 4>& o_easuCon0, std::array<uint32_t, 4>& o_easuCon1, std::array<uint32_t, 4>& o_easuCon2, std::array<uint32_t, 4>& o_easuCon3,
        std::array<uint32_t, 4>& o_rcasCon) const;

    std::shared_ptr<HJOGBaseShader> m_easuShader = nullptr;
    std::shared_ptr<HJOGBaseShader> m_rcasShader = nullptr;
    std::shared_ptr<HJOGBaseShader> m_sharpenShader = nullptr;
    std::shared_ptr<HJOGFBOCtrl> m_easuFbo = nullptr;
    std::shared_ptr<HJOGFBOCtrl> m_rcasFbo = nullptr;
    float m_sharpness = 1.0f;
    bool m_enableExtraSharpen = true;
    float m_extraSharpenBoost = 0.55f;
    float m_match = 1.0f;
    int m_inputWidth = 0;
    int m_inputHeight = 0;
    int m_scaleFactor = 2;
    bool m_bDynamicScaleRatio = true;
};

class HJRteComDrawXMirrorFBO : public HJRteComDrawFBO
{
public:
    HJ_DEFINE_CREATE(HJRteComDrawXMirrorFBO);
    HJRteComDrawXMirrorFBO();
    virtual ~HJRteComDrawXMirrorFBO() = default;
    
    virtual int init(HJBaseParam::Ptr i_param) override;
	//virtual void done() override;
};


class HJRteBlurHolder : public HJBaseSharedObject
{
public:
    HJ_DEFINE_CREATE(HJRteBlurHolder);
    HJRteBlurHolder() = default;
    virtual ~HJRteBlurHolder();
    HJRteBlurHolder(std::shared_ptr<HJOGFBOCtrl> i_fbo)
    {
        m_holdFbo = i_fbo;
    }
    std::shared_ptr<HJOGFBOCtrl> getFBOCtr();
private:
    std::shared_ptr<HJOGFBOCtrl> m_holdFbo = nullptr;
};

class HJRteComDrawBlurKernal : public HJBaseSharedObject
{
public:
    HJ_DEFINE_CREATE(HJRteComDrawBlurKernal);
    HJRteComDrawBlurKernal() = default;
    virtual ~HJRteComDrawBlurKernal() = default;
    
    int draw(GLuint textureId, int i_fitMode, int srcw, int srch, int dstw, int dsth, float* texMat = nullptr, bool i_bYFlip = false, bool i_bXFlip = false);

    std::shared_ptr<HJRteBlurHolder> getHolder();

private:
    HJRteBlurHolder::Ptr m_holder = nullptr;

    std::shared_ptr<HJOGBaseShader> m_pass1Shader = nullptr;
    std::shared_ptr<HJOGBaseShader> m_pass2Shader = nullptr;
};

class HJRteComDrawBlurCascadeFBO : public HJRteComDrawFBO
{
public:
    HJ_DEFINE_CREATE(HJRteComDrawBlurCascadeFBO);
    HJRteComDrawBlurCascadeFBO();
    virtual ~HJRteComDrawBlurCascadeFBO() = default;
    
    virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void done() override;
    virtual int draw(GLuint textureId, int i_fitMode, int srcw, int srch, int dstw, int dsth, float* texMat = nullptr, bool i_bYFlip = false, bool i_bXFlip = false) override;
    //virtual int adjustResolution(int i_width, int i_height) override;
    //virtual HJSizei getTargetWH() override; 
    //virtual GLuint getTextureId() override;
    //virtual int getWidth() override;
    //virtual int getHeight() override;
    void setCascadeNum(int i_num)
    {
        m_cascadeNum = i_num;
    }
    int getCascadeNum() const   
    {
        return m_cascadeNum;
    }
private:
    int m_cascadeNum = 3;
    std::vector<HJRteComDrawBlurKernal::Ptr> m_blurVector;
    std::shared_ptr<HJOGBaseShader> m_shader = nullptr;
};

class HJPBOReadWrapper;
class HJRteComDrawPBOFBO : public HJRteComDrawFBO
{
public:
    HJ_DEFINE_CREATE(HJRteComDrawPBOFBO);
    HJRteComDrawPBOFBO();
    virtual ~HJRteComDrawPBOFBO();

    virtual int init(HJBaseParam::Ptr i_param) override;
    virtual int unbind(bool i_bDraw) override;    

    void setReadCb(HJMediaDataReaderCb i_cb);
    //virtual int adjustResolution(int i_width, int i_height) override;
private:
    int priReadPBO();
    std::shared_ptr<HJPBOReadWrapper> m_pboReader = nullptr;
    HJMediaDataReaderCb m_readCb = nullptr;
    // int m_catchWidth = 0;
    // int m_catchHeight = 0;
};

class HJRteComDrawPBOFBODetect : public HJRteComDrawPBOFBO
{
public:
    HJ_DEFINE_CREATE(HJRteComDrawPBOFBODetect);
    HJRteComDrawPBOFBODetect();
    virtual ~HJRteComDrawPBOFBODetect() = default;
};
class HJRteComDrawPBOFBOTarget_0 : public HJRteComDrawPBOFBO
{
public:
    HJ_DEFINE_CREATE(HJRteComDrawPBOFBOTarget_0);
    HJRteComDrawPBOFBOTarget_0();
    virtual ~HJRteComDrawPBOFBOTarget_0() = default;
};
class HJRteComDrawPBOFBOTarget_1 : public HJRteComDrawPBOFBO
{
public:
    HJ_DEFINE_CREATE(HJRteComDrawPBOFBOTarget_1);
    HJRteComDrawPBOFBOTarget_1();
    virtual ~HJRteComDrawPBOFBOTarget_1() = default;
};
class HJRteComDrawPBOFBOTarget_2 : public HJRteComDrawPBOFBO
{
public:
    HJ_DEFINE_CREATE(HJRteComDrawPBOFBOTarget_2);
    HJRteComDrawPBOFBOTarget_2();
    virtual ~HJRteComDrawPBOFBOTarget_2() = default;
};
class HJRteComDrawPBOFBOTarget_3 : public HJRteComDrawPBOFBO
{
public:
    HJ_DEFINE_CREATE(HJRteComDrawPBOFBOTarget_3);
    HJRteComDrawPBOFBOTarget_3();
    virtual ~HJRteComDrawPBOFBOTarget_3() = default;
};

using HJCustomSourceFilterInit = std::function<int ()>; 
using HJCustomSourceFilterDraw = std::function<int (GLuint i_inTextureId, GLuint& outTextureId, int i_srcWidth, int i_srcHeight)>;
using HJCustomSourceFilterRelease = std::function<int ()>;

class HJRteComCustomSourceFilter : public HJRteComDrawFBO
{ 
public:
    HJ_DEFINE_CREATE(HJRteComCustomSourceFilter);
    HJRteComCustomSourceFilter();
    virtual ~HJRteComCustomSourceFilter();

    virtual int init(HJBaseParam::Ptr i_param) override;

    virtual int adjustResolution(int i_width, int i_height) override;

    virtual HJSizei getTargetWH() override;
    virtual int bind() override;
    virtual int unbind(bool i_bDraw) override;
    virtual int draw(GLuint textureId, int i_fitMode, int srcw, int srch, int dstw, int dsth, float* texMat = nullptr, bool i_bYFlip = false, bool i_bXFlip = false) override;
    virtual GLuint getTextureId() override;
    virtual int getWidth() override;
    virtual int getHeight() override;

private:
    int m_customWidth = 0;
    int m_customHeight = 0;
    GLuint m_outTextureId = 0;

    HJCustomSourceFilterInit m_initFunc = nullptr;
    HJCustomSourceFilterDraw m_drawFunc = nullptr;
    HJCustomSourceFilterRelease m_releaseFunc = nullptr;
};

NS_HJ_END



