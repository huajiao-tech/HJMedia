#include "HJPluginFFMuxer.h"
#include "HJFLog.h"
#include "HJMuxers.h"

NS_HJ_BEGIN

HJBaseMuxer::Ptr HJPluginFFMuxer::createMuxer()
{
	return std::make_shared<HJFFMuxer>();
}

void HJPluginFFMuxer::onWriteFrame(HJMediaFrame::Ptr& io_outFrame)
{
	io_outFrame = io_outFrame->deepDup();

	if (m_tsOffset == HJ_INT64_MAX) {
		m_tsOffset = -io_outFrame->getDTS();
	}

	io_outFrame->setPTSDTS(io_outFrame->getPTS() + m_tsOffset, io_outFrame->getDTS() + m_tsOffset);
}

NS_HJ_END
