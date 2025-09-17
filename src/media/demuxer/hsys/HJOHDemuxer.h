//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJMediaFrame.h"
#include "HJBaseDemuxer.h"
#include "HJPrerequisites.h"
#include "HJMediaUtils.h"

typedef struct OH_AVDemuxer OH_AVDemuxer;
typedef struct OH_AVSource OH_AVSource;
typedef struct OH_AVBuffer OH_AVBuffer;
typedef struct OH_AVFormat OH_AVFormat;

NS_HJ_BEGIN
//***********************************************************************************//
class HJOHDemuxer : public HJBaseDemuxer
{
public:
    using Ptr = std::shared_ptr<HJOHDemuxer>;
    HJOHDemuxer();
    HJOHDemuxer(const HJMediaUrl::Ptr& mediaUrl);
    virtual ~HJOHDemuxer();
    
    virtual int init() override;
    virtual int init(const HJMediaUrl::Ptr& mediaUrl) override;
    virtual void done() override;
    virtual int seek(int64_t pos) override;     //ms
    virtual int getFrame(HJMediaFrame::Ptr& frame) override;
    virtual void reset() override;
    
    virtual int64_t getDuration() override;  //ms
private:
    int makeMediaInfo();
    HJMediaFrame::Ptr procEofFrame();
    int analyzeMediaFormat();
    int selectTracks();
    
    static void OnDemuxerError(OH_AVDemuxer *demuxer, int32_t errorCode, void *userData);
    static void OnDemuxerFormatChanged(OH_AVDemuxer *demuxer, OH_AVFormat *format, void *userData);
    
    void priOnDemuxerError(OH_AVDemuxer *demuxer, int32_t errorCode);
    void priOnDemuxerFormatChanged(OH_AVDemuxer *demuxer, OH_AVFormat *format);
private:
    OH_AVSource*        m_source = nullptr;
    OH_AVDemuxer*       m_demuxer = nullptr;
    OH_AVBuffer*        m_sampleBuffer = nullptr;
    int32_t             m_videoTrackIndex = -1;
    int32_t             m_audioTrackIndex = -1;
    int64_t             m_lastAudioDTS{ HJ_NOTS_VALUE };   //ms
    int64_t             m_gapThreshold{ 1000 };
    bool                m_isEOF = false;
};
using HJOHDemuxerList = HJList<HJOHDemuxer::Ptr>;

NS_HJ_END