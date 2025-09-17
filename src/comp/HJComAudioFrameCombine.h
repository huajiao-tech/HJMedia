#pragma once

#include "HJComObject.h"
#include "HJAudioFifo.h"

NS_HJ_BEGIN

class HJComAudioFrameCombine : public HJBaseComSync
{
public:
    HJ_DEFINE_CREATE(HJComAudioFrameCombine);
    HJComAudioFrameCombine();
    virtual ~HJComAudioFrameCombine();
    
	virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void done() override;
protected:
    virtual int doSend(HJComMediaFrame::Ptr i_frame);
private:
    HJAudioFifo::Ptr m_audiofifo = nullptr;
    HJAudioInfo::Ptr m_audioInfo = nullptr;
};

NS_HJ_END



