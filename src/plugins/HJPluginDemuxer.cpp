#include "HJPluginDemuxer.h"
#include "HJGraph.h"
#include "HJFLog.h"
#include "HJSEIWrapper.h"

NS_HJ_BEGIN

#define RUNTASKLog		HJFLogi

int HJPluginDemuxer::done()
{
	m_quitting.store(true);
	m_demuxerSync.consLock([this] {
		if (m_demuxer) {
			m_demuxer->setQuit(true);
		}
	});

	return HJPlugin::done();
}

int HJPluginDemuxer::internalInit(HJKeyStorage::Ptr i_param)
{
	MUST_HAVE_PARAMETERS;
	GET_PARAMETER(std::function<void()>, demuxNotify);
	GET_PARAMETER(HJMediaUrl::Ptr, mediaUrl);
	GET_PARAMETER(HJLooperThread::Ptr, thread);
	m_listener = i_param->getValue<HJListener>("pluginListener");

	auto param = HJKeyStorage::dupFrom(i_param);
	(*param)["createThread"] = (thread == nullptr);
	int ret = HJPlugin::internalInit(param);
	if (ret < 0) {
		return ret;
	}

	do {
		if (mediaUrl) {
			ret = initDemuxer(mediaUrl);
			if (ret != HJ_OK) {
				break;
			}
			m_mediaUrl = mediaUrl;
		}

		m_demuxNotify = demuxNotify;
		return HJ_OK;
	} while (false);

	HJPluginDemuxer::internalRelease();
	return ret;
}

void HJPluginDemuxer::internalRelease()
{
	HJFLogi("{}, internalRelease() begin", getName());

	releaseDemuxer();
//	m_currentFrame = nullptr;		// 可不可以不置空，等对象析构时自行释放？

	HJPlugin::internalRelease();

	HJFLogi("{}, internalRelease() end", getName());
}

void HJPluginDemuxer::afterInit()
{
	if (m_demuxer) {
		setStatus(HJSTATUS_Ready, false);
		postTask();
	}
}

int HJPluginDemuxer::openURL(HJMediaUrl::Ptr i_url)
{
	return SYNC_PROD_LOCK([=] {
		CHECK_DONE_STATUS(HJErrAlreadyDone);

		auto ret = initDemuxer(i_url);
		if (ret == HJ_OK) {
			m_mediaUrl = i_url;
		}

		return ret;
	});
}

