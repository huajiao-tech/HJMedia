#pragma once

#include "HJPrerequisites.h"
#include "HJAggregatedMetrics.h"

NS_HJ_BEGIN

class HJAggregatedMetrics;

class HJAggregatedContext
{
public:

	HJ_DEFINE_CREATE(HJAggregatedContext);

	HJAggregatedContext();
	virtual ~HJAggregatedContext();

	int init();
	HJAggregatedMetrics* getAggregated();
private:
	HJAggregatedMetrics* m_aggregated = nullptr;
};


class HJAggregatedIns
{
public:
	HJ_DEFINE_CREATE(HJAggregatedIns);


	
	HJAggregatedIns();
	virtual ~HJAggregatedIns();
	
	int init(HJAggregatedMetrics* i_context, int interval, int64_t uid, const std::string& device, int sceneValue, HJAggregatedMetrics::MetricsReqCallback callback);

	//void destroyAggregate(int aggId);

	int addSceneInt64(uint32_t id, int64_t value);
	int addSceneStr(uint32_t id, const std::string value);

	int addMetricInt64(int64_t timestamp, uint32_t id, int64_t value);
	int addMetricFloat(int64_t timestamp, uint32_t id, float value);
	int addMetricStr(int64_t timestamp, uint32_t id, const std::string value);

private:
	HJAggregatedMetrics* m_context = nullptr;
	int m_handle = 0;
};

NS_HJ_END


