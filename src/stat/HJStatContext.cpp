#include "HJFLog.h"
#include "HJStatContext.h"
#include "HJTime.h"
#include "HJMonitorJsonInfo.h"
#include "HJStatQuality.h"
#include "HJBaseUtils.h"
#include "HJVersion.h"
#include "HJAggregated.h"

NS_HJ_BEGIN

#define TUPLE_SCENEIDX  0
#define TUPLE_METRICIDX 1
#define TUPLE_SCENEMAP  2
#define TUPLE_FLPEXTEN  3

enum AnyEnum
{
	V_INTER = 0,
	V_FLP,
	V_STR,
};
enum QualityDefaultSceneInfo {
	DefaultSceneInfoNone = 0,
	DefaultSceneInfoSnString = 1,// sn
};

#define FLP_EXTEN 1

#define JSTAT_NAME_CAT(A) ex##A

#define JSTAT_MAP_ATTR(ID, SceneID, MetricID, SceneMap, FLPEXTEND, func) {ID, std::make_tuple(SceneID, MetricID, SceneMap, FLPEXTEND, func)},

#define JSTAT_MAP_SPECIAL_FUN(ID, SceneID, MetricID, SceneMap, FLPEXTEND) JSTAT_MAP_ATTR(ID, SceneID, MetricID, SceneMap, FLPEXTEND, &HJStatContext::JSTAT_NAME_CAT(ID))
#define JSTAT_MAP_SINGLE(ID, SceneID, MetricID, SceneMap, FLPEXTEND)   JSTAT_MAP_ATTR(ID, SceneID, MetricID, SceneMap, FLPEXTEND, &HJStatContext::exJNotify_Default)
#define JSTAT_MAP_MULTIPLE(ID, SceneID, MetricID, SceneMap, FLPEXTEND) JSTAT_MAP_SPECIAL_FUN(ID, SceneID, MetricID, SceneMap, FLPEXTEND)

#define JSTAT_MAP_SINGLE_SPEFUN(ID, SceneID, MetricID, SceneMap, FLPEXTEND) {ID, std::make_tuple(SceneID, MetricID, SceneMap, FLPEXTEND, &HJStatContext::exJNotify_Default)},

#define JSTAT_FUN_DEF(ID) int HJStatContext::ex##ID(ProcParameters)

#define JSTAT_SCENE_MAP_EMPTY            HJStatContext::JStatSceneMap({})
#define JSTAT_SCENE_MAP_DEFAULT          HJStatContext::JStatSceneMap({{"SN", (uint32_t)DefaultSceneInfoSnString}})
#define JSTAT_SCENE_MAP_LiveStatSceneInfo HJStatContext::JStatSceneMap({{"SN", (uint32_t)LiveStatSceneInfoSnString},{"Platform",(uint32_t)LiveStatSceneInfoPlatformString},{"SDKVersion",(uint32_t)LiveStatSceneInfoSDKVersionString}})
#define JSTAT_SCENE_MAP_LiveStreamPeriod HJStatContext::JStatSceneMap({{"SN", (uint32_t)LiveStreamPeriodSceneInfoSnString},{"Platform",(uint32_t)LiveStreamPeriodSceneInfoPlatformString},{"SDKVersion",(uint32_t)LiveStreamPeriodSceneInfoSDKVersionString}})
#define JSTAT_SCENE_MAP_PLAYERDELAYCLOCK HJStatContext::JStatSceneMap({{"SN", (uint32_t)WatchStreamPeriodSnString},{"Platform",(uint32_t)WatchStreamPeriodPlatformString},{"SDKVersion",(uint32_t)WatchStreamPeriodSDKVersionString}, {"SessionId",(uint32_t)WatchStreamSessionIdString}})
#define JSTAT_SCENE_MAP_WatchEnterLiveConsume HJStatContext::JStatSceneMap({{"SN", (uint32_t)WatchEnterLiveConsumeSnString},{"Platform",(uint32_t)WatchEnterLiveConsumePlatformString},{"SDKVersion",(uint32_t)WatchEnterLiveConsumeSDKVersionString}})
#define JSTAT_SCENE_MAP_ScheduleUrlConsume HJStatContext::JStatSceneMap({{"SN", (uint32_t)WatchStreamScheduleConsumeSnString},{"Platform",(uint32_t)WatchStreamScheduleConsumePlatformString},{"SDKVersion",(uint32_t)WatchStreamScheduleConsumeSDKVersionString}})
#define JSTAT_SCENE_MAP_WatchStuck       HJStatContext::JStatSceneMap({{"SN", (uint32_t)WatchStuckRateSnString},{"Platform",(uint32_t)WatchStuckPlatformString},{"SDKVersion",(uint32_t)WatchStuckSDKVersionString}, {"SessionId",(uint32_t)WatchStuckSessionIdString}})

