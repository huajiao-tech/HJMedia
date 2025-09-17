#include "HJMonitorJsonInfo.h"
#include "HJFLog.h"

NS_HJ_BEGIN

HJMonitorJsonInfo::HJMonitorJsonInfo()
{

}
HJMonitorJsonInfo::HJMonitorJsonInfo(int64_t i_uid, const std::string& i_device, int64_t i_timestamp, int i_sceneType) :
	uid(i_uid),
	device(i_device),
	timestamp(i_timestamp),
	sceneType(i_sceneType)
{
	
}
HJMonitorJsonInfo::~HJMonitorJsonInfo()
{

}
void HJMonitorJsonInfo::addSceneAny(uint32_t i_id, std::any i_value)
{
	HJMQComplex complex = { i_id, i_value };
	sceneinfo.emplace_back(complex);
}
void HJMonitorJsonInfo::addMetricsAny(uint32_t i_id, std::any i_value)
{
	HJMQComplex complex = { i_id, i_value };
	metrics.emplace_back(complex);
}

void HJMonitorJsonInfo::addScene(uint32_t i_id, const std::string& i_value)
{
	addSceneAny(i_id, (std::string)i_value);
}
void HJMonitorJsonInfo::addMetrics(uint32_t i_id, int64_t i_value)
{
	addMetricsAny(i_id, (int64_t)i_value);
}
void HJMonitorJsonInfo::addMetrics(uint32_t i_id, float i_value)
{
	addMetricsAny(i_id, (float)i_value);
}
void HJMonitorJsonInfo::addMetrics(uint32_t i_id, const std::string& i_value)
{
	addMetricsAny(i_id, (std::string)i_value);
}
//
//static void priGetJson(json& jJsonArray, std::vector<HJMQComplex>& i_array)
//{
//	for (int i = 0; i < i_array.size(); i++)
//	{
//		json valItem;
//		HJMQComplex& complex = i_array[i];
//		valItem["id"] = complex.id;
//		const std::type_info& typeInfo = complex.value.type();
//		if (typeInfo == typeid(int64_t))
//		{
//			valItem["value"] = std::any_cast<int64_t>(complex.value);
//		}
//		else if (typeInfo == typeid(float))
//		{
//			valItem["value"] = std::any_cast<float>(complex.value);
//		}
//		else if (typeInfo == typeid(std::string))
//		{
//			valItem["value"] = std::any_cast<std::string>(complex.value);
//		}
//		jJsonArray.emplace_back(valItem);
//	}
//}
//
//void HJMonitorJsonInfo::priAddScene(json& MetricsInfo)
//{
//	json scene;
//	scene["value"] = sceneType;
//
//	json jArray = json::array();
//	if (!sceneinfo.empty())
//	{		
//		priGetJson(jArray, sceneinfo);	
//	}
//	scene["sceneinfo"] = jArray;
//	MetricsInfo["scene"] = scene;
//}
//void HJMonitorJsonInfo::priAddMetrics(json& MetricsInfo)
//{
//	if (!metrics.empty())
//	{
//		json jArray = json::array();
//		priGetJson(jArray, metrics);
//		MetricsInfo["metrics"] = jArray;
//	}
//}

int HJMonitorJsonInfo::deserialInfo(const HJYJsonObject::Ptr& i_obj)
{
	return 0;
}

int HJMonitorJsonInfo::priAddItem(int i_id, const std::any& i_value, HJYJsonObject::Ptr& io_itemObj)
{
	int i_err = HJ_OK;
	do
	{
		io_itemObj->setMember("id", (int)i_id);
		const std::type_info& typeInfo = i_value.type();
		if (typeInfo == typeid(int64_t))
		{
			io_itemObj->setMember("value", (int64_t)std::any_cast<int64_t>(i_value));
		}
		else if (typeInfo == typeid(float))
		{
			io_itemObj->setMember("value", (double)std::any_cast<float>(i_value));
		}
		else if (typeInfo == typeid(std::string))
		{
			io_itemObj->setMember("value", (std::string)std::any_cast<std::string>(i_value));
		}
	} while (false);
	return i_err;
}
int HJMonitorJsonInfo::priAddScene()
{
	int i_err = HJ_OK;
	do
	{
		if (m_obj)
		{
			HJYJsonObject::Ptr scnObj = std::make_shared<HJYJsonObject>("scene", m_obj);
			m_obj->addObj(scnObj);
			scnObj->setMember("value", (int)sceneType);

			std::vector<HJYJsonObject::Ptr> scnItems;
			for (auto item : sceneinfo)
			{
				auto itemObj = std::make_shared<HJYJsonObject>("scnItem", m_obj);

				i_err = priAddItem(item.id, item.value, itemObj);
				if (i_err != HJ_OK)
				{
					break;
				}
				scnItems.push_back(itemObj);
			}
			scnObj->setMember("sceneinfo", scnItems);
		}
	} while (false);
	return i_err;
}
int HJMonitorJsonInfo::priAddMetrics()
{
	int i_err = HJ_OK;
	do
	{
		if (m_obj)
		{
			std::vector<HJYJsonObject::Ptr> metricItems;
			if (!metrics.empty())
			{
				for (auto item : metrics)
				{
					auto itemObj = std::make_shared<HJYJsonObject>("metricsItem", m_obj);

					i_err = priAddItem(item.id, item.value, itemObj);
					if (i_err != HJ_OK)
					{
						break;
					}
					metricItems.push_back(itemObj);
				}
				m_obj->setMember("metrics", metricItems);
			}
		}
	} while (false);
	return i_err;
}
int HJMonitorJsonInfo::serialInfo(const HJYJsonObject::Ptr& i_obj)
{
	int i_err = HJ_OK;
	do
	{
		HJ_JSON_BASE_SERIAL(m_obj, i_obj, uid, device, timestamp, business);
		i_err = priAddScene();
		if (i_err < 0)
		{
			break;
		}
		i_err = priAddMetrics();
		if (i_err < 0)
		{
			break;
		}
	} while (false);
	return i_err;
}

NS_HJ_END
