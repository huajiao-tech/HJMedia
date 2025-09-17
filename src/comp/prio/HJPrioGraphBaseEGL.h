#pragma once

#include "HJPrerequisites.h"
#include "HJPrioGraph.h"
#include "HJNotify.h"

#if defined(HarmonyOS)
    #include <EGL/egl.h>
#else
    typedef void* EGLSurface;
#endif

NS_HJ_BEGIN

class HJOGRenderEnv;

class HJPrioGraphBaseEGL : public HJPrioGraphTimer
{
public:
    HJ_DEFINE_CREATE(HJPrioGraphBaseEGL);
    HJPrioGraphBaseEGL();
    virtual ~HJPrioGraphBaseEGL();
    
	virtual int init(HJBaseParam::Ptr i_param);
	virtual void done();
    int eglSurfaceProc(const std::string &i_renderTargetInfo);
    
protected:
	virtual int run() override;    
    HJListener m_renderListener = nullptr;
private:
    void priStatFrameRate();
    bool m_bNotifyNeedSurface = true;
    std::shared_ptr<HJOGRenderEnv> m_renderEnv = nullptr;
    
    int64_t m_statIdx = 0;
    int64_t m_statFirstTime = 0;
    int64_t m_statPreTime = 0;
};

NS_HJ_END



