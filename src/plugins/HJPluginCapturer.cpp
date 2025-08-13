#include "HJPluginCapturer.h"
#include "HJFLog.h"

NS_HJ_BEGIN

#define RUNTASKLog		//HJFLogi

int HJPluginCapturer::internalInit(HJKeyStorage::Ptr i_param)
{
	GET_PARAMETER(HJStreamInfo::Ptr, streamInfo);
	if (!streamInfo) {
		return HJErrInvalidParams;
	}
	GET_PARAMETER(HJLooperThread::Ptr, thread);
	GET_PARAMETER(HJListener, pluginListener);
	auto param = std::make_shared<HJKeyStorage>();
//	(*param)["thread"] = thread;
//	(*param)["createThread"] = static_cast<bool>(thread == nullptr);
	(*param)["createThread"] = false;
	if (pluginListener) {
		(*param)["pluginListener"] = pluginListener;
	}
	int ret = HJPlugin::internalInit(param);
	if (ret < 0) {
		return ret;
	}

	do {
		ret = initCapturer(streamInfo);
		if (ret < 0) {
			break;
		}

		return HJ_OK;
	} while (false);

	HJPluginCapturer::internalRelease();
	return ret;
}

void HJPluginCapturer::internalRelease()
{
	if (m_capturer != nullptr) {
		m_capturer->done();
		m_capturer = nullptr;
	}

	HJPlugin::internalRelease();
}

int HJPluginCapturer::runTask()
{
	RUNTASKLog("{}, enter", getName());
	int64_t enter = HJCurrentSteadyMS();
	std::string route = "0";
	int ret;
	do {
		HJMediaFrame::Ptr outFrame = nullptr;
		ret = SYNC_CONS_LOCK([&route, &outFrame, this] {
			if (m_status == HJSTATUS_Done) {
				route += "_1";
				return HJErrAlreadyDone;
			}
			if (m_status < HJSTATUS_Inited) {
				route += "_2";
				return HJ_WOULD_BLOCK;
			}
			if (m_status >= HJSTATUS_Stoped) {
				route += "_3";
				return HJ_WOULD_BLOCK;
			}

			auto err = m_capturer->getFrame(outFrame);
			if (err < 0) {
				route += "_4";
				HJFLoge("{}, m_capturer->getFrame() error({})", getName(), err);
				if (m_pluginListener) {
					m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_ERROR_CAPTURER_GETFRAME)));
				}
				m_status = HJSTATUS_Exception;
				return HJErrFatal;
			}
			return HJ_OK;
		});
		if (ret != HJ_OK) {
			break;
		}
		if (outFrame == nullptr) {
			route += "_5";
			ret = HJ_WOULD_BLOCK;
			break;
		}

		route += "_6";
		deliverToOutputs(outFrame);
	} while (false);

	RUNTASKLog("{}, leave, route({}), duration({}), ret({})", getName(), route, (HJCurrentSteadyMS() - enter), ret);
	return ret;
}

int HJPluginCapturer::initCapturer(const HJStreamInfo::Ptr& i_streamInfo)
{
	if (!i_streamInfo) {
		return HJErrInvalidParams;
	}
	auto streamInfo = i_streamInfo->dup();
	(*streamInfo)["newBufferCb"] = (HJRunnable)[=] {
//		internalUpdated();
		runTask();
	};

	int ret;
	do {
		m_capturer = createCapturer();
		if (m_capturer == nullptr) {
			ret = HJErrFatal;
			break;
		}

		m_capturer->setName(getName());
		ret = m_capturer->init(streamInfo);
		if (ret < 0) {
			break;
		}

		return HJ_OK;
	} while (false);

	if (m_capturer != nullptr) {
		m_capturer->done();
		m_capturer = nullptr;
	}
	m_status = HJSTATUS_Exception;
	return ret;
}

NS_HJ_END
