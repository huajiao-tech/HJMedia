#include "HJPluginMuxer.h"
#include "HJFLog.h"

NS_HJ_BEGIN

#define RUNTASKLog		//HJFLogi

int HJPluginMuxer::internalInit(HJKeyStorage::Ptr i_param)
{
	GET_PARAMETER(HJLooperThread::Ptr, thread);
	GET_PARAMETER(HJListener, pluginListener);
	auto param = std::make_shared<HJKeyStorage>();
	(*param)["thread"] = thread;
	(*param)["createThread"] = (thread == nullptr);
	if (pluginListener) {
		(*param)["pluginListener"] = pluginListener;
	}
	int ret = HJPlugin::internalInit(param);
	if (ret < 0) {
		return ret;
	}

	do {
		GET_PARAMETER(HJMediaUrl::Ptr, mediaUrl);
		GET_PARAMETER(HJAudioInfo::Ptr, audioInfo);
		GET_PARAMETER(HJVideoInfo::Ptr, videoInfo);
		std::weak_ptr<HJStatContext> statCtx;
        if(i_param) {
			statCtx = i_param->getValueObj<std::weak_ptr<HJStatContext>>("HJStatContext");
        }
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

		ret = initMuxer(url, mediaTypes, statCtx);
		if (ret != HJ_OK) {
			break;
		}

		return HJ_OK;
	} while (false);

	HJPluginMuxer::internalRelease();
	return ret;
}

void HJPluginMuxer::internalRelease()
{
	HJPlugin::internalRelease();

	m_muxer->done();
}

void HJPluginMuxer::onInputAdded(size_t i_srcKeyHash, HJMediaType i_type)
{
	m_inputKeyHash.store(i_srcKeyHash);
}

int HJPluginMuxer::runTask(int64_t* o_delay)
{
	RUNTASKLog("{}, enter", getName());
	int64_t enter = HJCurrentSteadyMS();
	std::string route = "0";
	size_t size = -1;
	int ret = HJ_OK;
	do {
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
				if (m_pluginListener) {
					m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_ERROR_MUXER_WRITEFRAME)));
				}
				m_status = HJSTATUS_Exception;
			}
			return HJ_OK;
		});
		if (err == HJErrAlreadyDone) {
			ret = HJErrAlreadyDone;
		}
	} while (false);

	RUNTASKLog("{}, leave, route({}), size({}), duration({}), ret({})", getName(), route, size, (HJCurrentSteadyMS() - enter), ret);
	return ret;
}

int HJPluginMuxer::initMuxer(std::string i_url, int i_mediaTypes, std::weak_ptr<HJStatContext> statCtx)
{
	int ret;
	do {
		m_muxer->done();
		//
		HJOptions::Ptr opts = HJCreates<HJOptions>();
        (*opts)["HJStatContext"] = statCtx;
		ret = m_muxer->init(i_url, i_mediaTypes, opts);
		if (m_quitting.load()) {
			ret = HJ_STOP;
			break;
		}
		if (ret < 0) {
			HJFLoge("{}, m_muxer->init(), error({})", getName(), ret);
			break;
		}
		m_mediaTypes = i_mediaTypes;
		m_dropping = true;
		return HJ_OK;
	} while (false);

	m_muxer->done();
	m_status = HJSTATUS_Exception;
	return ret;
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

NS_HJ_END
