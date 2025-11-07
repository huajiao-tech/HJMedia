#pragma once

#include "HJPrerequisites.h"
#include "HJRteGraph.h"
#include "HJRteGraphBaseEGL.h"

NS_HJ_BEGIN

class HJOGRenderWindowBridge;
class HJRteComSourceBridge;
class HJRteComLinkInfo;
class HJRteCom;

class HJRteGraphProc : public HJRteGraphBaseEGL
{
public:
    HJ_DEFINE_CREATE(HJRteGraphProc);

    HJRteGraphProc();
    virtual ~HJRteGraphProc();

    virtual int init(std::shared_ptr<HJBaseParam> i_param) override;
    virtual void done() override;
    
#if defined(HarmonyOS)
    virtual int procSurface(const std::shared_ptr<HJOGEGLSurface> & i_eglSurface, int i_targetState) override;

    std::shared_ptr<HJOGRenderWindowBridge> renderWindowBridgeAcquire();
    std::shared_ptr<HJOGRenderWindowBridge> renderWindowBridgeAcquireSoft();
#endif
    int openPNGSeq(HJBaseParam::Ptr i_param);
    void setDoubleScreen(bool i_bDoubleScreen, bool i_bLandscape);
    void setGiftPusher(bool i_bGiftPusher);

    void openEffect(HJBaseParam::Ptr i_param);
    void closeEffect(HJBaseParam::Ptr i_param);

    void graphProcAsync(std::function<int()> i_func);
    int connectCom(std::shared_ptr<HJRteCom> i_srcCom, std::shared_ptr<HJRteCom> i_dstCom, std::shared_ptr<HJOGBaseShader> i_shader = nullptr, std::shared_ptr<HJRteComLinkInfo> i_linkInfo = nullptr);
    void addSource(std::shared_ptr<HJRteCom> i_com);
    void addTarget(std::shared_ptr<HJRteCom> i_com);
    void addFilter(std::shared_ptr<HJRteCom> i_com);
protected:
    virtual int run() override;
private:
    int priConnect(std::shared_ptr<HJRteCom> i_srcCom, std::shared_ptr<HJRteCom> i_dstCom, std::shared_ptr<HJOGBaseShader> i_shader = nullptr, std::shared_ptr<HJRteComLinkInfo> i_linkInfo = nullptr);
    int testNotifyAndTryRender(std::shared_ptr<HJRteCom> i_header);
    int testRender(std::shared_ptr<HJRteCom> i_com);
    
    std::shared_ptr<HJRteComSourceBridge> m_videoSource = nullptr; 
};


NS_HJ_END



