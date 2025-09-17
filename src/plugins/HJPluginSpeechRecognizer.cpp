#include "HJPluginSpeechRecognizer.h"

#include <libavutil/frame.h>

#include "HJFLog.h"

NS_HJ_BEGIN

#define RUNTASKLog		//HJFLogi

int HJPluginSpeechRecognizer::internalInit(HJKeyStorage::Ptr i_param)
{
	GET_PARAMETER(HJAudioInfo::Ptr, audioInfo);
	if (!audioInfo) {
		return HJErrInvalidParams;
	}
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
		m_audioInfo = audioInfo;
		return HJ_OK;
	} while (false);

	HJPluginSpeechRecognizer::internalRelease();
	return ret;
}

void HJPluginSpeechRecognizer::internalRelease()
{
	m_audioInfo = nullptr;

	HJPlugin::internalRelease();
}

void HJPluginSpeechRecognizer::onInputAdded(size_t i_srcKeyHash, HJMediaType i_type)
{
	m_inputKeyHash.store(i_srcKeyHash);
}

int HJPluginSpeechRecognizer::runTask(int64_t* o_delay)
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

		auto audioInfo = inFrame->getAudioInfo();
		// 获取数据缓冲区
		uint8_t* data = nullptr;
		int data_size;
		HJMediaFrame::getDataFromAVFrame(inFrame, data, data_size);
		if (m_call) {
			// HJFLogi("wkshhh data_size: {} {}", data_size, inFrame.use_count());
			m_call(inFrame);
		}

		// 可选：如果需要将数据传递给下游插件
		// deliverToOutputs(inFrame);

	} while (false);

	RUNTASKLog("{}, leave, route({}), size({}), duration({}), ret({})",
		getName(), route, size, (HJCurrentSteadyMS() - enter), ret);
	return ret;
}

void HJPluginSpeechRecognizer::deliverToOutputs(HJMediaFrame::Ptr& i_mediaFrame)
{
	// 如果需要将数据传递给下游插件，可以调用父类方法
	HJPlugin::deliverToOutputs(i_mediaFrame);
}

NS_HJ_END
