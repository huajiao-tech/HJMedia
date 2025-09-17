#pragma once

#include "HJPrioComSourceBridge.h"
#if defined(HarmonyOS)
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#endif
NS_HJ_BEGIN
class HJOGCopyShaderStrip;
class HJPrioComFBOBase;
class HJPrioComFBOBridgeSrcEx : public HJPrioComSourceBridge
{
public:
    HJ_DEFINE_CREATE(HJPrioComFBOBridgeSrcEx);
    HJPrioComFBOBridgeSrcEx();
    virtual ~HJPrioComFBOBridgeSrcEx();
     
    virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void done() override;
    virtual int update(HJBaseParam::Ptr i_param) override;
    virtual int render(HJBaseParam::Ptr i_param) override;
#if defined(HarmonyOS) 
    int width();
    int height();
    float *getMatrix();
    GLuint texture();
#endif
private:
    int priRender();
    std::shared_ptr<HJPrioComFBOBase> m_fbo = nullptr;
    std::shared_ptr<HJOGCopyShaderStrip> m_draw = nullptr;
};

NS_HJ_END



