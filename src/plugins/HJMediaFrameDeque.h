#pragma once

#include "HJSyncObject.h"
#include "HJMediaFrame.h"

NS_HJ_BEGIN

class HJMediaFrameDeque : public HJSyncObject
{
public:
	using Ptr = std::shared_ptr<HJMediaFrameDeque>;

	HJMediaFrameDeque(const std::string& i_name = "HJMediaFrameDeque", size_t i_identify = -1);
	virtual ~HJMediaFrameDeque();

	size_t size();
	int64_t audioDuration();
	int64_t videoKeyFrames();
    int64_t audioSamples();

	bool deliver(HJMediaFrame::Ptr i_mediaFrame, size_t* o_size = nullptr, int64_t* o_audioDuration = nullptr, int64_t* o_videoKeyFrames = nullptr, int64_t* o_audioSamples = nullptr);
	HJMediaFrame::Ptr receive(size_t* o_size = nullptr, int64_t* o_audioDuration = nullptr, int64_t* o_videoKeyFrames = nullptr, int64_t* o_audioSamples = nullptr);
	HJMediaFrame::Ptr preview(size_t* o_size = nullptr, int64_t* o_audioDuration = nullptr, int64_t* o_videoKeyFrames = nullptr);
	void dropFrames(int64_t i_audioDutation, size_t* o_size = nullptr, int64_t* o_audioDuration = nullptr, int64_t* o_audioSamples = nullptr, int64_t* o_videoKeyFrames = nullptr, int64_t* o_videoFrames = nullptr);

	void flush();

protected:
	using MediaFrameDeque = std::deque<HJMediaFrame::Ptr>;
	using MediaFrameDequeIt = MediaFrameDeque::iterator;

	void internalRelease() override;

	void eraseFrame(HJMediaFrame::Ptr i_mediaFrame);
	void internalFlush();

	MediaFrameDeque m_deque;

private:
	int64_t m_audioDuration{};
	int64_t m_audioSamples{};
	int64_t m_videoKeyFrames{};
	int64_t m_videoFrames{};
};

NS_HJ_END
