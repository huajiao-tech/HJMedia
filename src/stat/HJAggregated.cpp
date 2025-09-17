#include "HJFLog.h" 
#include "HJAggregated.h"
#include "HJAggregatedMetrics.h"

NS_HJ_BEGIN

HJAggregatedContext::HJAggregatedContext()
{
	m_aggregated = new HJAggregatedMetrics();
	
}
HJAggregatedContext::~HJAggregatedContext()
{
	if (m_aggregated)
	{
		delete m_aggregated;
		m_aggregated = nullptr;		
	}
}
int HJAggregatedContext::init()
{
	int i_err = m_aggregated->init();
	if (i_err < 0)
	{
		HJFLoge("HJAggregatedMetrics int error");
	}
	return i_err;
}
HJAggregatedMetrics* HJAggregatedContext::getAggregated()
{
	return m_aggregated;
}

////////////////////////////////////////////////////////////////////
HJAggregatedIns::HJAggregatedIns()
{

}
HJAggregatedIns::~HJAggregatedIns()
{
	if ((m_handle > 0) && m_context)
	{
		m_context->destroyAggregate(m_handle);
	}
}

int HJAggregatedIns::init(HJAggregatedMetrics* i_context, int interval, int64_t uid, const std::string& device, int sceneValue, HJAggregatedMetrics::MetricsReqCallback callback)
{
	int i_err = 0;
	do
	{
		m_context = i_context;

		m_handle = m_context->createAggregate(interval, uid, device, sceneValue, callback);
		if (m_handle < 0)
		{
			HJFLoge("createAggregate error:{}", m_handle);
			break;
		}
	} while (false);
	return i_err;
}

int HJAggregatedIns::addSceneInt64(uint32_t id, int64_t value)
{
	int i_err = 0;
	do
	{
		if ((m_handle <= 0) || !m_context)
		{
			i_err = -1;
			HJFLoge("param error m_handle:{}", m_handle);
			break;
		}

		i_err = m_context->addSceneInt64(m_handle, id, value);

	} while (false);
	return i_err;
}
int HJAggregatedIns::addSceneStr(uint32_t id, const std::string value)
{
	int i_err = 0;
	do
	{
		if ((m_handle <= 0) || !m_context)
		{
			i_err = -1;
			HJFLoge("param error m_handle:{}", m_handle);
			break;
		}

		i_err = m_context->addSceneStr(m_handle, id, value);
	} while (false);
	return i_err;
}

int HJAggregatedIns::addMetricInt64(int64_t timestamp, uint32_t id, int64_t value)
{
	int i_err = 0;
	do
	{
		if ((m_handle <= 0) || !m_context)
		{
			i_err = -1;
			HJFLoge("param error m_handle:{}", m_handle);
			break;
		}
		i_err = m_context->addMetricInt64(m_handle, timestamp, id, value);


	} while (false);
	return i_err;
}
int HJAggregatedIns::addMetricFloat(int64_t timestamp, uint32_t id, float value)
{
	int i_err = 0;
	do
	{
		if ((m_handle <= 0) || !m_context)
		{
			i_err = -1;
			HJFLoge("param error m_handle:{}", m_handle);
			break;
		}

		i_err = m_context->addMetricFloat(m_handle, timestamp, id, value);

	} while (false);
	return i_err;
}
int HJAggregatedIns::addMetricStr(int64_t timestamp, uint32_t id, const std::string value)
{
	int i_err = 0;
	do
	{
		if ((m_handle <= 0) || !m_context)
		{
			i_err = -1;
			HJFLoge("param error m_handle:{}", m_handle);
			break;
		}
		i_err = m_context->addMetricStr(m_handle, timestamp, id, value);
	} while (false);
	return i_err;
}


NS_HJ_END