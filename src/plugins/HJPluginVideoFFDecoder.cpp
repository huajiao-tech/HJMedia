#include "HJPluginVideoFFDecoder.h"
#include "HJGraph.h"
#include "HJFLog.h"

NS_HJ_BEGIN

#define RUNTASKLog		HJFLogi

int HJPluginVideoFFDecoder::internalInit(HJKeyStorage::Ptr i_param)
{
	MUST_HAVE_PARAMETERS;
	GET_PARAMETER(HJVideoInfo::Ptr, videoInfo);
	GET_PARAMETER(HJLooperThread::Ptr, thread);
	GET_PARAMETER(bool, onlyFirstFrame);

	auto param = HJKeyStorage::dupFrom(i_param);
	if (videoInfo) {
		(*param)["streamInfo"] = std::static_pointer_cast<HJStreamInfo>(videoInfo);
	}
	(*param)["createThread"] = (thread == nullptr);
	int ret = HJPluginCodec::internalInit(param);
	if (ret < 0) {
		return ret;
	}

	m_onlyFirstFrame = onlyFirstFrame;
	return HJ_OK;
}

void HJPluginVideoFFDecoder::internalRelease()
{
	HJFLogi("{}, internalRelease() begin", getName());

	HJPluginCodec::internalRelease();

	HJFLogi("{}, internalRelease() end", getName());
}

int HJPluginVideoFFDecoder::deliver(size_t i_srcKeyHash, HJMediaFrame::Ptr& i_mediaFrame, size_t* o_size, int64_t* o_audioDuration, int64_t* o_videoKeyFrames, int64_t* o_audioSamples)
{
	size_t size = 0;
	if (!o_size) {
		o_size = &size;
	}
	auto ret = HJPluginCodec::deliver(i_srcKeyHash, i_mediaFrame, o_size, o_audioDuration, o_videoKeyFrames, o_audioSamples);
	if (ret == HJ_OK) {
		setInfoFrameSize(*o_size);
	}

	return ret;
}

