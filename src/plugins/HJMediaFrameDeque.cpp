#include "HJMediaFrameDeque.h"
#include "HJFFHeaders.h"

//#define HJ_PIN_DEQUE_MAX_SIZE	0

NS_HJ_BEGIN

HJMediaFrameDeque::HJMediaFrameDeque(const std::string& i_name, size_t i_identify)
	: HJSyncObject(i_name, i_identify)
{
}

HJMediaFrameDeque::~HJMediaFrameDeque()
{
	HJMediaFrameDeque::done();
}

size_t HJMediaFrameDeque::size()
{
	SYNCHRONIZED_SYNC_LOCK;
	if (m_status == HJSTATUS_Done) {
		return 0;
	}

	return m_deque.size();
}

int64_t HJMediaFrameDeque::audioDuration()
{
	SYNCHRONIZED_SYNC_LOCK;
	if (m_status == HJSTATUS_Done) {
		return 0;
	}

	return m_audioDuration;
}

int64_t HJMediaFrameDeque::videoKeyFrames()
{
	SYNCHRONIZED_SYNC_LOCK;
	if (m_status == HJSTATUS_Done) {
		return 0;
	}

	return m_videoKeyFrames;
}

int64_t HJMediaFrameDeque::audioSamples()
{
    SYNCHRONIZED_SYNC_LOCK;
	if (m_status == HJSTATUS_Done) {
		return 0;
	}

	return m_audioSamples;
}

bool HJMediaFrameDeque::deliver(HJMediaFrame::Ptr i_mediaFrame, size_t* o_size, int64_t* o_audioDuration, int64_t* o_videoKeyFrames, int64_t* o_audioSamples)
{
	SYNCHRONIZED_SYNC_LOCK;
	if (m_status == HJSTATUS_Done) {
		return false;
	}

	if (i_mediaFrame != nullptr) {
		m_deque.push_back(i_mediaFrame);

		if (i_mediaFrame->isAudio()) {
			m_audioDuration += i_mediaFrame->getDuration();
            
            HJAVFrame::Ptr avFrame = i_mediaFrame->getMFrame();
            if (avFrame) {
    			AVFrame* frame = avFrame->getAVFrame();
                if (frame) {
                    m_audioSamples += frame->nb_samples;
                }
            }
		}
		else if (i_mediaFrame->isVideo()) {
			m_videoFrames++;
			if (i_mediaFrame->isKeyFrame()) {
				m_videoKeyFrames++;
			}
		}
	}
	else {
		internalFlush();
	}

    if (o_size != nullptr) {
		*o_size = m_deque.size();
	}
    if (o_audioDuration != nullptr) {
		*o_audioDuration = m_audioDuration;
	}
	if (o_videoKeyFrames != nullptr) {
		*o_videoKeyFrames = m_videoKeyFrames;
	}
	if (o_audioSamples != nullptr) {
		*o_audioSamples = m_audioSamples;
	}
	return true;
}

HJMediaFrame::Ptr HJMediaFrameDeque::receive(size_t* o_size, int64_t* o_audioDuration, int64_t* o_videoKeyFrames, int64_t* o_audioSamples)
{
	SYNCHRONIZED_SYNC_LOCK;
	if (m_status == HJSTATUS_Done) {
		if (o_size != nullptr) {
			*o_size = 0;
		}
		if (o_audioDuration != nullptr) {
			*o_audioDuration = 0;
		}
		if (o_videoKeyFrames != nullptr) {
			*o_videoKeyFrames = 0;
		}
        if (o_audioSamples != nullptr) {
			*o_audioSamples = 0;
		}
		return nullptr;
	}

	HJMediaFrame::Ptr mediaFrame = nullptr;
	do {
		if (m_deque.empty()) {
			break;
		}

		MediaFrameDequeIt it = m_deque.begin();
		mediaFrame = *it;
		m_deque.erase(it);
		eraseFrame(mediaFrame);
	} while (false);

	if (o_size != nullptr) {
		*o_size = m_deque.size();
	}
	if (o_audioDuration != nullptr) {
		*o_audioDuration = m_audioDuration;
	}
	if (o_videoKeyFrames != nullptr) {
		*o_videoKeyFrames = m_videoKeyFrames;
	}
	if (o_audioSamples != nullptr) {
		*o_audioSamples = m_audioSamples;
	}
	return mediaFrame;
}

