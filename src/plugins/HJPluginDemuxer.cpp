#include "HJPluginDemuxer.h"
#include "HJGraph.h"
#include "HJFLog.h"
#include "HJSEIWrapper.h"

NS_HJ_BEGIN

#define RUNTASKLog		HJFLogi

int HJPluginDemuxer::internalInit(HJKeyStorage::Ptr i_param)
{
	MUST_HAVE_PARAMETERS;
//	GET_PARAMETER(std::function<void()>, demuxNotify);
	GET_PARAMETER(HJMediaUrl::Ptr, mediaUrl);
	GET_PARAMETER(HJLooperThread::Ptr, thread);

	auto param = HJKeyStorage::dupFrom(i_param);
	(*param)["createThread"] = (thread == nullptr);
	auto ret = HJPlugin::internalInit(param);
	if (ret != HJ_OK) {
		return ret;
	}

	m_runInitId = m_handler->genMsgId();
	m_runSeekId = m_handler->genMsgId();
	m_runSwitchAudioTrackId = m_handler->genMsgId();

	if (mediaUrl) {
		ret = postInit(mediaUrl, 0, InitReason::InternalInit);
		if (ret != HJ_OK) {
			return ret;
		}

		m_mediaUrl = mediaUrl;
	}

//	m_demuxNotify = demuxNotify;
	return HJ_OK;
}

void HJPluginDemuxer::internalRelease()
{
	releaseDemuxer();

	HJPlugin::internalRelease();
}

int HJPluginDemuxer::beforeDone()
{
	quitDemuxer();

	return HJPlugin::beforeDone();
}

int HJPluginDemuxer::openURL(HJMediaUrl::Ptr i_url)
{
	if (i_url == nullptr) {
		return HJErrInvalidParams;
	}

	auto initIndex = quitDemuxer();

	return SYNC_PROD_LOCK([=] {
		CHECK_DONE_STATUS(HJErrAlreadyDone);

		auto ret = postInit(i_url, initIndex, InitReason::OpenURL);
		if (ret != HJ_OK) {
			return ret;
		}

		m_mediaUrl = i_url;
		return HJ_OK;
	});
}

int  HJPluginDemuxer::reset(uint64_t i_delay)
{
	auto initIndex = quitDemuxer();

	return SYNC_PROD_LOCK([=] {
		CHECK_DONE_STATUS(HJErrAlreadyDone);
		if (!m_mediaUrl) {
			HJFLogi("{}, (!m_mediaUrl)", getName());
			return HJErrNotInited;
		}

		return postInit(m_mediaUrl, initIndex, InitReason::Reset, i_delay);
	});
}

int HJPluginDemuxer::seek(int64_t i_timestamp)
{
    HJFLogi("i_timestamp:{}", i_timestamp);
	HJLooperThread::Handler::Ptr handler{};
	int runTaskId{}, runSeekId{};
	auto ret = SYNC_CONS_LOCK([this, &handler, &runTaskId, &runSeekId] {
		CHECK_DONE_STATUS(HJErrAlreadyDone);
		handler = m_handler;
		runTaskId = m_runTaskId;
		runSeekId = m_runSeekId;
		return HJ_OK;
	});
	if (ret != HJ_OK) {
		return ret;
	}

	if (handler != nullptr) {
		handler->removeMessages(runTaskId);
		Wtr wPlugin = SHARED_FROM_THIS;
		handler->asyncAndClear([wPlugin, i_timestamp] {
			auto plugin = wPlugin.lock();
			if (plugin) {
				if (plugin->runSeek(i_timestamp) == HJ_OK) {
					plugin->postTask(0);
				}
			}
		}, runSeekId);
	}

	return HJ_OK;
}
#if 1
int HJPluginDemuxer::switchAudioTrack(int i_trackId)
{
	return SYNC_PROD_LOCK([this, i_trackId] {
		CHECK_DONE_STATUS(HJErrAlreadyDone);
		if (!m_demuxer) {
			return HJErrNotInited;
		}
		auto ret = m_demuxer->switchAudioTrack(i_trackId);
		if (ret == HJ_OK) {
			m_audioTrackId = i_trackId;
		}
		return ret;
	});
}
#else
int HJPluginDemuxer::switchAudioTrack(int i_trackId)
{
	HJLooperThread::Handler::Ptr handler{};
	int runTaskId{}, runSwitchAudioTrackId{};
	auto ret = SYNC_CONS_LOCK([this, &handler, &runTaskId, &runSwitchAudioTrackId] {
		CHECK_DONE_STATUS(HJErrAlreadyDone);
		handler = m_handler;
		runTaskId = m_runTaskId;
		runSwitchAudioTrackId = m_runSwitchAudioTrackId;
		return HJ_OK;
		});
	if (ret != HJ_OK) {
		return ret;
	}

	if (handler != nullptr) {
		handler->removeMessages(runTaskId);
		Wtr wPlugin = SHARED_FROM_THIS;
		handler->asyncAndClear([wPlugin, i_trackId] {
			auto plugin = wPlugin.lock();
			if (plugin) {
				if (plugin->runSwitchAudioTrack(i_trackId) == HJ_OK) {
					plugin->postTask(0);
				}
			}
			}, runSwitchAudioTrackId);
	}

	return HJ_OK;
}
#endif
HJMediaInfo::Ptr HJPluginDemuxer::getMediaInfo()
{
	return SYNC_CONS_LOCK([this] {
		CHECK_DONE_STATUS(static_cast<HJMediaInfo::Ptr>(nullptr));
		if (!m_demuxer) {
			return static_cast<HJMediaInfo::Ptr>(nullptr);
		}
		return m_demuxer->getMediaInfo();
	});
}