const std::unordered_map <int, std::tuple<int, int, HJStatContext::JStatSceneMap, int, JSNotifyHandler> > HJStatContext::m_notifyHandlerMap
{
	//JSTAT_MAP_SPECIAL_FUN(JNotify_SDK_Version,          QualitySceneUserSdkInfoStat, UserSdkInfoMetricsSdkVersionString,                JSTAT_SCENE_MAP_EMPTY,   FLP_EXTEN)

	JSTAT_MAP_SINGLE(JNotify_SDK_Version,          QualitySceneUserSdkInfoStat, UserSdkInfoMetricsSdkVersionString,                     JSTAT_SCENE_MAP_EMPTY,   FLP_EXTEN)
	JSTAT_MAP_SINGLE(JNotify_Play_DecodeStyle,          QualitySceneWatchDecodeInfoStat, WatchDecodeInfoMetricsDecodeTypeInt64,         JSTAT_SCENE_MAP_DEFAULT, FLP_EXTEN)
	JSTAT_MAP_SINGLE(JNotify_Play_AudioRenderDeviceNum, QualitySceneAudioRenderDeviceNumStat, AudioRenderDeviceNumStatMetricsNumInt64,  JSTAT_SCENE_MAP_DEFAULT, FLP_EXTEN)
	JSTAT_MAP_MULTIPLE(JNotify_Play_Stutter,              QualitySceneWatchStuckRatePeriod, -1,                                         JSTAT_SCENE_MAP_WatchStuck, FLP_EXTEN)
	JSTAT_MAP_SINGLE(JNotify_Play_Loading,              QualitySceneWatchStreamEventStat, WatchStreamEventStatMetricsLoadingInt64,      JSTAT_SCENE_MAP_DEFAULT, FLP_EXTEN)

	JSTAT_MAP_SINGLE(JNotify_Play_SecondOnTime,     QualitySceneWatchEnterLiveConsumePeriod, WatchEnterLiveConsumeMetricsDurationInt64, JSTAT_SCENE_MAP_WatchEnterLiveConsume, FLP_EXTEN)
//	JSTAT_MAP_SINGLE(JNotify_Play_SecondOnEntryTime,     QualitySceneWatchEnterLiveConsumePeriod, WatchEnterLiveConsumeMetricsEnterTimeInt64, JSTAT_SCENE_MAP_WatchEnterLiveConsume, FLP_EXTEN)
//	JSTAT_MAP_SINGLE(JNotify_Play_SecondOnFirstRenderTime,     QualitySceneWatchEnterLiveConsumePeriod, WatchEnterLiveConsumeMetricsRenderTimeInt64, JSTAT_SCENE_MAP_WatchEnterLiveConsume, FLP_EXTEN)
    JSTAT_MAP_SINGLE(JNotify_Publish_Failed,        QualitySceneLiveException, LiveExceptionMetricsPushFailedInt64,                     JSTAT_SCENE_MAP_DEFAULT, FLP_EXTEN)
    JSTAT_MAP_SINGLE(JNotify_Publish_Disconnected,  QualitySceneLiveException, LiveExceptionMetricsPushDisconnectionString,             JSTAT_SCENE_MAP_DEFAULT, FLP_EXTEN)

	JSTAT_MAP_MULTIPLE(JNotify_Publish_LiveInfo,    QualitySceneLiveStat, -1,                                                 JSTAT_SCENE_MAP_LiveStatSceneInfo, FLP_EXTEN)
	JSTAT_MAP_MULTIPLE(JNotify_Play_DelayClock,     QualitySceneWatchStreamPeriod, -1,                                        JSTAT_SCENE_MAP_PLAYERDELAYCLOCK, FLP_EXTEN)

	JSTAT_MAP_SINGLE(JNotify_Publish_FinalBPS,      QualitySceneLiveStreamPeriod, LiveStreamPeriodMetricsVideoBitrateInt64,   JSTAT_SCENE_MAP_LiveStreamPeriod, FLP_EXTEN)
	JSTAT_MAP_SINGLE(JNotify_Publish_FinalFPS,      QualitySceneLiveStreamPeriod, LiveStreamPeriodMetricsVideoFramerateInt64, JSTAT_SCENE_MAP_LiveStreamPeriod, FLP_EXTEN)
	JSTAT_MAP_SINGLE(JNotify_Publish_DropRatio,     QualitySceneLiveStreamPeriod, LiveStreamPeriodMetricsVideoPushDropInt64,  JSTAT_SCENE_MAP_LiveStreamPeriod, 1000000)
	JSTAT_MAP_SINGLE(JNotify_Publish_PushDelay,     QualitySceneLiveStreamPeriod, LiveStreamPeriodMetricsVideoPushDelayInt64, JSTAT_SCENE_MAP_LiveStreamPeriod, FLP_EXTEN)


	//JSTAT_MAP_SINGLE(JNotify_Play_NetDelay,    QualitySceneWatchStreamPeriod, WatchStreamPeriodNetDelayInt64,                 JSTAT_SCENE_MAP_DEFAULT, FLP_EXTEN)
	//JSTAT_MAP_SINGLE(JNotify_Play_PlayerDelay, QualitySceneWatchStreamPeriod, WatchStreamPeriodPlayDelayInt64,                JSTAT_SCENE_MAP_DEFAULT, FLP_EXTEN)
	//JSTAT_MAP_SINGLE(JNotify_Play_TotalDelay,  QualitySceneWatchStreamPeriod, WatchStreamPeriodTotalDelayInt64,               JSTAT_SCENE_MAP_DEFAULT, FLP_EXTEN)

	JSTAT_MAP_SINGLE(JNotify_Publish_PushStreamErrorStat,  QualityScenePushStreamErrorStat, PushStreamErrorStatMetricsReasonString, JSTAT_SCENE_MAP_DEFAULT, FLP_EXTEN)
	JSTAT_MAP_SINGLE(JNotify_Play_PullStreamErrorStat,  QualityScenePullStreamErrorStat, PullStreamErrorStatMetricsReasonString,    JSTAT_SCENE_MAP_DEFAULT, FLP_EXTEN)
	
	JSTAT_MAP_SINGLE(JNotify_Play_ScheduleUrlConsume, QualitySceneWatchStreamScheduleConsumePeriod, WatchStreamScheduleConsumeMetricsDurationInt64, JSTAT_SCENE_MAP_ScheduleUrlConsume, FLP_EXTEN)
};
const std::set<int> HJStatContext::m_aggregateSet
{
	QualitySceneLiveStreamPeriod
};

