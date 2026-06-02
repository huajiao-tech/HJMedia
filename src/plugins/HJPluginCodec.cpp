#include "HJPluginCodec.h"
#include "HJGraph.h"
#include "HJFLog.h"

#include <cassert>
#include <optional>

NS_HJ_BEGIN

int HJPluginCodec::internalInit(HJKeyStorage::Ptr i_param)
{
	auto ret = HJPlugin::internalInit(i_param);
	if (ret != HJ_OK) {
		return ret;
	}

	if (i_param) {
		GET_PARAMETER(HJStreamInfo::Ptr, streamInfo);

		if (streamInfo) {
			// TODO: decide whether to set ROI callback on mid-stream codec restart.
			if (i_param->haveValue("ROIEncodeCb"))
			{
				(*streamInfo)["ROIEncodeCb"] = i_param->getValueObj<HJRoiEncodeCb>("ROIEncodeCb");
			}
			ret = initCodec(streamInfo);
			if (ret < 0) {
				return ret;
			}
		}
	}

	return HJ_OK;
}

void HJPluginCodec::internalRelease()
{
	HJFLogi("{}, internalRelease() begin", getName());

	releaseCodec();

	HJPlugin::internalRelease();

	HJFLogi("{}, internalRelease() end", getName());
}

void HJPluginCodec::afterInit()
{
	if (m_codec) {
		m_status = HJSTATUS_Ready;
	}

	HJPlugin::afterInit();
}

void HJPluginCodec::onInputAdded(size_t i_srcKeyHash, HJMediaType i_type)
{
	HJPlugin::onInputAdded(i_srcKeyHash, i_type);

	m_inputKeyHash.store(i_srcKeyHash);
}

int HJPluginCodec::runFlush()
{
	auto ret = SYNC_PROD_LOCK([this] {
		CHECK_DONE_STATUS(HJErrAlreadyDone);
		if (m_status < HJSTATUS_Ready) {
			return HJ_WOULD_BLOCK;
		}
		if (m_status > HJSTATUS_EOF) {
			return HJ_WOULD_BLOCK;
		}

		m_codec->flush();
		passThroughReset();

		return HJ_OK;
	});
	if (ret < 0) {
		return ret;
	}

	return HJPlugin::runFlush();
}

int HJPluginCodec::initCodec(const HJStreamInfo::Ptr& i_streamInfo)
{
	if (!i_streamInfo) {
		return HJErrInvalidParams;
	}

	releaseCodec();
	m_codec = createCodec();
	if (m_codec == nullptr) {
		return HJErrFatal;
	}

	m_codec->setName(getName());
	auto ret = m_codec->init(i_streamInfo);
	if (ret < 0) {
		HJFLoge("{}, m_codec->init(), error({})", getName(), ret);
		releaseCodec();
		return ret;
	}

    //_lfs_
    passThroughReset();
	return HJ_OK;
}

void  HJPluginCodec::releaseCodec()
{
	if (m_codec) {
		m_codec->done();
		m_codec = nullptr;
	}
}

