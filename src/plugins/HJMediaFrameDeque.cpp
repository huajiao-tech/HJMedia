#include "HJMediaFrameDeque.h"
#include "HJFFHeaders.h"

NS_HJ_BEGIN

size_t HJMediaFrameDeque::size()
{
	SYNCHRONIZED_SYNC_LOCK;
	CHECK_DONE_STATUS(0);

	int cached = m_cached != nullptr ? 1 : 0;
	return m_deque.size() + cached;
}

int64_t HJMediaFrameDeque::audioDuration()
{
	SYNCHRONIZED_SYNC_LOCK;
	CHECK_DONE_STATUS(0);

	return m_info.audioDuration;
}

bool HJMediaFrameDeque::hasControlFrame()
{
	SYNCHRONIZED_SYNC_LOCK;
	CHECK_DONE_STATUS(false);

	if (m_cached && m_cached->isClearFrame()) {
		return true;
	}

	if (m_eofCount > 0) {
		return true;
	}

	return false;
}

bool HJMediaFrameDeque::deliver(HJMediaFrame::Ptr i_mediaFrame, FrameDequeInfo* o_info)
{
	if (i_mediaFrame == nullptr) {
		return false;
	}

	SYNCHRONIZED_SYNC_LOCK;
	CHECK_DONE_STATUS(false);

	if (i_mediaFrame->isEofFrame()) { 
		m_eofCount++;
	}

	m_deque.push_back(i_mediaFrame);

	m_info.dequeSize = static_cast<int64_t>(m_deque.size());
	if (i_mediaFrame->isAudio()) {
		m_info.audioDuration += i_mediaFrame->getDuration();
            
        HJAVFrame::Ptr avFrame = i_mediaFrame->getMFrame();
        if (avFrame) {
    		AVFrame* frame = avFrame->getAVFrame();
            if (frame) {
				m_info.audioSamples += frame->nb_samples;
				// 注意：PCM队列的sampleRate可能不一致，例如重采样的输入队列
				m_info.sampleRate = frame->sample_rate;
            }
        }
	}
	else if (i_mediaFrame->isVideo()) {
		m_info.videoFrames++;
		if (i_mediaFrame->isKeyFrame()) {
			m_info.videoKeyFrames++;
		}
	}

	if (o_info) {
		*o_info = m_info;
	}

	return true;
}

HJMediaFrame::Ptr HJMediaFrameDeque::receive(FrameDequeInfo* o_info)
{
	SYNCHRONIZED_SYNC_LOCK;
	CHECK_DONE_STATUS(nullptr);

	HJMediaFrame::Ptr mediaFrame = nullptr;
	do {
		if (m_cached != nullptr) {
			mediaFrame = m_cached;
			m_cached = nullptr;
			break;
		}

		if (m_deque.empty()) {
			break;
		}

		MediaFrameDequeIt it = m_deque.begin();
		mediaFrame = *it;
		m_deque.erase(it);

		eraseFrame(mediaFrame);
	} while (false);

	if (o_info) {
		*o_info = m_info;
	}

	if (mediaFrame && mediaFrame->isEofFrame()) {
		m_eofCount--;
	}

	return mediaFrame;
}

HJMediaFrame::Ptr HJMediaFrameDeque::preview(FrameDequeInfo* o_info)
{
	SYNCHRONIZED_SYNC_LOCK;
	CHECK_DONE_STATUS(nullptr);

	HJMediaFrame::Ptr mediaFrame = nullptr;
	do {
		if (m_cached != nullptr) {
			mediaFrame = m_cached;
			break;
		}

		if (m_deque.empty()) {
			break;
		}

		mediaFrame = *m_deque.begin();
	} while (false);

	if (o_info) {
		*o_info = m_info;
	}

	return mediaFrame;
}

