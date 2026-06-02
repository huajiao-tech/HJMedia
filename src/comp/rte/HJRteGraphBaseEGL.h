#pragma once

#include "HJPrerequisites.h"
#include "HJRteGraph.h"
#include "HJOGEGLSurface.h"
#include "HJNotify.h"

NS_HJ_BEGIN

class HJOGRenderEnv;
class HJOGEGLSurface;
class HJFBOCtrlPool;
class HJRteGraphBaseEGL : public HJRteGraphDrive
{
public:
    HJ_DEFINE_CREATE(HJRteGraphBaseEGL);
    HJRteGraphBaseEGL();
    virtual ~HJRteGraphBaseEGL();
    
	virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void done() override;
    const std::shared_ptr<HJOGRenderEnv>& getRenderEnv()
    {
        return m_renderEnv;
    }
    int eglSurfaceProc(const std::string &i_renderTargetInfo, std::shared_ptr<HJOGEGLSurface> & o_eglSurface);

    static std::shared_ptr<HJFBOCtrlPool>& getFBOCtrlPool()
    {
        return m_fbocPool;
    }

    int asyncClearOverride(HJThreadTaskFunc task, int i_id);
    int asyncOverride(HJThreadTaskFunc task, int64_t i_delayTime = 0);
    int syncOverride(HJThreadTaskFunc task);

protected:
	virtual int run() override;    
    
    virtual int runRender()
    {
        return 0;
    }
    virtual void priDoneOnGraphThread() {}

    void setCanRun(bool i_bCanRun)
    {
        m_bCanRun = i_bCanRun;
    }
    HJRenderMakeCurrent m_makeCurrentCb = nullptr;
private:
    void priStatFrameRate();
    bool m_bCanRun = false;
    std::shared_ptr<HJOGRenderEnv> m_renderEnv = nullptr;
    bool m_bNotifyNeedSurface = true;

    int64_t m_statFirstTime = 0;
    int64_t m_statPreTime = 0;
    int64_t m_statRenderSumTime = 0;
    int64_t m_statRenderIdx = 0;
    int64_t m_statCurElapseTime = 0;

    HJRenderInitCb m_renderInitCb = nullptr;
    HJRenderEveryStartCb m_renderEveryStartCb = nullptr;
    HJRenderEveryEndCb m_renderEveryEndCb = nullptr;
    thread_local static std::shared_ptr<HJFBOCtrlPool> m_fbocPool;
};

NS_HJ_END



