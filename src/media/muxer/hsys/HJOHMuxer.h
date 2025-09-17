//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJMediaFrame.h"
#include "HJBaseMuxer.h"
#include "HJBaseCodec.h"
#include "HJPrerequisites.h"
#include "HJMediaUtils.h"
#include "HJNotify.h"

typedef struct OH_AVMuxer OH_AVMuxer;
typedef struct OH_AVFormat OH_AVFormat;
typedef struct OH_AVBuffer OH_AVBuffer;

NS_HJ_BEGIN
//***********************************************************************************//
class HJOHMuxer : public HJBaseMuxer
{
public:
    using Ptr = std::shared_ptr<HJOHMuxer>;
    HJOHMuxer();
    HJOHMuxer(HJListener listener);
    virtual ~HJOHMuxer();

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
    int createMuxer();
    void destroyMuxer();
    int addTrack(const HJStreamInfo::Ptr& info);
    HJBaseCodec::Ptr getEncoder(const HJMediaType mediaType);
    OH_AVFormat* createFormat(const HJStreamInfo::Ptr& info);
    int writeSampleBuffer(const HJMediaFrame::Ptr& frame, int32_t trackId);
    
    static void OnMuxerError(OH_AVMuxer *muxer, int32_t errorCode, void *userData);
    void priOnMuxerError(OH_AVMuxer *muxer, int32_t errorCode);
private:
    OH_AVMuxer*             m_muxer = nullptr;
    int32_t                 m_videoTrackId = -1;
    int32_t                 m_audioTrackId = -1;
    HJTrackHolderMap        m_tracks;
    HJNipMuster::Ptr        m_nipMuster = nullptr;
    int64_t                 m_lastVideoDTS = HJ_INT64_MIN;
    int64_t                 m_lastAudioDTS = HJ_INT64_MIN;
    size_t                  m_videoFrameCnt = 0;
    size_t                  m_audioFrameCnt = 0;
    bool                    m_firstKeyFrame = false;
    bool                    m_muxerStarted = false;
    int                     m_outputFd = -1;
    //
    std::deque<HJMediaFrame::Ptr>   m_framesQueue;
    int64_t                 m_startDTSOffset = HJ_NOTS_VALUE;
};

NS_HJ_END