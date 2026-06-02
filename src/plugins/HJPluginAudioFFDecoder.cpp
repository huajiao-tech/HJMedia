#include "HJPluginAudioFFDecoder.h"
#include "HJGraph.h"
#include "HJFLog.h"

NS_HJ_BEGIN

#define RUNTASKLog		HJFLogi

int HJPluginAudioFFDecoder::internalInit(HJKeyStorage::Ptr i_param)
{
	MUST_HAVE_PARAMETERS;
	GET_PARAMETER(HJAudioInfo::Ptr, audioInfo);
	GET_PARAMETER(HJLooperThread::Ptr, thread);

	auto param = HJKeyStorage::dupFrom(i_param);
	if (audioInfo) {
        (*param)["streamInfo"] = std::static_pointer_cast<HJStreamInfo>(audioInfo);
	}
	(*param)["createThread"] = (thread == nullptr);
	return HJPluginCodec::internalInit(param);
}

void HJPluginAudioFFDecoder::onInputAdded(size_t i_srcKeyHash, HJMediaType i_type)
{
	HJPluginCodec::onInputAdded(i_srcKeyHash, i_type);

	auto input = m_inputs[i_srcKeyHash];
	input->eventFlags = EVENT_FLAG_AUDIO_DURATION;
}

int HJPluginAudioFFDecoder::runTask(int64_t* o_delay)
{
//	addInIdx();
	auto log = logRunTask();
//    log = true;
	if (log) {
		RUNTASKLog("{}, enter", getName());
	}

	std::string route{};
	int64_t size{ -1 };
	int ret{ HJ_OK };
	do {
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

		if (inFrame->isFlushFrame()) {
            route += "_4";
			ret = processFlushFrame(route, inFrame);
			if (ret != HJ_OK) {
                route += "_5";
				break;
			}
		}

        if (inFrame->isEofFrame()) {
			route += "_6";
			ret = processEofFrame(route, inFrame);
			if (ret != HJ_OK) {
				route += "_7";
				break;
			}
		}
        
		ret = processMediaFrame(route, inFrame);
		if (ret < 0) {
            route += "_8";
			break;
		}

		for (;;) {
            route += "_9";
			auto [err, outFrame] = getOutputFrame(route);
			if (err < 0) {
                route += "_A";
				ret = err;
				break;
			}
			if (outFrame == nullptr) {
				route += HJFMT("_B({})", err);
//                route += "_9";
				break;
			}

			deliverToOutputs(outFrame);

			if (outFrame->isEofFrame()) {
				route += "_C";
				HJFLogi("{}, (outFrame->isEofFrame())", getName());
				IF_FALSE_BREAK(setStatus(HJSTATUS_EOF), HJErrAlreadyDone);
				break;
			}
		}
        
        route += "_D";
	} while (false);

	// log
//	addOutIdx();
	if (log) {
		RUNTASKLog("{}, leave, route({}), size({}), task duration({}), ret({})", getName(), route, size, (HJCurrentSteadyMS() - m_enterTimestamp), ret);
	}
	return ret;
}

HJBaseCodec::Ptr HJPluginAudioFFDecoder::createCodec()
{
	return HJBaseCodec::createADecoder();
}

NS_HJ_END
