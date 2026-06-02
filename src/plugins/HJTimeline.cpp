#include "HJTimeline.h"
#include "HJTime.h"
#include "HJFLog.h"

NS_HJ_BEGIN

void HJTimeline::internalRelease()
{
	HJFLogi("{}, internalRelease() begin", getName());

	m_listeners.clear();
	m_streamIndex = -1;
	m_paused = false;
	HJSyncObject::internalRelease();

	HJFLogi("{}, internalRelease() end", getName());
}

void HJTimeline::addListener(const std::string& i_name, HJListener i_listener)
{
	if (i_listener != nullptr) {
		SYNC_PROD_LOCK([this, i_name, i_listener] {
			CHECK_DONE_STATUS();

			m_listeners[i_name] = i_listener;
		});
	}
}

void HJTimeline::removeListener(const std::string& i_name)
{
	SYNC_PROD_LOCK([this, i_name] {
		CHECK_DONE_STATUS();

		m_listeners.erase(i_name);
	});
}

bool HJTimeline::setTimestamp(int64_t i_streamIndex, int64_t i_timestamp, double i_speed)
{
	if (i_streamIndex < 0 || i_speed <= 0.0f) {
		return false;
	}

	auto ret = SYNC_PROD_LOCK([this, i_streamIndex, i_timestamp, i_speed] {
		CHECK_DONE_STATUS(false);
		if (i_streamIndex < m_streamIndex) {
			return false;
		}

		m_streamIndex = i_streamIndex;
		m_timestamp = i_timestamp;
		m_speed = i_speed;
		if (!m_paused) {
			m_sysTimestamp = HJCurrentSteadyMS();
		}

		//notify(std::move(HJMakeNotification(HJ_TIMELINE_NOTIFY_UPDATED)));

		auto time = HJCurrentSteadyMS();
		if (m_updateTime < 0 || time >= m_updateTime + 2000) {
			m_updateTime = time;
			HJFLogi("{} setTimestamp(i_streamIndex({}), i_timestamp({}), i_speed({}))", getName(), i_streamIndex, i_timestamp, i_speed);
		}

		return true;
	});

	if (ret) {
		notify(std::move(HJMakeNotification(HJ_TIMELINE_NOTIFY_UPDATED)));
	}

	return ret;
}

bool HJTimeline::getTimestamp(int64_t& o_streamIndex, int64_t& o_timestamp, double& o_speed)
{
	return SYNC_CONS_LOCK([this, &o_streamIndex, &o_timestamp, &o_speed] {
		CHECK_DONE_STATUS(false);
		if (m_streamIndex < 0) {
			return false;
		}

		o_streamIndex = m_streamIndex;
		if (m_paused) {
			o_timestamp = m_timestamp;
		}
		else {
			int64_t now = HJCurrentSteadyMS();
			o_timestamp = m_timestamp + ((int64_t)((now - m_sysTimestamp) * m_speed));// +m_syncOffset;
		}
		o_speed = m_speed;

		return true;
	});
}

void HJTimeline::flush()
{
	SYNC_PROD_LOCK([this] {
		CHECK_DONE_STATUS();

		m_streamIndex = -1;
		m_paused = false;
	});
}

void HJTimeline::play()
{
	auto ret = SYNC_PROD_LOCK([this] {
		CHECK_DONE_STATUS(false);

		if (m_paused) {
			m_sysTimestamp = HJCurrentSteadyMS();
			m_paused = false;

			return true;
		}

		return false;
	});
	if (ret) {
		notify(HJMakeNotification(HJ_TIMELINE_NOTIFY_PLAY));
	}
}

void HJTimeline::pause()
{
	auto ret = SYNC_PROD_LOCK([this] {
		CHECK_DONE_STATUS(false);

		if (!m_paused) {
			if (m_streamIndex >= 0) {
				int64_t now = HJCurrentSteadyMS();
				m_timestamp = m_timestamp + (now - m_sysTimestamp) * m_speed;
			}
			m_paused = true;

			return true;
		}

		return false;
	});
	if (ret) {
		notify(HJMakeNotification(HJ_TIMELINE_NOTIFY_PAUSE));
	}
}

void HJTimeline::notify(HJNotification::Ptr i_ntf)
{
	ListenerMap listeners;
    SYNC_CONS_LOCK([&listeners, this] {
		CHECK_DONE_STATUS();
		listeners = m_listeners;
	});

	for (const auto& [name, listener] : listeners) {
		listener(i_ntf);
	}
}

NS_HJ_END
