#pragma once

#include "HJPluginMuxer.h"
#include "HJFFMuxer.h"

NS_HJ_BEGIN

class HJPluginFFMuxer : public HJPluginMuxer
{
public:
	HJ_DEFINE_CREATE(HJPluginFFMuxer);

	HJPluginFFMuxer(const std::string& i_name = "HJPluginFFMuxer", size_t i_identify = 0, HJKeyStorage::Ptr i_graphInfo = nullptr)
		: HJPluginMuxer(i_name, i_identify, i_graphInfo) {}
	virtual ~HJPluginFFMuxer() { done(); }

protected:
	virtual HJBaseMuxer::Ptr createMuxer() override;
	virtual void onWriteFrame(HJMediaFrame::Ptr& io_outFrame) override;
	virtual void setQuit() override {};

	int64_t m_tsOffset{ HJ_INT64_MAX };
};

NS_HJ_END
