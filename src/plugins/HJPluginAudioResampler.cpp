#include "HJPluginAudioResampler.h"
#include "HJGraph.h"
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

	m_converter = std::make_shared<HJAudioConverter>(audioInfo);
	m_converter->setName(getName());

	if (fifo) {
		m_fifo = std::make_shared<HJAudioFifo>(audioInfo->m_channels, audioInfo->m_sampleFmt, audioInfo->m_samplesRate);
		ret = m_fifo->init(audioInfo->m_sampleCnt);
		if (ret != HJ_OK) {
			return ret;
		}
	}

	m_audioInfo = audioInfo;
	return HJ_OK;
}

void HJPluginAudioResampler::internalRelease()
{
	HJFLogi("{}, internalRelease() begin", getName());

	m_last_time = HJ_NOTS_VALUE;
	m_audioInfo = nullptr;
	m_converter = nullptr;
	m_fifo = nullptr;

	HJPlugin::internalRelease();

	HJFLogi("{}, internalRelease() end", getName());
}
/*
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
*/
void HJPluginAudioResampler::onInputAdded(size_t i_srcKeyHash, HJMediaType i_type)
{
	m_inputKeyHash.store(i_srcKeyHash);
}

int HJPluginAudioResampler::runFlush()
{
	auto ret = SYNC_PROD_LOCK([this] {
		CHECK_DONE_STATUS(HJErrAlreadyDone);

		m_last_time = HJ_NOTS_VALUE;
		if (m_fifo != nullptr) {
			m_fifo->reset();
		}

		return HJ_OK;
	});
	if (ret < 0) {
		return ret;
	}

	return HJPlugin::runFlush();
}
/*
void HJPluginAudioResampler::onOutputAdded(size_t i_dstKeyHash, HJMediaType i_type)
{
	m_outputKeyHash.store(i_dstKeyHash);
}
*/
int HJPluginAudioResampler::runTask(int64_t* o_delay)
{
//    addInIdx();
	auto log = logRunTask();
	if (log) {
		RUNTASKLog("{}, enter", getName());
	}

	std::string route{};
	int64_t size{ -1 };
	int ret{ HJ_OK };
	do {
#if 0
		route += "_0";
		auto inputKeyHash = m_inputKeyHash.load();
		HJMediaFrame::Ptr inFrame{};
		std::tie(ret, inFrame) = receiveInputFrame(route, inputKeyHash, size);
		if (ret != HJ_OK) {
			break;
		}
		if (inFrame == nullptr) {
			ret = HJ_WOULD_BLOCK;
			break;
		}

		if (inFrame->isClearFrame()) {
			route += "_1";
			auto err = runFlush();
			if (err < 0) {
				ret = err;
			}
			break;
		}

		if (inFrame->isEofFrame()) {
			route += "_2";
			deliverToOutputs(inFrame);
			ret = HJ_OK;
			break;
		}

		route += "_3";
		ret = processInputFrame(route, inFrame);
#else
		auto inputKeyHash = m_inputKeyHash.load();
		HJMediaFrame::Ptr inFrame{};
		std::tie(ret, inFrame) = receiveInputFrame(route, inputKeyHash, size);
		if (ret != HJ_OK) {
            route += "_0";
			break;
		}
		if (inFrame == nullptr) {
            route += "_1";
			ret = HJ_WOULD_BLOCK;
			break;
		}

		if (inFrame->isClearFrame()) {
			route += "_2";
			auto err = runFlush();
			if (err < 0) {
                route += "_3";
				ret = err;
			}
			break;
		}

		HJMediaFrame::Ptr outFrame{};
		bool fifo{ false };
		if (inFrame->isEofFrame()) {
			route += "_4";
			outFrame = inFrame;
		}
		else {
            route += "_5";
			std::tie(ret, outFrame, fifo) = processMediaFrame(route, inFrame);
            if (ret != HJ_OK) {
                route += "_6";
                break;
            }
		}

		if (outFrame != nullptr) {
            route += "_7";
			deliverToOutputs(outFrame);
		}
		else {
            route += "_8";
			for (; fifo;) {
                route += "_9";
				int err{};
				std::tie(err, outFrame) = getAndDeliverOutputFrame(route);
				if (err < 0) {
                    route += "_A";
					ret = err;
					break;
				}
				if (outFrame == nullptr) {
                    route += "_B";
					break;
				}
            }
		}
#endif
        route += "_C";
	} while (false);
//    addOutIdx();
	if (log) {
		RUNTASKLog("{}, leave, route({}), size({}), task duration({}), ret({})", getName(), route, size, (HJCurrentSteadyMS() - m_enterTimestamp), ret);
	}
	return ret;
}
/*
int HJPluginAudioResampler::processInputFrame(std::string& route, HJMediaFrame::Ptr& inFrame)
{
	HJMediaFrame::Ptr outFrame{};
	bool fifo{ false };
	int streamIndex{};
	auto ret = SYNC_PROD_LOCK([this, &route, &inFrame, &outFrame, &fifo, &streamIndex] {
		if (m_status == HJSTATUS_Done) {
			route += "_30";
			return HJErrAlreadyDone;
		}
		if (m_status >= HJSTATUS_Stoped) {
			route += "_22";
			return HJErrInvalidData;
		}

		outFrame = m_converter->convert(std::move(inFrame));
		if (outFrame == nullptr) {
			route += "_31";
			HJFLoge("{}, (m_converter->convert() == nullptr)", getName());
			return HJ_WOULD_BLOCK;
		}
		outFrame->m_streamIndex = inFrame-> m_streamIndex;

		if (m_fifo != nullptr) {
			route += "_32";
			outFrame->setAVTimeBase(HJTimeBase{ 1, m_audioInfo->m_samplesRate });
			auto err = m_fifo->addFrame(std::move(outFrame));
			if (err < 0) {
				route += "_33";
				HJFLoge("{}, m_fifo->addFrame() error({})", getName(), err);
				return HJErrFatal;
			}
            fifo = true;
			streamIndex = outFrame->m_streamIndex;
		}

		route += "_34";
		return HJ_OK;
	});
	if (ret == HJ_OK) {
		if (fifo) {
			route += "_35";
			ret = drainFifoFrames(route, streamIndex);
		}
		else {
			route += "_36";
			deliverToOutputs(outFrame);
		}
	}
	else if (ret == HJErrFatal) { 
		IF_FALSE_RETURN(setStatus(HJSTATUS_Exception), HJErrAlreadyDone);
		//if (m_pluginListener) {
		//	route += "_37";
		//	m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_ERROR_AUDIOFIFO_ADDFRAME)));
		//}
		report(EVENT_PLUGIN_NOTIFY_ID, HJ_PLUGIN_NOTIFY_ERROR_AUDIOFIFO_ADDFRAME, getID());
	}
	else if (ret == HJ_WOULD_BLOCK) {
		ret = HJ_OK;
	}

	return ret;
}

int HJPluginAudioResampler::drainFifoFrames(std::string& route, int streamIndex)
{
	int ret = HJ_OK;
	for (;;) {
		route += "_20";
		HJMediaFrame::Ptr outFrame{};
		ret = SYNC_CONS_LOCK([&route, &outFrame, this] {
			if (m_status == HJSTATUS_Done) {
				route += "_21";
				return HJErrAlreadyDone;
			}

			outFrame = m_fifo->getFrame();

			route += "_22";
			return HJ_OK;
		});
		if (ret != HJ_OK) {
			break;
		}
		if (outFrame == nullptr) {
			route += "_23";
			break;
		}

		route += "_24";
		outFrame->m_streamIndex = streamIndex;
		deliverToOutputs(outFrame);
	}

	return ret;
}
*/
std::tuple<int, HJMediaFrame::Ptr, bool> HJPluginAudioResampler::processMediaFrame(std::string& route, HJMediaFrame::Ptr& inFrame)
{
	int ret{ HJ_OK };
	HJMediaFrame::Ptr outFrame{};
	bool fifo{ false };
	int reason{ -1 };
	do {
		ret = SYNC_PROD_LOCK([this, &route, &inFrame, &outFrame, &fifo, &reason] {
			if (m_status == HJSTATUS_Done) {
				route += "_30";
				return HJErrAlreadyDone;
			}
			if (m_status >= HJSTATUS_Stoped) {
				route += "_22";
				return HJErrInvalidData;
			}

			m_streamIndex = inFrame->m_streamIndex;
			outFrame = m_converter->convert(std::move(inFrame));
			if (outFrame == nullptr) {
				route += "_31";
				HJFLoge("{}, (m_converter->convert() == nullptr)", getName());
				reason = 0;
				return HJErrFatal;
			}
			outFrame->m_streamIndex = m_streamIndex;

			if (m_fifo != nullptr) {
				route += "_32";
				outFrame->setAVTimeBase(HJTimeBase{ 1, m_audioInfo->m_samplesRate });
				auto err = m_fifo->addFrame(std::move(outFrame));
				if (err < 0) {
					route += "_33";
					HJFLoge("{}, m_fifo->addFrame() error({})", getName(), err);
					reason = 1;
					return HJErrFatal;
				}
				fifo = true;
				outFrame = nullptr;
			}

			route += "_34";
			return HJ_OK;
		});
		if (ret == HJErrFatal) {
			IF_FALSE_BREAK(setStatus(HJSTATUS_Exception), HJErrAlreadyDone);
			//if (m_pluginListener) {
			//	route += "_37";
			//	if (reason == 0) {
			//		m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_ERROR_AUDIOCONVERTER_CONVERT)));
			//	}
			//	else {
			//		m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_ERROR_AUDIOFIFO_ADDFRAME)));
			//	}
			//}
			if (reason == 0) {
				report(EVENT_PLUGIN_NOTIFY_ID, HJ_PLUGIN_NOTIFY_ERROR_AUDIOCONVERTER_CONVERT, getID());
			}
			else {
				report(EVENT_PLUGIN_NOTIFY_ID, HJ_PLUGIN_NOTIFY_ERROR_AUDIOFIFO_ADDFRAME, getID());
			}
		}

	} while (false);

	return std::make_tuple(ret, outFrame, fifo);
}

std::tuple<int, HJMediaFrame::Ptr> HJPluginAudioResampler::getAndDeliverOutputFrame(std::string& route)
{
	int ret{ HJ_OK };
	HJMediaFrame::Ptr outFrame{};
	do { 
		ret = SYNC_PROD_LOCK([&route, &outFrame, this] {
			if (m_status == HJSTATUS_Done) {
				route += "_21";
				return HJErrAlreadyDone;
			}

			outFrame = m_fifo->getFrame();

			route += "_22";
			return HJ_OK;
		});
		if (ret != HJ_OK) {
			break;
		}
		if (outFrame == nullptr) {
			route += "_23";
			break;
		}

		route += "_24";
		outFrame->m_streamIndex = m_streamIndex;
		//
		auto pts = outFrame->getPTS();
		auto delta = (HJ_NOTS_VALUE != m_last_time) ? pts - m_last_time : 0;
		m_last_time = pts;
		if (delta > 25) {
			HJFLogi("{}, deliver Outputs frame:{}, pts:{}, delta:{}", getName(), outFrame->formatInfo(), pts, delta);
		}
		//
		deliverToOutputs(outFrame);
	} while (false);

	return std::make_tuple(ret, outFrame);
}

NS_HJ_END