int  HJPluginDemuxer::reset(uint64_t i_delay)
{
	m_demuxerSync.consLock([this] {
		m_resetting.store(true);
		if (m_demuxer) {
			m_demuxer->setQuit(true);
		}
	});

	return SYNC_PROD_LOCK([=] {
		CHECK_DONE_STATUS(HJErrAlreadyDone);
		if (!m_mediaUrl) {
			HJFLogi("{}, (!m_mediaUrl)", getName());
			return HJErrNotInited;
		}

		return initDemuxer(m_mediaUrl, i_delay);
	});
}
#if 0
int HJPluginDemuxer::runTask(int64_t* o_delay)
{
    addInIdx();
	int64_t enter = HJCurrentSteadyMS();
	bool log = false;
	if (m_lastEnterTimestamp < 0 || enter >= m_lastEnterTimestamp + LOG_INTERNAL) {
		m_lastEnterTimestamp = enter;
		log = true;
	}
	if (log) {
		RUNTASKLog("{}, enter", getName());
		if (m_demuxNotify) {
			m_demuxNotify();
		}
	}
    
	std::string route{};
	int ret = HJ_OK;
	HJMediaFrame::Ptr currentFrame{};
	do {
		ret = SYNC_CONS_LOCK([&route, &currentFrame, this] {
			if (m_status == HJSTATUS_Done) {
				route += "_0";
				return HJErrAlreadyDone;
			}
			if (m_status < HJSTATUS_Ready) {
				route += "_1";
				return HJ_WOULD_BLOCK;
			}
			if (m_status >= HJSTATUS_EOF) {
				route += "_2";
				return HJ_WOULD_BLOCK;
			}
			currentFrame = m_currentFrame;
			m_currentFrame = nullptr;	// m_currentFrame只在runTask()和interRelease()中修改和使用，所以可以在读锁中修改

			if (currentFrame == nullptr) {
				route += "_3";
				auto err = m_demuxer->getFrame(currentFrame);
				if (m_quitting.load() || m_resetting.load()) {
					route += "_4";
					HJFLogi("{}, m_demuxer->getFrame(), (m_quitting.load() || m_resetting.load())", getName());
					return HJ_STOP;
				}
				if (err < 0) {
					route += "_5";
					HJFLoge("{}, m_demuxer->getFrame(), error({})", getName(), err);
					return HJErrFatal;
				}
			}
			if (currentFrame == nullptr) {
				route += "_6";
				return HJ_WOULD_BLOCK;
			}

			currentFrame->m_streamIndex += m_streamIndexOffset;
			m_streamIndex = currentFrame->m_streamIndex;

			if (currentFrame->isEofFrame()) { 
				route += "_7";
				HJFLogi("{}, (currentFrame->isEofFrame())", getName());
				return HJ_EOF;
			}

			return HJ_OK;
		});
		if (ret == HJ_EOF) {
			if (m_pluginListener) {
				route += "_8";
				ret = m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_DEMUXER_EOF)));
				if (ret != HJ_OK) {
					IF_FALSE_BREAK(setStatus(HJSTATUS_EOF), HJErrAlreadyDone);
				}
			}
		}
		if (ret != HJ_OK) {
			if (ret == HJ_STOP) {
				IF_FALSE_BREAK(setStatus(HJSTATUS_Stoped), HJErrAlreadyDone);
			}
			else if (ret == HJErrFatal) {
				IF_FALSE_BREAK(setStatus(HJSTATUS_Exception), HJErrAlreadyDone);
				if (m_pluginListener) {
					route += "_7";
					ret = m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_ERROR_DEMUXER_GETFRAME)));
				}
			}
			break;
		}

		ret = canDeliverToOutputs(currentFrame);
		if (ret != HJ_OK) {
			route += "_8";
			if (ret == HJ_WOULD_BLOCK) {
				route += "_9";
				if (!SYNC_CONS_LOCK([currentFrame, this] {
					if (m_status == HJSTATUS_Done) {
						return false;
					}
					m_currentFrame = currentFrame;
					return true;
				})) {
					ret = HJErrAlreadyDone;
				}
			}
			else {
				route += "_10";
				HJFLoge("{}, canDeliverToOutputs error({})", getName(), ret);
				IF_FALSE_BREAK(setStatus(HJSTATUS_Exception), HJErrAlreadyDone);
			}
			break;
		}

		route += "_12";
		deliverToOutputs(currentFrame);

		if (currentFrame->isEofFrame()) {
			route += "_13";
			IF_FALSE_BREAK(setStatus(HJSTATUS_EOF), HJErrAlreadyDone);
		}
	} while (false);
    addOutIdx();
	if (log) {
		RUNTASKLog("{}, leave, route({}), task duration({}), ret({})", getName(), route, (HJCurrentSteadyMS() - enter), ret);
	}
	return ret;
}
#else
int HJPluginDemuxer::runTask(int64_t* o_delay)
{
	addInIdx();
	int64_t enter = HJCurrentSteadyMS();
	auto log = (m_lastEnterTimestamp < 0 || enter >= m_lastEnterTimestamp + LOG_INTERNAL);
	if (log) {
		m_lastEnterTimestamp = enter;
		RUNTASKLog("{}, enter", getName());
		if (m_demuxNotify) {
			m_demuxNotify();
		}
	}

	std::string route{};
	int ret = HJ_OK;
	do {
		if (m_currentFrame == nullptr) {
			route += "_0";
			ret = getFrameFromDemuxer(route);
			if (ret == HJ_EOF) {
				route += "_1";
				if (m_pluginListener) {
					route += "_2";
					ret = m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_DEMUXER_EOF)));
					if (ret != HJ_OK) {
						route += "_3";
						IF_FALSE_BREAK(setStatus(HJSTATUS_EOF), HJErrAlreadyDone);
					}
				}
			}
			if (ret != HJ_OK) {
				route += "_4";
				if (ret == HJ_STOP) {
					route += "_5";
					IF_FALSE_BREAK(setStatus(HJSTATUS_Stoped), HJErrAlreadyDone);
				}
				else if (ret == HJErrFatal) {
					route += "_6";
					IF_FALSE_BREAK(setStatus(HJSTATUS_Exception), HJErrAlreadyDone);
					if (m_pluginListener) {
						route += "_7";
						ret = m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_ERROR_DEMUXER_GETFRAME)));
					}
				}
				break;
			}
		}
	
		ret = canDeliverToOutputs(m_currentFrame);
		if (ret != HJ_OK) {
			route += "_8";
			if (ret < 0) {
				route += "_9";
				HJFLoge("{}, canDeliverToOutputs error({})", getName(), ret);
				IF_FALSE_BREAK(setStatus(HJSTATUS_Exception), HJErrAlreadyDone);
			}
			break;
		}

		deliverToOutputs(m_currentFrame);

		if (m_currentFrame->isEofFrame()) {
			route += "_10";
			IF_FALSE_BREAK(setStatus(HJSTATUS_EOF), HJErrAlreadyDone);
		}

		m_currentFrame = nullptr;
	} while (false);
	addOutIdx();
	if (log) {
		RUNTASKLog("{}, leave, route({}), task duration({}), ret({})", getName(), route, (HJCurrentSteadyMS() - m_lastEnterTimestamp), ret);
	}
	return ret;
}
#endif
void HJPluginDemuxer::deliverToOutputs(HJMediaFrame::Ptr& i_mediaFrame)
{
	if (i_mediaFrame->isVideo()) {
		(*i_mediaFrame)["passThroughDemuxSystemTime"] = HJCurrentSteadyMS();
		(*i_mediaFrame)["passThroughIsKey"] = i_mediaFrame->isKeyFrame();
//		HJFLogi("{}, videoFrame({})", getName(), i_mediaFrame->getPTS());
	}
	else if (i_mediaFrame->isAudio()) {
//		HJFLogi("{}, audioFrame({})", getName(), i_mediaFrame->getPTS());
	}
	else if (i_mediaFrame->isEofFrame()) {
		HJFLogi("{}, EOFFrame({})", getName(), i_mediaFrame->getPTS());
	}
	else {
		HJFLogi("{}, unknownFrame({})", getName(), i_mediaFrame->getPTS());
	}

	OutputMap outputs;
	m_outputSync.consLock([&outputs, this] {
		outputs = m_outputs;
	});
	for (auto it = outputs.begin(); it != outputs.end(); ++it) {
		auto output = it->second;
		auto plugin = output->plugin.lock();
		if (plugin != nullptr) {
			if (i_mediaFrame->isEofFrame() ||
				(i_mediaFrame->isVideo() && output->type & HJMEDIA_TYPE_VIDEO) ||
				(i_mediaFrame->isAudio() && output->type & HJMEDIA_TYPE_AUDIO)) {
				auto err = plugin->deliver(output->myKeyHash, i_mediaFrame);
				if (err < 0) {
					HJFLoge("{}, plugin->deliver() error({})", getName(), err);
				}
			}
		}
	}
}