uint32_t HJPluginDemuxer::getMediaType() {
	return SYNC_CONS_LOCK([this] {
		return m_mediaType;
	});
}

int HJPluginDemuxer::runSeek(int64_t i_timestamp)
{
	bool setReady = false;
	int ret = SYNC_PROD_LOCK([this, &setReady, i_timestamp] {
		CHECK_DONE_STATUS(HJErrAlreadyDone);
		if (m_status < HJSTATUS_Ready) {
			return HJ_WOULD_BLOCK;
		}
		if (m_status >= HJSTATUS_Stoped) {
			return HJ_WOULD_BLOCK;
		}

		auto err = m_demuxer->seek(i_timestamp);
		if (m_quittingDemuxer.load()) {
			HJFLogi("{}, m_demuxer->seek(), (m_quittingDemuxer.load())", getName());
			return HJ_STOP;
		}
		if (err < 0) {
			HJFLoge("{}, m_demuxer->seek(), error({})", getName(), err);
			return HJErrFatal;
		}

		if (m_status == HJSTATUS_EOF) {
			setReady = true;
		}
		return HJ_OK;
	});
	if (ret == HJ_OK) {
		if (setReady) {
			IF_FALSE_RETURN(setStatus(HJSTATUS_Ready), HJErrAlreadyDone);
		}

		m_currentFrameSync.prodLock([this] {
			m_currentFrame = nullptr;
		});
		runFlush();
		report(EVENT_SEEK_SUCCEEDED_ID, getID());
	}
	else if (ret == HJ_STOP) {
		IF_FALSE_RETURN(setStatus(HJSTATUS_Stoped), HJErrAlreadyDone);
	}
	else if (ret == HJErrFatal) {
		IF_FALSE_RETURN(setStatus(HJSTATUS_Exception), HJErrAlreadyDone);

		report(EVENT_PLUGIN_NOTIFY_ID, HJ_PLUGIN_NOTIFY_ERROR_DEMUXER_SEEK, getID());
	}

	return ret;
}

int HJPluginDemuxer::runSwitchAudioTrack(int i_trackId)
{
	return SYNC_PROD_LOCK([this, i_trackId] {
		CHECK_DONE_STATUS(HJErrAlreadyDone);
		if (m_status < HJSTATUS_Ready) {
			return HJ_WOULD_BLOCK;
		}
		if (m_status >= HJSTATUS_Stoped) {
			return HJ_WOULD_BLOCK;
		}
		return m_demuxer ? m_demuxer->switchAudioTrack(i_trackId) : HJErrNotAlready;
	});
}

