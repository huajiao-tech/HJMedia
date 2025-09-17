#pragma once

#include "HJPrerequisites.h"
#include "HJRteGraph.h"
#include "HJRteGraphBaseEGL.h"

NS_HJ_BEGIN

class HJOGRenderWindowBridge;
class HJRteComSourceBridge;

class HJRteGraphProc : public HJRteGraphBaseEGL
{
public:
    HJ_DEFINE_CREATE(HJRteGraphProc);

    HJRteGraphProc();
    virtual ~HJRteGraphProc();

    virtual int init(std::shared_ptr<HJBaseParam> i_param) override;
    virtual void done() override;
    
#if defined(HarmonyOS)
    std::shared_ptr<HJOGRenderWindowBridge> renderWindowBridgeAcquire();
    std::shared_ptr<HJOGRenderWindowBridge> renderWindowBridgeAcquireSoft();
#endif
    int openPNGSeq(HJBaseParam::Ptr i_param);
    void setDoubleScreen(bool i_bDoubleScreen, bool i_bLandscape);
    void setGiftPusher(bool i_bGiftPusher);

    void openEffect(HJBaseParam::Ptr i_param);
    void closeEffect(HJBaseParam::Ptr i_param);

protected:
    virtual int run() override;
private:
    int testNotifyAndTryRender(std::shared_ptr<HJRteCom> i_header);
    int testRender(std::shared_ptr<HJRteCom> i_com);
    
    std::shared_ptr<HJRteComSourceBridge> m_videoSource = nullptr; 
};


NS_HJ_END



