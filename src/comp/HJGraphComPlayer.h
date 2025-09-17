#pragma once

#include "HJGraphComObject.h"

NS_HJ_BEGIN

class HJComDemuxer;
class HJComVideoDecoder;
class HJComVideoRender;
class HJMediaInfo;
class HJOGRenderWindowBridge;

class HJGraphComPlayer : public HJBaseGraphComAsyncThread
{
public:
    HJ_DEFINE_CREATE(HJGraphComPlayer);
    HJGraphComPlayer();
    virtual ~HJGraphComPlayer();
    
    
	virtual int init(HJBaseParam::Ptr i_param);
	virtual void done();

private:
    int priReady(std::shared_ptr<HJMediaInfo> i_mediaInfo);
    std::shared_ptr<HJComDemuxer> m_demuxer = nullptr;
    std::shared_ptr<HJComVideoDecoder> m_videoDecoder = nullptr;
    std::shared_ptr<HJComVideoRender> m_videoRender = nullptr;
    HJBaseNotify m_notify = nullptr;
    
    std::shared_ptr<HJOGRenderWindowBridge> m_bridge = nullptr;
    std::shared_ptr<HJOGRenderWindowBridge> m_softBridge = nullptr;
    int m_HJDeviceType = 0;
};

NS_HJ_END