HJStatContext::HJStatContext()
{

}
HJStatContext::~HJStatContext()
{
	HJFLogi("{} ~HJStatContext() enter", m_insName);
	priDone();
	HJFLogi("{} ~HJStatContext() end", m_insName);
}
void HJStatContext::done()
{
	priDone();
}
int HJStatContext::init(int64_t i_uid, const std::string& i_device, const std::string& i_sn, int i_interval, int64_t i_clockOffset, JSMonitorFun i_monitorNtfy)
{
	int i_err = 0;
	do
	{
		m_uid = i_uid;
		setDevice(i_device);
		m_sn = i_sn;
		m_interval = i_interval;
		m_monitorNtfy = i_monitorNtfy;
		m_clockOffset = i_clockOffset;
		m_sessionID = HJFMT("{}", (int64_t)HJCurrentSteadyMS());
		HJFLogi("{} init uid:{} device:{} sn:{} interval:{} closeoff:{}", getInsName(), m_uid, i_device, m_sn, m_interval, m_clockOffset);

		m_aggregatedCtx = HJAggregatedContext::Create();
		i_err = m_aggregatedCtx->init();
		if (i_err < 0)
		{
			HJFLoge("{} m_aggregatedCtx int error:{}", getInsName(), i_err);
			break;
		}
		i_err = HJStatBaseNotify::heartBeatInit();
		if (i_err < 0)
		{
			break;
		}
	} while (false);
	return i_err;
}
//int64_t HJStatContext::priGetClockTimeFromTime(int64_t i_curSysTime)
//{
//	return i_curSysTime + m_clockOffset;
//}
//int64_t HJStatContext::priGetCurClockTime()
//{
//	return priGetClockTimeFromTime(GET_CURRENT_MILLSEC());
//}
void HJStatContext::priDone()
{
	if (!m_bDone)
	{
		m_bDone = true;

		HJStatBaseNotify::heartBeatJoin();
		
		m_aggregateMap.clear();
	}
	
}
bool HJStatContext::priIsAggegrate(int i_sceneKey)
{
	auto it = m_aggregateSet.find(i_sceneKey);
	return it != m_aggregateSet.end();
}
int HJStatContext::priGetSceneId(int i_type) const
{
	//const std::tuple<int, int, JSNotifyHandler>& tuple = m_notifyHandlerMap.at(i_type);
	auto& tuple = m_notifyHandlerMap.at(i_type);
	return std::get<TUPLE_SCENEIDX>(tuple);
}
int HJStatContext::priGetMetricId(int i_type) const
{
	auto& tuple = m_notifyHandlerMap.at(i_type);
	return std::get<TUPLE_METRICIDX>(tuple);
}
int HJStatContext::priGetFlpExten(int i_type) const
{
	auto& tuple = m_notifyHandlerMap.at(i_type);
	return std::get<TUPLE_FLPEXTEN>(tuple);
}
JSNotifyHandler HJStatContext::priGetNotifyFun(int i_type) const
{
	//const std::tuple<int, int, JSNotifyHandler>& tuple = m_notifyHandlerMap.at(i_type);
	auto& tuple = m_notifyHandlerMap.at(i_type);
	return std::get<JSNotifyHandler>(tuple);
}
void HJStatContext::setDevice(const std::string& i_device)
{
	std::lock_guard<std::mutex> lock(m_mtx_device);
	m_device = i_device;
}
std::string HJStatContext::priGetDevice() const
{
	std::lock_guard<std::mutex> lock(m_mtx_device);
	return m_device;
}
void HJStatContext::priMonitorCall(const std::string& i_name, int i_nType, const std::string& i_info)
{
	if (m_monitorNtfy)
	{
		HJFLogd("{} monitorCall name:{} type:{} info:{}", getInsName(), i_name, i_nType, i_info);
		m_monitorNtfy(i_name, i_nType, i_info);
	}
}
int HJStatContext::priTryCreateAggegrate(int i_type, int i_sceneKey)
{
	int i_err = 0;
	std::string device = priGetDevice();
	std::lock_guard<std::mutex> lock(m_mtx);
	do
	{
		auto it = m_aggregateMap.find(i_sceneKey);
		if (it == m_aggregateMap.end())
		{
			HJAggregatedIns::Ptr aggregate = HJAggregatedIns::Create();

			HJAggregatedMetrics::MetricsReqCallback callback = [this](HJAggregatedMetrics::MetricsReq* req)
			{
				if (req)
				{				
					HJMonitorJsonInfo info(req->uid, priGetDevice(), this->GetClockTimeFromTime(req->timestamp), req->scene.value);
					for (auto it = req->scene.sceneInfo.begin(); it != req->scene.sceneInfo.end(); it++) {
						info.addSceneAny(it->first, it->second);
						//const std::type_info& typeInfo = it->second.type();
						//if (typeInfo == typeid(std::string))
						//{
						//	std::string value = std::any_cast<std::string>(it->second);
						//	info.addScene(it->first, value.c_str());
						//	break;
						//}
					}

					for (auto it = req->metrics.begin(); it != req->metrics.end(); it++) {
						const std::type_info& typeInfo = it->second.type();
						if (typeInfo == typeid(std::string)) {
							std::string value = std::any_cast<std::string>(it->second);
							info.addMetrics(it->first, value.c_str());
						}
						else if (typeInfo == typeid(int64_t)) {
							int64_t value = std::any_cast<int64_t>(it->second);
							info.addMetrics(it->first, value);

#if 0
							if (req->scene.value == QualitySceneLiveStreamPeriod)
							{
								if (it->first == LiveStreamPeriodMetricsVideoPushDelayInt64)
								{
									HJFLogi("pushdelaystat aggerate push delay value:{}", value);
								}
							}
#endif
						}
						else if (typeInfo == typeid(float)) {
							float value = std::any_cast<float>(it->second);
							info.addMetrics(it->first, value);
						}
					}
					//HJFLogi("aggregate:{}", str);
					priMonitorCall("Aggregated", 0, info.initSerial());
					
				}
			};

			i_err = aggregate->init(m_aggregatedCtx->getAggregated(), m_interval, m_uid, device, i_sceneKey, callback);
			if (i_err < 0)
			{
				break;
			}

			i_err = priAddSceneMap(i_type, nullptr, &aggregate);
			if (i_err < 0)
			{
				HJFLoge("priTryCreateAggegrate priAddSceneMap error");
				break;
			}
			//if (!m_sn.empty())
			//{
			//	aggregate->addSceneStr((uint32_t)DefaultSceneInfoSnString, m_sn);
			//}
			m_aggregateMap[i_sceneKey] = aggregate;		
		}
	} while (0);
	return i_err;
}
int HJStatContext::priMapAnyType(const std::any& i_any, int i_floatExtend, std::string& o_valstr, int64_t& o_val64t, float &o_valFloat)
{
	int typeMatch = V_INTER;
	do 
	{
		const std::type_info& typeInfo = i_any.type();
		if ((typeInfo == typeid(int))
			|| (typeInfo == typeid(int16_t))
			|| (typeInfo == typeid(int64_t))
			|| (typeInfo == typeid(uint32_t))
			|| (typeInfo == typeid(uint16_t))
			|| (typeInfo == typeid(uint64_t))
			|| (typeInfo == typeid(unsigned long long)) //android is not uint64_t, so add compare; windows is equal to uint64_t;
			|| (typeInfo == typeid(long long))
			|| (typeInfo == typeid(long)) 
			|| (typeInfo == typeid(unsigned long))
			)
		{
			typeMatch = V_INTER;
			if (typeInfo == typeid(int))
			{
				o_val64t = std::any_cast<int>(i_any);
			}
			else if (typeInfo == typeid(int16_t))
			{
				o_val64t = std::any_cast<int16_t>(i_any);
			}
			else if (typeInfo == typeid(int64_t))
			{
				o_val64t = std::any_cast<int64_t>(i_any);
			}
			else if (typeInfo == typeid(uint16_t))
			{
				o_val64t = std::any_cast<uint16_t>(i_any);
			}
			else if (typeInfo == typeid(uint32_t))
			{
				o_val64t = std::any_cast<uint32_t>(i_any);
			}
			else if (typeInfo == typeid(uint64_t))
			{
				o_val64t = std::any_cast<uint64_t>(i_any);
			}
			else if (typeInfo == typeid(unsigned long long))
			{
				o_val64t = std::any_cast<unsigned long long>(i_any);
			}
			else if (typeInfo == typeid(long long))
			{
				o_val64t = std::any_cast<long long>(i_any);
			}
			else if (typeInfo == typeid(long))
			{
				o_val64t = std::any_cast<long>(i_any);
			}
			else if (typeInfo == typeid(unsigned long))
			{
				o_val64t = std::any_cast<unsigned long>(i_any);
			}
		}
		else if ((typeInfo == typeid(float))
			|| (typeInfo == typeid(double)))
		{
			typeMatch = V_FLP;
			float valuefloat = 0.f;
			if (typeInfo == typeid(float))
			{
				valuefloat = std::any_cast<float>(i_any);
			}
			else if (typeInfo == typeid(double))
			{
				valuefloat = std::any_cast<double>(i_any);
			}

			//not use float convert to int64_t
			typeMatch = V_INTER;
			o_val64t = (int64_t)(valuefloat * i_floatExtend);
		}
		else if (typeInfo == typeid(std::string))
		{
			typeMatch = V_STR;
			o_valstr = std::any_cast<std::string>(i_any);
		}
		else
		{
			typeMatch = -1;
			HJFLoge("typeInfo is error");
			break;
		}
	} while (0);
	return typeMatch;
}
int HJStatContext::notifyRef(const std::string& i_name, int i_type, const std::any& i_any)
{
	int i_err = 0;
	do
	{
		auto it = m_notifyHandlerMap.find(i_type);
		if (it == m_notifyHandlerMap.end()) {
			i_err = -1;
			HJFLoge("stat key is not exist, error key:{}", i_type);
			break;
		}

		int sceneKey = priGetSceneId(i_type);

		bool bAggregate = priIsAggegrate(sceneKey);
		//HJFLogi("type:{} sceneKey:{} bAggregate:{}", i_type, sceneKey, bAggregate);

		if (bAggregate)
		{
			i_err = priTryCreateAggegrate(i_type, sceneKey);
			if (i_err < 0)
			{
				break;
			}
		}
		std::string oInfo = "";
		i_err = std::bind(priGetNotifyFun(i_type), this, i_name, bAggregate, i_type, i_any, std::ref(oInfo))();
		if (i_err < 0)
		{
			HJFLoge("bind error:{}", i_err);
			break;
		}
		if (!oInfo.empty())
		{
			priMonitorCall(i_name, i_type, oInfo);
		}

	} while (0);
	return i_err;
}
int HJStatContext::notify(const std::string& i_name, int i_type, std::any i_any)
{
	return notifyRef(i_name, i_type, i_any);
}
//const std::string& i_name, bool i_bAggregated, int i_type, const std::any& i_any, std::string& o_info
//JSTAT_FUN_DEF(JNotify_SDK_Version)
//{
//	int i_err = 0;
//	do
//	{
//		const std::type_info& typeInfo = i_any.type();
//		if (typeInfo == typeid(std::string))
//		{
//			std::string version = std::any_cast<std::string>(i_any);
//
//			int metricId = priGetMetricId(i_type);
//			if (metricId < 0)
//			{
//				i_err = -1;
//				HJFLoge("defualt but metricId < 0, config is error, please reconfig");
//				break;
//			}
//			int sceneId = priGetSceneId(i_type);
//			int64_t timestamp = priGetCurClockTime();
//			HJMonitorJsonInfo info(m_uid, priGetDevice(), timestamp, sceneId);
//
//			info.addMetrics((uint32_t)metricId, version);
//
//			o_info = info.encodeToJson();
//		}
//		else
//		{
//			i_err = -1;
//			HJFLoge("version is not string error");
//			break;
//		}
//	} while (false);
//	return i_err;
//}

