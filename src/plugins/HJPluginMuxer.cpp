#include "HJPluginMuxer.h"
#include "HJFLog.h"

NS_HJ_BEGIN

#define RUNTASKLog		//HJFLogi

int HJPluginMuxer::internalInit(HJKeyStorage::Ptr i_param)
{
	MUST_HAVE_PARAMETERS;
	GET_PARAMETER(HJLooperThread::Ptr, thread);
//	GET_PARAMETER(HJListener, pluginListener);
	auto param = HJKeyStorage::dupFrom(i_param);
//	(*param)["thread"] = thread;
	(*param)["createThread"] = (thread == nullptr);
//	if (pluginListener) {
//		(*param)["pluginListener"] = pluginListener;
//	}
	auto ret = HJPlugin::internalInit(param);
	if (ret != HJ_OK) {
		return ret;
	}

	GET_PARAMETER(HJMediaUrl::Ptr, mediaUrl);
	GET_PARAMETER(HJAudioInfo::Ptr, audioInfo);
	GET_PARAMETER(HJVideoInfo::Ptr, videoInfo);
	auto statCtx = i_param->getValueObj<std::weak_ptr<HJStatContext>>("HJStatContext");
 	std::string url{};
	int mediaTypes{};
	if (audioInfo != nullptr) {
		mediaTypes |= HJMEDIA_TYPE_AUDIO;
	}
	if (videoInfo != nullptr) {
		mediaTypes |= HJMEDIA_TYPE_VIDEO;
	}
	if (mediaUrl != nullptr) {
		url = mediaUrl->getUrl();
	}

	Wtr wMuxer = SHARED_FROM_THIS;
	if (!m_handler->async([wMuxer, url, mediaTypes, statCtx] {
		auto muxer = wMuxer.lock();
		if (muxer != nullptr) {
			if (muxer->runInit(url, mediaTypes, statCtx) == HJ_OK) {
				muxer->postTask();
			}
		}
	})) {
		return HJErrFatal;
	}
/*
	ret = initMuxer(url, mediaTypes, statCtx);
	if (ret != HJ_OK) {
		break;
	}
*/
	return HJ_OK;
}

void HJPluginMuxer::internalRelease()
{
	releaseMuxer();

	HJPlugin::internalRelease();

//	m_muxer->done();
}

int HJPluginMuxer::beforeDone()
{
	m_quitting.store(true);

	setQuit();

	return HJPlugin::beforeDone();
}

void HJPluginMuxer::onInputAdded(size_t i_srcKeyHash, HJMediaType i_type)
{
	m_inputKeyHash.store(i_srcKeyHash);
}

int HJPluginMuxer::runTask(int64_t* o_delay)
{
	RUNTASKLog("{}, enter", getName());
	int64_t enter = HJCurrentSteadyMS();
	std::string route{};
	int64_t size = -1;
	int ret{ HJ_OK };
	do {
#if 0
		auto inFrame = receive(m_inputKeyHash.load(), &size);
		if (inFrame == nullptr) {
			route += "_1";
			ret = HJ_WOULD_BLOCK;
			break;
		}

		auto err = SYNC_CONS_LOCK([&route, &inFrame, this] {
			if (m_status == HJSTATUS_Done) {
				route += "_2";
				return HJErrAlreadyDone;
			}
			if (m_status < HJSTATUS_Ready) {
				route += "_3";
				return HJErrNotInited;
			}
			if (m_status >= HJSTATUS_Stoped) {
				route += "_4";
				return HJErrFatal;
			}

			if (m_dropping) {
				route += "_5";
				if (dropping(inFrame)) {
					route += "_6";
					return HJ_OK;
				}
			}

			onWriteFrame(inFrame);
			if (inFrame->isVideo()) {
				route += HJFMT("_VDTS({})", inFrame->getDTS());
			}
			else if (inFrame->isAudio()) {
				route += HJFMT("_ADTS({})", inFrame->getDTS());
			}

			if (inFrame->getExtraTS() == HJ_NOTS_VALUE)
			{
				inFrame->setExtraTS(HJCurrentSteadyMS());
			}
			deliverToOutputs(inFrame);
//			if (m_status == HJSTATUS_Running) return HJ_OK;

			auto err = m_muxer->writeFrame(inFrame);
			if (m_quitting.load()) {
				route += "_7";
				m_status = HJSTATUS_Stoped;
				return HJ_STOP;
			}
			if (err < 0) {
				route += "_8";
				HJFLoge("{}, m_muxer->writeFrame(), error({})", getName(), err);
				//if (m_pluginListener) {
				//	m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_ERROR_MUXER_WRITEFRAME)));
				//}
				report(EVENT_PLUGIN_NOTIFY_ID, HJ_PLUGIN_NOTIFY_ERROR_MUXER_WRITEFRAME, getID());
				m_status = HJSTATUS_Exception;
			}
			return HJ_OK;
		});
		if (err == HJErrAlreadyDone) {
			ret = HJErrAlreadyDone;
		}
#else
		auto status = SYNC_CONS_LOCK([this] {
			return m_status;
		});
		if (status == HJSTATUS_Done) {
			route += "_0";
			ret = HJErrAlreadyDone;
			break;
		}
		if (status < HJSTATUS_Ready) {
			route += "_1";
			ret = HJ_WOULD_BLOCK;
			break;
		} 
		if (status >= HJSTATUS_Stoped) {
			route += "_2";
			ret = HJErrFatal;
			break;
		}

		auto inFrame = receive(m_inputKeyHash.load(), &size);
		if (inFrame == nullptr) {
			route += "_3";
			ret = HJ_WOULD_BLOCK;
			break;
		}

		if (m_dropping) {
			route += "_4";
			if (dropping(inFrame)) {
				route += "_5";
				return HJ_OK;
			}
		}

		onWriteFrame(inFrame);
		if (inFrame->isVideo()) {
			route += HJFMT("_VDTS({})", inFrame->getDTS());
		}
		else if (inFrame->isAudio()) {
			route += HJFMT("_ADTS({})", inFrame->getDTS());
		}

		if (inFrame->getExtraTS() == HJ_NOTS_VALUE)
		{
			route += "_6";
			inFrame->setExtraTS(HJCurrentSteadyMS());
		}
//		deliverToOutputs(inFrame);

		ret = SYNC_CONS_LOCK([&route, &inFrame, this] {
			if (m_status == HJSTATUS_Done) {
				route += "_7";
				return HJErrAlreadyDone;
			}

			auto err = m_muxer->writeFrame(inFrame);
			if (m_quitting.load()) {
				route += "_8";
				return HJ_STOP;
			}
			if (err < 0) {
				route += "_9";
				HJFLoge("{}, m_muxer->writeFrame(), error({})", getName(), err);
				return HJErrFatal;
			}

			route += "_A";
			return HJ_OK;
		});
		if (ret == HJ_STOP) {
			IF_FALSE_BREAK(setStatus(HJSTATUS_Stoped), HJErrAlreadyDone);
		}
		else if (ret == HJErrFatal) {
			IF_FALSE_RETURN(setStatus(HJSTATUS_Exception), HJErrAlreadyDone);
			//if (m_pluginListener) {
			//	m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_ERROR_MUXER_WRITEFRAME)));
			//}
			report(EVENT_PLUGIN_NOTIFY_ID, HJ_PLUGIN_NOTIFY_ERROR_MUXER_WRITEFRAME, getID());
		}
#endif
		route += "_B";
	} while (false);

	RUNTASKLog("{}, leave, route({}), size({}), duration({}), ret({})", getName(), route, size, (HJCurrentSteadyMS() - enter), ret);
	return ret;
}

