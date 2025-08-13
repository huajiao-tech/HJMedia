#pragma once

#include "HJSyncObject.h"
#include "HJMediaFrame.h"

NS_HJ_BEGIN

class HJMediaFrameDeque : public HJSyncObject
{
public:
	using Ptr = std::shared_ptr<HJMediaFrameDeque>;

	HJMediaFrameDeque(const std::string& i_name = "HJMediaFrameDeque", size_t i_identify = -1);

	void setMaxSize(int i_maxSize);
	bool isFull();
	size_t size();
	uint64_t duration();

	bool deliver(HJMediaFrame::Ptr i_mediaFrame);
	HJMediaFrame::Ptr receive(size_t& o_size);
	HJMediaFrame::Ptr preview(size_t& o_size);

	void flush();

protected:
	using MediaFrameDeque = std::deque<HJMediaFrame::Ptr>;
	using MediaFrameDequeIt = MediaFrameDeque::iterator;

	void internalRelease() override;

	MediaFrameDeque m_deque;

private:
	int m_maxSize;
};

NS_HJ_END
