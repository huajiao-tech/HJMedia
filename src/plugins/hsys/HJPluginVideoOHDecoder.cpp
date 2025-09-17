#include "HJPluginVideoOHDecoder.h"
#include "HJGraph.h"
#include "HJFLog.h"

NS_HJ_BEGIN

#define RUNTASKLog		HJFLogi

int HJPluginVideoOHDecoder::internalInit(HJKeyStorage::Ptr i_param)
{
    MUST_HAVE_PARAMETERS;
	MUST_GET_PARAMETER(HJOGRenderWindowBridge::Ptr, bridge);
	GET_PARAMETER(HJVideoInfo::Ptr, videoInfo);
	GET_PARAMETER(HJLooperThread::Ptr, thread);

	auto param = HJKeyStorage::dupFrom(i_param);
	if (videoInfo) {
		(*param)["streamInfo"] = std::static_pointer_cast<HJStreamInfo>(videoInfo);
	}
	(*param)["createThread"] = (thread == nullptr);
    int ret = HJPluginCodec::internalInit(param);
    if (ret < 0) {
        return ret;
    }

	m_bridge = bridge;
	return HJ_OK;
}

void HJPluginVideoOHDecoder::internalRelease()
{
	HJFLogi("{}, internalRelease() begin", getName());

	HJPluginCodec::internalRelease();

	HJFLogi("{}, internalRelease() end", getName());
}

int HJPluginVideoOHDecoder::deliver(size_t i_srcKeyHash, HJMediaFrame::Ptr& i_mediaFrame, size_t* o_size, int64_t* o_audioDuration, int64_t* o_videoKeyFrames, int64_t* o_audioSamples)
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

