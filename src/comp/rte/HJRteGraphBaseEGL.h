#pragma once

#include "HJPrerequisites.h"
#include "HJRteGraph.h"

#if defined(HarmonyOS)
    #include <EGL/egl.h>
#else
    typedef void* EGLSurface;
#endif

NS_HJ_BEGIN

class HJOGRenderEnv;
class HJOGEGLSurface;
class HJRteGraphBaseEGL : public HJRteGraphTimer
{
public:
    HJ_DEFINE_CREATE(HJRteGraphBaseEGL);
    HJRteGraphBaseEGL();
    virtual ~HJRteGraphBaseEGL();
    
	virtual int init(HJBaseParam::Ptr i_param);
	virtual void done();
#if defined(HarmonyOS)
    virtual int procSurface(const std::shared_ptr<HJOGEGLSurface> & i_eglSurface, int i_targetState)
    {
        return 0;
    }
#endif
    const std::shared_ptr<HJOGRenderEnv>& getRenderEnv()
    {
        return m_renderEnv;
    }
    int eglSurfaceProc(const std::string &i_renderTargetInfo, std::shared_ptr<HJOGEGLSurface> & o_eglSurface);
protected:
	//virtual int run() override;    
    
private:
    void priStatFrameRate();
    
    std::shared_ptr<HJOGRenderEnv> m_renderEnv = nullptr;
    
    int64_t m_statIdx = 0;
    int64_t m_statFirstTime = 0;
    int64_t m_statPreTime = 0;
};

NS_HJ_END



