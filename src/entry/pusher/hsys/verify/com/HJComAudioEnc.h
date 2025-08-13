#pragma once

#include "HJComObject.h"
#include "HJAEncFDKAAC.h"

NS_HJ_BEGIN

class HJComAudioEnc : public HJBaseComSync
{
public:
    HJ_DEFINE_CREATE(HJComAudioEnc);
    HJComAudioEnc();
    virtual ~HJComAudioEnc();
    
	virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void done() override;
    virtual int doSend(HJComMediaFrame::Ptr i_frame) override;
    
private:
    HJAEncFDKAAC::Ptr m_encoder = nullptr;
    HJAudioInfo::Ptr m_audioInfo = nullptr;
};

NS_HJ_END



