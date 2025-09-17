#pragma once

#include "HJPrioGraphBaseEGL.h"

NS_HJ_BEGIN

class HJPrioComSourceSeries;
class HJPrioComGiftSeq;
class HJOGRenderWindowBridge;
class HJPrioComSourceBridge;

class HJPrioGraphProc : public HJPrioGraphBaseEGL
{
public:
    HJ_DEFINE_CREATE(HJPrioGraphProc);
    HJPrioGraphProc();
    virtual ~HJPrioGraphProc();
    
	virtual int init(HJBaseParam::Ptr i_param);
	virtual void done();
    
#if defined(HarmonyOS)
    std::shared_ptr<HJOGRenderWindowBridge> renderWindowBridgeAcquire();
    std::shared_ptr<HJOGRenderWindowBridge> renderWindowBridgeAcquireSoft();
#endif
    int openPNGSeq(HJBaseParam::Ptr i_param);
    void setDoubleScreen(bool i_bDoubleScreen, bool i_bLandscape);
    void setGiftPusher(bool i_bGiftPusher);
       
    void openEffect(HJBaseParam::Ptr i_param);
    void closeEffect(HJBaseParam::Ptr i_param);
private:
    std::shared_ptr<HJPrioComSourceBridge> m_videoCapture = nullptr;
    int m_pngSegIdx = 0;
    std::weak_ptr<HJPrioComGiftSeq> m_pngseqWtr;
};

NS_HJ_END



