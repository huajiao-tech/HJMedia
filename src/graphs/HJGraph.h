#pragma once

#include "HJSyncObject.h"
#include "HJPlugin.h"
#include "HJInstanceSettings.h"
#include "HJCacheObserver.h"

NS_HJ_BEGIN

typedef enum HJGraphType
{
	HJGraphType_NONE = 0,
	HJGraphType_LIVESTREAM,
	HJGraphType_VOD,
} HJGraphType;


#define IF_FALSE_BREAK(CONDITION, RET)		if (!(CONDITION)) { \
												ret = (RET); \
												break; \
											}

#define IF_FAIL_BREAK(CONDITION, RET)		if ((CONDITION) != HJ_OK) { \
												ret = (RET); \
												break; \
											}

#define GET_INFO(TYPE, NANE)	auto NANE = (i_info != nullptr) ? i_info->getValue<TYPE>(#NANE) : (TYPE)0

class HJGraph : public HJSyncObject
{
public:
	using Ptr = std::shared_ptr<HJGraph>;
	using Wtr = std::weak_ptr<HJGraph>;
	using Information = HJNotification::Ptr;

	HJGraph(const std::string& i_name = "HJGraph", size_t i_identify = -1);
	virtual ~HJGraph();

	virtual int setInfo(const Information i_info) = 0;
	virtual int getInfo(Information io_info) = 0;
	virtual bool hasAudio();
	void setSettings(HJInstanceSettings& i_settings) {
		m_settings.setSettings(i_settings);
	}
	const HJInstanceSettingsManager& getSettings() const {
		return m_settings;
	}
	// _TODO_ ��������ָ������ã�
	const HJCacheObserver::Ptr& getCacheObserver() {
		return m_cacheObserver;
	}

protected:
	using HJPluginsDeque = std::deque<HJPlugin::Ptr>;
    using HJPluginsWtrDeque = std::deque<HJPlugin::Wtr>;
	using HJPluginsDequeIt = HJPluginsDeque::iterator;
	using HJThreadsDeque = std::deque<HJLooperThread::Ptr>;
	using HJThreadsDequeIt = HJThreadsDeque::iterator;
	using PluginStatuses = std::unordered_map<std::string, std::atomic<HJStatus>>;

	virtual void internalRelease() override;

	int connectPlugins(HJPlugin::Ptr i_src, HJPlugin::Ptr i_dst, HJMediaType i_type, int i_trackId = 0);
	void addPlugin(HJPlugin::Ptr i_plugin);
	void removePlugin(HJPlugin::Ptr i_plugin);
	void addThread(HJLooperThread::Ptr i_thread);
	void removeThread(HJLooperThread::Ptr i_thread);

	HJPluginsDeque m_plugins;
	HJThreadsDeque m_threads;
	HJInstanceSettingsManager m_settings;
	HJCacheObserver::Ptr m_cacheObserver{};

	PluginStatuses m_pluginStatuses;
    HJPluginsWtrDeque m_pluginQueueWtr;
};

class HJGraphPlayer : public HJGraph
{
public:
	using Ptr = std::shared_ptr<HJGraphPlayer>;

	HJGraphPlayer(const std::string& i_name = "HJGraphPlayer", size_t i_identify = -1);
	virtual ~HJGraphPlayer();
	
	static HJGraphPlayer::Ptr createGraph(HJGraphType i_type, int i_curIdx);

	virtual int openURL(HJMediaUrl::Ptr i_url) = 0;
	virtual int setMute(bool i_mute) = 0;
	virtual bool isMuted() = 0;

protected:
	using FrameSizes = std::unordered_map<std::string, std::atomic<size_t>>;
	using AudioDurations = std::unordered_map<std::string, std::atomic<int64_t>>;
	using VideoKeyFrames = std::unordered_map<std::string, std::atomic<int64_t>>;

	FrameSizes m_frameSizes;
	AudioDurations m_audioDurations;
	VideoKeyFrames m_videoKeyFrames;
	std::atomic<HJMediaType> m_mediaType{};
	HJSync m_mediaTypeSync;
};

NS_HJ_END
