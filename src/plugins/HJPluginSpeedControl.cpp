#include "HJPluginSpeedControl.h"
#include "HJFLog.h"
#include "HJGraphLivePlayer.h"

NS_HJ_BEGIN

#define RUNTASKLog		HJFLogi

HJPluginSpeedControl::HJPluginSpeedControl(const std::string& name, size_t i_identify, HJKeyStorage::Ptr i_graphInfo)
	: HJPlugin(name, i_identify, i_graphInfo)
{
	if (i_graphInfo) {
		auto graph = i_graphInfo->getValue<HJGraph::Ptr>("graph");
		if (graph) {
			m_cacheObserver = graph->getCacheObserver();
			auto& settings = graph->getSettings();
			if (settings.getAutoDelayEnable()) {
				m_autoCacheController = HJCreates<HJAutoCacheController>(
					settings.getCacheTargetDuration(),
					settings.getCacheLimitedRange(),
					1.f,
					settings.getStutterRatioMax(),
					settings.getStutterRatioDelta(),
					settings.getStutterFactorMax(),
					settings.getObserveTime()
				);
			}
		}
	}
}

int HJPluginSpeedControl::internalInit(HJKeyStorage::Ptr i_param)
{
	MUST_HAVE_PARAMETERS;
	MUST_GET_PARAMETER(HJAudioInfo::Ptr, audioInfo);
	GET_PARAMETER(HJLooperThread::Ptr, thread);
	
	auto param = HJKeyStorage::dupFrom(i_param);
	(*param)["createThread"] = (thread == nullptr);
	int ret = HJPlugin::internalInit(param);
	if (ret < 0) {
		return ret;
	}
#if 0
	if (!m_autoCacheController)
	{
		auto graph = m_graph.lock();
		if (!graph) {
			HJFLoge("{}, (!graph)", getName());
			return HJErrFatal;
		}
		m_cacheObserver = graph->getCacheObserver();
		if (!m_cacheObserver) {
			HJFLoge("{}, error, cache observer is null", getName());
			return HJErrInvalidParams;
		}

		auto& settings = graph->getSettings();
		if (settings.getAutoDelayEnable())
		{
			m_autoCacheController = HJCreates<HJAutoCacheController>(
				settings.getCacheTargetDuration(),
				settings.getCacheLimitedRange(),
				1.f,
				settings.getStutterRatioMax(),
				settings.getStutterRatioDelta(),
				settings.getStutterFactorMax(),
				settings.getObserveTime());
			m_autoCacheController->setName(getName());
			Wtr wControl = SHARED_FROM_THIS;
			m_autoCacheController->setOnOptionsCallback([wControl](std::shared_ptr<HJParams> params) {
				auto control = wControl.lock();
				if (control) {
					control->onOptionsCallback(params);
				}
			});
		}
	}
#else
	if (m_cacheObserver.lock() == nullptr) {
		HJFLoge("{}, error, cache observer is null", getName());
		return HJErrFatal;
	}
	if (m_autoCacheController != nullptr) {
		m_autoCacheController->setName(getName());
		Wtr wControl = SHARED_FROM_THIS;
		m_autoCacheController->setOnOptionsCallback([wControl](std::shared_ptr<HJParams> params) {
			auto control = wControl.lock();
			if (control) {
				control->onOptionsCallback(params);
			}
		});
	}
#endif
	m_audioProcessor = HJAudioProcessor::create(HJAudioProcessorSonic);
	if (m_audioProcessor == nullptr) {
		return HJErrFatal;
	}
	ret = m_audioProcessor->init(audioInfo);
	if (HJ_OK != ret) {
		HJFLoge("error, audio processor create failed:{}", ret);
	}
	return ret;
}

void HJPluginSpeedControl::onOptionsCallback(std::shared_ptr<HJParams> params)
{
	//if (m_pluginListener) {
	//	auto ntf = HJMakeNotification(HJ_PLUGIN_NOTIFY_AUTODELAY_PARAMS);
	//	(*ntf)["auto_params"] = params;
	//	m_pluginListener(std::move(ntf));
	//}
	report(EVENT_GRAPH_AUTODELAY_PARAMS_ID, params);
}

