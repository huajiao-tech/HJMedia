#pragma once


#include "HJPrerequisites.h"
#include "HJJsonBase.h"
//#include <nlohmann/json.hpp>

//using json = nlohmann::json;

NS_HJ_BEGIN

typedef struct HJMQComplex
{
	uint32_t id = 0;
	std::any value;
} HJMQComplex;


class HJMonitorJsonInfo : public HJJsonBase
{
public:
	HJMonitorJsonInfo();
	HJMonitorJsonInfo(int64_t i_uid, const std::string& i_device, int64_t i_timestamp, int i_sceneType);
	virtual ~HJMonitorJsonInfo();

	void setUid(int64_t i_uid)
	{
		uid = i_uid;
	}
	void setDevice(const std::string& i_device)
	{
		device = i_device;
	}
	void setTimeStamp(int64_t i_timestamp)
	{
		timestamp = i_timestamp;
	}
	void setSceneType(int i_sceneType)
	{
		sceneType = i_sceneType;
	}

	void addSceneAny(uint32_t i_id, std::any i_value);
	void addMetricsAny(uint32_t i_id, std::any i_value);

	void addScene(uint32_t i_id, const std::string& i_value);
	void addMetrics(uint32_t i_id, int64_t i_value);
	void addMetrics(uint32_t i_id, float i_value);
	void addMetrics(uint32_t i_id, const std::string& i_value);

	virtual int deserialInfo(const HJYJsonObject::Ptr& i_obj = nullptr) override;
	virtual int serialInfo(const HJYJsonObject::Ptr& i_obj = nullptr) override;

private:
	int priAddItem(int i_id, const std::any& i_value, HJYJsonObject::Ptr& io_obj);
	int priAddScene();
	int priAddMetrics();

	int64_t uid = 0;
	std::string device = "windows";
	int business = 1;
	int64_t timestamp = 0;

	int sceneType = 0;

	std::vector<HJMQComplex> sceneinfo;
	std::vector<HJMQComplex> metrics;
};

NS_HJ_END

