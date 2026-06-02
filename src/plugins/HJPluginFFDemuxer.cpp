#include "HJPluginFFDemuxer.h"
#include "HJDemuxers.h"

#include "HJFLog.h"

NS_HJ_BEGIN

HJBaseDemuxer::Ptr HJPluginFFDemuxer::createDemuxer()
{
	//return std::make_shared<HJFFDemuxer>();
	return HJCreates<HJComplexDemuxer>();
}

NS_HJ_END
