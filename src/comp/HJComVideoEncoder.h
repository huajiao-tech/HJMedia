#pragma once

#include "HJComObject.h"
#include "HJGraphComObject.h"

NS_HJ_BEGIN

typedef struct NativeWindow NativeWindow;
class HJBaseCodec;
class HJVideoInfo;

typedef struct HOVideoHWEncoderInfo
{
    std::string m_codecName = "h264";
    int m_nWidth = 0;
    int m_nHeight = 0;
    int m_nBitRate = 0;
    int m_nFrameRate = 0;
    int m_nKeyInterval = 0;
} HOVideoHWEncoderInfo;

class HJComVideoEncoder : public HJBaseComAsyncThread
{
public:
    HJ_DEFINE_CREATE(HJComVideoEncoder);
    HJComVideoEncoder();
    virtual ~HJComVideoEncoder();
    
	virtual int init(HJBaseParam::Ptr i_param) override;
	virtual void done() override;
    
protected:
	virtual int run() override;    
private:
    NativeWindow *m_nativeWindow = nullptr;
    std::shared_ptr<HJBaseCodec> m_baseEncoder = nullptr;
    HOVideoSurfaceCb m_surfaceCb = nullptr;
    std::shared_ptr<HJVideoInfo> m_videoInfo = nullptr;
};

NS_HJ_END



