#include "HJPlugin.h"
#include "HJGraph.h"
#include "HJFLog.h"

HJEnumToStringFuncImplBegin(HJPluginNofityType)
	HJEnumToStringItem(HJ_PLUGIN_NOTIFY_NONE),
	HJEnumToStringItem(HJ_PLUGIN_NOTIFY_ERROR_MUXER_INIT),
	HJEnumToStringItem(HJ_PLUGIN_NOTIFY_ERROR_MUXER_WRITEFRAME),
	HJEnumToStringItem(HJ_PLUGIN_NOTIFY_ERROR_CODEC_INIT),
	HJEnumToStringItem(HJ_PLUGIN_NOTIFY_ERROR_CODEC_RUN),
	HJEnumToStringItem(HJ_PLUGIN_NOTIFY_ERROR_CODEC_GETFRAME),
	HJEnumToStringItem(HJ_PLUGIN_NOTIFY_ERROR_AUDIOFIFO_ADDFRAME),
	HJEnumToStringItem(HJ_PLUGIN_NOTIFY_ERROR_CAPTURER_GETFRAME),
	HJEnumToStringItem(HJ_PLUGIN_NOTIFY_ERROR_DEMUXER_INIT),
	HJEnumToStringItem(HJ_PLUGIN_NOTIFY_ERROR_DEMUXER_GETFRAME),
	HJEnumToStringItem(HJ_PLUGIN_NOTIFY_VIDEORENDER_FRAME),
	HJEnumToStringItem(HJ_PLUGIN_NOTIFY_VIDEORENDER_FIRST_FRAME),
	HJEnumToStringItem(HJ_PLUGIN_NOTIFY_ERROR_AUDIORENDER_INIT),
	HJEnumToStringItem(HJ_PLUGIN_NOTIFY_AUDIORENDER_START_PLAYING),
	HJEnumToStringItem(HJ_PLUGIN_NOTIFY_AUDIORENDER_STOP_PLAYING),
	HJEnumToStringItem(HJ_PLUGIN_NOTIFY_AUDIORENDER_START_BUFFERING),
	HJEnumToStringItem(HJ_PLUGIN_NOTIFY_AUDIORENDER_STOP_BUFFERING),
	HJEnumToStringItem(HJ_PLUGIN_NOTIFY_AUDIOCACHE_DURATION),
	HJEnumToStringItem(HJ_PLUGIN_NOTIFY_AUTODELAY_PARAMS),
HJEnumToStringFuncImplEnd(HJPluginNofityType);

NS_HJ_BEGIN

HJPlugin::HJPlugin(const std::string& i_name, HJKeyStorage::Ptr i_graphInfo)
	: HJSyncObject(i_name, -1)
{
	m_insName = m_name;
	if (i_graphInfo) {
		const std::any* anyObj = i_graphInfo->getStorage("insIdx");
		if (anyObj) {
			m_insIdx = std::any_cast<int>(*anyObj);
			m_insName += HJFMT("_{}", m_insIdx);
		}

		auto graph = i_graphInfo->getValue<HJGraph::Ptr>("graph");
		if (graph) {
			m_graph = graph;
		}
	}
}

HJPlugin::~HJPlugin()
{
	HJPlugin::done();
}

const std::string& HJPlugin::getName() const {
	return m_insName;
}

int HJPlugin::done()
{
	m_quitting.store(true);
	return HJSyncObject::done();
}

int HJPlugin::internalInit(HJKeyStorage::Ptr i_param)
{
	int ret = HJSyncObject::internalInit(i_param);
	if (ret < 0) {
		return ret;
	}

	do {
		if (i_param) {
			GET_PARAMETER(HJLooperThread::Ptr, thread);
			GET_PARAMETER(bool, createThread);
			GET_PARAMETER(HJListener, pluginListener);

			if (thread != nullptr) {
				m_handler = thread->createHandler();
				if (m_handler == nullptr) {
					ret = HJErrFatal;
					break;
				}
			}
			else if (createThread) {
				m_thread = HJLooperThread::quickStart(getName());
				if (m_thread == nullptr) {
					ret = HJErrFatal;
					break;
				}

				m_handler = m_thread->createHandler();
				if (m_handler == nullptr) {
					ret = HJErrFatal;
					break;
				}
			}

			if (m_handler) {
                m_runTaskId = m_handler->genMsgId();
            }
			m_pluginListener = pluginListener;
		}

		return HJ_OK;
	} while (false);

	HJPlugin::internalRelease();
	return ret;
}

