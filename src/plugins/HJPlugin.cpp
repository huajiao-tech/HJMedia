#include "HJPlugin.h"
#include "HJGraph.h"
#include "HJFLog.h"

HJEnumToStringFuncImplBegin(HJPluginNotifyType)
	HJEnumToStringItem(HJ_PLUGIN_NOTIFY_NONE),
	HJEnumToStringItem(HJ_PLUGIN_NOTIFY_EOF),
	HJEnumToStringItem(HJ_PLUGIN_NOTIFY_ERROR_MUXER_INIT),
	HJEnumToStringItem(HJ_PLUGIN_NOTIFY_ERROR_MUXER_WRITEFRAME),
	HJEnumToStringItem(HJ_PLUGIN_NOTIFY_ERROR_CODEC_INIT),
	HJEnumToStringItem(HJ_PLUGIN_NOTIFY_ERROR_CODEC_RUN),
	HJEnumToStringItem(HJ_PLUGIN_NOTIFY_ERROR_CODEC_GETFRAME),
	HJEnumToStringItem(HJ_PLUGIN_NOTIFY_ERROR_AUDIOCONVERTER_CONVERT),
	HJEnumToStringItem(HJ_PLUGIN_NOTIFY_ERROR_AUDIOFIFO_ADDFRAME),
	HJEnumToStringItem(HJ_PLUGIN_NOTIFY_ERROR_CAPTURER_GETFRAME),
	HJEnumToStringItem(HJ_PLUGIN_NOTIFY_ERROR_DEMUXER_INIT),
	HJEnumToStringItem(HJ_PLUGIN_NOTIFY_ERROR_DEMUXER_GETFRAME),
	HJEnumToStringItem(HJ_PLUGIN_NOTIFY_DEMUXER_EOF),
	HJEnumToStringItem(HJ_PLUGIN_NOTIFY_DEMUXER_DURATION),
	HJEnumToStringItem(HJ_PLUGIN_NOTIFY_VIDEORENDER_FRAME),
	HJEnumToStringItem(HJ_PLUGIN_NOTIFY_VIDEORENDER_FIRST_FRAME),
	HJEnumToStringItem(HJ_PLUGIN_NOTIFY_VIDEORENDER_EOF),
	HJEnumToStringItem(HJ_PLUGIN_NOTIFY_ERROR_AUDIORENDER_INIT),
	HJEnumToStringItem(HJ_PLUGIN_NOTIFY_ERROR_AUDIORENDER_RUNTASK),
	HJEnumToStringItem(HJ_PLUGIN_NOTIFY_AUDIORENDER_START_PLAYING),
	HJEnumToStringItem(HJ_PLUGIN_NOTIFY_AUDIORENDER_STOP_PLAYING),
	HJEnumToStringItem(HJ_PLUGIN_NOTIFY_AUDIORENDER_START_BUFFERING),
	HJEnumToStringItem(HJ_PLUGIN_NOTIFY_AUDIORENDER_STOP_BUFFERING),
	HJEnumToStringItem(HJ_PLUGIN_NOTIFY_AUDIORENDER_EOF),
	HJEnumToStringItem(HJ_PLUGIN_NOTIFY_AUDIORENDER_FRAME),
	HJEnumToStringItem(HJ_PLUGIN_NOTIFY_AUDIOCACHE_DURATION),
	HJEnumToStringItem(HJ_PLUGIN_NOTIFY_AUTODELAY_PARAMS),
	HJEnumToStringItem(HJ_PLUGIN_NOTIFY_PLUGIN_SETINFOS),
HJEnumToStringFuncImplEnd(HJPluginNotifyType);

NS_HJ_BEGIN

static size_t getKeyHash(const std::string& i_name, HJMediaType i_type, int i_trackId)
{
	size_t h1 = std::hash<std::string>{}(i_name);
	size_t h2 = std::hash<int>{}(static_cast<int>(i_type));
	size_t h3 = std::hash<int>{}(i_trackId);
	// Improved combine to reduce trivial shifts/collisions (Boost-style hash_combine).
	size_t seed = h1;
	seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	seed ^= h3 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	return seed;
}

static uint64_t getHash64(const char* str) {
	uint64_t hash = 14695981039346656037ULL; // FNV offset basis for 64-bit
	while (*str) {
		hash ^= static_cast<uint8_t>(*str++);
		hash *= 1099511628211ULL; // FNV prime for 64-bit
	}
	return hash;
}

