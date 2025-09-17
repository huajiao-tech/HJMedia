#pragma once

#include "HJComObject.h"
#include "HJGraphComObject.h"

NS_HJ_BEGIN

class HJBaseCodec;
class HJOGRenderWindowBridge;
class HJMediaFrame;
class HJVidKeyStrategy;

class HJComVideoDecoder : public HJBaseComAsyncThread
{
public:
    HJ_DEFINE_CREATE(HJComVideoDecoder);
    HJComVideoDecoder();
    virtual ~HJComVideoDecoder();
    
	virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void done() override;
        
    static int s_maxMediaSize;
protected:
	virtual int run() override;    
private:
    int priResetCodec(const std::shared_ptr<HJMediaFrame>& i_frame);
    int priTryDrainOuter();
    int priTryInput();
   
    std::shared_ptr<HJOGRenderWindowBridge> m_bridge = nullptr;
    std::shared_ptr<HJBaseCodec> m_baseDecoder = nullptr;
    int m_HJDeviceType = 0;
    bool m_bReady = false;
    std::shared_ptr<HJVidKeyStrategy> m_keyStrategy = nullptr;
    bool m_bUseKeyStrategy = false;
};

NS_HJ_END



