//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include <queue>
#include "HJMediaFrame.h"
#include "HJBaseCodec.h"
#include "HJOHCodecUtils.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJOHADecoder : public HJBaseCodec
{
public:
	HJ_DECLARE_PUWTR(HJOHADecoder);
	HJOHADecoder();
	virtual ~HJOHADecoder();

    virtual int init(const HJStreamInfo::Ptr& info) override;
    virtual int getFrame(HJMediaFrame::Ptr& frame) override;
    virtual int run(const HJMediaFrame::Ptr frame) override;
    virtual int flush() override;
    virtual bool checkFrame(const HJMediaFrame::Ptr frame) override;
    virtual int reset() override;
    virtual void done() override;

private:
    static void OnError(OH_AVCodec *codec, int32_t errorCode, void *userData);
    static void OnStreamChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData);
    static void OnNeedInputBuffer(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData);
    static void OnNewOutputBuffer(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData);

    static HJMediaFrame::Ptr makeFrame(OH_AVBuffer *buffer, const HJAudioInfo::Ptr& info);
private:
    OH_AVCodec*             m_decoder = nullptr;
    HJOHACodecUserData::Ptr m_userData = nullptr;
    // bool                        m_isChanged = false;
    // HJMediaFrame::Ptr           m_storeFrame = nullptr;
};

NS_HJ_END