int HJStatContext::priAddSceneMap(int i_type, HJMonitorJsonInfo* io_monitorInfo, std::shared_ptr<HJAggregatedIns>* io_aggregate)
{
	int i_err = 0;
	auto& tuple = m_notifyHandlerMap.at(i_type);
	const HJStatContext::JStatSceneMap& sceneMap = std::get<TUPLE_SCENEMAP>(tuple);

	for (const auto& [key, value] : sceneMap)
	{
		const std::type_info& typeInfo = value.type();
		if (key == "SN")
		{
			if (m_sn.empty())
			{
				HJFLoge("sn value is empty error");
				break;
			}
			if (typeInfo == typeid(uint32_t))
			{
				uint32_t id = std::any_cast<uint32_t>(value);

				if (io_aggregate)
				{
					(*io_aggregate)->addSceneStr(id, m_sn);
				}
				else
				{
					io_monitorInfo->addScene(id, m_sn);
				}
			}
			else
			{
				HJFLoge("sn value is not uint32_t error");
				i_err = -1;
				break;
			}
		}
		else if (key == "Platform")
		{
			if (typeInfo == typeid(uint32_t))
			{
				std::string platform = HJBaseUtils::getPlatform();
				uint32_t id = std::any_cast<uint32_t>(value);
				if (io_aggregate)
				{
					(*io_aggregate)->addSceneStr(id, platform);
				}
				else
				{
					io_monitorInfo->addScene(id, platform);
				}
			}
			else
			{
				HJFLoge("platform value is not uint32_t error");
				i_err = -1;
				break;
			}
		}
		else if (key == "SDKVersion")
		{
			std::string version = HJ_VERSION;
			if (typeInfo == typeid(uint32_t))
			{
				uint32_t id = std::any_cast<uint32_t>(value);

				if (io_aggregate)
				{
					(*io_aggregate)->addSceneStr(id, version);
				}
				else
				{
					io_monitorInfo->addScene(id, version);
				}
			}
			else
			{
				HJFLoge("SDKVersion value is not uint32_t error");
				i_err = -1;
				break;
			}
		}
		else if ("SessionId" == key)
		{
			uint32_t id = std::any_cast<uint32_t>(value);
			if (io_aggregate)
			{
				(*io_aggregate)->addSceneStr(id, m_sessionID);
			}
			else
			{
				io_monitorInfo->addScene(id, m_sessionID);
			}
		}
	}
	return i_err;
}

