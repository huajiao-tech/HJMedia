//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJMediaFrame.h"
#include "HJBaseDemuxer.h"
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
    
    virtual int init();
    virtual int init(const HJMediaUrl::Ptr& mediaUrl);
    virtual void done();
    virtual int seek(int64_t pos);     //ms
    virtual int getFrame(HJMediaFrame::Ptr& frame);
    virtual void reset();
    
    virtual int64_t getDuration();  //ms
private:
    AVDictionary* getFormatOptions();
    int makeMediaInfo();
    HJMediaFrame::Ptr procEofFrame();
    double getRotation(AVStream *st);
    int analyzeProtocol();
private:
    AVFormatContext*    m_ic = NULL;
	//HJFrameParser::Ptr m_parser = nullptr;
};
using HJFFDemuxerList = HJList<HJFFDemuxer::Ptr>;

NS_HJ_END
