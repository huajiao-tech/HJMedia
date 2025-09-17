#include "HJAggregatedMetrics.h"

NS_HJ_BEGIN

HJAggregatedMetrics::HJAggregatedMetrics(const std::string& i_name, size_t i_identify)
	: HJSyncObject(i_name, i_identify)
{
}

HJAggregatedMetrics::~HJAggregatedMetrics()
{
	HJAggregatedMetrics::done();
}

int HJAggregatedMetrics::internalInit(HJKeyStorage::Ptr i_param)
{
	int ret = HJSyncObject::internalInit(i_param);
	if (ret < 0) {
		return ret;
	}

	do {
		mThread = HJLooperThread::quickStart(getName());
		if (!mThread) {
			ret = HJErrFatal;
			break;
		}

		mHandler = mThread->createHandler();
		if (!mHandler) {
			ret = HJErrFatal;
			break;
		}

		return HJ_OK;
	} while (false);

	HJAggregatedMetrics::internalRelease();
	return ret;
}

void HJAggregatedMetrics::internalRelease()
{
	{
		SYNCHRONIZED_LOCK(mAggregateSync);
		for (auto it = mAggregateMap.begin(); it != mAggregateMap.end();) {
			if (mHandler) {
				mHandler->closeTimer(it->first);
			}
			it = mAggregateMap.erase(it);
		}
	}

	if (mThread) {
		mThread->done();
		mThread = nullptr;
	}
	mHandler = nullptr;
}

int HJAggregatedMetrics::createAggregate(int interval, int64_t uid, const std::string& device, int sceneValue, MetricsReqCallback callback)
{
	return SYNC_CONS_LOCK([=] {
		CHECK_DONE_STATUS(HJErrAlreadyDone);

		int id = mHandler->genMsgId();
		auto aggregate = std::make_shared<Aggregate>();
		aggregate->req.uid = uid;
		aggregate->req.scene.value = sceneValue;
		aggregate->req.device = device;
		aggregate->callback = callback;
		{
			SYNCHRONIZED_LOCK(mAggregateSync);
			mAggregateMap[id] = aggregate;
		}

		mHandler->openTimer([this, id](uint64_t now, uint64_t next) {
			std::shared_ptr<Aggregate> aggregate;
			{
				SYNCHRONIZED_LOCK(mAggregateSync);
				aggregate = mAggregateMap.find2(id);
			}

			if (aggregate) {
				SYNCHRONIZED_LOCK(aggregate->sync);

				int64_t end = now - (next - now);
				int64_t start = end - (next - now);
				aggregateOnce(aggregate, start, end);

				if (!aggregate->req.metrics.empty() && aggregate->callback) {
					aggregate->req.timestamp = end;
					aggregate->callback(&aggregate->req);
				}
			}
		}, 1, interval, id);

		return id;
		});
}

void HJAggregatedMetrics::destroyAggregate(int aggId)
{
	SYNC_CONS_LOCK([=] {
		CHECK_DONE_STATUS();

		mHandler->closeTimer(aggId);

		{
			SYNCHRONIZED_LOCK(mAggregateSync);
			mAggregateMap.erase(aggId);
		}
	});
}

int HJAggregatedMetrics::addSceneInt64(int aggId, uint32_t id, int64_t value)
{
	return addSceneInfo(aggId, id, value);
}

int HJAggregatedMetrics::addSceneStr(int aggId, uint32_t id, const std::string value)
{
	return addSceneInfo(aggId, id, value);
}

int HJAggregatedMetrics::addSceneInfo(int aggId, uint32_t id, std::any value)
{
	return SYNC_CONS_LOCK([=] {
		CHECK_DONE_STATUS(HJErrAlreadyDone);

		std::shared_ptr<Aggregate> aggregate;
		{
			SYNCHRONIZED_LOCK(mAggregateSync);
			aggregate = mAggregateMap.find2(aggId);
		}

		if (aggregate) {
			SYNCHRONIZED_LOCK(aggregate->sync);
			aggregate->req.scene.sceneInfo[id] = value;

			return HJ_OK;
		}
		else {
			return HJErrInvalidParams;
		}
	});
}

