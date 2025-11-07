#pragma once


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
    using customDrawFun = std::function<int(HJRteDriftInfo::Ptr i_driftInfo, HJRteDriftInfo::Ptr& o_driftInfo)>;
    
    HJ_DEFINE_CREATE(HJRteComDraw);
    HJRteComDraw();
    virtual ~HJRteComDraw();
        
	virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void done() override;


    virtual HJSizei getTargetWH();    
    static std::string mapWindowRenderMode(HJRteWindowRenderMode i_windowRenderMode);
    
    int render(HJRteDriftInfo::Ptr& o_param);
    
    void setCustomDraw(customDrawFun i_func)
    {
        m_customDraw = i_func;
    }
    
protected:
    virtual int attach();
    virtual int detach();
private:
    customDrawFun m_customDraw = nullptr;
};

class HJRteComDrawEGL : public HJRteComDraw
{
public:
    HJ_DEFINE_CREATE(HJRteComDrawEGL);
    HJRteComDrawEGL();
    virtual ~HJRteComDrawEGL() = default;
        
	virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void done() override;
        
    virtual void reset() override;  
    virtual HJSizei getTargetWH() override;   
#if defined(HarmonyOS)
    void setSurface(const std::shared_ptr<HJOGEGLSurface> & i_eglSurface);
#endif 

    virtual int bindEx() override;
    virtual int unbindEx() override;
    virtual int renderEx(const std::shared_ptr<HJRteComLink>&i_link, HJRteDriftInfo::Ptr& o_driftInfo) override;
    
protected:
    virtual int attach() override;
    virtual int detach() override;
    std::weak_ptr<HJOGEGLSurface> m_eglSurfaceWtr;
private:
    int m_curRenderIdx = 0;
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
    
    virtual int bindEx() override;
    virtual int unbindEx() override;
    virtual int renderEx(const std::shared_ptr<HJRteComLink>&i_link, HJRteDriftInfo::Ptr& o_driftInfo) override;

    virtual int drawEx(GLuint textureId, const std::string& i_fitMode, int srcw, int srch, int dstw, int dsth, float* texMat = nullptr, bool i_bYFlip = false, bool i_bXFlip = false)
    {
        return HJ_OK;
    }

    virtual int adjustResolution(int i_width, int i_height);
    
    int check(int i_width, int i_height, bool i_bTransparency = true);
    //int tryRestart(int i_width, int i_height, bool i_bTransparency = true);

    void setAdjustResolution(bool i_bAdjustResolution)
    {
        m_isAdjustResolution = i_bAdjustResolution;
    }
    bool isAdjustResolution() const
    {
        return m_isAdjustResolution;
    }

#if defined(HarmonyOS)
    GLuint getTextureId();
#endif
    const std::shared_ptr<HJOGBaseShader> & getShader() const 
    {
        return m_endShader;
    }

    static int tryRestartFbo(std::shared_ptr<HJOGFBOCtrl>& o_fbo, int i_width, int i_height, bool i_bTranspareny = true);
    
protected:
    virtual int attach() override;
    virtual int detach() override;
    std::shared_ptr<HJOGBaseShader> m_endShader = nullptr;
    std::shared_ptr<HJOGFBOCtrl> m_endFbo = nullptr;
private:
    bool m_isBinded = false;
    bool m_isAdjustResolution = true;
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

class HJRteComDrawBlurFBO : public HJRteComDrawFBO
{
public:
    HJ_DEFINE_CREATE(HJRteComDrawBlurFBO);
    HJRteComDrawBlurFBO();
    virtual ~HJRteComDrawBlurFBO() = default;
    
    virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void done() override;
    virtual int drawEx(GLuint textureId, const std::string& i_fitMode, int srcw, int srch, int dstw, int dsth, float* texMat = nullptr, bool i_bYFlip = false, bool i_bXFlip = false) override;
    virtual int adjustResolution(int i_width, int i_height) override;
private:
    //int priTryRestartFbo(int i_width, int i_height, std::shared_ptr<HJOGFBOCtrl>& o_fbo);
    std::shared_ptr<HJOGFBOCtrl> m_pass1Fbo = nullptr;
    std::shared_ptr<HJOGBaseShader> m_pass1Shader = nullptr;
    std::shared_ptr<HJOGBaseShader> m_pass2Shader = nullptr;
};




NS_HJ_END