//const std::string& i_name, bool i_bAggregated, int i_type, const std::any& i_any, std::string& o_info
JSTAT_FUN_DEF(JNotify_Default)
{
	int i_err = 0;
	do 
	{
		int metricId = priGetMetricId(i_type);
		if (metricId < 0)
		{
			i_err = -1;
			HJFLoge("defualt but metricId < 0, config is error, please reconfig");
			break;
		}
		int sceneId = priGetSceneId(i_type);
		int64_t timestamp = GetCurClockTime();

		int64_t value64_t = 0;
		float valuefloat = 0.f;
		std::string valueStr = "";
		int typeMatch = priMapAnyType(i_any, priGetFlpExten(i_type), valueStr, value64_t, valuefloat);	
		if (i_bAggregated)
		{
			HJAggregatedIns::Ptr aggregate = m_aggregateMap[sceneId];
			switch (typeMatch)
			{
			case V_INTER:
				aggregate->addMetricInt64(timestamp, metricId, value64_t);
				break;
			case V_FLP:
				aggregate->addMetricFloat(timestamp, metricId, valuefloat);
				break;
			case V_STR:
				aggregate->addMetricStr(timestamp, metricId, valueStr);
				break;
			}	
		}
		else
		{
			HJMonitorJsonInfo info(m_uid, priGetDevice(), timestamp, sceneId);
			i_err = priAddSceneMap(i_type, &info, nullptr);
			if (i_err < 0)
			{
				HJFLoge("JNotify_Default priAddSceneMap error");
				break;
			}
			switch (typeMatch)
			{
			case V_INTER:
				info.addMetrics((uint32_t)metricId, value64_t);
				break;
			case V_FLP:
				info.addMetrics((uint32_t)metricId, valuefloat);
				break;
			case V_STR:
				info.addMetrics((uint32_t)metricId, valueStr);
				break;
			}
			o_info = info.initSerial();
		}

	} while (false);
	return i_err;
}

