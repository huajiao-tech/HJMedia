//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJMediaFrame.h"
#include "HJBaseDemuxer.h"
#include <atomic>
#include <list>
//#include "HJPacketChecker.h"
//#include "HJFrameParser.h"

typedef struct AVFormatContext AVFormatContext;
typedef struct AVDictionary AVDictionary;
typedef struct AVStream AVStream;
typedef struct AVCodecParameters AVCodecParameters;

NS_HJ_BEGIN
//***********************************************************************************//
class HJFFDemuxer : public HJBaseDemuxer
{
public:
    using Ptr = std::shared_ptr<HJFFDemuxer>;
    HJFFDemuxer();
    HJFFDemuxer(const HJMediaUrl::Ptr& mediaUrl);
    virtual ~HJFFDemuxer();
    
    virtual int init() override;
    virtual int init(const HJMediaUrl::Ptr& mediaUrl) override;
    virtual void done() override;
    virtual int seek(int64_t pos) override;     //ms
    virtual int getFrame(HJMediaFrame::Ptr& frame) override;
    virtual int switchAudioTrack(int trackID) override;
    virtual void reset() override;
    
    virtual int64_t getDuration() override;  //ms
private:
    AVDictionary* getFormatOptions();
    int makeMediaInfo();
    HJMediaFrame::Ptr procEofFrame();
    double getRotation(AVStream *st);
    int analyzeProtocol();
    //
    void procSEI(const HJMediaFrame::Ptr& mvf);

    int checkAudioCodecParameters(const HJAudioInfo::Ptr& audioInfo, AVCodecParameters* codecParams);
    void applyPendingAudioTrackSwitch();
    bool isPendingAudioTrackSwitch(const int trackID) const;
    HJAudioInfo::Ptr makeAudioStreamInfo(AVFormatContext* ic, AVStream* st, int trackID, bool storeCodecParams);
    HJAudioTrackDisplayInfo::Ptr makeAudioTrackDisplayInfo(AVStream* st, const HJAudioInfo::Ptr& audioInfo, int trackID);

    int updateStatus();
    
    int seek_keyframe(int64_t target_pos);
    int readFrameInternal(HJMediaFrame::Ptr& frame);

    int getSeekFlags();
private:
    AVFormatContext*    m_ic = NULL;
    int64_t			    m_lastAudioDTS{ HJ_NOTS_VALUE };   //ms
    int64_t			    m_gapThreshold{ 1000 };
    std::list<HJMediaFrame::Ptr> m_gopFrames;
    std::atomic<int>     m_pendingAudioTrackID{ -1 };
    int                  m_switchingAudioTrackID{ -1 };
	//HJFrameParser::Ptr m_parser = nullptr;
    //HJPacketChecker     m_packetChecker;
};
using HJFFDemuxerList = HJList<HJFFDemuxer::Ptr>;

NS_HJ_END
