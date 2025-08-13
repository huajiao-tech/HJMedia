#include "HJPluginAudioResampler.h"
#include "HJFLog.h"

NS_HJ_BEGIN

#define RUNTASKLog		//HJFLogi

int HJPluginAudioResampler::internalInit(HJKeyStorage::Ptr i_param)
{
	GET_PARAMETER(HJAudioInfo::Ptr, audioInfo);
	if (!audioInfo) {
		return HJErrInvalidParams;
	}
	GET_PARAMETER(HJLooperThread::Ptr, thread);
	GET_PARAMETER(HJListener, pluginListener);
	auto param = std::make_shared<HJKeyStorage>();
	(*param)["thread"] = thread;
	(*param)["createThread"] = static_cast<bool>(thread == nullptr);
	if (pluginListener) {
		(*param)["pluginListener"] = pluginListener;
	}
	int ret = HJPlugin::internalInit(param);
	if (ret < 0) {
		return ret;
	}

	do {
		m_fifo = std::make_shared<HJAudioFifo>(audioInfo->m_channels, audioInfo->m_sampleFmt, audioInfo->m_samplesRate);
		ret = m_fifo->init(audioInfo->m_sampleCnt);
		if (HJ_OK != ret) {
			break;
		}

		m_audioInfo = audioInfo;
		return HJ_OK;
	} while (false);

	HJPluginAudioResampler::internalRelease();
	return ret;
}

void HJPluginAudioResampler::internalRelease()
{
	m_audioInfo = nullptr;
	m_converter = nullptr;
	m_fifo = nullptr;

	HJPlugin::internalRelease();
}

void HJPluginAudioResampler::onInputAdded(size_t i_srcKeyHash, HJMediaType i_type)
{
	m_inputKeyHash.store(i_srcKeyHash);
}

int HJPluginAudioResampler::runTask()
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

		auto err = SYNC_CONS_LOCK([&route, &inFrame, this] {
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

			if (m_status < HJSTATUS_Ready) {
				route += "_5";
				auto audioInfo = inFrame->getAudioInfo();
				if (audioInfo == nullptr) {
					route += "_6";
					HJFLoge("{}, (audioInfo == nullptr)", getName());
					return HJErrFatal;
				}
				if (audioInfo->m_samplesRate != m_audioInfo->m_samplesRate ||
					audioInfo->m_channels != m_audioInfo->m_channels) {
					route += "_7";
					m_converter = std::make_shared<HJAudioConverter>(m_audioInfo);
				}

				m_status = HJSTATUS_Ready;
			}

			if (m_converter != nullptr) {
				route += "_8";
				inFrame = m_converter->convert(std::move(inFrame));
				if (inFrame == nullptr) {
					route += "_9";
					HJFLogi("{}, (inFrame == nullptr)", getName());
					return HJ_WOULD_BLOCK;
				}
			}

			inFrame->setAVTimeBase(HJTimeBase{ 1, m_audioInfo->m_samplesRate });
			auto err = m_fifo->addFrame(std::move(inFrame));
			if (err < 0) {
				route += "_10";
				HJFLoge("{}, m_fifo->addFrame() error({})", getName(), err);
				if (m_pluginListener) {
					m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_ERROR_AUDIOFIFO_ADDFRAME)));
				}
				m_status = HJSTATUS_Exception;
				return HJErrFatal;
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
			route += "_11";
			HJMediaFrame::Ptr outFrame{};
			err = SYNC_CONS_LOCK([&route, &outFrame, this] {
				if (m_status == HJSTATUS_Done) {
					route += "_12";
					return HJErrAlreadyDone;
				}
				outFrame = m_fifo->getFrame();
				return HJ_OK;
			});
			if (err == HJErrAlreadyDone) {
				ret = HJErrAlreadyDone;
				break;
			}
			if (outFrame == nullptr) {
				route += "_13";
				break;
			}

			route += "_14";
			deliverToOutputs(outFrame);
		}
	} while (false);

	RUNTASKLog("{}, leave, route({}), size({}), duration({}), ret({})", getName(), route, size, (HJCurrentSteadyMS() - enter), ret);
	return ret;
}

void HJPluginAudioResampler::deliverToOutputs(HJMediaFrame::Ptr& i_mediaFrame)
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

	HJPlugin::deliverToOutputs(i_mediaFrame);
}

NS_HJ_END
