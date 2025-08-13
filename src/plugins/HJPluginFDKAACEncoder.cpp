#include "HJPluginFDKAACEncoder.h"
#include "HJAEncFDKAAC.h"
#include "HJFLog.h"

NS_HJ_BEGIN

#define RUNTASKLog		//HJFLogi

int HJPluginFDKAACEncoder::internalInit(HJKeyStorage::Ptr i_param)
{
	GET_PARAMETER(HJAudioInfo::Ptr, audioInfo);
	if (!audioInfo) {
		return HJErrInvalidParams;
	}
	GET_PARAMETER(HJLooperThread::Ptr, thread);
	GET_PARAMETER(HJListener, pluginListener);
	auto param = std::make_shared<HJKeyStorage>();
	(*param)["streamInfo"] = std::static_pointer_cast<HJStreamInfo>(audioInfo);
	(*param)["thread"] = thread;
	if (pluginListener) {
		(*param)["pluginListener"] = pluginListener;
	}
	return HJPluginCodec::internalInit(param);
}

int HJPluginFDKAACEncoder::runTask()
{
	RUNTASKLog("{}, enter", getName());
	int64_t enter = HJCurrentSteadyMS();
	std::string route = "0";
	size_t size = -1;
	int ret = HJ_OK;
	do {
		auto inFrame = receive(m_inputKeyHash.load(), size);
		if (inFrame == nullptr) {
			route += "_1";
			ret = HJ_WOULD_BLOCK;
			break;
		}

		auto err = SYNC_CONS_LOCK([&route, inFrame, this] {
			if (m_status == HJSTATUS_Done) {
				route += "_2";
				return HJErrAlreadyDone;
			}
			if (m_status < HJSTATUS_Inited) {
				route += "_3";
				return HJErrNotInited;
			}
			if (m_status >= HJSTATUS_Stoped) {
				route += "_4";
				return HJErrFatal;
			}
			auto err = m_codec->run(inFrame);
			if (err < 0) {
				route += "_5";
				HJFLoge("{}, m_codec->run() error({})", getName(), err);
				if (m_pluginListener) {
					m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_ERROR_CODEC_RUN)));
				}
				m_status = HJSTATUS_Exception;
				return HJErrExcep;
			}
			return HJ_OK;
		});
		if (err != HJ_OK) {
			if (err == HJErrAlreadyDone) {
				ret = HJErrAlreadyDone;
			}
			break;
		}

		for (;;) {
			route += "_6";
			HJMediaFrame::Ptr outFrame{};
			err = SYNC_CONS_LOCK([&route, &outFrame, this] {
				if (m_status == HJSTATUS_Done) {
					route += "_7";
					return HJErrAlreadyDone;
				}
				auto err = m_codec->getFrame(outFrame);
				if (err < 0) {
					route += "_8";
					HJFLoge("{}, 0, m_codec->getFrame() error({})", getName(), err);
					if (m_pluginListener) {
						m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_ERROR_CODEC_GETFRAME)));
					}
					m_status = HJSTATUS_Exception;
					return HJErrExcep;
				}
				return HJ_OK;
			});
			if (err != HJ_OK) {
				if (err == HJErrAlreadyDone) {
					ret = HJErrAlreadyDone;
				}
				break;
			}
			if (outFrame == nullptr) {
				route += "_9";
				break;
			}

			route += "_10";
			deliverToOutputs(outFrame);
		}
	} while (false);

	RUNTASKLog("{}, leave, route({}), size({}), duration({}), ret({})", getName(), route, size, (HJCurrentSteadyMS() - enter), ret);
	return ret;
}

HJBaseCodec::Ptr HJPluginFDKAACEncoder::createCodec()
{
	return std::make_shared<HJAEncFDKAAC>();
}

void HJPluginFDKAACEncoder::deliverToOutputs(HJMediaFrame::Ptr& i_mediaFrame)
{
	auto info = i_mediaFrame->getAudioInfo();
	sample_size += info->getSampleCnt();
	auto now = HJCurrentSteadyMS();
	if (start_time > 0) {
		if ((now - start_time) / 1000 >= duration_count * 5) {
			auto samples = duration_count * 5 * info->getSampleRate();
			HJFLogi("{}, ({})s, samples({}), sample_size diff({}), delay({})",
				getName(), duration_count * 5, samples, sample_size - samples, now - i_mediaFrame->getPTS());
			duration_count++;
		}
	}
	else {
		start_time  = now;
	}

	HJPluginCodec::deliverToOutputs(i_mediaFrame);
}

NS_HJ_END
