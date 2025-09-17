#pragma once

#include "HJPrerequisites.h"
#include "HJLooperThread.h"
#include "HJPointerMap.h"

NS_HJ_BEGIN

class HJAggregatedMetrics : public HJSyncObject
{
public:
	using InfoMap = std::unordered_map<uint32_t, std::any>;

	struct Scene {
		uint32_t value;
		InfoMap sceneInfo;
	};

	struct MetricsReq {
		uint64_t uid;
		Scene scene;
		std::string device;
		InfoMap metrics;
		int64_t timestamp;
	};

public:
	using MetricsReqCallback = std::function<void(MetricsReq*)>;

	HJAggregatedMetrics(const std::string& i_name = "HJAggregatedMetrics", size_t i_identify = -1);
	virtual ~HJAggregatedMetrics();

	virtual int internalInit(HJKeyStorage::Ptr i_param) override;
	virtual void internalRelease() override;

	int createAggregate(int interval, int64_t uid, const std::string& device, int sceneValue, MetricsReqCallback callback);
	void destroyAggregate(int aggId);

	int addSceneInt64(int aggId, uint32_t id, int64_t value);
	int addSceneStr(int aggId, uint32_t id, const std::string value);

	int addMetricInt64(int aggId, int64_t timestamp, uint32_t id, int64_t value);
	int addMetricFloat(int aggId, int64_t timestamp, uint32_t id, float value);
	int addMetricStr(int aggId, int64_t timestamp, uint32_t id, const std::string value);

private:
	int addSceneInfo(int aggId, uint32_t id, std::any value);
	int addMetric(int aggId, int64_t timestamp, uint32_t id, std::any value);

private:
	class Metric {
	public:
		using Ptr = std::shared_ptr<Metric>;
		Metric(int64_t t, std::any v) : timestamp(t), value(v) { }

		int64_t timestamp;
		std::any value;
	};
	using MetricDeque = std::deque<Metric::Ptr>;
	using MetricDequeIt = MetricDeque::iterator;

	class Metrics {
	public:
		using Ptr = std::shared_ptr<Metrics>;
		virtual ~Metrics() { deque.clear(); }

		MetricDeque deque;
		std::string typeName;
	};
	using MetricsMap = HJPointerMap<uint32_t, Metrics::Ptr>;

	class Aggregate {
	public:
		using Ptr = std::shared_ptr<Aggregate>;
		virtual ~Aggregate() { metricsMap.clear(); }

		MetricsMap metricsMap;
 
		HJSync sync;

		MetricsReq req;
		MetricsReqCallback callback;
	};
	using AggregateMap = HJPointerMap<int, Aggregate::Ptr>;

	void aggregateOnce(std::shared_ptr<Aggregate> aggregate, int64_t start, int64_t end);

private:
	AggregateMap mAggregateMap;
	HJSync mAggregateSync;

	HJLooperThread::Ptr mThread{};
	HJLooperThread::Handler::Ptr mHandler{};
};

NS_HJ_END