int HJPluginDemuxer::runTask(int64_t* o_delay)
{
//	addInIdx();
	auto log = logRunTask();
	if (log) {
		RUNTASKLog("{}, enter", getName());
//		if (m_demuxNotify) {
//			m_demuxNotify();
//		}
	}

	std::string route{};
	int ret{ HJ_OK };
	do {
		auto currentFrame = loadCurrentFrame();
		if (currentFrame == nullptr) {
			route += "_0";
			std::tie(ret, currentFrame) = getOutputFrame(route);
			if (ret != HJ_OK) {
				route += "_1";
				break;
			}
		}

		if (currentFrame->isEofFrame()) {
			route += "_2";
			ret = runEof(currentFrame->m_streamIndex);
			if (ret != HJ_OK) {
				route += "_3";
				break;
			}
		}
		else {
			route += "_4";
			auto res = query(QUERY_CAN_DELIVER_TO_OUTPUTS_ID, getID(), currentFrame);
			if (!res.isOk()) {
				route += "_5";
				ret = res.code;
				break;
			}
			if (!res.value) {
				route += "_6";
				storeCurrentFrame(currentFrame);
				ret = HJ_WOULD_BLOCK;
				break;
			}
		}

		deliverToOutputs(currentFrame);
		route += "_7";
	} while (false);

//	addOutIdx();
	if (log) {
		RUNTASKLog("{}, leave, route({}), task duration({}), ret({})", getName(), route, (HJCurrentSteadyMS() - m_enterTimestamp), ret);
	}
	return ret;
}

int HJPluginDemuxer::runEof(int i_streamIndex)
{
	IF_FALSE_RETURN(setStatus(HJSTATUS_EOF), HJErrAlreadyDone);

	auto res = query(QUERY_CAN_PLUGIN_EOF_ID, getID(), i_streamIndex);
    if (!res.isOk()) {
        return res.code;
    }
	if (!res.value) {
        return HJ_WOULD_BLOCK;
    }

	return HJ_OK;
}

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
		HJFLoge("{}, unknownFrame({})", getName(), i_mediaFrame->getPTS());
	}

	OutputMap outputs;
	m_outputSync.consLock([&outputs, this] {
		outputs = m_outputs;
	});
	for (auto it = outputs.begin(); it != outputs.end(); ++it) {
		auto output = it->second;
		auto plugin = output->plugin.lock();
		if (plugin != nullptr) {
			if ((output->type & HJMEDIA_TYPE_VIDEO &&
				(i_mediaFrame->isVideo() || (i_mediaFrame->isEofFrame() && m_mediaType & HJMEDIA_TYPE_VIDEO))) ||
				(output->type & HJMEDIA_TYPE_AUDIO &&
				(i_mediaFrame->isAudio() || (i_mediaFrame->isEofFrame() && m_mediaType & HJMEDIA_TYPE_AUDIO)))) {
				auto err = plugin->deliver(output->myKeyHash, i_mediaFrame);
				if (err < 0) {
					HJFLoge("{}, plugin->deliver() error({})", getName(), err);
				}
			}
		}
	}
}

int HJPluginDemuxer::postInit(const HJMediaUrl::Ptr& i_url, int i_initIndex, InitReason i_reason, int64_t i_delay)
{
	Wtr wDemuxer = SHARED_FROM_THIS;
	if (!m_handler->asyncAndClear([wDemuxer, i_url, i_initIndex, i_reason] {
		auto demuxer = wDemuxer.lock();
		if (demuxer) {
			if (demuxer->runInit(i_url, i_initIndex, i_reason) == HJ_OK) {
				demuxer->postTask();
			}
		}
	}, m_runInitId, i_delay)) {
		return HJErrFatal;
	}

	return HJ_OK;
}

