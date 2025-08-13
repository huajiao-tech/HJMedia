#pragma once

#include "HJPluginMuxer.h"
#include "HJFFMuxer.h"

NS_HJ_BEGIN

class HJPluginFFMuxer : public HJPluginMuxer
{
public:
	HJ_DEFINE_CREATE(HJPluginFFMuxer);

	HJPluginFFMuxer(const std::string& i_name = "HJPluginFFMuxer", size_t i_identify = -1);

protected:
	virtual void afterInit() override;
	virtual void deliverToOutputs(HJMediaFrame::Ptr& i_mediaFrame) override;
	virtual void onWriteFrame(HJMediaFrame::Ptr& io_outFrame);

	int64_t m_tsOffset{ HJ_INT64_MAX };
};

NS_HJ_END
