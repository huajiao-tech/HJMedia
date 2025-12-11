#include "HJPluginVideoOHEncoder.h"
#include "HJVEncOHCodec.h"
#include "HJFLog.h"
#include "HJSEIWrapper.h"
#include "HJFFHeaders.h"

NS_HJ_BEGIN

#define RUNTASKLog		//HJFLogi

int HJPluginVideoOHEncoder::internalInit(HJKeyStorage::Ptr i_param)
{
	GET_PARAMETER(HJOHSurfaceCb, surfaceCb);
	GET_PARAMETER(HJVideoInfo::Ptr, videoInfo);
	if (surfaceCb == nullptr || videoInfo == nullptr) {
		return HJErrInvalidParams;
	}
	GET_PARAMETER(HJLooperThread::Ptr, thread);
	GET_PARAMETER(HJListener, pluginListener);
	auto param = std::make_shared<HJKeyStorage>();
	(*param)["streamInfo"] = std::static_pointer_cast<HJStreamInfo>(videoInfo);
//	(*param)["thread"] = thread;
	(*param)["createThread"] = false;
	if (pluginListener) {
		m_listener = pluginListener;
		(*param)["pluginListener"] = pluginListener;
	}

	if (i_param->haveValue("ROIEncodeCb"))
    {
        (*param)["ROIEncodeCb"] = i_param->getValueObj<HJRoiEncodeCb>("ROIEncodeCb");
    }

	int ret = HJPluginCodec::internalInit(param);
	if (ret < 0) {
		return ret;
	}

	do {
		HJVEncOHCodec::Ptr encoder = std::dynamic_pointer_cast<HJVEncOHCodec>(m_codec);
		if (encoder == nullptr) {
			ret = HJErrFatal;
			break;
		}
		m_nativeWindow = encoder->getNativeWindow();
		if (m_nativeWindow == nullptr) {
			ret = HJErrFatal;
			break;
		}

		

		m_surfaceCb = surfaceCb;
		m_surfaceCb(m_nativeWindow, videoInfo->m_width, videoInfo->m_height, true);

		return HJ_OK;
	} while (false);

	HJPluginVideoOHEncoder::internalRelease();
	return ret;
}

void HJPluginVideoOHEncoder::internalRelease()
{
	if (m_nativeWindow && m_surfaceCb) {
		m_surfaceCb(m_nativeWindow, 0, 0, false);
	}
	m_nativeWindow = nullptr;
	m_surfaceCb = nullptr;

	HJPluginCodec::internalRelease();
}

int HJPluginVideoOHEncoder::runTask(int64_t* o_delay)
{
	RUNTASKLog("{}, enter", getName());
	int64_t enter = HJCurrentSteadyMS();
	std::string route = "0";
	int ret;
	do {
		HJMediaFrame::Ptr outFrame{};
		ret = SYNC_CONS_LOCK([&route, &outFrame, this] {
			if (m_status == HJSTATUS_Done) {
				route += "_1";
				return HJErrAlreadyDone;
			}
			if (m_status < HJSTATUS_Inited) {
				route += "_2";
				return HJ_WOULD_BLOCK;
			}
			if (m_status >= HJSTATUS_Stoped) {
				route += "_3";
				return HJ_WOULD_BLOCK;
			}
			auto err = m_codec->getFrame(outFrame);
			if (err < 0) {
				route += "_4";
				HJFLoge("{}, 0, m_codec->getFrame() error({})", getName(), err);
				if (m_pluginListener) {
					m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_ERROR_CODEC_GETFRAME)));
				}
				m_status = HJSTATUS_Exception;
				return HJErrFatal;
			}
			return HJ_OK;
		});
		if (ret != HJ_OK) {
			break;
		}
		if (outFrame == nullptr) {
			route += "_5";
			ret = HJ_WOULD_BLOCK;
			break;
		}
		requireSEI(outFrame);

		route += "_6";
		deliverToOutputs(outFrame);
	} while (false);

	RUNTASKLog("{}, leave, route({}), duration({}), ret({})", getName(), route, (HJCurrentSteadyMS() - enter), ret);
	return ret;
}

HJBaseCodec::Ptr HJPluginVideoOHEncoder::createCodec()
{
	return HJBaseCodec::createEncoder(HJMEDIA_TYPE_VIDEO, HJDEVICE_TYPE_OHCODEC);
}

int HJPluginVideoOHEncoder::initCodec(const HJStreamInfo::Ptr& i_streamInfo)
{
	if (!i_streamInfo) {
		return HJErrInvalidParams;
	}

	auto streamInfo = i_streamInfo->dup();
	(*streamInfo)["newBufferCb"] = (HJRunnable)[=] {
//		internalUpdated();
		runTask();
	};

	return HJPluginCodec::initCodec(streamInfo);
}

void HJPluginVideoOHEncoder::deliverToOutputs(HJMediaFrame::Ptr& i_mediaFrame)
{
	auto info = i_mediaFrame->getVideoInfo();
	frame_size++;
	auto now = HJCurrentSteadyMS();
	if (start_time > 0) {
		if ((now - start_time) / 1000 >= v_duration_count * 5) {
			auto frames = v_duration_count * 5 * info->m_frameRate;
			HJFLogi("{}, ({})s, frames({}), sample_size diff({}), delay({}), bitrate({})",
				getName(), v_duration_count * 5, frames, frame_size - frames, now - i_mediaFrame->getPTS(), m_bitrate);
			v_duration_count++;
		}
	}
	else {
		start_time  = now;
	}

	HJPluginCodec::deliverToOutputs(i_mediaFrame);
}

int HJPluginVideoOHEncoder::adjustBitrate(int i_newBitrate)
{
	if (i_newBitrate <= 0) {
		return HJErrInvalidParams;
	}

	return SYNC_CONS_LOCK([=] {
		CHECK_DONE_STATUS(HJErrAlreadyDone);
		if (m_codec == nullptr)	{
			return HJErrNotInited;

		}
		auto err = std::dynamic_pointer_cast<HJVEncOHCodec>(m_codec)->adjustBitrate(i_newBitrate);
		if (err < 0) {
			return HJErrInvalidParams;
		}

		m_bitrate = i_newBitrate;
		return HJ_OK;
	});
}

void HJPluginVideoOHEncoder::requireSEI(HJMediaFrame::Ptr& mvf)
{
	if(!mvf || !m_listener) {
		return;
	}
	auto info = mvf->getVideoInfo();

	auto ntfy = HJMakeNotification(HJ_PLUGIN_NOTIFY_PLUGIN_SEI_REQUIRE);
	(*ntfy)["timestamp"] = mvf->getPTS();
	(*ntfy)["key_frame"] = mvf->isKeyFrame();
	m_listener(ntfy);
	//
	std::vector<HJSEIData> userSEIDatas = ntfy->getValue<std::vector<HJSEIData>>("user_sei_datas");
	if (userSEIDatas.empty()) {
		return;
	}
	HJSEINals::Ptr seiNals = HJCreates<HJSEINals>();
	for (auto& seiData : userSEIDatas) {
		bool isH265 = (AV_CODEC_ID_H265 == info->m_codecID);
		auto out_nal = HJSEIWrapper::makeSEINal({ seiData }, isH265, HJSEIWrapper::HJNALFormat::AVCC);
		seiNals->addData(out_nal);
	}
	(*mvf)[HJMediaFrame::STORE_KEY_SEIINFO] = seiNals;

	return;
}

NS_HJ_END
