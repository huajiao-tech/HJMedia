#pragma once

#include "HJComObject.h"
#include "HJGraphComObject.h"
#include "HJVEncOHCodec.h"

NS_HJ_BEGIN

typedef struct HOVideoHWEncoderInfo
{
    std::string m_codecName = "h264";
    int m_nWidth = 0;
    int m_nHeight = 0;
    int m_nBitRate = 0;
    int m_nFrameRate = 0;
    int m_nKeyInterval = 0;
} HOVideoHWEncoderInfo;

class HJComVideoEnc : public HJBaseComAsyncThread
{
public:
    HJ_DEFINE_CREATE(HJComVideoEnc);
    HJComVideoEnc();
    virtual ~HJComVideoEnc();
    
	virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void done() override;
    
protected:
	virtual int run() override;    
private:
//    HOVideoHWEncoder::Ptr m_videoRTMPEncoder = nullptr;
//    HOVideoHWEncoderInfo m_videoInfo;
//    HOVideoHWEncoder::HOVideoSurfaceCb m_surfaceCb = nullptr;
    NativeWindow *m_nativeWindow = nullptr;
    HJVEncOHCodec::Ptr m_encoder = nullptr;
    HOVideoSurfaceCb m_surfaceCb = nullptr;
    HJVideoInfo::Ptr m_videoInfo = nullptr;
};

NS_HJ_END



