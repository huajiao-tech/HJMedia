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

void HJPluginAudioFFDecoder::internalRelease()
{
	HJFLogi("{}, internalRelease() begin", getName());

	HJPluginCodec::internalRelease();

	HJFLogi("{}, internalRelease() end", getName());
}

int HJPluginAudioFFDecoder::deliver(size_t i_srcKeyHash, HJMediaFrame::Ptr& i_mediaFrame, size_t* o_size, int64_t* o_audioDuration, int64_t* o_videoKeyFrames, int64_t* o_audioSamples)
{
	int64_t audioDuration = 0;
	if (!o_audioDuration) {
		o_audioDuration = &audioDuration;
	}
	auto ret = HJPluginCodec::deliver(i_srcKeyHash, i_mediaFrame, o_size, o_audioDuration, o_videoKeyFrames, o_audioSamples);
	if (ret == HJ_OK) {
		setInfoAudioDuration(*o_audioDuration);
	}

	return ret;
}

void HJPluginAudioFFDecoder::onOutputUpdated()
{
	auto input = getInput(m_inputKeyHash.load());
	if (input) {
		auto plugin = input->plugin.lock();
		if (plugin) {
			plugin->onOutputUpdated();
		}
	}
}

int HJPluginAudioFFDecoder::runTask(int64_t* o_delay)
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
	int64_t audioDuration = 0;
	int ret;
	do {
		ret = SYNC_CONS_LOCK([&route, this] {
			if (m_status == HJSTATUS_Done) {
				route += "_0";
				return HJErrAlreadyDone;
			}
			if (m_status < HJSTATUS_Inited) {
				route += "_1";
				return HJ_WOULD_BLOCK;
			}
			if (m_status >= HJSTATUS_EOF) {
				route += "_2";
				return HJ_WOULD_BLOCK;
			}
			return HJ_OK;
		});
		if (ret != HJ_OK) {
			break;
		}

		auto inFrame = receive(m_inputKeyHash.load(), &size, &audioDuration);
		if (inFrame == nullptr) {
			route += "_3";
			ret = HJ_WOULD_BLOCK;
			break;
		}
		else {
			route += "_4";
			setInfoAudioDuration(audioDuration);
		}

		int err;
		if (inFrame->isFlushFrame()) {
			route += "_5";
			err = SYNC_PROD_LOCK([&route, inFrame, this] {
				if (m_status == HJSTATUS_Done) {
					route += "_6";
					return HJErrAlreadyDone;
				}
				if (m_status < HJSTATUS_Inited) {
					route += "_7";
					return HJ_WOULD_BLOCK;
				}

				HJStreamInfo::Ptr streamInfo = inFrame->getAudioInfo();
				if (!streamInfo) {
					route += "_8";
					auto mediaInfo = inFrame->getMediaInfo();
					if (!mediaInfo) {
						route += "_9";
						HJFLoge("{}, (!mediaInfo)", getName());
						setStatus(HJSTATUS_Exception, false);
						return HJErrFatal;
					}
					streamInfo = mediaInfo->getAudioInfo();
					if (!streamInfo) {
						route += "_10";
						HJFLoge("{}, (!streamInfo)", getName());
						setStatus(HJSTATUS_Exception, false);
						return HJErrFatal;
					}
				}
				auto err = initCodec(streamInfo);
				if (err < 0) {
					route += "_11";
					setStatus(HJSTATUS_Exception, false);
					return HJErrFatal;
				}

				setStatus(HJSTATUS_Ready, false);
				return HJ_OK;
			});
			if (err != HJ_OK) {
				if (err == HJErrAlreadyDone) {
					ret = HJErrAlreadyDone;
				}
				else if (err == HJErrFatal) {
					if (m_pluginListener) {
						route += "_12";
						m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_ERROR_CODEC_INIT)));
					}
				}
				break;
			}
		}

		err = SYNC_CONS_LOCK([&route, inFrame, this] {
			if (m_status == HJSTATUS_Done) {
				route += "_13";
				return HJErrAlreadyDone;
			}
			if (m_status < HJSTATUS_Ready) {
				route += "_14";
				return HJ_WOULD_BLOCK;
			}
			if (m_status >= HJSTATUS_EOF) {
				route += "_15";
				return HJ_WOULD_BLOCK;
			}

			m_streamIndex = inFrame->m_streamIndex;
			auto err = m_codec->run(inFrame);
			if (err < 0) {
				route += "_16";
				HJFLoge("{}, m_codec->run() error({})", getName(), err);
				return HJErrFatal;
			}

			if (inFrame->isEofFrame()) {
				route += "_17";
				HJFLogi("{}, (inFrame->isEofFrame())", getName());
			}

			return HJ_OK;
		});
		if (err != HJ_OK) {
			if (err == HJErrAlreadyDone) {
				ret = HJErrAlreadyDone;
			}
			else if (err == HJErrFatal) {
				IF_FALSE_BREAK(setStatus(HJSTATUS_Exception), HJErrAlreadyDone);
				if (m_pluginListener) {
					route += "_18";
					m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_ERROR_CODEC_RUN)));
				}
			}
			break;
		}

		for (;;) {
			route += "_19";
			HJMediaFrame::Ptr outFrame{};
			err = SYNC_CONS_LOCK([&route, &outFrame, this] {
				if (m_status == HJSTATUS_Done) {
					route += "_20";
					return HJErrAlreadyDone;
				}
				auto err = m_codec->getFrame(outFrame);
				if (err < 0) {
					route += "_21";
					HJFLoge("{}, m_codec->getFrame() error({})", getName(), err);
					return HJErrFatal;
				}
				if (outFrame == nullptr) {
					route += "_22";
					return HJ_WOULD_BLOCK;
				}
				return HJ_OK;
			});
			if (err != HJ_OK) {
				if (err == HJErrAlreadyDone) {
					ret = HJErrAlreadyDone;
				}
				else if (err == HJErrFatal) {
					IF_FALSE_BREAK(setStatus(HJSTATUS_Exception), HJErrAlreadyDone);
					if (m_pluginListener) {
						route += "_23";
						m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_ERROR_CODEC_GETFRAME)));
					}
				}
				break;
			}

			route += "_24";
			outFrame->m_streamIndex = m_streamIndex;
			deliverToOutputs(outFrame);

			if (outFrame->isEofFrame()) {
				route += "_25";
				HJFLogi("{}, (outFrame->isEofFrame())", getName());
				IF_FALSE_BREAK(setStatus(HJSTATUS_EOF), HJErrAlreadyDone);
				break;
			}
		}
	} while (false);
    addOutIdx();
	if (log) {
		RUNTASKLog("{}, leave, route({}), size({}), task duration({}), ret({})", getName(), route, size, (HJCurrentSteadyMS() - enter), ret);
	}
	return ret;
}

HJBaseCodec::Ptr HJPluginAudioFFDecoder::createCodec()
{
	return HJBaseCodec::createADecoder();
}

void HJPluginAudioFFDecoder::setInfoAudioDuration(int64_t i_audioDuration)
{
	auto graph = m_graph.lock();
	if (graph) {
		HJGraph::Information info = HJMakeNotification(HJ_PLUGIN_SETINFO_AUDIODECODER_audioDuration, i_audioDuration);
		info->setName(HJSyncObject::getName());
		graph->setInfo(std::move(info));
	}
}

NS_HJ_END