JSTAT_FUN_DEF(JNotify_Publish_LiveInfo)
{
	int i_err = 0;
	do
	{
		if (i_bAggregated)
		{
			i_err = -1;
			HJFLoge("publish live info baggreated error");
			break;
		}
		const std::type_info& typeInfo = i_any.type();
		if (typeInfo != typeid(JStatPublishLiveInfo))
		{
			i_err = -1;
			HJFLoge("publish live info is not JStatPublishLiveInfo error");
			break;
		}

		int sceneId = priGetSceneId(i_type);
		int64_t timestamp = GetCurClockTime();

		HJMonitorJsonInfo info(m_uid, priGetDevice(), timestamp, sceneId);
		i_err = priAddSceneMap(i_type, &info, nullptr);
		if (i_err < 0)
		{
			HJFLoge("JNotify_Publish_LiveInfo priAddSceneMap error");
			break;
		}

		JStatPublishLiveInfo liveInfo = std::any_cast<JStatPublishLiveInfo>(i_any);
		info.addMetrics((uint32_t)LiveStatMetricsVideoEncodeString, (std::string)liveInfo.m_encodeType);
		info.addMetrics((uint32_t)LiveStatMetricsVideoWidthInt64, (int64_t)liveInfo.m_width);
		info.addMetrics((uint32_t)LiveStatMetricsVideoHeightInt64, (int64_t)liveInfo.m_height);
		info.addMetrics((uint32_t)LiveStatMetricsPreFpsInt64, (int64_t)liveInfo.m_preFps);
		info.addMetrics((uint32_t)LiveStatMetricsPreKbpsInt64, (int64_t)liveInfo.m_preBps);
		info.addMetrics((uint32_t)LiveStatMetricsHighCpuInt64, (int64_t)liveInfo.m_highCpu);
		info.addMetrics((uint32_t)LiveStatMetricsVideoGamutString, (std::string)liveInfo.m_gamut);
		info.addMetrics((uint32_t)LiveStatMetricsSdkVersionString, (std::string)liveInfo.m_version);
		o_info = info.initSerial();
	} while (false);
	return i_err;
}