int HJPluginDemuxer::initDemuxer(const HJMediaUrl::Ptr& i_url, uint64_t)
{
	int ret;
	do {
		m_demuxerSync.prodLock([this] {
			m_resetting.store(false);
			if (m_demuxer) {
				m_demuxer->done();
				m_streamIndexOffset = m_streamIndex + 1;
			}
			m_demuxer = createDemuxer();
		});
		if (m_demuxer == nullptr) {
			ret = HJErrFatal;
			break;
		}

		m_demuxer->setName(getName());
		ret = m_demuxer->init(i_url);
		if (m_quitting.load() || m_resetting.load()) {
			HJFLogi("{}, m_demuxer->init(), (m_quitting.load() || m_resetting.load())", getName());
			ret = HJ_STOP;
			break;
		}
		if (ret < 0) {
			HJFLoge("{}, m_demuxer->init(), error({})", getName(), ret);
			break;
		}

		m_mediaInfo = m_demuxer->getMediaInfo();
		if (!m_mediaInfo) {
			HJFLoge("{}, m_demuxer->getMediaInfo() error, (!m_mediaInfo)", getName());
			ret = HJErrFatal;
			break;
		}

		setInfoMediaType();
		return HJ_OK;
	} while (false);

	releaseDemuxer();
	return ret;
}

