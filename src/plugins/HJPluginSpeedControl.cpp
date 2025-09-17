#include "HJPluginSpeedControl.h"
#include "HJFLog.h"
#include "HJGraphLivePlayer.h"

NS_HJ_BEGIN

#define RUNTASKLog		HJFLogi

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

	do {
		if (!m_autoCacheController)
		{
			auto graph = m_graph.lock();
			if (!graph) {
				HJFLoge("{}, (!graph)", getName());
				ret = HJErrFatal;
				break;
			}
			m_cacheObserver = graph->getCacheObserver();
			if (!m_cacheObserver) {
				ret = HJErrInvalidParams;
				HJFLoge("{}, error, cache observer is null", getName());
				break;
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
				m_autoCacheController->setOnOptionsCallback([&](std::shared_ptr<HJParams> params) {
					if (m_pluginListener) {
						auto ntf = HJMakeNotification(HJ_PLUGIN_NOTIFY_AUTODELAY_PARAMS);
						(*ntf)["auto_params"] = params;
						m_pluginListener(std::move(ntf));
					}
				});
			}
		}

		m_audioProcessor = HJAudioProcessor::create(HJAudioProcessorSonic);
		ret = m_audioProcessor->init(audioInfo);
		if (HJ_OK != ret) {
			HJFLoge("error, audio processor create failed:{}", ret);
			break;
		}
		return HJ_OK;
	} while (false);

	HJPluginSpeedControl::internalRelease();
	return ret;
}

void HJPluginSpeedControl::internalRelease()
{
	HJFLogi("{}, internalRelease() begin", getName());

	m_audioProcessor = nullptr;

	HJPlugin::internalRelease();

	HJFLogi("{}, internalRelease() end", getName());
}

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

void HJPluginSpeedControl::onInputAdded(size_t i_srcKeyHash, HJMediaType i_type)
{
	m_inputKeyHash.store(i_srcKeyHash);
}

void HJPluginSpeedControl::onOutputAdded(size_t i_dstKeyHash, HJMediaType i_type)
{
	m_outputKeyHash.store(i_dstKeyHash);
}

int HJPluginSpeedControl::runTask(int64_t* o_delay)
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
	} while (false);
    addOutIdx();
	if (log) {
		RUNTASKLog("{}, leave, route({}), size({}), task duration({}), ret({})", getName(), route, size, (HJCurrentSteadyMS() - enter), ret);
	}
	return ret;
}

NS_HJ_END