void HJPlugin::internalRelease()
{
	{
		InputMap inputs;
		m_inputSync.prodLock([&inputs, this] {
			inputs = m_inputs;
			m_inputs.clear();
		});
		for (auto it : inputs) {
			auto input = it.second;
			input->mediaFrames.done();
			auto plugin = (input->plugin).lock();
			if (plugin != nullptr) {
				plugin->onOutputDisconnected(input->myKeyHash);
			}
		}
	}

	{
		OutputMap outputs;
		m_outputSync.prodLock([&outputs, this] {
			outputs = m_outputs;
			m_outputs.clear();
		});
		for (auto it : outputs) {
			auto output = it.second;
			auto plugin = (output->plugin).lock();
			if (plugin != nullptr) {
				plugin->onInputDisconnected(output->myKeyHash);
			}
		}
	}

	if (m_thread) {
		m_thread->done();
		m_thread = nullptr;
	}
	m_handlerSync.prodLock([this] {
		m_handler = nullptr;
	});
//	m_runTaskId = -1;

//	m_pluginListener = nullptr;		// 外部应该设置​Lambda表达式和弱指针参数，此处不需要置空，调用应该在锁外
//	m_graph.reset();				// 不需要reset()，调用应该在锁外

	HJSyncObject::internalRelease();

	setInfoStatus(m_status);
}

void HJPlugin::afterInit()
{
	setInfoStatus(m_status);

	postTask();
}

int HJPlugin::addInputPlugin(Ptr i_plugin, HJMediaType i_type, int i_trackId)
{
	if (i_plugin == nullptr || i_trackId < 0) {
		return HJErrInvalidParams;
	}

	return m_inputSync.prodLock([=] {
		if (m_quitting.load()) {
			return HJErrAlreadyDone;
		}

		auto srcKeyHash = getKeyHash(i_plugin->getName(), i_type, i_trackId);
		auto it = m_inputs.find(srcKeyHash);
		if (it != m_inputs.end()) {
			return HJErrAlreadyExist;
		}

		auto input = std::make_shared<Input>();
		input->type = i_type;
		input->trackId = i_trackId;
		input->plugin = i_plugin;
		input->myKeyHash = getKeyHash(getName(), i_type, i_trackId);
		m_inputs[srcKeyHash] = input;

		onInputAdded(srcKeyHash, i_type);
		return HJ_OK;
	});
}

int HJPlugin::addOutputPlugin(Ptr i_plugin, HJMediaType i_type, int i_trackId)
{
	if (i_plugin == nullptr || i_trackId < 0) {
		return HJErrInvalidParams;
	}

	return m_outputSync.prodLock([=] {
		if (m_quitting.load()) {
			return HJErrAlreadyDone;
		}

		auto dstKeyHash = getKeyHash(i_plugin->getName(), i_type, i_trackId);
		auto it = m_outputs.find(dstKeyHash);
		if (it != m_outputs.end()) {
			return HJErrAlreadyExist;
		}

		auto input = std::make_shared<Output>();
		input->type = i_type;
		input->trackId = i_trackId;
		input->plugin = i_plugin;
		input->myKeyHash = getKeyHash(getName(), i_type, i_trackId);
		m_outputs[dstKeyHash] = input;

		onOutputAdded(dstKeyHash, i_type);
		return HJ_OK;
	});
}

void HJPlugin::onInputDisconnected(size_t i_srcKeyHash)
{
	m_inputSync.prodLock([=] {
		m_inputs.erase(i_srcKeyHash);
	});
}

void HJPlugin::onOutputDisconnected(size_t i_dstKeyHash)
{
	m_outputSync.prodLock([=] {
		m_outputs.erase(i_dstKeyHash);
	});
}

int HJPlugin::deliver(size_t i_srcKeyHash, HJMediaFrame::Ptr& i_mediaFrame, size_t* o_size, int64_t* o_audioDuration, int64_t* o_videoKeyFrames, int64_t* o_audioSamples)
{
	auto input = getInput(i_srcKeyHash);
	if (input == nullptr) {
		return HJErrNotFind;
	}

	if (!input->mediaFrames.deliver(i_mediaFrame, o_size, o_audioDuration, o_videoKeyFrames, o_audioSamples)) {
		return HJErrFatal;
	}

	onInputUpdated();
	return HJ_OK;
}

void HJPlugin::onInputUpdated()
{
	postTask();
}

void HJPlugin::onOutputUpdated()
{
	postTask();
}

bool HJPlugin::setStatus(HJStatus i_status, bool i_underLock)
{
	if (i_underLock) {
		if (!SYNC_PROD_LOCK([i_status, this] {
			CHECK_DONE_STATUS(false);
			m_status = i_status;
			return true;
		})) {
			return false;
		}
	}
	else {
		if (m_status == HJSTATUS_Done) {
			return false;
		}
		m_status = i_status;
	}

	setInfoStatus(i_status);
	return true;
}

HJPlugin::Input::Ptr HJPlugin::getInput(size_t i_srcKeyHash)
{
	return m_inputSync.consLock([=] {
		if (m_quitting.load()) {
			return static_cast<Input::Ptr>(nullptr);
		}

		auto it = m_inputs.find(i_srcKeyHash);
		if (it == m_inputs.end()) {
			return static_cast<Input::Ptr>(nullptr);
		}
		return it->second;
	});
}

