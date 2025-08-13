//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJUtilitys.h"

typedef struct AVCodecParameters AVCodecParameters;
typedef struct AVPacket AVPacket;
typedef struct AVCodecContext AVCodecContext;
typedef struct AVCodecParserContext AVCodecParserContext;

NS_HJ_BEGIN
//***********************************************************************************//
class HJFrameParser : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJFrameParser>;
    HJFrameParser(const HJBuffer::Ptr params, const int codecID);
    virtual ~HJFrameParser();

    virtual int init(const HJBuffer::Ptr params, const int codecID);
    virtual int parse(HJBuffer::Ptr inData);
    virtual void done();
protected:
    AVCodecContext*         m_avctx = NULL;
	AVCodecParserContext*   m_parser = NULL;
};

NS_HJ_END
