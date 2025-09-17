#pragma once

#include "HJSyncObject.h"

NS_HJ_BEGIN

class HJTimeline : public HJSyncObject
{
public:
	HJ_DEFINE_CREATE(HJTimeline);
	using Listener = std::function<void()>;

	HJTimeline(const std::string& i_name = "HJTimeline", size_t i_identify = -1)
		: HJSyncObject(i_name, i_identify) { }
	virtual ~HJTimeline() {
		HJTimeline::done();
	};

	void addListener(const std::string& i_name, Listener i_listener);
	void removeListener(const std::string& i_name);
	bool setTimestamp(int64_t i_streamIndex, int64_t i_timestamp, double i_speed);
	bool getTimestamp(int64_t& o_streamIndex, int64_t& o_timestamp, double& o_speed);
	void flush();
	void play();
	void pause();

protected:
	void internalRelease() override;

	void notifyUpdated();

	using ListenerMap = std::unordered_map<std::string, Listener>;
	using ListenerMapIt = ListenerMap::iterator;

	ListenerMap m_listeners;
	double m_speed{ 1.0f };
	int64_t m_streamIndex{ -1 };
	int64_t m_timestamp{ -1 };
	int64_t m_sysTimestamp{ -1 };
	bool m_paused{};
//	int64_t m_syncOffset{};
//	HJSync m_timestampSync;

	int64_t m_updateTime{ -1 };
};

NS_HJ_END
