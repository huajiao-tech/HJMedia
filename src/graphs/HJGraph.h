#pragma once

#include "HJSyncObject.h"
#include "HJPlugin.h"

NS_HJ_BEGIN

#define IF_FALSE_BREAK(CONDITION, RET)		if (!(CONDITION)) { \
												ret = (RET); \
												break; \
											}

#define IF_FAIL_BREAK(CONDITION, RET)		if ((CONDITION) != HJ_OK) { \
												ret = (RET); \
												break; \
											}

class HJGraph : public HJSyncObject
{
public:
	HJ_DEFINE_CREATE(HJGraph);

	HJGraph(const std::string& i_name = "HJGraph", size_t i_identify = -1);

protected:
	using HJPluginsDeque = std::deque<HJPlugin::Ptr>;
	using HJPluginsDequeIt = HJPluginsDeque::iterator;
	using HJThreadsDeque = std::deque<HJLooperThread::Ptr>;
	using HJThreadsDequeIt = HJThreadsDeque::iterator;

	virtual void internalRelease() override;

	int connectPlugins(HJPlugin::Ptr i_src, HJPlugin::Ptr i_dst, HJMediaType i_type, int i_trackId = 0);
	void addPlugin(HJPlugin::Ptr i_plugin);
	void removePlugin(HJPlugin::Ptr i_plugin);
	void addThread(HJLooperThread::Ptr i_thread);
	void removeThread(HJLooperThread::Ptr i_thread);

	HJPluginsDeque m_plugins;
	HJThreadsDeque m_threads;
};

NS_HJ_END
