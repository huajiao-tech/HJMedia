#pragma once

#include "HJComObject.h"
#include "HJGraphComObject.h"

NS_HJ_BEGIN

class HJOGRenderWindowBridge;
class HJSysTimerSync;
class HJMediaFrame;

class HJComVideoRender : public HJBaseComAsyncThread
{
public:
    HJ_DEFINE_CREATE(HJComVideoRender);
    HJComVideoRender();
    virtual ~HJComVideoRender();
    
	virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void done() override;
    virtual void join() override;

    static int s_maxMediaSize;
protected:
	virtual int run() override;    
private:
    int priTryDraw(int64_t &o_sleepTime);

    int m_HJDeviceType = 0;
    std::shared_ptr<HJOGRenderWindowBridge> m_bridge = nullptr;
    std::shared_ptr<HJOGRenderWindowBridge> m_softBridge = nullptr;
    bool m_bEnableSoft = true;
    std::atomic<bool> m_bQuit{false};

    std::shared_ptr<HJMediaFrame> m_curFrame = nullptr;
    std::shared_ptr<HJSysTimerSync> m_timeSync = nullptr;
    
    bool m_bSoftBridgeTest = true;
};

NS_HJ_END



