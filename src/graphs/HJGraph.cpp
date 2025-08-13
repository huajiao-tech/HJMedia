#include "HJGraph.h"
#include "HJFLog.h"

NS_HJ_BEGIN

HJGraph::HJGraph(const std::string& i_name, size_t i_identify)
	: HJSyncObject(i_name, i_identify)
{
}

void HJGraph::internalRelease()
{
	for (auto plugin : m_plugins) {
		plugin->done();
	}
	m_plugins.clear();

	for (auto thread : m_threads) {
		thread->done();
	}
	m_threads.clear();

	HJSyncObject::internalRelease();
}

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
	m_plugins.push_back(i_plugin);
}

void HJGraph::removePlugin(HJPlugin::Ptr i_plugin)
{
	for (auto it = m_plugins.begin(); it != m_plugins.end(); it++) {
		if (*it == i_plugin) {
			m_plugins.erase(it);
			i_plugin->done();
			break;
		}
	}
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

NS_HJ_END
