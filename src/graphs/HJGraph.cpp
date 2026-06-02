#include "HJGraph.h"
#include "HJFLog.h"
#include "HJGraphLivePlayer.h"
#include "HJGraphVodPlayer.h"
#include "HJGraphMusicPlayer.h"
#include "HJCoreVersion.h"
NS_HJ_BEGIN

HJGraph::HJGraph(const std::string& i_name, size_t i_identify)
	: HJSyncObject(i_name, i_identify)
{
	m_queryBus = std::make_shared<HJQueryBus>();
	m_eventBus = std::make_shared<HJEventBus>();
}

HJGraph::~HJGraph()
{
	done();
	m_queryBus = nullptr;
	m_eventBus = nullptr;
}

void HJGraph::internalRelease()
{
	for (auto it : m_plugins) {
		it.second->done();
	}
	m_plugins.clear();

	for (auto thread : m_threads) {
		thread->done();
	}
	m_threads.clear();

	HJSyncObject::internalRelease();
}
/*
bool HJGraph::hasAudio()
{
	return false;
}
*/
int HJGraph::connectPlugins(HJPlugin::Ptr i_src, HJPlugin::Ptr i_dst, HJMediaType i_type, int i_trackId)
{
	if (i_src == nullptr || i_dst == nullptr || i_trackId < 0) {
		return HJErrInvalidParams;
	}

	int ret;
	do {
		ret = i_src->addOutputPlugin(i_dst, i_type, i_trackId);
		if (ret < 0) {
			break;
		}

		ret = i_dst->addInputPlugin(i_src, i_type, i_trackId);
		if (ret < 0) {
			break;
		}

		return HJ_OK;
	} while (false);

	HJFLogi("{}, connectPlugins, error({})", getName(), ret);
	return ret;
}

void HJGraph::addPlugin(HJPlugin::Ptr i_plugin)
{
	m_plugins[i_plugin->getID()] = i_plugin;
}

void HJGraph::removePlugin(HJPlugin::Ptr i_plugin)
{
	m_plugins.erase(i_plugin->getID());
	i_plugin->done();
}

void HJGraph::addThread(HJLooperThread::Ptr i_thread)
{
	m_threads.push_back(i_thread);
}

void HJGraph::removeThread(HJLooperThread::Ptr i_thread)
{
	for (auto it = m_threads.begin(); it != m_threads.end(); it++) {
		if (*it == i_thread) {
			m_threads.erase(it);
			i_thread->done();
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////

HJGraphPlayer::HJGraphPlayer(const std::string& i_name, size_t i_identify):
	HJGraph(i_name, i_identify)
{
	HJFLogi("hjmedia version: {} core version: {}", HJ_VERSION, HJCoreVersion::getVersionDetail());
}
HJGraphPlayer::~HJGraphPlayer()
{
	done();
}

HJGraphPlayer::Ptr HJGraphPlayer::createGraph(HJGraphType i_type, int i_curIdx)
{
	HJGraphPlayer::Ptr graph = nullptr;
	switch (i_type)
	{
	case HJGraphType_LIVESTREAM:
		graph = HJGraphLivePlayer::Create<HJGraphLivePlayer>("HJGraphLivePlayer" + HJFMT("_{}", i_curIdx));
		HJFLogi("HJGraphLivePlayer createGraph");
		break;
	case HJGraphType_VOD:
		graph = HJGraphVodPlayer::Create<HJGraphVodPlayer>("HJGraphVodPlayer" + HJFMT("_{}", i_curIdx));
		HJFLogi("HJGraphVodPlayer createGraph");
		break;
	case HJGraphType_MUSIC:
		graph = HJGraphMusicPlayer::Create<HJGraphMusicPlayer>("HJGraphMusicPlayer" + HJFMT("_{}", i_curIdx));
		HJFLogi("HJGraphMusicPlayer createGraph");
		break;
	default:
		break;
	}
	return graph;
}


int HJGraphPlayer::registerEventHandler_mediaType()
{
	return m_eventBus->registerHandler(EVENT_MEDIA_TYPE_ID, [this](uint32_t mediaType) {
		m_mediaType.store(mediaType);
	});
}

NS_HJ_END