int HJPluginCodec::processFlushFrame(std::string& route, HJMediaFrame::Ptr& inFrame)
{
	HJStreamInfo::Ptr streamInfo = getType() == HJMEDIA_TYPE_VIDEO ?
		std::static_pointer_cast<HJStreamInfo>(inFrame->getVideoInfo()) : std::static_pointer_cast<HJStreamInfo>(inFrame->getAudioInfo());
	if (streamInfo == nullptr) {
		route += "_10";
		auto mediaInfo = inFrame->getMediaInfo();
		if (mediaInfo != nullptr) {
			route += "_11";
			streamInfo = getType() == HJMEDIA_TYPE_VIDEO ?
				std::static_pointer_cast<HJStreamInfo>(mediaInfo->getVideoInfo()) : std::static_pointer_cast<HJStreamInfo>(mediaInfo->getAudioInfo());
		}
		if (streamInfo == nullptr) {
			route += "_12";
			//if (m_pluginListener) {
			//	route += "_13";
			//	m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_ERROR_CODEC_INVALID_DATA)));
			//}
			report(EVENT_PLUGIN_NOTIFY_ID, HJ_PLUGIN_NOTIFY_ERROR_CODEC_INVALID_DATA, getID());
			return HJErrInvalidData;
		}
	}

	auto ret = SYNC_PROD_LOCK([this, &route, streamInfo] {
		if (m_status == HJSTATUS_Done) {
			route += "_14";
			return HJErrAlreadyDone;
		}

		if (initCodec(streamInfo) < 0) {
			route += "_15";
			return HJErrFatal;
		}

		route += "_16";
		return HJ_OK;
	});
	if (ret == HJ_OK) {
		IF_FALSE_RETURN(setStatus(HJSTATUS_Ready), HJErrAlreadyDone);
	}
	else if (ret == HJErrFatal) {
		IF_FALSE_RETURN(setStatus(HJSTATUS_Exception), HJErrAlreadyDone);
		//if (m_pluginListener) {
		//	route += "_17";
		//	m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_ERROR_CODEC_INIT)));
		//}
		report(EVENT_PLUGIN_NOTIFY_ID, HJ_PLUGIN_NOTIFY_ERROR_CODEC_INIT, getID());
	}

	return ret;
}

int HJPluginCodec::processMediaFrame(std::string& route, HJMediaFrame::Ptr& inFrame)
{
	auto ret = SYNC_PROD_LOCK([this, &route, inFrame] {
		if (m_status == HJSTATUS_Done) {
			route += "_20";
			return HJErrAlreadyDone;
		}
		if (m_status < HJSTATUS_Ready) {
			route += "_21";
			return HJErrInvalidData;
		}
		if (m_status >= HJSTATUS_EOF) {
			route += "_22";
			return HJErrInvalidData;
		}

		m_streamIndex = inFrame->m_streamIndex;
		if (getType() == HJMEDIA_TYPE_VIDEO) {
			route += "_23";
			passThroughSetInput(inFrame);
		}
		auto err = m_codec->run(inFrame);
		if (err < 0) {
			route += "_24";
			HJFLoge("{}, m_codec->run() error({})", getName(), err);
			return HJErrFatal;
		}

		route += "_25";
		return err;
	});
	if (ret == HJErrInvalidData) {
		//if (m_pluginListener) {
		//	route += "_26";
		//	m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_ERROR_CODEC_INVALID_DATA)));
		//}
		report(EVENT_PLUGIN_NOTIFY_ID, HJ_PLUGIN_NOTIFY_ERROR_CODEC_INVALID_DATA, getID());
	}
	if (ret == HJErrFatal) {
		IF_FALSE_RETURN(setStatus(HJSTATUS_Exception), HJErrAlreadyDone);
		//if (m_pluginListener) {
		//	route += "_27";
		//	m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_ERROR_CODEC_RUN)));
		//}
		report(EVENT_PLUGIN_NOTIFY_ID, HJ_PLUGIN_NOTIFY_ERROR_CODEC_RUN, getID());
	}

	return ret;
}

int HJPluginCodec::processEofFrame(std::string& route, HJMediaFrame::Ptr& inFrame)
{
	auto status = SYNC_PROD_LOCK([this] {
		return m_status;
	});
	if (status == HJSTATUS_Inited) {
		route += "_40";
		deliverToOutputs(inFrame);

		HJFLogi("{}, (inFrame->isEofFrame())", getName());
		return HJ_WOULD_BLOCK;
	}

	route += "_41";
	return HJ_OK;
}

