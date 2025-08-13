#pragma once


#include "HJComObject.h"
#include "HJOGRenderEnv.h"

NS_HJ_BEGIN

class HJComVideoCapture : public HJBaseRenderCom
{
public:
    HJ_DEFINE_CREATE(HJComVideoCapture);
    HJComVideoCapture();
    virtual ~HJComVideoCapture();
        
	virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void done() override;
    
    HJOGRenderWindowBridge::Ptr renderWindowBridgeAcquire();
protected:
    virtual int  doRenderUpdate() override;
    virtual int  doRenderDraw(int i_targetWidth, int i_targetHeight) override;
    HJOGRenderWindowBridge::Ptr m_bridge = nullptr;
private:
    
};

NS_HJ_END