int HJPluginDemuxer::runInit(const HJMediaUrl::Ptr& i_url, int i_initIndex, InitReason i_reason)
{
	if (i_reason == InitReason::InternalInit || i_reason == InitReason::OpenURL) {
		m_hasReportedStreamOpened = false;
	}

	auto ret = SYNC_PROD_LOCK([this, i_url, i_initIndex] {
		CHECK_DONE_STATUS(HJErrAlreadyDone);
		auto ret = initDemuxer(i_url, i_initIndex);
		if (ret == HJ_OK && m_mediaUrl != nullptr) {
			m_mediaUrl->erase("startTimestamp");
			m_mediaUrl->erase("audioTrackID");
		}
		return ret;
	});
	if (ret == HJErrAlreadyDone) {
		return HJErrAlreadyDone;
	}
	else if (ret < 0) {
		IF_FALSE_RETURN(setStatus(HJSTATUS_Exception), HJErrAlreadyDone);

		report(EVENT_PLUGIN_NOTIFY_ID, HJ_PLUGIN_NOTIFY_ERROR_DEMUXER_INIT, getID());
	}
	else if (ret == HJ_STOP) {
		IF_FALSE_RETURN(setStatus(HJSTATUS_Stoped), HJErrAlreadyDone);
	}
	else if (ret == HJ_OK) {
		auto shouldReportStreamOpened =
			i_reason == InitReason::InternalInit ||
			i_reason == InitReason::OpenURL ||
			(i_reason == InitReason::Reset && !m_hasReportedStreamOpened);

		report(EVENT_MEDIA_TYPE_ID, m_mediaType);

		if (shouldReportStreamOpened) {
			report(EVENT_STREAM_OPENED_ID, getID());
			m_hasReportedStreamOpened = true;
		}

		IF_FALSE_RETURN(setStatus(HJSTATUS_Ready), HJErrAlreadyDone);
	}

	return ret;
}

int HJPluginDemuxer::initDemuxer(const HJMediaUrl::Ptr& i_url, int i_initIndex)
{
	releaseDemuxer();

	m_quittingDemuxer.store(false);
	m_demuxerSync.prodLock([this, i_initIndex] {
		if (m_quitIndex < i_initIndex) {
			m_demuxer = createDemuxer();
		}
	});
	if (m_demuxer == nullptr) {
		return HJ_STOP;
	}

	int64_t startTimestamp = 0;
	if (i_url->haveValue("startTimestamp")) {
		startTimestamp = i_url->getValue<int64_t>("startTimestamp");
	}
	int requestedAudioTrackID = -1;
	if (i_url->haveValue("audioTrackID")) {
		requestedAudioTrackID = i_url->getValue<int>("audioTrackID");
	}

	m_demuxer->setName(getName());
	auto ret = m_demuxer->init(i_url);
	if (m_quittingDemuxer.load()) {
		HJFLogi("{}, m_demuxer->init(), (m_quittingDemuxer.load())", getName());
		return HJ_STOP;
	}
	if (ret < 0) {
		HJFLoge("{}, m_demuxer->init(), error({})", getName(), ret);
		return ret;
	}

	auto mediaInfo = m_demuxer->getMediaInfo();
	if (!mediaInfo) {
		HJFLoge("{}, m_demuxer->getMediaInfo() error, (!m_mediaInfo)", getName());
		return HJErrFatal;
	}


	m_mediaType = 0;
	if (mediaInfo->getVideoInfo()) {
		m_mediaType |= HJMEDIA_TYPE_VIDEO;
	}
	if (mediaInfo->getAudioInfo()) {
		m_mediaType |= HJMEDIA_TYPE_AUDIO;
	}
	auto duration = mediaInfo->getDuration();
	m_duration.store(duration);
	if (requestedAudioTrackID >= 0) {
		ret = m_demuxer->switchAudioTrack(requestedAudioTrackID);
		if (ret == HJ_OK) {
			m_audioTrackId = requestedAudioTrackID;
		}
		else if (ret == HJErrNotFind) {
			HJFLogw("{}, requested init audioTrackID({}) not found, fallback to current logic",
				getName(), requestedAudioTrackID);
			ret = HJ_OK;
		}
		else {
			HJFLoge("{}, m_demuxer->switchAudioTrack({}) error({})",
				getName(), requestedAudioTrackID, ret);
			return ret;
		}
	}
	if (m_audioTrackId < 0) {
		m_audioTrackId = mediaInfo->getSelectedAudioTrackID();
	}
	else if (requestedAudioTrackID < 0 || m_audioTrackId != requestedAudioTrackID) {
		ret = m_demuxer->switchAudioTrack(m_audioTrackId);
		if (ret < 0) {
			HJFLoge("{}, m_demuxer->switchAudioTrack(), error({})", getName(), ret);
			return ret;
		}
	}

	if (startTimestamp > 0) {
		startTimestamp = std::min<int64_t>(startTimestamp, duration);
		ret = m_demuxer->seek(startTimestamp);
		if (m_quittingDemuxer.load()) {
			HJFLogi("{}, m_demuxer->seek(), (m_quittingDemuxer.load())", getName());
			return HJ_STOP;
		}
		if (ret < 0) {
			HJFLoge("{}, m_demuxer->seek(), error({})", getName(), ret);
			return ret;
		}
	}

	return HJ_OK;
}

