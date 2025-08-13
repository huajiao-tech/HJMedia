//***********************************************************************************//
//HJediaKit FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJUtilitys.h"

typedef struct AVCodecParameters AVCodecParameters;
typedef struct AVPacket AVPacket;

NS_HJ_BEGIN
//***********************************************************************************//
class HJBaseParser : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJBaseParser>;

    virtual int init(const AVCodecParameters* codecParams) = 0;
    virtual int parse(HJBuffer::Ptr inData) = 0;
    virtual int parse(const AVPacket* pkt) = 0;
    virtual void done() = 0;
protected:
};

NS_HJ_END
