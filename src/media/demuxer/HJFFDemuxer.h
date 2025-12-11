//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJMediaFrame.h"
#include "HJBaseDemuxer.h"
//#include "HJPacketChecker.h"
//#include "HJFrameParser.h"

typedef struct AVFormatContext AVFormatContext;
typedef struct AVDictionary AVDictionary;
typedef struct AVStream AVStream;

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
private:
    AVFormatContext*    m_ic = NULL;
    int64_t			    m_lastAudioDTS{ HJ_NOTS_VALUE };   //ms
    int64_t			    m_gapThreshold{ 1000 };
	//HJFrameParser::Ptr m_parser = nullptr;
    //HJPacketChecker     m_packetChecker;
};
using HJFFDemuxerList = HJList<HJFFDemuxer::Ptr>;

NS_HJ_END
