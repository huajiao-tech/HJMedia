#include "HJPlugin.h"
#include "HJFLog.h"

HJEnumToStringFuncImplBegin(HJPluginNofityType)
HJEnumToStringItem(HJ_PLUGIN_NOTIFY_NONE),
HJEnumToStringItem(HJ_PLUGIN_NOTIFY_ERROR_MUXER_INIT),
HJEnumToStringItem(HJ_PLUGIN_NOTIFY_ERROR_MUXER_WRITEFRAME),
HJEnumToStringItem(HJ_PLUGIN_NOTIFY_ERROR_CODEC_RUN),
HJEnumToStringItem(HJ_PLUGIN_NOTIFY_ERROR_CODEC_GETFRAME),
HJEnumToStringItem(HJ_PLUGIN_NOTIFY_ERROR_AUDIOFIFO_ADDFRAME),
HJEnumToStringItem(HJ_PLUGIN_NOTIFY_ERROR_CAPTURER_GETFRAME),
HJEnumToStringFuncImplEnd(HJPluginNofityType);

NS_HJ_BEGIN

HJPlugin::HJPlugin(const std::string& i_name, size_t i_identify)
	: HJSyncObject(i_name, i_identify)
{
}

int HJPlugin::internalInit(HJKeyStorage::Ptr i_param)
{
	int ret = HJSyncObject::internalInit(i_param);
	if (ret < 0) {
		return ret;
	}

	GET_PARAMETER(HJLooperThread::Ptr, thread);
	GET_PARAMETER(bool, createThread);
	GET_PARAMETER(HJListener, pluginListener);

	do {
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
		return HJ_OK;
	} while (false);

	HJPlugin::internalRelease();
	return ret;
}

void HJPlugin::internalRelease()
{
	for (auto it : m_inputs) {
		auto input = it.second;
		auto plugin = (input->plugin).lock();
		if (plugin != nullptr) {
			plugin->onOutputDisconnected(input->myKeyHash);
		}
	}
	m_inputSync.prodLock([=] {
		m_inputs.clear();
	});

	for (auto it : m_outputs) {
		auto output = it.second;
		auto plugin = (output->plugin).lock();
		if (plugin != nullptr) {
			plugin->onInputDisconnected(output->myKeyHash);
		}
	}
	m_outputSync.prodLock([=] {
		m_outputs.clear();
	});

	if (m_thread) {
		m_thread->done();
		m_thread = nullptr;
	}
	m_handler = nullptr;

    m_pluginListener = nullptr;
	m_runTaskId = -1;
	m_streamIndex = -1;

	HJSyncObject::internalRelease();
}

void HJPlugin::afterInit()
{
	postTask();
}

void HJPlugin::beforeDone() {
	m_quitting.store(true);
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

int HJPlugin::setInputMaxSize(size_t i_srcKeyHash, int i_maxSize)
{
	if (i_maxSize < 0) {
		return HJErrInvalidParams;
	}

	auto input = getInput(i_srcKeyHash);
	if (input == nullptr) {
		return HJErrNotFind;
	}

	input->mediaFrames.setMaxSize(i_maxSize);
	return HJ_OK;
}

int HJPlugin::canDeliverToInput(size_t i_srcKeyHash)
{
	auto input = getInput(i_srcKeyHash);
	if (input == nullptr) {
		return HJErrNotFind;
	}
	if (input->mediaFrames.isFull()) {
		return HJErrFull;
	}

	return HJ_OK;
}

int HJPlugin::deliver(size_t i_srcKeyHash, HJMediaFrame::Ptr& i_mediaFrame)
{
	auto input = getInput(i_srcKeyHash);
	if (input == nullptr) {
		return HJErrNotFind;
	}

	if (input->mediaFrames.isFull()) {
		return HJErrFull;
	}
	if (!input->mediaFrames.deliver(i_mediaFrame)) {
		return HJErrFatal;
	}

	internalUpdated();
	return HJ_OK;
}

void HJPlugin::onOutputUpdated()
{
	internalUpdated();
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

void HJPlugin::postTask()
{
	if (m_handler != nullptr) {
		m_handler->asyncAndClear([=] {
			if (runTask() == HJ_OK) {
				internalUpdated();
			}
		}, m_runTaskId);
	}
}

void HJPlugin::internalUpdated()
{
	SYNC_CONS_LOCK([=] {
		postTask();
	});
}

HJMediaFrame::Ptr HJPlugin::receive(size_t i_srcKeyHash, size_t& o_size)
{
	auto input = getInput(i_srcKeyHash);
	if (input == nullptr) {
		return nullptr;
	}

	auto mediaFrame = input->mediaFrames.receive(o_size);
	auto plugin = input->plugin.lock();

	if (mediaFrame && plugin) {
		plugin->onOutputUpdated();
	}

	return mediaFrame;
}

HJMediaFrame::Ptr HJPlugin::preview(size_t i_srcKeyHash, size_t& o_size)
{
	auto input = getInput(i_srcKeyHash);
	if (input == nullptr) {
		return nullptr;
	}

	return input->mediaFrames.preview(o_size);
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

size_t HJPlugin::getKeyHash(const std::string& i_name, HJMediaType i_type, int i_trackId)
{
	// 联合哈希计算
	size_t h1 = std::hash<std::string>()(i_name);
	size_t h2 = std::hash<int>()(static_cast<int>(i_type));
	size_t h3 = std::hash<int>()(i_trackId);

	// 哈希组合算法（参见Boost.hash_combine）
	return h1 ^ (h2 << 1) ^ (h3 << 2);
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
