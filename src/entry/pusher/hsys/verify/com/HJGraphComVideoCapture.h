#pragma once

#include "HJComVideoCapture.h"
#include "HJComVideoFilterPNGSeq.h"
#include "HJGraphBaseRender.h"
#include "HJNotify.h"

NS_HJ_BEGIN

class HJComVideoCapture;
class HJComVideoFilterPNGSeq;

class HJGraphComVideoCapture : public HJGraphBaseRender
{
public:
    HJ_DEFINE_CREATE(HJGraphComVideoCapture);
    HJGraphComVideoCapture();
    virtual ~HJGraphComVideoCapture();
    
	virtual int init(HJBaseParam::Ptr i_param);
	virtual void done();
    HJOGRenderWindowBridge::Ptr renderWindowBridgeAcquire();
    
    int openPNGSeq(HJBaseParam::Ptr i_param);
    //int openAlphaVideo(HJBaseParam::Ptr i_param);
    
private:
    std::shared_ptr<HJComVideoCapture> m_videoCapture = nullptr;
    int m_pngSegIdx = 0;
    HJListener m_renderListener = nullptr;
};

NS_HJ_END



