#include "HJMediaFrameDeque.h"

#define HJ_PIN_DEQUE_MAX_SIZE	0

NS_HJ_BEGIN

HJMediaFrameDeque::HJMediaFrameDeque(const std::string& i_name, size_t i_identify)
	: HJSyncObject(i_name, i_identify)
	, m_maxSize(HJ_PIN_DEQUE_MAX_SIZE)
{
}

void HJMediaFrameDeque::setMaxSize(int i_maxSize)
{
	SYNCHRONIZED_SYNC_LOCK;
	if (m_status == HJSTATUS_Done) {
		return;
	}

	m_maxSize = i_maxSize;
}

bool HJMediaFrameDeque::isFull()
{
	SYNCHRONIZED_SYNC_LOCK;
	if (m_status == HJSTATUS_Done) {
		return true;
	}

	return m_maxSize > 0 ? m_deque.size() >= m_maxSize : false;
}

size_t HJMediaFrameDeque::size()
{
	SYNCHRONIZED_SYNC_LOCK;
	if (m_status == HJSTATUS_Done) {
		return 0;
	}

	return m_deque.size();
}

uint64_t HJMediaFrameDeque::duration()
{
	SYNCHRONIZED_SYNC_LOCK;
	if (m_status == HJSTATUS_Done) {
		return 0;
	}

	return 0;
}

bool HJMediaFrameDeque::deliver(HJMediaFrame::Ptr i_mediaFrame)
{
	SYNCHRONIZED_SYNC_LOCK;
	if (m_status == HJSTATUS_Done) {
		return false;
	}

	if (i_mediaFrame != nullptr)	{
		m_deque.push_back(i_mediaFrame);
	}
	else {
		m_deque.clear();
	}
	
	return true;
}

HJMediaFrame::Ptr HJMediaFrameDeque::receive(size_t& o_size)
{
	SYNCHRONIZED_SYNC_LOCK;
	if (m_status == HJSTATUS_Done) {
		return nullptr;
	}

	if (m_deque.empty()) {
		o_size = 0;
		return nullptr;
	}

	MediaFrameDequeIt it = m_deque.begin();
	auto mediaFrame = *it;
	m_deque.erase(it);
	o_size = m_deque.size();

	return mediaFrame;
}

HJMediaFrame::Ptr HJMediaFrameDeque::preview(size_t& o_size)
{
	SYNCHRONIZED_SYNC_LOCK;
	if (m_status == HJSTATUS_Done) {
		return nullptr;
	}

	o_size = m_deque.size();
	if (m_deque.empty()) {
		return nullptr;
	}

	MediaFrameDequeIt it = m_deque.begin();
	return *it;
}
/*
int64_t SLMediaFrameDeque::getNextPacketPts()
{
	SYCHHRONIZED(mDequeSync);
	if (mDeque.empty()) {
		return -1;
	}

	MediaFrameDequeIt it = mDeque.begin();
	return (*it)->getPTS();
}
*/
void HJMediaFrameDeque::flush()
{
	SYNCHRONIZED_SYNC_LOCK;
	if (m_status == HJSTATUS_Done) {
		return;
	}

	m_deque.clear();
}

void HJMediaFrameDeque::internalRelease()
{
	m_deque.clear();

	HJSyncObject::internalRelease();
}

NS_HJ_END
