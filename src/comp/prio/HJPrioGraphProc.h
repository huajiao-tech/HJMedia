#pragma once

#include "HJPrioGraphBaseEGL.h"
#include "HJMediaData.h"
#include "HJFacePointMgr.h"

NS_HJ_BEGIN

class HJPrioComSourceSeries;
class HJPrioComGiftSeq;
class HJOGRenderWindowBridge;
class HJPrioComSourceBridge;
class HJFacePointMgr;

class HJPrioGraphProc : public HJPrioGraphBaseEGL
{
public:
    HJ_DEFINE_CREATE(HJPrioGraphProc);
    HJPrioGraphProc();
    virtual ~HJPrioGraphProc();
    
	virtual int init(HJBaseParam::Ptr i_param);
	virtual void done();
    void setFaceInfo(HJFacePointsWrapper::Ptr faceInfo);
#if defined(HarmonyOS)
    std::shared_ptr<HJOGRenderWindowBridge> renderWindowBridgeAcquire();
    std::shared_ptr<HJOGRenderWindowBridge> renderWindowBridgeAcquireSoft();
#endif
    int openPNGSeq(HJBaseParam::Ptr i_param);
    void setDoubleScreen(bool i_bDoubleScreen, bool i_bLandscape);
    void setGiftPusher(bool i_bGiftPusher);
       
    void openEffect(HJBaseParam::Ptr i_param);
    void closeEffect(HJBaseParam::Ptr i_param);
    
    int openFaceMgr();
    void closeFaceMgr();
    
    void openPBO(HJMediaDataReaderCb i_cb);
    void closePBO();
    
    int openFaceu(HJBaseParam::Ptr i_param);
    void closeFaceu();

    //void registerFaceCom(std::shared_ptr<HJPrioCom> i_com);
    void registSubscriber(HJFaceSubscribeFuncPtr i_subscriberPtr);

    void setFaceProtected(bool i_bProtected);
private:
    int priOpenEffect(HJBaseParam::Ptr i_param);
    int priCloseEffect(HJBaseParam::Ptr i_param);

    std::shared_ptr<HJPrioComSourceBridge> m_videoCapture = nullptr;
    std::shared_ptr<HJFacePointMgr> m_faceMgr = nullptr;
    int m_pngSegIdx = 0;
    std::weak_ptr<HJPrioComGiftSeq> m_pngseqWtr;
    bool m_bFaceProtected = false;
    HJFaceStatus m_faceStatus = HJFaceStatus_Unknown;
};

NS_HJ_END



