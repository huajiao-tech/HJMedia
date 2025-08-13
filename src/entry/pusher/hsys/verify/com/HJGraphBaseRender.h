#pragma once

#include "HJGraphComObject.h"
#include "HJOGRenderEnv.h"

NS_HJ_BEGIN

using HJGraphBaseRenderUpdate = std::function<int()>;
using HJGraphBaseRenderDraw   = std::function<int(int i_targetWidth, int i_targetHeight)>;

class HJGraphBaseRender : public HJBaseGraphComAsyncTimerThread
{
public:
    HJ_DEFINE_CREATE(HJGraphBaseRender);
    HJGraphBaseRender();
    virtual ~HJGraphBaseRender();
    
	virtual int init(HJBaseParam::Ptr i_param);
	virtual void done();
    int eglSurfaceProc(const std::string &i_renderTargetInfo);
   
    void setUpdateCb(HJGraphBaseRenderUpdate i_cb)
    {
        m_updateCb = i_cb;
    }
    void setDrawCb(HJGraphBaseRenderDraw i_cb)
    {
        m_drawCb = i_cb;
    }
    
    //HJOGRenderWindowBridge::Ptr renderWindowBridgeAcquire();
protected:
	virtual int run() override;    
    
private:
    int priDrawEveryTarget(EGLSurface i_eglSurface, int i_targetWidth, int i_targetHeight);
    void priStatFrameRate();
    
    HJOGRenderEnv::Ptr m_renderEnv = nullptr;
    HJGraphBaseRenderUpdate m_updateCb = nullptr;
    HJGraphBaseRenderDraw   m_drawCb = nullptr;
    
    int64_t m_statIdx = 0;
    int64_t m_statFirstTime = 0;
    int64_t m_statPreTime = 0;
    
    int64_t m_renderIdx = 0;
};

NS_HJ_END