void HJPluginSpeedControl::internalRelease()
{
	HJFLogi("{}, internalRelease() begin", getName());

	m_audioProcessor = nullptr;

	HJPlugin::internalRelease();

	HJFLogi("{}, internalRelease() end", getName());
}
/*
void HJPluginSpeedControl::onOutputUpdated()
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
void HJPluginSpeedControl::onInputAdded(size_t i_srcKeyHash, HJMediaType i_type)
{
	m_inputKeyHash.store(i_srcKeyHash);
}
/*
void HJPluginSpeedControl::onOutputAdded(size_t i_dstKeyHash, HJMediaType i_type)
{
	m_outputKeyHash.store(i_dstKeyHash);
}
*/
int HJPluginSpeedControl::runTask(int64_t* o_delay)
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
		auto inFrame = receive(m_inputKeyHash.load(), &size);
		if (inFrame == nullptr) {
			route += "_1";
			ret = HJ_WOULD_BLOCK;
			break;
		}

		HJMediaFrame::Ptr outFrame{};
		auto err = SYNC_CONS_LOCK([&route, inFrame, &outFrame, this] {
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

			float speed = 1.0f;
			if (m_autoCacheController && m_cacheObserver) {
				auto stats = m_cacheObserver->getStats();
				auto watchTime = m_cacheObserver->getWatchTime();
				auto stutterRatio = m_cacheObserver->getWinStutterRatio();
				speed = m_autoCacheController->update(stats.duration, stutterRatio, stats.maximum, stats.minimum, stats.average, watchTime);
				//HJFLogi("stats.duration:{}, stutterRatio:{:3f}, stats.maximum:{}, stats.minimum:{}, stats.average:{}, watchTime:{}, speed:{}", stats.duration, stutterRatio, stats.maximum, stats.minimum, stats.average, watchTime, speed);
			}

			if (speed != m_audioProcessor->getSpeed()) {
				m_audioProcessor->reset();
				m_audioProcessor->setSpeed(speed);
				HJFLogi("new speed:{}", speed);
			}
			if (1.0f != speed) {
				m_streamIndex = inFrame->m_streamIndex;
				m_audioProcessor->addFrame(inFrame);
				outFrame = m_audioProcessor->getFrame();
				if (outFrame) {
					outFrame->m_streamIndex = m_streamIndex;
				}
			} else {
                outFrame = inFrame;
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
			route += "_5";
			break;
		}

		route += "_6";
		deliverToOutputs(outFrame);
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
		if (inFrame->isEofFrame()) {
			route += "_4";
			outFrame = inFrame;
		}
		else {
			route += "_5";
			std::tie(ret, outFrame) = getOutputFrame(route, inFrame);
			if (ret != HJ_OK) {
				route += "_6";
				break;
			}
			if (outFrame == nullptr) {
				route += "_7";
				break;
			}
		}

		deliverToOutputs(outFrame);
#endif
		route += "_8";
	} while (false);
//    addOutIdx();
	if (log) {
		RUNTASKLog("{}, leave, route({}), size({}), task duration({}), ret({})", getName(), route, size, (HJCurrentSteadyMS() - m_enterTimestamp), ret);
	}
	return ret;
}

std::tuple<int, HJMediaFrame::Ptr> HJPluginSpeedControl::getOutputFrame(std::string& route, HJMediaFrame::Ptr& inFrame)
{
	HJMediaFrame::Ptr outFrame{};
	auto ret = SYNC_PROD_LOCK([this, &route, inFrame, &outFrame] {
		if (m_status == HJSTATUS_Done) {
			route += "_2";
			return HJErrAlreadyDone;
		}
		if (m_status >= HJSTATUS_Stoped) {
			route += "_4";
			return HJErrInvalidData;
		}

		float speed = 1.0f;
		auto cacheObserver = m_cacheObserver.lock();
		if (m_autoCacheController && cacheObserver) {
			auto stats = cacheObserver->getStats();
			auto watchTime = cacheObserver->getWatchTime();
			auto stutterRatio = cacheObserver->getWinStutterRatio();
			speed = m_autoCacheController->update(stats.duration, stutterRatio, stats.maximum, stats.minimum, stats.average, watchTime);
			//HJFLogi("stats.duration:{}, stutterRatio:{:3f}, stats.maximum:{}, stats.minimum:{}, stats.average:{}, watchTime:{}, speed:{}", stats.duration, stutterRatio, stats.maximum, stats.minimum, stats.average, watchTime, speed);
		}

		if (speed != m_audioProcessor->getSpeed()) {
			m_audioProcessor->reset();
			m_audioProcessor->setSpeed(speed);
			HJFLogi("new speed:{}", speed);
		}
		if (1.0f != speed) {
			m_streamIndex = inFrame->m_streamIndex;
			m_audioProcessor->addFrame(inFrame);
			outFrame = m_audioProcessor->getFrame();
			if (outFrame) {
				outFrame->m_streamIndex = m_streamIndex;
			}
		}
		else {
			outFrame = inFrame;
		}

		return HJ_OK;
	});

    return std::make_tuple(ret, outFrame);
}

NS_HJ_END