int HJPluginVideoFFDecoder::runTask(int64_t* o_delay)
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

		ret = canDeliverToOutputs();
		if (ret != HJ_OK) {
			route += "_3";
			break;
		}

		auto inFrame = receive(m_inputKeyHash.load(), &size);
		if (inFrame == nullptr) {
			route += "_4";
			ret = HJ_WOULD_BLOCK;
			break;
		}

		setInfoFrameSize(size);

		int err;
		if (inFrame->isFlushFrame()) {
			route += "_6";
			err = SYNC_PROD_LOCK([&route, inFrame, this] {
				if (m_status == HJSTATUS_Done) {
					route += "_7";
					return HJErrAlreadyDone;
				}
				if (m_status < HJSTATUS_Inited) {
					route += "_8";
					return HJ_WOULD_BLOCK;
				}

				HJStreamInfo::Ptr streamInfo = inFrame->getVideoInfo()->dup();
				if (!streamInfo) {
					route += "_9";
					auto mediaInfo = inFrame->getMediaInfo();
					if (!mediaInfo) {
						route += "_10";
						HJFLoge("{}, (!mediaInfo)", getName());
						setStatus(HJSTATUS_Exception, false);
						return HJErrFatal;
					}
					streamInfo = mediaInfo->getVideoInfo()->dup();
					if (!streamInfo) {
						route += "_11";
						HJFLoge("{}, (!streamInfo)", getName());
						setStatus(HJSTATUS_Exception, false);
						return HJErrFatal;
					}
				}
				(*streamInfo)["threads"] = 2;
				auto err = initCodec(streamInfo);
				if (err < 0) {
					route += "_12";
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
						route += "_13";
						m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_ERROR_CODEC_INIT)));
					}
				}
				break;
			}
		}

		err = SYNC_CONS_LOCK([&route, inFrame, this] {
			if (m_status == HJSTATUS_Done) {
				route += "_14";
				return HJErrAlreadyDone;
			}
			if (m_status < HJSTATUS_Ready) {
				route += "_15";
				return HJ_WOULD_BLOCK;
			}
			if (m_status >= HJSTATUS_EOF) {
				route += "_16";
				return HJ_WOULD_BLOCK;
			}

			passThroughSetInput(inFrame);
			m_streamIndex = inFrame->m_streamIndex;
			auto err = m_codec->run(inFrame);
			if (err < 0) {
				route += "_17";
				HJFLoge("{}, 0 m_codec->run() error({})", getName(), err);
				return HJErrFatal;
			}

			if (inFrame->isEofFrame()) {
				route += "_18";
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
					route += "_19";
					m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_ERROR_CODEC_RUN)));
				}
			}
			break;
		}

		for (;;) {
			route += "_20";
			HJMediaFrame::Ptr outFrame{};
			err = SYNC_CONS_LOCK([&route, &outFrame, inFrame, this] {
				if (m_status == HJSTATUS_Done) {
					route += "_21";
					return HJErrAlreadyDone;
				}
				auto err = m_codec->getFrame(outFrame);
				if (err < 0) {
					route += "_22";
					HJFLoge("{}, 0, m_codec->getFrame() error({})", getName(), err);
					return HJErrFatal;
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

			if (m_firstFrame) {
				route += "_24";
				for (int i = 0; outFrame == nullptr && err == HJ_OK && i < 5; i++) {
					route += "_25";
					err = SYNC_CONS_LOCK([&route, &outFrame, inFrame, this] {
						if (m_status == HJSTATUS_Done) {
							route += "_26";
							return HJErrAlreadyDone;
						}
						auto err = m_codec->run(inFrame);
						if (err < 0) {
							route += "_27";
							HJFLoge("{}, 1 m_codec->run() error({})", getName(), err);
							return HJErrFatal;
						}
						err = m_codec->getFrame(outFrame);
						if (err < 0) {
							route += "_28";
							HJFLoge("{}, 1, m_codec->getFrame() error({})", getName(), err);
							return HJErrExcep;
						}
						return HJ_OK;
					});
				}
				if (err != HJ_OK) {
					if (err == HJErrAlreadyDone) {
						ret = HJErrAlreadyDone;
					}
					else if (err == HJErrFatal) {
						IF_FALSE_BREAK(setStatus(HJSTATUS_Exception), HJErrAlreadyDone);
						if (m_pluginListener) {
							route += "_29";
							m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_ERROR_CODEC_RUN)));
						}
					}
					else if (err == HJErrExcep) {
						IF_FALSE_BREAK(setStatus(HJSTATUS_Exception), HJErrAlreadyDone);
						if (m_pluginListener) {
							route += "_30";
							m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_ERROR_CODEC_GETFRAME)));
						}
					}
					break;
				}
				if (outFrame == nullptr) {
					route += "_31";
					HJFLoge("{}, firstFrame run 5 times, (outFrame == nullptr)", getName());
				}

				m_firstFrame = false;
			}

			if (outFrame == nullptr) {
				route += "_32";
				break;
			}

			route += "_33";
			passThroughSetOutput(outFrame);
			outFrame->m_streamIndex = m_streamIndex;
			deliverToOutputs(outFrame);

			if (outFrame->isEofFrame()) {
				route += "_34";
				HJFLogi("{}, (outFrame->isEofFrame())", getName());
				IF_FALSE_BREAK(setStatus(HJSTATUS_EOF), HJErrAlreadyDone);
				break;
			}
		}

		if (m_onlyFirstFrame) {
			route += "_35";
			IF_FALSE_BREAK(setStatus(HJSTATUS_Stoped), HJErrAlreadyDone);

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
	} while (false);
    addOutIdx();
	if (log) {
		RUNTASKLog("{}, leave, route({}), size({}), task duration({}), ret({})", getName(), route, size, (HJCurrentSteadyMS() - enter), ret);
	}
	return ret;
}

HJBaseCodec::Ptr HJPluginVideoFFDecoder::createCodec()
{
	return HJBaseCodec::createVDecoder();
}

void HJPluginVideoFFDecoder::setInfoFrameSize(size_t i_size)
{
	auto graph = m_graph.lock();
	if (graph) {
		HJGraph::Information info = HJMakeNotification(HJ_PLUGIN_SETINFO_VIDEODECODER_frameSize, i_size);
		info->setName(HJSyncObject::getName());
		graph->setInfo(std::move(info));
	}
}

int HJPluginVideoFFDecoder::canDeliverToOutputs()
{
	auto graph = m_graph.lock();
	if (!graph) {
		HJFLoge("{}, (!graph)", getName());
		return HJErrExcep;
	}

	auto param = HJMakeNotification(HJ_PLUGIN_GETINFO_VIDEODECODER_canDeliverToOutputs);
	return graph->getInfo(param);
}

NS_HJ_END
