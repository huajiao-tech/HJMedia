#pragma once

#include "HJPrerequisites.h"
#include "HJUIBaseItem.h"
#include <glad/gl.h> //#define GLAD_GL_IMPLEMENTATION  only implementation once;
#include "HJDataDequeue.h"
#include "HJAsyncCache.h"
#include "shared-memory-queue.h"

NS_HJ_BEGIN

//typedef enum HJPlayerVideoCodecType
//{
//	HJPlayerVideoCodecType_SoftDefault = 0,
//	HJPlayerVideoCodecType_OHCODEC = 1,
//	HJPlayerVideoCodecType_VIDEOTOOLBOX = 2,
//	HJPlayerVideoCodecType_MEDIACODEC = 3,
//} HJPlayerVideoCodecType;

class HJMediaFrame;
class HJYuvTexCvt;
class HJGraphPlayer;
class HJStatContext;
class HJEventBus;
class HJUIItemPlayerPlugin : public HJUIBaseItem
{
public:
	    
	HJ_DEFINE_CREATE(HJUIItemPlayerPlugin);
    
	HJUIItemPlayerPlugin();
	virtual ~HJUIItemPlayerPlugin();

	virtual int run() override;
    
private:
	int priRepeats(int i_repeats);
	int priInit();
	int priOpen();
	int priReset();
	int priDone();
	int priResume();
	int priPause();
	int priStartRecording();
	int priStopRecording();
	int priSeek(int64_t i_curTime);
	int priTryGetParam();

	void registerHandlers(const std::shared_ptr<HJEventBus> i_bus);

//	std::string m_url = "F:/Sequence/1.mp4";
//	std::string m_url = "https://file-6-huajiao.6.cn/dynamic/mpc/34c62af1063dfdb4df05facccbcf5163.mp4";
//	std::string m_url = "F:/Sequence/34c62af1063dfdb4df05facccbcf5163.mp4";
//	std::string m_url = "F:/Sequence/98f380f6ed6eb248b8c463d7e7387ea7.mp4";
//	std::string m_url = "F:/Sequence/dfjy.webm";
	std::string m_url = "F:/Sequence/c58733ac51124fe38cdc6540a7b8fa46.mkv";
	std::string m_recordUrl = "F:/Sequence/record.mp4";
	
//	bool m_isLive = false;
	int m_ret{ 0 };
	int m_playerType = 2;
	bool m_isPaused = false;
	int m_loopCount = 0;
	int m_curTrackId = 0;
	int64_t m_curTimeMs = 0;
	int64_t m_seekTimeMs = 0;
//	int64_t m_maxTimeMs = 60 * 60 * 1000;
	bool m_isSeeking = false;
	std::atomic<bool> m_isStreamOpened{ false };
	std::vector<int> m_trackIds{};
	int64_t m_durationMs{ 0 };
	uint64_t m_firstTime = 0;
	float m_volume{ 1.0f };
	std::shared_ptr<HJYuvTexCvt> m_yuvTexCvt = nullptr;
	HJAsyncCache<std::shared_ptr<HJMediaFrame>> m_asyncCache;

	std::shared_ptr<HJGraphPlayer> m_player = nullptr;
//	std::shared_ptr<HJGraphVodPlayer> m_player = nullptr;
	video_queue_t* m_vq{};
	enum queue_state m_prevState = SHARED_QUEUE_STATE_INVALID;

	std::shared_ptr<HJStatContext> m_statCtx = nullptr;

	// temp
	bool m_isRecording{};
};

NS_HJ_END