HJPlugin::Output::Ptr HJPlugin::getOutput(size_t i_dstKeyHash)
{
	return m_outputSync.consLock([=] {
		if (m_quitting.load()) {
			return static_cast<Output::Ptr>(nullptr);
		}

		auto it = m_outputs.find(i_dstKeyHash);
		if (it == m_outputs.end()) {
			return static_cast<Output::Ptr>(nullptr);
		}
		return it->second;
	});
}

void HJPlugin::postTask(int64_t i_delay)
{
	HJLooperThread::Handler::Ptr handler{};
	m_handlerSync.consLock([&handler, this] {
		handler = m_handler;
	});

	if (handler != nullptr) {
		Wtr wPlugin = SHARED_FROM_THIS;
		handler->asyncAndClear([wPlugin] {
			auto plugin = wPlugin.lock();
			if (plugin) {
				int64_t delay = 0;
				if (plugin->runTask(&delay) == HJ_OK) {
					plugin->postTask(delay);
				}
			}
		}, m_runTaskId, i_delay);
	}
}

HJMediaFrame::Ptr HJPlugin::receive(size_t i_srcKeyHash, size_t* o_size, int64_t* o_audioDuration, int64_t* o_videoKeyFrames, int64_t* o_audioSamples)
{
	auto input = getInput(i_srcKeyHash);
	if (input == nullptr) {
		return nullptr;
	}

	auto mediaFrame = input->mediaFrames.receive(o_size, o_audioDuration, o_videoKeyFrames, o_audioSamples);
	auto plugin = input->plugin.lock();

	if (mediaFrame && plugin) {
		plugin->onOutputUpdated();
	}

	return mediaFrame;
}

HJMediaFrame::Ptr HJPlugin::preview(size_t i_srcKeyHash, size_t* o_size, int64_t* o_audioDuration, int64_t* o_videoKeyFrames)
{
	auto input = getInput(i_srcKeyHash);
	if (input == nullptr) {
		return nullptr;
	}

	return input->mediaFrames.preview(o_size, o_audioDuration, o_videoKeyFrames);
}

int64_t HJPlugin::audioSamplesOfInput(size_t i_srcKeyHash)
{
    auto input = getInput(i_srcKeyHash);
	if (input == nullptr) {
		return 0;
	}
    
    return input->mediaFrames.audioSamples();
}

int64_t HJPlugin::audioDurationOfInput(size_t i_srcKeyHash)
{
	auto input = getInput(i_srcKeyHash);
	if (input == nullptr) {
		return 0;
	}

	return input->mediaFrames.audioDuration();
}

void HJPlugin::deliverToOutputs(HJMediaFrame::Ptr& i_mediaFrame)
{
	OutputMap outputs;
	m_outputSync.consLock([&outputs, this] {
		outputs = m_outputs;
	});
	for (auto it = outputs.begin(); it != outputs.end(); ++it) {
		auto output = it->second;
		auto plugin = output->plugin.lock();
		if (plugin != nullptr) {
			auto err = plugin->deliver(output->myKeyHash, i_mediaFrame);
			if (err < 0) {
				HJFLoge("{}, plugin->deliver() error({})", getName(), err);
			}
		}
	}
}

void HJPlugin::setInfoStatus(HJStatus i_status) {
	auto graph = m_graph.lock();
	if (graph) {
		HJGraph::Information info = HJMakeNotification(HJ_PLUGIN_SETINFO_PLUGIN_status, static_cast<int64_t>(i_status));
		info->setName(HJSyncObject::getName());
		graph->setInfo(std::move(info));
	}
}

size_t HJPlugin::getKeyHash(const std::string& i_name, HJMediaType i_type, int i_trackId)
{
#if 0
	// 联合哈希计算
	size_t h1 = std::hash<std::string>()(i_name);
	size_t h2 = std::hash<int>()(static_cast<int>(i_type));
	size_t h3 = std::hash<int>()(i_trackId);

	// 哈希组合算法（参见Boost.hash_combine）
	return h1 ^ (h2 << 1) ^ (h3 << 2);
#else
	size_t h1 = std::hash<std::string>{}(i_name);
	size_t h2 = std::hash<int>{}(static_cast<int>(i_type));
	size_t h3 = std::hash<int>{}(i_trackId);

	// 改进的组合方式，避免简单的位移和异或
	// 使用Boost风格的hash_combine
	size_t seed = h1;
	seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	seed ^= h3 + 0x9e3779b9 + (seed << 6) + (seed >> 2);

	return seed;
#endif
}

const std::string& HJPlugin::Output::name()
{
	auto p = plugin.lock();
	if (p) {
		return p->getName();
	}
	else {
		static std::string emptyStr;
		return emptyStr;
	}
}

HJPlugin::Input::~Input()
{
	mediaFrames.flush();
}

NS_HJ_END
