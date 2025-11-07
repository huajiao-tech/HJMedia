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
class HJOHAEncoder : public HJBaseCodec
{
public:
	HJ_DECLARE_PUWTR(HJOHAEncoder);
	HJOHAEncoder();
	virtual ~HJOHAEncoder();

    virtual int init(const HJStreamInfo::Ptr& info) override;
    virtual int getFrame(HJMediaFrame::Ptr& frame) override;
    virtual int run(const HJMediaFrame::Ptr frame) override;
    virtual int flush() override;
    virtual void done() override;

    int64_t getFirstTime() const { return m_firstTime; }
    int64_t getFirstOutTime() const { return m_firstOutTime; }
    void setFirstOutTime(int64_t t) { m_firstOutTime = t; }
private:
    static void OnError(OH_AVCodec *codec, int32_t errorCode, void *userData);
    static void OnStreamChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData);
    static void OnNeedInputBuffer(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData);
    static void OnNewOutputBuffer(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData);

    static HJMediaFrame::Ptr makeFrame(OH_AVBuffer *buffer, const HJAudioInfo::Ptr& info, const int64_t firstTime, int64_t& outTime);
private:
    OH_AVCodec*             m_encoder = nullptr;
    HJOHACodecUserData::Ptr m_userData = nullptr;
    bool                    m_isFirstFrame = true;
    int64_t                 m_firstTime = HJ_NOTS_VALUE;
    int64_t                 m_firstOutTime = HJ_NOTS_VALUE;
};

NS_HJ_END