JSTAT_FUN_DEF(JNotify_Play_DelayClock)
{
	int i_err = 0;
	do
	{
		if (i_bAggregated)
		{
			i_err = -1;
			HJFLoge("JNotify_Play_DelayClock  info is not aggreated error");
			break;
		}
		const std::type_info& typeInfo = i_any.type();
		if (typeInfo != typeid(JStatPlayDelayClockInfo))
		{
			i_err = -1;
			HJFLoge("JNotify_Play_DelayClock is not JStatPlayDelayClockInfo error");
			break;
		}

		int sceneId = priGetSceneId(i_type);
		int64_t timestamp = GetCurClockTime();

		HJMonitorJsonInfo info(m_uid, priGetDevice(), timestamp, sceneId);
		i_err = priAddSceneMap(i_type, &info, nullptr);
		if (i_err < 0)
		{
			HJFLoge("JNotify_Publish_LiveInfo priAddSceneMap error");
			break;
		}

		JStatPlayDelayClockInfo clockInfo = std::any_cast<JStatPlayDelayClockInfo>(i_any);

		info.addMetrics((uint32_t)WatchStreamPeriodCaptureTimeInt64, (int64_t)clockInfo.m_captureTime);
		info.addMetrics((uint32_t)WatchStreamPeriodUploadTimeInt64, (int64_t)clockInfo.m_uploadTime);
		info.addMetrics((uint32_t)WatchStreamPeriodRecvTimeInt64, (int64_t)clockInfo.m_recvTime);
		info.addMetrics((uint32_t)WatchStreamPeriodRenderTimeInt64, (int64_t)clockInfo.m_renderTime);

		o_info = info.initSerial();
	} while (false);
	return i_err;
}