std::tuple<int, HJMediaFrame::Ptr> HJPluginCodec::getOutputFrame(std::string& route)
{
	HJMediaFrame::Ptr outFrame{};
	auto ret = SYNC_PROD_LOCK([this, &route, &outFrame] {
		if (m_status == HJSTATUS_Done) {
			route += "_30";
			return HJErrAlreadyDone;
		}
		if (m_status < HJSTATUS_Ready) {
			route += "_31";
			return HJ_WOULD_BLOCK;
		}
		if (m_status >= HJSTATUS_EOF) {
			route += "_32";
			return HJ_WOULD_BLOCK;
		}

		auto err = m_codec->getFrame(outFrame);
		if (err < 0) {
			route += "_33";
			HJFLoge("{}, m_codec->getFrame() error({})", getName(), err);
			return HJErrFatal;
		}

        if (outFrame != nullptr) {
			route += "_34";
			outFrame->m_streamIndex = m_streamIndex;
			if (getType() == HJMEDIA_TYPE_VIDEO) {
				route += "_35";
				passThroughSetOutput(outFrame);
			}
        }

		route += "_36";
		return err;
	});
	if (ret == HJErrFatal) {
		IF_FALSE_RETURN(setStatus(HJSTATUS_Exception), std::make_tuple(HJErrAlreadyDone, nullptr));
		//if (m_pluginListener) {
		//	route += "_37";
		//	m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_ERROR_CODEC_GETFRAME)));
		//}
		report(EVENT_PLUGIN_NOTIFY_ID, HJ_PLUGIN_NOTIFY_ERROR_CODEC_GETFRAME, getID());
	}

	return std::make_tuple(ret, outFrame);
}

void HJPluginCodec::passThroughSetInput(const HJMediaFrame::Ptr& i_inFrame)
{
	if (i_inFrame == nullptr || i_inFrame->isEofFrame()) {
		return;
	}

	if ((i_inFrame->haveStorage("passThroughDemuxSystemTime") && i_inFrame->haveStorage("passThroughIsKey")) || i_inFrame->getSEINals())
	{
		int64_t demuxSysTime = 0;
		if (i_inFrame->haveStorage("passThroughDemuxSystemTime")) {
			demuxSysTime = i_inFrame->getInt64("passThroughDemuxSystemTime");
		}
		bool isKey = false;
        if (i_inFrame->haveStorage("passThroughIsKey")) {
			isKey = i_inFrame->getBool("passThroughIsKey");
		}
		auto seiNals = i_inFrame->getSEINals();
		//
		passThroughAdd(i_inFrame->getDTS(), std::move(HJCodecPassThroughInfo::Create<HJCodecPassThroughInfo>(demuxSysTime, isKey, seiNals)));
	}        
}

void HJPluginCodec::passThroughSetOutput(const HJMediaFrame::Ptr& i_outFrame)
{
	if (i_outFrame == nullptr || i_outFrame->isEofFrame()) {
		return;
	}

  	int64_t throughKey = i_outFrame->getDTS();
	HJCodecPassThroughInfo::Ptr throughInfo = passThroughGet(throughKey);
	if (throughInfo)
	{
		(*i_outFrame)["passThroughDemuxSystemTime"] = throughInfo->getDemuxSysTime();
		(*i_outFrame)["passThroughIsKey"] = throughInfo->isKey();
		auto nals = throughInfo->getSEINals();
		if (nals) {
            (*i_outFrame)[HJMediaFrame::STORE_KEY_SEIINAL] = nals;
		}
		passThroughRemove(throughKey);
        if (throughInfo->isKey())
        {
            HJFLogd("{} passThroughMap after remove isKey:{} map size:{} ", getName(), throughInfo->isKey(), m_passThroughMap.size());
        }
	}
}

void HJPluginCodec::passThroughAdd(int64_t i_key, HJCodecPassThroughInfo::Ptr i_info)
{
	m_passThroughMap[i_key] = i_info;
}

HJCodecPassThroughInfo::Ptr HJPluginCodec::passThroughGet(int64_t i_key)
{
	if (m_passThroughMap.find(i_key) != m_passThroughMap.end())
	{
		return m_passThroughMap[i_key];
	}
	return nullptr;
}

void HJPluginCodec::passThroughRemove(int64_t i_key)
{
	if (m_passThroughMap.find(i_key) != m_passThroughMap.end())
	{
		m_passThroughMap.erase(i_key);
	}
}

void HJPluginCodec::passThroughReset()
{
	m_passThroughMap.clear();
}

NS_HJ_END