HJPlugin::HJPlugin(const std::string& i_name, size_t i_identify, HJKeyStorage::Ptr i_graphInfo)
	: HJSyncObject(i_name, i_identify)
{
	m_insName = m_name;
	if (i_graphInfo) {
		const std::any* anyObj = i_graphInfo->getStorage("insIdx");
		if (anyObj) {
			auto insIdx = std::any_cast<int>(*anyObj);
			m_insName += HJFMT("_{}", insIdx);
		}

		auto graph = i_graphInfo->getValue<HJGraph::Ptr>("graph");
		if (graph) {
//			m_graph = graph;
			m_graphQueryBus = graph->queryBus();
			m_graphEventBus = graph->eventBus();
		}
	}

	if (m_id == 0) {
		m_id = getHash64(m_name.c_str());
	}
}

HJPlugin::~HJPlugin()
{
	done();
	clearInputsAndOutputs();
//	m_graph.reset();
	m_graphQueryBus.reset();
	m_graphEventBus.reset();
}

const std::string& HJPlugin::getName() const {
	return m_insName;
}

int HJPlugin::init(HJKeyStorage::Ptr i_param)
{
	auto ret = HJSyncObject::init(i_param);
	if (ret == HJ_OK) {
		report(EVENT_STATUS_UPDATED_ID, getID());
	}

	return ret;
}

int HJPlugin::done()
{
	auto ret = HJSyncObject::done();
	if (ret == HJ_OK) {
		report(EVENT_STATUS_UPDATED_ID, getID());
	}

	return ret;
}

int HJPlugin::internalInit(HJKeyStorage::Ptr i_param)
{
	int ret = HJSyncObject::internalInit(i_param);
	if (ret != HJ_OK) {
		return ret;
	}

	if (i_param) {
		GET_PARAMETER(HJLooperThread::Ptr, thread);
		GET_PARAMETER(bool, createThread);
//		GET_PARAMETER(HJListener, pluginListener);
		if (thread == nullptr && createThread) {
			m_thread = HJLooperThread::quickStart(getName());
			if (m_thread == nullptr) {
				return HJErrFatal;
			}
			thread = m_thread;
		}
		if (thread != nullptr) {
			auto ok = m_handlerSync.prodLock([this, thread] {
				m_handler = thread->createHandler();
				if (m_handler == nullptr) {
					return false;
				}
				m_runTaskId = m_handler->genMsgId();
				return true;
			});
			if (!ok) {
				return HJErrFatal;
			}
		}
//		m_pluginListener = pluginListener;
	}

	return HJ_OK;
}

void HJPlugin::internalRelease()
{
	HJFLogi("{}, internalRelease() begin", getName());

	if (m_thread) {
		m_thread->done();
		m_thread = nullptr;
	}
	m_handlerSync.prodLock([this] {
		m_handler = nullptr;
	});
	//	m_pluginListener = nullptr;		// External side should keep lambda/weak args; no need to clear here.
	HJSyncObject::internalRelease();

	HJFLogi("{}, internalRelease() end", getName());
}

int HJPlugin::beforeDone()
{
	m_quitting.store(true);
	return HJSyncObject::beforeDone();
}

void HJPlugin::afterInit()
{
	postTask();
	HJSyncObject::afterInit();
}

