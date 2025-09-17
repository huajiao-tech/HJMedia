#include "HJPluginAudioResampler.h"
#include "HJFLog.h"

NS_HJ_BEGIN

#define RUNTASKLog		HJFLogi

int HJPluginAudioResampler::internalInit(HJKeyStorage::Ptr i_param)
{
	MUST_HAVE_PARAMETERS;
	MUST_GET_PARAMETER(HJAudioInfo::Ptr, audioInfo);
	GET_PARAMETER(HJLooperThread::Ptr, thread);
	GET_PARAMETER(bool, fifo);

	auto param = HJKeyStorage::dupFrom(i_param);
	(*param)["createThread"] = (thread == nullptr);
	int ret = HJPlugin::internalInit(param);
	if (ret < 0) {
		return ret;
	}

	do {
		m_converter = std::make_shared<HJAudioConverter>(audioInfo);

		if (fifo) {
			m_fifo = std::make_shared<HJAudioFifo>(audioInfo->m_channels, audioInfo->m_sampleFmt, audioInfo->m_samplesRate);
			ret = m_fifo->init(audioInfo->m_sampleCnt);
			if (ret != HJ_OK) {
				break;
			}
		}

		m_audioInfo = audioInfo;
		return HJ_OK;
	} while (false);

	HJPluginAudioResampler::internalRelease();
	return ret;
}

void HJPluginAudioResampler::internalRelease()
{
	HJFLogi("{}, internalRelease() begin", getName());

	m_audioInfo = nullptr;
	m_converter = nullptr;
	m_fifo = nullptr;

	HJPlugin::internalRelease();

	HJFLogi("{}, internalRelease() end", getName());
}

void HJPluginAudioResampler::onOutputUpdated()
{
	auto input = getInput(m_inputKeyHash.load());
	if (input) {
		auto plugin = input->plugin.lock();
		if (plugin) {
			plugin->onOutputUpdated();
		}
	}
}

void HJPluginAudioResampler::onInputAdded(size_t i_srcKeyHash, HJMediaType i_type)
{
	m_inputKeyHash.store(i_srcKeyHash);
}

void HJPluginAudioResampler::onOutputAdded(size_t i_dstKeyHash, HJMediaType i_type)
{
	m_outputKeyHash.store(i_dstKeyHash);
}

int HJPluginAudioResampler::runTask(int64_t* o_delay)
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
	}

	std::string route{};
	size_t size = -1;
	int ret = HJ_OK;
	do {
		auto inFrame = receive(m_inputKeyHash.load(), &size);
		if (inFrame == nullptr) {
			route += "_1";
			ret = HJ_WOULD_BLOCK;
			break;
		}

		HJMediaFrame::Ptr outFrame{};
		auto err = SYNC_CONS_LOCK([&route, &inFrame, &outFrame, this] {
			if (m_status == HJSTATUS_Done) {
				route += "_2";
				return HJErrAlreadyDone;
			}
			if (m_status < HJSTATUS_Inited) {
				route += "_3";
				return HJ_WOULD_BLOCK;
			}
			if (m_status >= HJSTATUS_Stoped) {
				route += "_4";
				return HJ_WOULD_BLOCK;
			}

			if (!inFrame->isEofFrame()) {
				m_streamIndex = inFrame->m_streamIndex;
				inFrame = m_converter->convert(std::move(inFrame));
				if (inFrame == nullptr) {
					route += "_9";
					HJFLogi("{}, (inFrame == nullptr)", getName());
					return HJ_WOULD_BLOCK;
				}
				inFrame->m_streamIndex = m_streamIndex;
			}
			else {
				int a = 0;
			}

			if (m_fifo != nullptr) {
				route += "_10";
				inFrame->setAVTimeBase(HJTimeBase{ 1, m_audioInfo->m_samplesRate });
				m_streamIndex = inFrame->m_streamIndex;
				auto err = m_fifo->addFrame(std::move(inFrame));
				if (err < 0) {
					route += "_11";
					HJFLoge("{}, m_fifo->addFrame() error({})", getName(), err);
					return HJErrFatal;
				}
			}
			else {
				route += "_12";
				outFrame = inFrame;
			}
			return HJ_OK;
		});
		if (err != HJ_OK) {
			if (err == HJErrAlreadyDone) {
				ret = HJErrAlreadyDone;
			}
			else if (err == HJErrFatal) {
				setStatus(HJSTATUS_Exception);
				if (m_pluginListener) {
					m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_ERROR_AUDIOFIFO_ADDFRAME)));
				}
			}
			break;
		}

		if (outFrame != nullptr) {
			route += "_13";
			deliverToOutputs(outFrame);
		}
		else {
			for (;;) {
				route += "_14";
				HJMediaFrame::Ptr outFrame{};
				err = SYNC_CONS_LOCK([&route, &outFrame, this] {
					if (m_status == HJSTATUS_Done) {
						route += "_15";
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
					route += "_16";
					break;
				}

				route += "_17";
				outFrame->m_streamIndex = m_streamIndex;
				deliverToOutputs(outFrame);
			}
		}
	} while (false);
    addOutIdx();
	if (log) {
		RUNTASKLog("{}, leave, route({}), size({}), task duration({}), ret({})", getName(), route, size, (HJCurrentSteadyMS() - enter), ret);
	}
	return ret;
}

NS_HJ_END