bool HJMediaFrameDeque::store(HJMediaFrame::Ptr i_mediaFrame)
{
	SYNCHRONIZED_SYNC_LOCK;
	CHECK_DONE_STATUS(false);

	if (m_cached != nullptr && m_cached->isClearFrame()) {
		return false;
	}

	if (m_cached != nullptr && m_cached->isEofFrame()) {
		m_eofCount--;
	}
	if (i_mediaFrame != nullptr && i_mediaFrame->isEofFrame()) {
		m_eofCount++;
	}

	m_cached = i_mediaFrame;
	return true;
}

// 注意：此接口只对压缩帧队列才有效，PCM队列的audioDuration可能永远为0
bool HJMediaFrameDeque::dropFrames(int64_t i_audioDutation, FrameDequeInfo* o_info)
{
	if (i_audioDutation <= 0) {
		return false;
	}

	SYNCHRONIZED_SYNC_LOCK;
	CHECK_DONE_STATUS(false);

	bool droppedVideo = false;
	for (auto it = m_deque.begin(); it != m_deque.end();) {
		auto mediaFrame = *it;
		if (mediaFrame->isFlushFrame() || mediaFrame->isEofFrame()) {
			break; // 控制帧不丢
		}

		const bool hasVideo = (m_info.videoFrames > 0);
		const bool audioOk = (m_info.audioDuration <= i_audioDutation);

		if (hasVideo) {
			// 音频已达标且尚未丢过视频帧，直接退出
			if (audioOk && !droppedVideo) {
				break;
			}

			const bool firstVideoOk = mediaFrame->isVideo() && mediaFrame->isKeyFrame();

			// 已经触发过丢视频帧，并且音频达标且首个视频帧已是关键帧/控制帧
			if (audioOk && droppedVideo && firstVideoOk) {
				break;
			}

			if (mediaFrame->isVideo()) {
				if (mediaFrame->isKeyFrame()) {
					if (m_info.videoKeyFrames <= 1) {
						break; // 至少保留一个关键帧
					}
					// 音频未达标，允许继续丢当前关键帧推进
				}
				else {
					if (m_info.videoKeyFrames == 0) {
						break; // 队列里没有关键帧可保，避免破坏条件
					}
					// 非关键视频帧：继续丢，直到队头变为关键帧/控制帧
				}
				droppedVideo = true;
			}
			// 队头是音频帧：继续前进，直到满足首帧关键帧/控制帧约束
		}
		else {
			if (audioOk) {
				break; // 纯音频，音频达标即停
			}
		}

		it = m_deque.erase(it);
		eraseFrame(mediaFrame);
	}

	if (o_info) {
		*o_info = m_info;
	}

	return true;
}

void HJMediaFrameDeque::flush(bool i_storeClearFrame)
{
	SYNCHRONIZED_SYNC_LOCK;
	CHECK_DONE_STATUS();

	internalFlush();
	if (i_storeClearFrame) {
		m_cached = HJMediaFrame::makeClearFrame();
	}
}

void HJMediaFrameDeque::internalRelease()
{
	internalFlush();

	HJSyncObject::internalRelease();
}

void HJMediaFrameDeque::eraseFrame(HJMediaFrame::Ptr i_mediaFrame)
{
	m_info.dequeSize = static_cast<int64_t>(m_deque.size());
	if (i_mediaFrame->isAudio()) {
		m_info.audioDuration -= i_mediaFrame->getDuration();

		HJAVFrame::Ptr avFrame = i_mediaFrame->getMFrame();
		if (avFrame) {
			AVFrame* frame = avFrame->getAVFrame();
			if (frame) {
				m_info.audioSamples -= frame->nb_samples;
			}
		}
	}
	else if (i_mediaFrame->isVideo()) {
		m_info.videoFrames--;
		if (i_mediaFrame->isKeyFrame()) {
			m_info.videoKeyFrames--;
		}
	}
}

void HJMediaFrameDeque::internalFlush()
{
	m_deque.clear();
	m_info.flush();
	m_cached = nullptr;
	m_eofCount = 0;
}

NS_HJ_END
