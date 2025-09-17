#pragma once

#include "HJPrerequisites.h"
#include "HJTimerTask.h"

NS_HJ_BEGIN

typedef enum JStatNotifyType {
	JNotify_None = -1,

	JNotify_SDK_Version,

	JNotify_Play_SecondOnTime,
	//	JNotify_Play_SecondOnEntryTime,
	//	JNotify_Play_SecondOnFirstRenderTime,

	JNotify_Play_DecodeStyle,  //hw soft
	JNotify_Play_AudioRenderDeviceNum,
	JNotify_Play_Stutter,

	JNotify_Play_Loading,

	JNotify_Publish_Failed,
	JNotify_Publish_Disconnected,

	JNotify_Publish_EncodeStyle, //hw soft

	//struct once info
	JNotify_Publish_LiveInfo,

	//aggregate
	JNotify_Publish_FinalBPS,
	JNotify_Publish_FinalFPS,
	JNotify_Publish_DropRatio,
	JNotify_Publish_PushDelay,

	//JNotify_Play_NetDelay,
	//JNotify_Play_PlayerDelay,
	//JNotify_Play_TotalDelay,
	JNotify_Play_DelayClock,

	JNotify_Publish_PushStreamErrorStat,
	JNotify_Play_PullStreamErrorStat,

	JNotify_Play_ScheduleUrlConsume,
} JStatNotifyType;

// JNotify_Publish_LiveInfo
struct JStatPublishLiveInfo
{
	std::string m_encodeType = "tx265";
	int m_width = 720;
	int m_height = 1280;
	int m_preFps = 15;
	int m_preBps = 2000;
	int m_highCpu = 0;
	std::string m_gamut = "BT709";
	std::string m_version = "1.0.0";
};

// JNotify_Play_DelayClock
struct JStatPlayDelayClockInfo
{
	int64_t m_captureTime = 0;
	int64_t m_uploadTime = 0;
	int64_t m_recvTime = 0;
	int64_t m_renderTime = 0;
};

struct JStatPlayStuckInfo
{
	int64_t m_watchDuration = 0;
	int64_t m_watchStuckDuration = 0;
};

class HJStatBaseNotify
{
public:
	using Ptr = std::shared_ptr<HJStatBaseNotify>;
	using Wtr = std::weak_ptr<HJStatBaseNotify>;
	virtual ~HJStatBaseNotify();
	virtual int notify(const std::string& i_name, int i_type, std::any i_any)
	{
		return 0;
	}
	virtual int notifyRef(const std::string& i_name, int i_type, const std::any& i_any)
	{
		return 0;
	}
	void setInsName(const std::string& i_insName);
    const std::string &getInsName()const 
    {
        return m_insName;
    }
	int heartBeatInit();
	void heartBeatJoin();
	int heartbeatRegist(const std::string& i_unqiueName, int i_interval, TimerTaskFun i_cb);
	int heartbeatUnRegist(const std::string& i_unqiueName);

	int64_t GetClockTimeFromTime(int64_t i_curSysTime);
	int64_t GetCurClockTime();
	static HJStatBaseNotify::Ptr GetPtr(HJStatBaseNotify::Wtr i_wr);
protected:
	int64_t m_clockOffset = 0;
	std::shared_ptr<HJTimerTask> m_heartBeat = nullptr;
	std::string m_insName = "";
};

NS_HJ_END

