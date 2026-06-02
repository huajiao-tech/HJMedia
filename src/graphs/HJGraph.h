#pragma once

#include "HJSyncObject.h"
#include <vector>
#include "HJPlugin.h"
#include "HJInstanceSettings.h"
#include "HJCacheObserver.h"
//#include "HJData.h"
#include "HJGraphInfo.h"

NS_HJ_BEGIN

#define HJ_NAME_DECLARE(NAME) \
static const std::string NAME##_STRING = #NAME; \
static const constexpr uint64_t NAME##_HASH64 = HJMsgHash64(#NAME);

typedef enum HJGraphType
{
	HJGraphType_NONE = 0,
	HJGraphType_LIVESTREAM,
	HJGraphType_VOD,
	HJGraphType_MUSIC,
} HJGraphType;

class HJGraph : public HJSyncObject
{
public:
	using Ptr = std::shared_ptr<HJGraph>;
	using Wtr = std::weak_ptr<HJGraph>;
	using Information = HJNotification::Ptr;

	HJGraph(const std::string& i_name = "HJGraph", size_t i_identify = 0);
	virtual ~HJGraph();

//	virtual int setInfo(const Information i_info) = 0;
//	virtual int getInfo(Information io_info) = 0;
//	virtual bool hasAudio();
	void setSettings(HJInstanceSettings& i_settings) {
		m_settings.setSettings(i_settings);
	}
	const HJInstanceSettingsManager& getSettings() const {
		return m_settings;
	}
	const HJCacheObserver::Ptr& getCacheObserver() {
		return m_cacheObserver;
	}
/*
	// TODO：对于不同类型T，该如何处理？
	template <typename T, typename... Args>
	int addConditionTrigger(
		HJConditionTrigger::Condition i_condition,
		HJConditionTrigger::Callback i_callback,
		HJData<T>* first_data, Args... rest_datas) {
		std::vector<HJData<T>*> all_datas = { first_data, rest_datas... };
		HJConditionTrigger::Ptr trigger = std::make_shared<HJConditionTrigger>(i_condition, i_callback);
		for (auto data : all_datas) {
			data->addTrigger(trigger);
		}

		return HJ_OK;
	}
*/
	HJQueryBus::Ptr& queryBus() { return m_queryBus; }
	const HJQueryBus::Ptr& queryBus() const { return m_queryBus; }
	HJEventBus::Ptr& eventBus() { return m_eventBus; }
	const HJEventBus::Ptr& eventBus() const { return m_eventBus; }

protected:
//	using HJPluginsWtrDeque = std::deque<HJPlugin::Wtr>;
	using HJPluginsMap = std::unordered_map<uint32_t, HJPlugin::Ptr>;
	using HJPluginsMapIt = HJPluginsMap::iterator;
	using HJThreadsDeque = std::deque<HJLooperThread::Ptr>;
	using HJThreadsDequeIt = HJThreadsDeque::iterator;
//	using PluginStatuses = std::unordered_map<std::string, std::atomic<HJStatus>>;

	virtual void internalRelease() override;

	int connectPlugins(HJPlugin::Ptr i_src, HJPlugin::Ptr i_dst, HJMediaType i_type, int i_trackId = 0);
	void addPlugin(HJPlugin::Ptr i_plugin);
	void removePlugin(HJPlugin::Ptr i_plugin);
	void addThread(HJLooperThread::Ptr i_thread);
	void removeThread(HJLooperThread::Ptr i_thread);

	HJPluginsMap m_plugins;
	HJThreadsDeque m_threads;

	HJQueryBus::Ptr m_queryBus{};
	HJEventBus::Ptr m_eventBus{};

	HJInstanceSettingsManager m_settings;
	HJCacheObserver::Ptr m_cacheObserver{};

//	PluginStatuses m_pluginStatuses;
//	HJPluginsWtrDeque m_pluginQueueWtr;
};

// TODO：后续将HJGraphPlayer单独定义在一个文件中
class HJGraphPlayer : public HJGraph
{
public:
	using Ptr = std::shared_ptr<HJGraphPlayer>;

	HJGraphPlayer(const std::string& i_name = "HJGraphPlayer", size_t i_identify = 0);
	virtual ~HJGraphPlayer();
	
	static HJGraphPlayer::Ptr createGraph(HJGraphType i_type, int i_curIdx);

	virtual int openURL(HJMediaUrl::Ptr i_url) = 0;
	virtual int setMute(bool i_mute) = 0;
	virtual bool isMuted() = 0;
	virtual int setVolume(float i_volume) {
		(void)i_volume;
		return HJErrNotSupport;
	}
	virtual float getVolume() {
		return 1.0f;
	}
	virtual int pause() {
		return HJErrNotSupport;
	}
	virtual int resume() {
		return HJErrNotSupport;
	}
	virtual bool isPaused() const {
		return false;
	}
#if defined (WINDOWS)
	virtual int resetAudioDevice(const std::string& i_deviceName = "") = 0;
#endif
	virtual int64_t getCurrentTimestamp() {
		return 0;
	}
	virtual int64_t getDuration() {
		return 0;
	}
	virtual int seek(int64_t i_timestamp) {
		return HJErrNotSupport;
	}
	virtual std::vector<int> getAudioTrackIds() {
		return {};
	}
	virtual HJAudioTrackDisplayInfoVector getAudioTrackDisplayInfos() {
		return {};
	}
	virtual int switchAudioTrack(int i_trackId) {
		(void)i_trackId;
		return HJErrNotSupport;
	}

	virtual int setEnableSEIUUids(const std::string& i_uuid, bool bKeyMustCb) {
		return HJErrNotSupport;
	}

	virtual int setRepeats(int i_repeats) {
		(void)i_repeats;
		return HJErrNotSupport;
	}

protected:
//	using AudioDuration = std::unordered_map<std::string, HJData<int64_t>>;
//	using VideoFrames = std::unordered_map<std::string, HJData<int64_t>>;
//	using VideoKeyFrames = std::unordered_map<std::string, std::atomic<int64_t>>;

//	AudioDuration m_audioDuration;
//	VideoFrames m_videoFrames;
//	VideoKeyFrames m_videoKeyFrames;

	std::unordered_map<size_t, std::atomic<int64_t>> m_audioDuration;
	std::unordered_map<size_t, std::atomic<int64_t>> m_videoFrames;
	std::unordered_map<size_t, std::atomic<int64_t>> m_videoKeyFrames;
	std::atomic<uint32_t> m_mediaType{};

	virtual int registerEventHandler_mediaType();

public:
	virtual int openRecorder(HJKeyStorage::Ptr i_param) {
		(void)i_param;
		return HJErrNotSupport;
	}
	virtual void closeRecorder() {}
};

NS_HJ_END