int HJPluginMuxer::runInit(const std::string i_url, int i_mediaTypes, std::weak_ptr<HJStatContext> i_statCtx)
{
	auto ret = SYNC_PROD_LOCK([this, i_url, i_mediaTypes, i_statCtx] {
		CHECK_DONE_STATUS(HJErrAlreadyDone);
		return initMuxer(i_url, i_mediaTypes, i_statCtx);
	});
	if (ret == HJErrAlreadyDone) {
		return HJErrAlreadyDone;
	}
	else if (ret < 0) {
		IF_FALSE_RETURN(setStatus(HJSTATUS_Exception), HJErrAlreadyDone);
		//if (m_pluginListener) {
		//	m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_ERROR_MUXER_INIT)));
		//}
		report(EVENT_PLUGIN_NOTIFY_ID, HJ_PLUGIN_NOTIFY_ERROR_MUXER_INIT, getID());
	}
	else if (ret == HJ_STOP) {
		IF_FALSE_RETURN(setStatus(HJSTATUS_Stoped), HJErrAlreadyDone);
	}
	else if (ret == HJ_OK) {
		IF_FALSE_RETURN(setStatus(HJSTATUS_Ready), HJErrAlreadyDone);
	}

	return ret;
}

int HJPluginMuxer::initMuxer(std::string i_url, int i_mediaTypes, std::weak_ptr<HJStatContext> statCtx)
{
	releaseMuxer();

	m_muxerSync.prodLock([this] {
		m_muxer = createMuxer();
	});
	if (m_muxer == nullptr) {
		return HJ_STOP;
	}

	m_muxer->setName(getName());
	HJOptions::Ptr opts = HJCreates<HJOptions>();
    (*opts)["HJStatContext"] = statCtx;
	auto ret = m_muxer->init(i_url, i_mediaTypes, opts);
	if (m_quitting.load()) {
		HJFLogi("{}, m_muxer->init(), (m_quitting.load())", getName());
		return HJ_STOP;
	}
	if (ret < 0) {
		HJFLoge("{}, m_muxer->init(), error({})", getName(), ret);
		return ret;
	}

	m_mediaTypes = i_mediaTypes;
	m_dropping = true;

	return HJ_OK;
}

void HJPluginMuxer::releaseMuxer()
{
	if (m_muxer) {
		m_muxer->done();
		m_muxerSync.prodLock([this] {
			m_muxer = nullptr;
		});
	}
}

bool HJPluginMuxer::dropping(HJMediaFrame::Ptr i_mediaFrame)
{
	if (m_mediaTypes & HJMEDIA_TYPE_VIDEO) {
		if (i_mediaFrame->isVideo() && i_mediaFrame->isKeyFrame()) {
			m_dropping = false;
		}
	}
	else {
		m_dropping = false;
	}

	return m_dropping;
}

void HJPluginMuxer::setQuit() {
	auto muxer = m_muxerSync.consLock([this] {
		return m_muxer;
	});
	if (muxer) {
		muxer->setQuit(true);
	}
}

NS_HJ_END
