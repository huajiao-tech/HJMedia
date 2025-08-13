//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJMediaFrame.h"
#include "HJBaseCodec.h"
#include "HJBSFParser.h"
#include "multimedia/player_framework/native_avcodec_videoencoder.h"
#include "multimedia/player_framework/native_avbuffer_info.h"
#include <native_window/external_window.h>
#include "HJDataDequeue.h"
#include "HJBuffer.h"

NS_HJ_BEGIN

class HJVEncOHCodecInfo
{
public:
    using Ptr = std::shared_ptr<HJVEncOHCodecInfo>;
    HJVEncOHCodecInfo()
    {
        
    }
    virtual ~HJVEncOHCodecInfo()
    {
        
    }
    int64_t m_timestamp = 0;    
    uint32_t m_index = 0;
    OH_AVBuffer *m_buffer = nullptr;
};

//***********************************************************************************//
class HJVEncOHCodec : public HJBaseCodec
{
public:
    using Ptr = std::shared_ptr<HJVEncOHCodec>;
    HJVEncOHCodec();
    virtual ~HJVEncOHCodec();
    
    virtual int init(const HJStreamInfo::Ptr& info) override;
    virtual int getFrame(HJMediaFrame::Ptr& frame) override;
    int adjustBitrate(int i_newBitrate);
    int setEof();
    int64_t getEncIdx();
    
    NativeWindow *getNativeWindow();
private:
    static void OnCodecError(OH_AVCodec *codec, int32_t errorCode, void *userData);
    static void OnCodecFormatChange(OH_AVCodec *codec, OH_AVFormat *format, void *userData);
    static void OnNeedInputBuffer(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData);
    static void OnNewOutputBuffer(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData);

    int priStart();
    void priOnCodecError(OH_AVCodec *codec, int32_t errorCode);
    void priOnCodecFormatChange(OH_AVCodec *codec, OH_AVFormat *format);
    void priOnNewOutputBuffer(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer); 
    static std::string s_h264mime; 
    static std::string s_h265mime;
      
    HJBuffer::Ptr m_headerBuf = nullptr;
    OH_AVCodec *m_encoder = nullptr;
    NativeWindow *m_nativeWindow = nullptr;
    HJSpDataDeque<HJVEncOHCodecInfo::Ptr> m_outputQueue;    
    HJCodecParameters::Ptr m_keyCodecParams = nullptr;
    HJRunnable m_newBufferCb = nullptr;
    bool m_bEof = false;
    int64_t m_index = 0;
};

NS_HJ_END