int HJAggregatedMetrics::addMetricInt64(int aggId, int64_t timestamp, uint32_t id, int64_t value)
{
	return addMetric(aggId, timestamp, id, value);
}

int HJAggregatedMetrics::addMetricFloat(int aggId, int64_t timestamp, uint32_t id, float value)
{
	return addMetric(aggId, timestamp, id, value);
}

int HJAggregatedMetrics::addMetricStr(int aggId, int64_t timestamp, uint32_t id, const std::string value)
{
	return addMetric(aggId, timestamp, id, value);
}

int HJAggregatedMetrics::addMetric(int aggId, int64_t timestamp, uint32_t id, std::any value)
{
	return SYNC_CONS_LOCK([=] {
		CHECK_DONE_STATUS(HJErrAlreadyDone);

		std::shared_ptr<Aggregate> aggregate;
		{
			SYNCHRONIZED_LOCK(mAggregateSync);
			aggregate = mAggregateMap.find2(aggId);
		}

		if (aggregate) {
			const std::type_info& typeInfo = value.type();
			std::string name;
			if (typeInfo == typeid(int64_t)) {
				name = typeid(int64_t).name();
			}
			else if (typeInfo == typeid(float)) {
				name = typeid(float).name();
			}
			else if (typeInfo == typeid(std::string)) {
				name = typeid(std::string).name();
			}

			SYNCHRONIZED_LOCK(aggregate->sync);
			auto metrics = aggregate->metricsMap.find2(id);
			if (metrics) {
				if (metrics->typeName.compare(name) != 0) {
					return HJErrInvalidParams;
				}
			}
			else {
				metrics = std::make_shared<Metrics>();
				metrics->typeName = name;
				aggregate->metricsMap[id] = metrics;
			}

			metrics->deque.push_back(std::make_shared<Metric>(timestamp, value));
			return HJ_OK;
		}
		else {
			return HJErrInvalidParams;
		}
	});
}

void HJAggregatedMetrics::aggregateOnce(std::shared_ptr<Aggregate> aggregate, int64_t start, int64_t end)
{
	aggregate->req.metrics.clear();
	for (auto it = aggregate->metricsMap.begin(); it != aggregate->metricsMap.end(); it++) {
		uint32_t id = it->first;
		std::shared_ptr<Metrics> metrics = it->second;

		int count = 0;
		if (metrics->typeName.compare(typeid(int64_t).name()) == 0) {
			int64_t avg = 0;
			for (auto mit = metrics->deque.begin(); mit != metrics->deque.end();) {
				if ((*mit)->timestamp <= start) {
					mit = metrics->deque.erase(mit);
				}
				else if ((*mit)->timestamp <= end) {
					count++;
					avg += std::any_cast<int64_t>((*mit)->value);
					mit = metrics->deque.erase(mit);
				}
				else {
					break;
				}
			}

			if (count > 0) {
				aggregate->req.metrics[id] = int64_t(avg / count);
			}
		}
		else if (metrics->typeName.compare(typeid(float).name()) == 0) {
			float avg = 0;
			for (auto mit = metrics->deque.begin(); mit != metrics->deque.end();) {
				if ((*mit)->timestamp <= start) {
					mit = metrics->deque.erase(mit);
				}
				else if ((*mit)->timestamp <= end) {
					count++;
					avg += std::any_cast<float>((*mit)->value);
					mit = metrics->deque.erase(mit);
				}
				else {
					break;
				}
			}

			if (count > 0) {
				aggregate->req.metrics[id] = float(avg / count);
			}
		}
		else if (metrics->typeName.compare(typeid(std::string).name()) == 0) {
			std::string value;
			for (auto mit = metrics->deque.begin(); mit != metrics->deque.end();) {
				if ((*mit)->timestamp <= start) {
					mit = metrics->deque.erase(mit);
				}
				else if ((*mit)->timestamp <= end) {
					count++;
					value = std::any_cast<std::string>((*mit)->value);
					mit = metrics->deque.erase(mit);
				}
				else {
					break;
				}
			}

			if (count > 0) {
				aggregate->req.metrics[id] = value;
			}
		}
	}
}

NS_HJ_END