int HJPluginVideoOHDecoder::runTask(int64_t* o_delay)
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
	int ret = HJ_WOULD_BLOCK;
	do {
        HJMediaFrame::Ptr inFrame;
        auto err = SYNC_CONS_LOCK([&route, &inFrame, this] {
            if (m_status == HJSTATUS_Done) {
                route += "_0";
                return HJErrAlreadyDone;
            }
            if (m_status >= HJSTATUS_EOF) {
                route += "_1";
                return HJ_WOULD_BLOCK;
            }
            inFrame = m_currentFrame;
            m_currentFrame = nullptr;
            return HJ_OK;
        });
        if (err != HJ_OK) {
            if (err == HJErrAlreadyDone) {
                ret = HJErrAlreadyDone;
            }
            break;
        }

        if (!inFrame) {
            route += "_2";
            inFrame = receive(m_inputKeyHash.load(), &size);
            if (inFrame) {
                route += "_3";
                setInfoFrameSize(size);
                
                if (inFrame->isFlushFrame()) {
                    route += "_4";
                    err = SYNC_PROD_LOCK([&route, inFrame, this] {
                        if (m_status == HJSTATUS_Done) {
                            route += "_5";
                            return HJErrAlreadyDone;
                        }
                        if (m_status < HJSTATUS_Inited) {
                            route += "_6";
                            return HJ_WOULD_BLOCK;
                        }
                
                        HJStreamInfo::Ptr streamInfo = inFrame->getVideoInfo()->dup();
                        if (!streamInfo) {
                            route += "_7";
                            auto mediaInfo = inFrame->getMediaInfo();
                            if (!mediaInfo) {
                                route += "_8";
                                HJFLoge("{}, (!mediaInfo)", getName());
                                setStatus(HJSTATUS_Exception, false);
                                return HJErrFatal;
                            }
                            streamInfo = mediaInfo->getVideoInfo()->dup();
                            if (!streamInfo) {
                                route += "_9";
                                HJFLoge("{}, (!streamInfo)", getName());
                                setStatus(HJSTATUS_Exception, false);
                                return HJErrFatal;
                            }
                        }
                        (*streamInfo)["HJOGRenderWindowBridge::Ptr"] = m_bridge;
                        auto err = initCodec(streamInfo);
                        if (err < 0) {
                            route += "_10";
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
                                route += "_11";
                                m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_ERROR_CODEC_INIT)));
                            }
                        }
                        break;
                    }
                }
            }
        }

        if (inFrame) {
            err = SYNC_CONS_LOCK([&route, &ret, inFrame, this] {
                if (m_status == HJSTATUS_Done) {
                    route += "_12";
                    return HJErrAlreadyDone;
                }
                if (m_status < HJSTATUS_Ready) {
                    route += "_13";
                    return HJ_WOULD_BLOCK;
                }
                if (m_status >= HJSTATUS_EOF) {
                    route += "_14";
                    return HJ_WOULD_BLOCK;
                }

                passThroughSetInput(inFrame);
                m_streamIndex = inFrame->m_streamIndex;
                auto err = m_codec->run(inFrame);
                if (err < 0) {
                    route += "_15";
                    HJFLoge("{}, m_codec->run() error({})", getName(), err);
                    return HJErrFatal;
                }
                else if (err == HJ_WOULD_BLOCK) {
                    route += "_16";
                    m_currentFrame = inFrame;
                }
                else {
                    route += "_17";
                    if (m_firstFrame.load() > 0) {
                        route += "_18";
                        m_currentFrame = inFrame;
                        m_firstFrame.fetch_sub(1);
                    }
                    ret = HJ_OK;
                }
                    
                return HJ_OK;
            });
            if (err != HJ_OK) {
                if (err == HJErrAlreadyDone) {
                    ret = HJErrAlreadyDone;
                }
                else if (err == HJErrFatal) {
                    IF_FALSE_BREAK(setStatus(HJSTATUS_Inited), HJErrAlreadyDone);
                    if (m_pluginListener) {
                        route += "_19";
                        m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_ERROR_CODEC_RUN)));
                    }
                }
                break;
            }
        }

        err = canDeliverToOutputs();
        if (err != HJ_OK) {
            route += "_20";
            break;
        }

        for (;;) {
            route += "_21";
            HJMediaFrame::Ptr outFrame{};
            err = SYNC_CONS_LOCK([&route, &outFrame, inFrame, this] {
                if (m_status == HJSTATUS_Done) {
                    route += "_21";
                    return HJErrAlreadyDone;
                }
                if (m_status < HJSTATUS_Ready) {
                    route += "_22";
                    return HJ_WOULD_BLOCK;
                }
                if (m_status >= HJSTATUS_EOF) {
                    route += "_23";
                    return HJ_WOULD_BLOCK;
                }
                
                auto err = m_codec->getFrame(outFrame);
                if (err < 0) {
                    route += "_24";
                    HJFLoge("{}, m_codec->getFrame() error({})", getName(), err);
                    return HJErrFatal;
                }

                return HJ_OK;
            });
            if (err != HJ_OK) {
                if (err == HJErrAlreadyDone) {
                    ret = HJErrAlreadyDone;
                }
                else if (err == HJErrFatal) {
                    route += "_26";
                    IF_FALSE_BREAK(setStatus(HJSTATUS_Inited), HJErrAlreadyDone);
                    if (m_pluginListener) {
                        route += "_27";
                        m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_ERROR_CODEC_GETFRAME)));
                    }
                }
                break;
            }
        
            if (outFrame == nullptr) {
                route += "_28";
                break;
            }
        
            route += "_29";
            passThroughSetOutput(outFrame);
            outFrame->m_streamIndex = m_streamIndex;
            deliverToOutputs(outFrame);
        
            if (outFrame->isEofFrame()) {
                route += "_30";
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

HJBaseCodec::Ptr HJPluginVideoOHDecoder::createCodec()
{
	HJBaseCodec::Ptr codec = HJBaseCodec::createVDecoder(HJDEVICE_TYPE_OHCODEC);
    if (codec)
    {
        codec->setName(getName());
    }
    return codec;
}

int HJPluginVideoOHDecoder::initCodec(const HJStreamInfo::Ptr& i_streamInfo)
{
    Wtr wDecoder = SHARED_FROM_THIS;
   	(*i_streamInfo)["onNeedInputBuffer"] = (HJRunnable)[wDecoder] {
        auto decoder = wDecoder.lock();
        if (decoder) {
		    decoder->postTask();
        }
	};
    (*i_streamInfo)["onNewOutputBuffer"] = (HJRunnable)[wDecoder] {
        auto decoder = wDecoder.lock();
        if (decoder) {
            decoder->m_firstFrame.store(0);
		    decoder->postTask();
        }
	};
    
    return HJPluginCodec::initCodec(i_streamInfo);
}

void HJPluginVideoOHDecoder::setInfoFrameSize(size_t i_size)
{
	auto graph = m_graph.lock();
	if (graph) {
		HJGraph::Information info = HJMakeNotification(HJ_PLUGIN_SETINFO_VIDEODECODER_frameSize, i_size);
		info->setName(HJSyncObject::getName());
		graph->setInfo(std::move(info));
	}
}

int HJPluginVideoOHDecoder::canDeliverToOutputs()
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