HJMediaFrame::Ptr HJMediaFrameDeque::preview(size_t* o_size, int64_t* o_audioDuration, int64_t* o_videoKeyFrames)
{
	SYNCHRONIZED_SYNC_LOCK;
	if (m_status == HJSTATUS_Done) {
		if (o_size != nullptr) {
			*o_size = -1;
		}
		if (o_audioDuration != nullptr) {
			*o_audioDuration = -1;
		}
		if (o_videoKeyFrames != nullptr) {
			*o_videoKeyFrames = -1;
		}
		return nullptr;
	}

	HJMediaFrame::Ptr mediaFrame = nullptr;
	do {
		if (m_deque.empty()) {
			break;
		}

		mediaFrame = *m_deque.begin();
	} while (false);

	if (o_size != nullptr) {
		*o_size = m_deque.size();
	}
	if (o_audioDuration != nullptr) {
		*o_audioDuration = m_audioDuration;
	}
	if (o_videoKeyFrames != nullptr) {
		*o_videoKeyFrames = m_videoKeyFrames;
	}
	return mediaFrame;
}

void HJMediaFrameDeque::dropFrames(int64_t i_audioDutation, size_t* o_size, int64_t* o_audioDuration, int64_t* o_audioSamples, int64_t* o_videoKeyFrames, int64_t* o_videoFrames)
{
	SYNCHRONIZED_SYNC_LOCK;
	CHECK_DONE_STATUS();

    for (auto it = m_deque.begin(); it != m_deque.end();) {
        auto mediaFrame = *it;
		if (mediaFrame->isFlushFrame() || mediaFrame->isEofFrame()) {
			break;
		}

		if (m_videoFrames > 0) {
			if (mediaFrame->isVideo() && mediaFrame->isKeyFrame() &&
				(m_audioDuration <= i_audioDutation || m_videoKeyFrames <= 1)) {
				break;
			}
		}
		else {
			if (m_audioDuration <= i_audioDutation) {
				break;
			}
		}

		it = m_deque.erase(it);
		eraseFrame(mediaFrame);
	}

	if (o_size) {
		*o_size = m_deque.size();
	}
	if (o_audioDuration) {
		*o_audioDuration = m_audioDuration;
	}
	if (o_audioSamples) {
		*o_audioSamples = m_audioSamples;
	}
	if (o_videoKeyFrames) {
		*o_videoKeyFrames = m_videoKeyFrames;
	}
	if (o_videoFrames) {
		*o_videoFrames = m_videoFrames;
	}
}

void HJMediaFrameDeque::flush()
{
	SYNCHRONIZED_SYNC_LOCK;
	if (m_status == HJSTATUS_Done) {
		return;
	}

	internalFlush();
}

void HJMediaFrameDeque::internalRelease()
{
	internalFlush();

	HJSyncObject::internalRelease();
}

void HJMediaFrameDeque::eraseFrame(HJMediaFrame::Ptr i_mediaFrame)
{
	if (i_mediaFrame->isAudio()) {
		m_audioDuration -= i_mediaFrame->getDuration();

		HJAVFrame::Ptr avFrame = i_mediaFrame->getMFrame();
		if (avFrame) {
			AVFrame* frame = avFrame->getAVFrame();
			if (frame) {
				m_audioSamples -= frame->nb_samples;
			}
		}
	}
	else if (i_mediaFrame->isVideo()) {
		m_videoFrames--;
		if (i_mediaFrame->isKeyFrame()) {
			m_videoKeyFrames--;
		}
	}
}

void HJMediaFrameDeque::internalFlush()
{
	m_deque.clear();
	m_audioDuration = 0;
	m_audioSamples = 0;
	m_videoKeyFrames = 0;
	m_videoFrames = 0;
}

NS_HJ_END