int HJPlugin::addInputPlugin(Ptr i_plugin, HJMediaType i_type, int i_trackId)
{
	if (i_plugin == nullptr || i_trackId < 0) {
		return HJErrInvalidParams;
	}

	return m_inputSync.prodLock([this, i_plugin, i_type, i_trackId] {
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

	return m_outputSync.prodLock([this, i_plugin, i_type, i_trackId] {
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
	m_inputSync.prodLock([this, i_srcKeyHash] {
		m_inputs.erase(i_srcKeyHash);
	});
}

void HJPlugin::onOutputDisconnected(size_t i_dstKeyHash)
{
	m_outputSync.prodLock([this, i_dstKeyHash] {
		m_outputs.erase(i_dstKeyHash);
	});
}

int HJPlugin::deliver(size_t i_srcKeyHash, HJMediaFrame::Ptr& i_mediaFrame)
{
	auto input = getInput(i_srcKeyHash);
	if (input == nullptr) {
		return HJErrNotFind;
	}

	HJMediaFrameDeque::FrameDequeInfo info;
	if (!input->mediaFrames.deliver(i_mediaFrame, &info)) {
		return HJErrFatal;
	}

	int16_t mask = 0;// = EVENT_FLAG_DEQUE_SIZE;
	if (i_mediaFrame->isAudio()) {
		mask |= EVENT_FLAG_MASK_AUDIO;
	}
	else if (i_mediaFrame->isVideo()) {
		mask |= EVENT_FLAG_MASK_VIDEO;
	}
	reportFrameDequeInfo(input->eventFlags & mask, info, false);
	onInputUpdated();
	return HJ_OK;
}

int HJPlugin::flush(size_t i_srcKeyHash)
{
	HJFLogi("{}, flush()", getName());
	auto input = getInput(i_srcKeyHash);
	if (input == nullptr) {
		return HJErrNotFind;
	}

	input->mediaFrames.flush(true);
	HJMediaFrameDeque::FrameDequeInfo info;
	reportFrameDequeInfo(input->eventFlags, info);
	onInputUpdated();

	return HJ_OK;
}

void HJPlugin::onOutputUpdated()
{
	postTask();
}

void HJPlugin::setPause(bool i_pause)
{
	bool changed = false;
	SYNC_PROD_LOCK([this, i_pause, &changed] {
		CHECK_DONE_STATUS();
		if (m_paused != i_pause) {
			m_paused = i_pause;
			changed = true;
		}
	});
	if (changed) {
		onPauseStateChanged(i_pause);
	}
}

void HJPlugin::onInputUpdated()
{
	postTask();
}

bool HJPlugin::setStatus(HJStatus i_status)
{
	if (!SYNC_PROD_LOCK([this, i_status] {
		CHECK_DONE_STATUS(false);
		m_status = i_status;
		return true;
	})) {
		return false;
	}

	report(EVENT_STATUS_UPDATED_ID, getID());
//	setInfoStatus(i_status);
	return true;
}

HJPlugin::Input::Ptr HJPlugin::getInput(size_t i_srcKeyHash)
{
	return m_inputSync.consLock([this, i_srcKeyHash] {
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
	return m_outputSync.consLock([this, i_dstKeyHash] {
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
	auto [handler, runTaskId] = m_handlerSync.consLock([this] {
		return std::make_tuple(m_handler, m_runTaskId);
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
		}, runTaskId, i_delay);
	}
}

int HJPlugin::runFlush()
{
	OutputMap outputs;
	m_outputSync.consLock([&outputs, this] {
		outputs = m_outputs;
	});

	for (auto it = outputs.begin(); it != outputs.end(); ++it) {
		auto output = it->second;
		auto plugin = output->plugin.lock();
		if (plugin != nullptr) {
			plugin->flush(output->myKeyHash);
		}
	}
	return HJ_OK;
}

HJMediaFrame::Ptr HJPlugin::receive(size_t i_srcKeyHash, int64_t* o_size)
{
	auto input = getInput(i_srcKeyHash);
	if (input == nullptr) {
		return nullptr;
	}

	HJMediaFrameDeque::FrameDequeInfo info;
	auto mediaFrame = input->mediaFrames.receive(&info);
	if (mediaFrame) {
		int16_t mask = 0;// SET_INFO_FLAG_DEQUE_SIZE;
		if (mediaFrame->isAudio()) {
			mask |= EVENT_FLAG_MASK_AUDIO;
		}
		else if (mediaFrame->isVideo()) {
			mask |= EVENT_FLAG_MASK_VIDEO;
		}
		reportFrameDequeInfo(input->eventFlags & mask, info);
		auto plugin = input->plugin.lock();
		if (plugin) {
			plugin->onOutputUpdated();
		}
		if (o_size) {
            *o_size = info.dequeSize;
		}
	}

	return mediaFrame;
}

HJMediaFrame::Ptr HJPlugin::preview(size_t i_srcKeyHash)
{
	auto input = getInput(i_srcKeyHash);
	if (input == nullptr) {
		return nullptr;
	}

	return input->mediaFrames.preview();
}

bool HJPlugin::store(size_t i_srcKeyHash, HJMediaFrame::Ptr i_mediaFrame)
{
	auto input = getInput(i_srcKeyHash);
	if (input == nullptr) {
		return false;
	}

	return input->mediaFrames.store(i_mediaFrame);
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

bool HJPlugin::logRunTask()
{
	m_enterTimestamp = HJCurrentSteadyMS();
	if (m_lastLogTimestamp < 0 || m_enterTimestamp >= m_lastLogTimestamp + LOG_INTERNAL) {
		m_lastLogTimestamp = m_enterTimestamp;
		return true;
	}

	return false;
}
/*
void HJPlugin::setInfoStatus(HJStatus i_status) {
	auto graph = m_graph.lock();
	if (graph) {
		HJGraph::Information info = HJMakeNotification(HJ_PLUGIN_SETINFO_PLUGIN_status, static_cast<int64_t>(i_status));
		info->setName(HJSyncObject::getName());
		graph->setInfo(std::move(info));
	}
}
*/
void HJPlugin::reportFrameDequeInfo(const uint16_t i_flags, const HJMediaFrameDeque::FrameDequeInfo& i_info, bool reduce)
{
	if (i_flags == 0) {
		return;
	}
/*
	auto graph = m_graph.lock();
	if (graph) {
		if (i_flags & EVENT_FLAG_VIDEO_FRAMES) {
			HJGraph::Information info = HJMakeNotification(HJ_PLUGIN_SETINFO_PLUGIN_videoFrames, i_info.videoFrames);
			info->setName(HJSyncObject::getName());
			graph->setInfo(std::move(info));
		}
		if (i_flags & EVENT_FLAG_AUDIO_DURATION) {
			int64_t audioDuration{};
			if (i_info.audioSamples > 0 && i_info.sampleRate > 0) {
				audioDuration = i_info.audioSamples * 1000 / i_info.sampleRate;
			}
			else if (i_info.audioDuration > 0) {
				audioDuration = i_info.audioDuration;
			}
			HJGraph::Information info = HJMakeNotification(HJ_PLUGIN_SETINFO_PLUGIN_audioDuration, audioDuration);
			info->setName(HJSyncObject::getName());
			graph->setInfo(std::move(info));
		}
		if (i_flags & EVENT_FLAG_VIDEO_KEY_FRAMES) {
			HJGraph::Information info = HJMakeNotification(HJ_PLUGIN_SETINFO_PLUGIN_videoKeyFrames, i_info.videoKeyFrames);
			info->setName(HJSyncObject::getName());
			graph->setInfo(std::move(info));
		}
	}
*/
	auto bus = m_graphEventBus.lock();
	if (bus != nullptr) {
		if (i_flags & EVENT_FLAG_AUDIO_DURATION) {
			int64_t audioDuration{};
			if (i_info.audioSamples > 0 && i_info.sampleRate > 0) {
				audioDuration = i_info.audioSamples * 1000 / i_info.sampleRate;
			}
			else if (i_info.audioDuration > 0) {
				audioDuration = i_info.audioDuration;
			}
			bus->report(EVENT_AUDIO_DURATION_ID, getID(), audioDuration, reduce);
		}
		if (i_flags & EVENT_FLAG_VIDEO_FRAMES) {
			bus->report(EVENT_VIDEO_FRAMES_ID, getID(), i_info.videoFrames, reduce);
		}
		if (i_flags & EVENT_FLAG_VIDEO_KEY_FRAMES) {
			bus->report(EVENT_VIDEO_KEY_FRAMES_ID, getID(), i_info.videoKeyFrames, reduce);
		}
	}
}

void HJPlugin::clearInputsAndOutputs()
{
	InputMap inputs;
	m_inputSync.prodLock([this, &inputs] {
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

	OutputMap outputs;
	m_outputSync.prodLock([this, &outputs] {
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

std::tuple<int, HJMediaFrame::Ptr> HJPlugin::receiveInputFrame(std::string& route, size_t inputKeyHash, int64_t& size)
{
	int ret{ HJ_OK };
	HJMediaFrame::Ptr inFrame{};
	do {
		auto status = SYNC_CONS_LOCK([this] {
			return m_status;
		});
		if (status == HJSTATUS_Done) {
			route += "_00";
			ret = HJErrAlreadyDone;
			break;
		}
		if (status < HJSTATUS_Inited) {
			route += "_01";
			ret = HJ_WOULD_BLOCK;
			break;
		}
		inFrame = receive(inputKeyHash, &size);
		route += "_02";
		ret = HJ_OK;
	} while (false);
	return std::make_tuple(ret, inFrame);
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