JSTAT_FUN_DEF(JNotify_Play_Stutter)
{
	int i_err = 0;
	do
	{
		if (i_bAggregated)
		{
			i_err = -1;
			HJFLoge("JNotify_Play_Stutter  info is not aggreated error");
			break;
		}
		const std::type_info& typeInfo = i_any.type();
		if (typeInfo != typeid(JStatPlayStuckInfo))
		{
			i_err = -1;
			HJFLoge("JNotify_Play_Stutter is not JStatPlayStuckInfo error");
			break;
		}

		int sceneId = priGetSceneId(i_type);
		int64_t timestamp = GetCurClockTime();

		HJMonitorJsonInfo info(m_uid, priGetDevice(), timestamp, sceneId);
		i_err = priAddSceneMap(i_type, &info, nullptr);
		if (i_err < 0)
		{
			HJFLoge("JNotify_Publish_LiveInfo priAddSceneMap error");
			break;
		}

		JStatPlayStuckInfo clockInfo = std::any_cast<JStatPlayStuckInfo>(i_any);

		info.addMetrics((uint32_t)WatchDurationInt64, (int64_t)clockInfo.m_watchDuration);
		info.addMetrics((uint32_t)WatchStuckDurationInt64, (int64_t)clockInfo.m_watchStuckDuration);
		
		o_info = info.initSerial();
	} while (false);
	return i_err;
}

NS_HJ_END