int HJPluginDemuxer::quitDemuxer()
{
	m_quittingDemuxer.store(true);

	HJBaseDemuxer::Ptr demuxer{};
	int initIndex{};
	m_demuxerSync.prodLock([this, &demuxer, &initIndex] {
		demuxer = m_demuxer;
		m_quitIndex = m_initIndex;
		m_initIndex++;
		initIndex = m_initIndex;
	});
	if (demuxer) {
		demuxer->setQuit(true);
	}

	return initIndex;
}

void HJPluginDemuxer::releaseDemuxer()
{
	m_duration.store(0);
	if (m_demuxer) {
		m_demuxer->done();
		m_demuxerSync.prodLock([this] {
			m_demuxer = nullptr;
		});

		m_streamIndexOffset = m_streamIndex + 1;
		m_currentFrameSync.prodLock([this] {
			m_currentFrame = nullptr;
		});
	}
}

void HJPluginDemuxer::procSEI(const HJMediaFrame::Ptr& mvf)
{
	//if (!m_pluginListener) {
	//	return;
	//}
	auto seiNals = mvf->getSEINals();
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
		//auto ntfy = HJMakeNotification(HJ_PLUGIN_NOTIFY_PLUGIN_SEI_INFOS);
		//(*ntfy)["user_sei_datas"] = userSEIDatas;
		//m_pluginListener(std::move(ntfy));
        report(EVENT_GRAPH_SEI_INFOS_ID, userSEIDatas, mvf->isKeyFrame());
	}
	return;	
}

HJMediaFrame::Ptr HJPluginDemuxer::loadCurrentFrame()
{
	return m_currentFrameSync.prodLock([this] {
		auto currentFrame = m_currentFrame;
		m_currentFrame = nullptr;
		return currentFrame;
	});
}

std::tuple<int, HJMediaFrame::Ptr> HJPluginDemuxer::getOutputFrame(std::string& route)
{
	int ret{ HJ_OK };
	HJMediaFrame::Ptr currentFrame{};
	do { 
		ret = SYNC_PROD_LOCK([this, &route, &currentFrame] {
			if (m_status == HJSTATUS_Done) {
				route += "_00";
				return HJErrAlreadyDone;
			}
			if (m_status < HJSTATUS_Ready) {
				route += "_01";
				return HJ_WOULD_BLOCK;
			}
			if (m_status >= HJSTATUS_EOF) {
				route += "_02";
				return HJ_WOULD_BLOCK;
			}

			auto err = m_demuxer->getFrame(currentFrame);
			if (m_quittingDemuxer.load()) {
				route += "_03";
				HJFLogi("{}, m_demuxer->getFrame(), (m_quittingDemuxer.load())", getName());
				return HJ_STOP;
			}
			if (err < 0) {
				route += "_04";
				HJFLoge("{}, m_demuxer->getFrame(), error({})", getName(), err);
				return HJErrFatal;
			}

			if (currentFrame == nullptr) {
				route += "_05";
				return HJ_WOULD_BLOCK;
			}

			currentFrame->m_streamIndex += m_streamIndexOffset;
			m_streamIndex = currentFrame->m_streamIndex;

			route += "_06";
			return HJ_OK;
		});
		if (ret == HJ_OK) {
			//procSEI(currentFrame);
		}
		else if (ret == HJ_STOP) {
			IF_FALSE_BREAK(setStatus(HJSTATUS_Stoped), HJErrAlreadyDone);
		}
		else if (ret == HJErrFatal) {
			IF_FALSE_BREAK(setStatus(HJSTATUS_Exception), HJErrAlreadyDone);

			report(EVENT_PLUGIN_NOTIFY_ID, HJ_PLUGIN_NOTIFY_ERROR_DEMUXER_GETFRAME, getID());
		}
	} while (false);

	return std::make_tuple(ret, currentFrame);
}

void HJPluginDemuxer::storeCurrentFrame(HJMediaFrame::Ptr& surrentFrame)
{
	m_currentFrameSync.prodLock([this, surrentFrame] {
		m_currentFrame = surrentFrame;
	});
}

NS_HJ_END
