#pragma once

#include "HJPrerequisites.h"
#include "HJComObject.h"
#include "HJACaptureOH.h"
#include "HJMediaUtils.h"

NS_HJ_BEGIN

class HJComAudioCapture : public HJBaseComAsyncThread
{
public:
    HJ_DEFINE_CREATE(HJComAudioCapture);
    HJComAudioCapture();
    virtual ~HJComAudioCapture();
    
	virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void done() override;
    
    void setMute(bool i_bMute);
    
protected:
	virtual int run();
private:
    HJACaptureOH::Ptr m_capture = nullptr;
    HJAudioInfo::Ptr m_audioInfo = nullptr;
};

NS_HJ_END



