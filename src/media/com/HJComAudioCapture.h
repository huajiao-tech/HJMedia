#pragma once

#include "HJPrerequisites.h"
#include "HJComObject.h"
#include "HJMediaUtils.h"

NS_HJ_BEGIN

class HJBaseCapture;
class HJAudioInfo;

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
    std::shared_ptr<HJBaseCapture> m_baseCapture = nullptr;
    std::shared_ptr<HJAudioInfo> m_audioInfo = nullptr;
};

NS_HJ_END



