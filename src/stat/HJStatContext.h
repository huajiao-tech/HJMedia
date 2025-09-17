#pragma once

#include "HJPrerequisites.h"
#include "HJStatBaseNotify.h"
#include <set>

NS_HJ_BEGIN

class HJAggregatedContext;
class HJAggregatedIns;
class HJMonitorJsonInfo;

using JSMonitorFun = std::function<void(const std::string& i_name, int i_nType, const std::string& i_info)>;
using JSAggregateMap = std::map<int, std::shared_ptr<HJAggregatedIns>>;

#define JS_STAT_MAP_DEC(ID) int ex##ID(ProcParameters);

#define ProcParameters const std::string& i_name, bool i_bAggregated, int i_type, const std::any& i_any, std::string &o_info

class HJStatContext;
typedef int (HJStatContext::* JSNotifyHandler)(ProcParameters);

class HJStatContext : public HJStatBaseNotify
{
public:
	HJ_DEFINE_CREATE(HJStatContext);

//	using Ptr = std::shared_ptr<HJStatContext>;
	using WPtr = std::weak_ptr<HJStatContext>;

	using JStatSceneMap = std::unordered_map<std::string, std::any>;

	HJStatContext();
	virtual ~HJStatContext();

	int init(int64_t i_uid, const std::string &i_device, const std::string &i_sn, int i_interval, int64_t i_clockOffset, JSMonitorFun i_monitorNtfy);
	void setDevice(const std::string& i_device);
	void done();
	//int exJSNotify_Play_SecondOn(ProcParameters);
	JS_STAT_MAP_DEC(JNotify_Default)
	JS_STAT_MAP_DEC(JNotify_Publish_LiveInfo)
	JS_STAT_MAP_DEC(JNotify_Play_DelayClock)
	JS_STAT_MAP_DEC(JNotify_Play_Stutter)
	//JS_STAT_MAP_DEC(JNotify_SDK_Version)

	virtual int notify(const std::string& i_name, int i_type, std::any i_any) override;
	virtual int notifyRef(const std::string& i_name, int i_type, const std::any& i_any) override;
private:
	//int64_t priGetClockTimeFromTime(int64_t i_curSysTime);
	//int64_t priGetCurClockTime();
	JSMonitorFun m_monitorNtfy = nullptr;
	JSAggregateMap m_aggregateMap;
	std::shared_ptr<HJAggregatedContext> m_aggregatedCtx = nullptr;

	void priDone();
	void priMonitorCall(const std::string& i_name, int i_nType, const std::string& i_info);
	int priGetSceneId(int i_type) const;
	int priGetMetricId(int i_type) const;
	int priGetFlpExten(int i_type) const;

	int priAddSceneMap(int i_type, HJMonitorJsonInfo* io_monitorInfo, std::shared_ptr<HJAggregatedIns> *io_aggregate);

	bool priIsAggegrate(int i_sceneKey);
	int priTryCreateAggegrate(int i_type, int i_sceneKey);
	int priMapAnyType(const std::any& i_any, int i_floatExtend, std::string& o_valstr, int64_t& o_val64t, float& o_valFloat);

	std::string priGetDevice() const;
	JSNotifyHandler priGetNotifyFun(int i_type) const;
	bool m_bDone = false;
	int64_t m_uid = 0;
	std::string m_sn = "";
	int m_interval = 10;

	std::string m_device = "";
	mutable std::mutex m_mtx_device;
	std::mutex m_mtx;

	std::string m_sessionID = "";

	//int64_t m_clockOffset = 0;
	//uint32_t business = 1;

	static const std::unordered_map <int, std::tuple<int, int, JStatSceneMap, int, JSNotifyHandler> > m_notifyHandlerMap;
	static const std::set<int> m_aggregateSet;
};

NS_HJ_END

