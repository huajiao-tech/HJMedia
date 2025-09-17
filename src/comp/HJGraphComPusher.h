#pragma once

#include "HJGraphComObject.h"

NS_HJ_BEGIN

class HJComVideoEncoder;
class HJComAudioCapture;
class HJComAudioFrameCombine;
class HJComAudioEncoder;
class HJComInterleave;
class HJComRTMP;

class HJGraphComPusher : public HJBaseGraphComAsyncThread
{
public:
    HJ_DEFINE_CREATE(HJGraphComPusher);
    HJGraphComPusher();
    virtual ~HJGraphComPusher();
    
    
	virtual int init(HJBaseParam::Ptr i_param);
	virtual void done();
    void setMute(bool i_mute);
    
    void removeInterleave();
    int openRecorder(HJBaseParam::Ptr i_param);
    void closeRecorder();
private:
    int priConnect();
  
    std::shared_ptr<HJComVideoEncoder> m_videoEnc = nullptr;
    std::shared_ptr<HJComAudioCapture> m_audioCapture = nullptr;
    std::shared_ptr<HJComAudioFrameCombine> m_audioFrameCombine = nullptr;
    std::shared_ptr<HJComAudioEncoder> m_audioEnc = nullptr;
    std::shared_ptr<HJComInterleave> m_interleave = nullptr;
    std::shared_ptr<HJComRTMP>     m_rtmp = nullptr;

};

NS_HJ_END