void HJPluginDemuxer::releaseDemuxer()
{
	m_demuxerSync.prodLock([this] {
		if (m_demuxer) {
			m_demuxer->done();
			m_demuxer = nullptr;
		}
	});
}

void HJPluginDemuxer::setInfoMediaType()
{
	auto graph = m_graph.lock();
	if (graph) {
		HJGraph::Information info = HJMakeNotification(HJ_PLUGIN_SETINFO_DEMUXER_mediaType);
		info->setName(HJSyncObject::getName());
		int mediaType = 0;
		if (m_mediaInfo) {
			if (m_mediaInfo->getVideoInfo()) {
				mediaType |= HJMEDIA_TYPE_VIDEO;
			}
			if (m_mediaInfo->getAudioInfo()) {
				mediaType |= HJMEDIA_TYPE_AUDIO;
			}
		}
		(*info)["mediaType"] = static_cast<HJMediaType>(mediaType);
		graph->setInfo(std::move(info));
	}
}

int HJPluginDemuxer::canDeliverToOutputs(const HJMediaFrame::Ptr& i_currentFrame)
{
	auto graph = m_graph.lock();
	if (!graph) {
		HJFLoge("{}, (!graph)", getName());
		return HJErrExcep;
	}

	auto param = HJMakeNotification(HJ_PLUGIN_GETINFO_DEMUXER_canDeliverToOutputs);
	(*param)["mediaFrame"] = i_currentFrame;
	return graph->getInfo(param);
}

int HJPluginDemuxer::getFrameFromDemuxer(std::string& route)
{
	return SYNC_CONS_LOCK([&route, this] {
		if (m_status == HJSTATUS_Done) {
			route += "_20";
			return HJErrAlreadyDone;
		}
		if (m_status < HJSTATUS_Ready) {
			route += "_21";
			return HJ_WOULD_BLOCK;
		}
		if (m_status >= HJSTATUS_EOF) {
			route += "_22";
			return HJ_WOULD_BLOCK;
		}

		auto err = m_demuxer->getFrame(m_currentFrame);
		if (m_quitting.load() || m_resetting.load()) {
			route += "_23";
			HJFLogi("{}, m_demuxer->getFrame(), (m_quitting.load() || m_resetting.load())", getName());
			return HJ_STOP;
		}
		if (err < 0) {
			route += "_24";
			HJFLoge("{}, m_demuxer->getFrame(), error({})", getName(), err);
			return HJErrFatal;
		}
		if (m_currentFrame == nullptr) {
			route += "_25";
			return HJ_WOULD_BLOCK;
		}

		m_currentFrame->m_streamIndex += m_streamIndexOffset;
		m_streamIndex = m_currentFrame->m_streamIndex;

		if (m_currentFrame->isEofFrame()) {
			route += "_26";
			HJFLogi("{}, (currentFrame->isEofFrame())", getName());
			return HJ_EOF;
		}

		proSEI(m_currentFrame);

		return HJ_OK;
	});
}

void HJPluginDemuxer::proSEI(const HJMediaFrame::Ptr& mvf)
{
	if (!m_listener) {
		return;
	}
	auto seiNals = mvf->getSEI();
	if (seiNals && seiNals->size() > 0)
	{
		std::vector<HJSEIData> userSEIDatas{};
		const auto& nals = seiNals->getDatas();
		for (const auto& nal : nals) {
			auto userDatas = HJSEIWrapper::parseSEINals(nal);
			//for (const auto& userData : userDatas) {
			//	std::string userMsg = std::string(userData.data.begin(), userData.data.end());
			//	HJFLogi("{}, sei nal uuid:{}, msg:{}", getName(), userData.uuid, userMsg);
			//}
			if (userDatas.size() > 0) {
				userSEIDatas.insert(userSEIDatas.end(), userDatas.begin(), userDatas.end());
			}
		}
		auto ntfy = HJMakeNotification(HJ_PLUGIN_NOTIFY_PLUGIN_SEI_INFOS);
		(*ntfy)["user_sei_datas"] = userSEIDatas;
		m_listener(std::move(ntfy));
	}
	return;	
}

NS_HJ_END
