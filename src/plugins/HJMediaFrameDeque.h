#pragma once

#include "HJSyncObject.h"
#include "HJMediaFrame.h"

NS_HJ_BEGIN

// 音视频帧队列，统计音频时长/采样数和视频关键帧计数；详见 doc/HJMediaFrameDeque.md

class HJMediaFrameDeque : public HJSyncObject
{
public:
	using Ptr = std::shared_ptr<HJMediaFrameDeque>;
	using Condition = std::function<bool(HJMediaFrame::Ptr)>;

	struct FrameDequeInfo {
		int64_t dequeSize{};
		int64_t audioDuration{};
		int64_t audioSamples{};
		int64_t videoKeyFrames{};
		int64_t videoFrames{};
		int sampleRate{};

		void flush() {
			dequeSize = 0;
			audioDuration = 0;
			audioSamples = 0;
			sampleRate = 0;
			videoKeyFrames = 0;
			videoFrames = 0;
		}
	};

	HJMediaFrameDeque(const std::string& i_name = "HJMediaFrameDeque", size_t i_identify = 0)
		: HJSyncObject(i_name, i_identify) {}
	virtual ~HJMediaFrameDeque() { done(); }

	size_t size();
	int64_t audioDuration();
	bool hasControlFrame();

	bool deliver(HJMediaFrame::Ptr i_mediaFrame, FrameDequeInfo* o_info = nullptr);
	HJMediaFrame::Ptr receive(FrameDequeInfo* o_info = nullptr);
	HJMediaFrame::Ptr preview(FrameDequeInfo* o_info = nullptr);
	bool store(HJMediaFrame::Ptr i_mediaFrame);
	bool dropFrames(int64_t i_audioDutation, FrameDequeInfo* o_info = nullptr);
	void flush(bool i_storeClearFrame = false);

private:
	using MediaFrameDeque = std::deque<HJMediaFrame::Ptr>;
	using MediaFrameDequeIt = MediaFrameDeque::iterator;

	void internalRelease() override;

	void eraseFrame(HJMediaFrame::Ptr i_mediaFrame);
	void internalFlush();

	HJMediaFrame::Ptr m_cached{};
	MediaFrameDeque m_deque;
	FrameDequeInfo m_info;
	int m_eofCount{};
};

NS_HJ_END
