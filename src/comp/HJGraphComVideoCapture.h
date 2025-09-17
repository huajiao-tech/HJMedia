#pragma once

#include "HJPrioGraphBaseEGL.h"
#include "HJNotify.h"

NS_HJ_BEGIN

class HJPrioComSourceBridge;
class HJPrioComGiftSeq;
class HJOGRenderWindowBridge;

class HJGraphComVideoCapture : public HJPrioGraphBaseEGL
{
public:
    HJ_DEFINE_CREATE(HJGraphComVideoCapture);
    HJGraphComVideoCapture();
    virtual ~HJGraphComVideoCapture();
    
	virtual int init(HJBaseParam::Ptr i_param);
	virtual void done();
    
#if defined(HarmonyOS)
    std::shared_ptr<HJOGRenderWindowBridge> renderWindowBridgeAcquire();
#endif
    int openPNGSeq(HJBaseParam::Ptr i_param);
    void setDoubleScreen(bool i_bDoubleScreen, bool i_bLandscape);
    void setGiftPusher(bool i_bGiftPusher);
    //int openAlphaVideo(HJBaseParam::Ptr i_param);
    
private:
    std::shared_ptr<HJPrioComSourceBridge> m_videoCapture = nullptr;
    int m_pngSegIdx = 0;
    HJListener m_renderListener = nullptr;
    std::weak_ptr<HJPrioComGiftSeq> m_pngseqWtr;
};

NS_HJ_END



