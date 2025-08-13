//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once

#include "HJBaseCapture.h"
#include "HJDataDequeue.h"
#include "HJBuffer.h"
#include "ohaudio/native_audiocapturer.h"
#include "ohaudio/native_audiostreambuilder.h"
#include "ohaudio/native_audiostream_base.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJACaptureOH : public HJBaseCapture
{
public:
    using Ptr = std::shared_ptr<HJACaptureOH>;
    HJACaptureOH();
    virtual ~HJACaptureOH();
    
    virtual int init(const HJStreamInfo::Ptr& info) override;
    virtual int getFrame(HJMediaFrame::Ptr& frame)  override;
    
    void setMute(bool i_bMute);
private:
    static int32_t OnReadDataStatic(OH_AudioCapturer *capturer, 
                                   void *userData, 
                                   void *buffer, 
                                   int32_t bufferLen);
    int OnReadData(unsigned char *buffer, int bufferLen);
    
    OH_AudioCapturer* m_capturer = nullptr;
    OH_AudioStreamBuilder* m_builder = nullptr;
    HJSpDataDeque<HJMediaFrame::Ptr> m_outputQueue;
    std::atomic<bool> m_bMute{false};
    HJRunnable m_newBufferCb = nullptr;
};

NS_HJ_END

