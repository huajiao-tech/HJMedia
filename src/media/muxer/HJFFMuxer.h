//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJMediaFrame.h"
#include "HJBaseMuxer.h"
#include "HJBaseCodec.h"

typedef struct AVFormatContext AVFormatContext;
typedef struct AVDictionary AVDictionary;

NS_HJ_BEGIN
//***********************************************************************************//
class HJFFMuxer : public HJBaseMuxer
{
public:
    using Ptr = std::shared_ptr<HJFFMuxer>;
	HJFFMuxer();
    HJFFMuxer(HJListener listener);
	virtual ~HJFFMuxer();

	virtual int init(const HJMediaInfo::Ptr& mediaInfo, HJOptions::Ptr opts = nullptr) override;
    virtual int init(const std::string url, int mediaTypes = HJMEDIA_TYPE_AV, HJOptions::Ptr opts = nullptr) override;
    virtual int addFrame(const HJMediaFrame::Ptr& frame) override;
	virtual int writeFrame(const HJMediaFrame::Ptr& frame) override;
	virtual void done() override;
    //
    const size_t& getVideoFrameCnt() {
        return m_videoFrameCnt;
    }
    const size_t& getAudioFrameCnt() {
        return m_audioFrameCnt;
    }
private:
    int deliver(const HJMediaFrame::Ptr& frame, bool isDup = false);
    int procFrame(const HJMediaFrame::Ptr& frame, bool isDup = false);
    int gatherMediaInfo(const HJMediaFrame::Ptr& frame);
    int64_t waitStartDTSOffset(HJMediaFrame::Ptr frame);
    //
    int createAVIO();
    void destroyAVIO();
    HJBaseCodec::Ptr getEncoder(const HJMediaType mediaType);
    AVDictionary* getFormatOptions();
private:
    AVFormatContext*        m_ic = NULL;
	HJTrackHolderMap		m_tracks;
	HJNipMuster::Ptr        m_nipMuster = nullptr;
    int64_t                 m_lastVideoDTS = HJ_INT64_MIN;
	int64_t					m_lastAudioDTS = HJ_INT64_MIN;
    size_t                  m_videoFrameCnt = 0;
    size_t                  m_audioFrameCnt = 0;
    bool				    m_firstKeyFrame = false;
    //
    std::deque<HJMediaFrame::Ptr>	m_framesQueue;
    int64_t					m_startDTSOffset = HJ_NOTS_VALUE;
};

NS_HJ_END

