#pragma once

#include "HJPluginDemuxer.h"

NS_HJ_BEGIN

class HJPluginFFDemuxer : public HJPluginDemuxer
{
public:
	HJ_DEFINE_CREATE(HJPluginFFDemuxer);

	HJPluginFFDemuxer(const std::string& i_name = "HJPluginFFDemuxer", HJKeyStorage::Ptr i_graphInfo = nullptr)
		: HJPluginDemuxer(i_name, i_graphInfo) { }
	virtual ~HJPluginFFDemuxer() {
		HJPluginFFDemuxer::done();
	}

protected:
	virtual int internalInit(HJKeyStorage::Ptr i_param) override;
	virtual HJBaseDemuxer::Ptr createDemuxer() override;
	virtual int initDemuxer(const HJMediaUrl::Ptr& i_url, uint64_t i_delay = 0) override;

	virtual void asyncInitDemuxer(const HJMediaUrl::Ptr& i_url);

	int m_initId{ -1 };
};

NS_HJ_END
