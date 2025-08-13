//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJMediaFrame.h"
#include "HJBaseCodec.h"
#include "aacenc_lib.h"
#include "HJDataDequeue.h"
#include "HJBuffer.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJAEncFDKAAC : public HJBaseCodec
{
public:
    using Ptr = std::shared_ptr<HJAEncFDKAAC>;
    HJAEncFDKAAC();
    virtual ~HJAEncFDKAAC();
    
    virtual int init(const HJStreamInfo::Ptr& info) override;
    virtual int getFrame(HJMediaFrame::Ptr& frame) override;
    virtual int run(const HJMediaFrame::Ptr frame) override;
    
private:
    static int s_bytesPerSample;

    HANDLE_AACENCODER  m_aacHandle = nullptr;

    int m_inElemSize = 0;
    int m_inIdentifier = IN_AUDIO_DATA;
    int m_outIdentifier = OUT_BITSTREAM_DATA;
    int m_inSize = 0;
    unsigned char m_outBuffer[1024 * 1024];
    int m_outSize = 0;
    int m_outElemSize = 0;
    bool m_bSendHeader = false;    
    int m_samplePerFrame = 1024;
    HJBuffer::Ptr m_headerBuffer = nullptr;
    HJSpDataDeque<HJMediaFrame::Ptr> m_outputQueue;
    HJCodecParameters::Ptr m_keyCodecParams = nullptr;
    
    int64_t m_statIdx = 0;
    int64_t m_statSum = 0;
    
    bool m_bADTS = false;
};

NS_HJ_END
