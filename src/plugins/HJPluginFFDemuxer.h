#pragma once

#include "HJPluginDemuxer.h"

NS_HJ_BEGIN

class HJPluginFFDemuxer : public HJPluginDemuxer
{
public:
	HJ_DEFINE_CREATE(HJPluginFFDemuxer);

	HJPluginFFDemuxer(const std::string& i_name = "HJPluginFFDemuxer", size_t i_identify = 0, HJKeyStorage::Ptr i_graphInfo = nullptr)
		: HJPluginDemuxer(i_name, i_identify, i_graphInfo) {}

protected:
	virtual HJBaseDemuxer::Ptr createDemuxer() override;
};

NS_HJ_END
