#include "HJPluginAVInterleave.h"
#include "HJFLog.h"

NS_HJ_BEGIN

#define RUNTASKLog		//HJFLogi

int HJPluginAVInterleave::internalInit(HJKeyStorage::Ptr i_param)
{
	GET_PARAMETER(HJLooperThread::Ptr, thread);
	GET_PARAMETER(HJListener, pluginListener);
	auto param = std::make_shared<HJKeyStorage>();
	(*param)["thread"] = thread;
	(*param)["createThread"] = (thread == nullptr);
	if (pluginListener) {
		(*param)["pluginListener"] = pluginListener;
	}
	return HJPlugin::internalInit(param);
}

void HJPluginAVInterleave::onInputAdded(size_t i_srcKeyHash, HJMediaType i_type)
{
	if (i_type == HJMEDIA_TYPE_AUDIO) {
		m_flag.fetch_or(HJMEDIA_TYPE_AUDIO);
		m_inputAudioKeyHash.store(i_srcKeyHash);
	}
	else if (i_type == HJMEDIA_TYPE_VIDEO) {
		m_flag.fetch_or(HJMEDIA_TYPE_VIDEO);
		m_inputVideoKeyHash.store(i_srcKeyHash);
	}
}

int HJPluginAVInterleave::runTask(int64_t* o_delay)
{
	RUNTASKLog("{}, enter", getName());
	std::string route = "0";
	int64_t enter = HJCurrentSteadyMS();
	int ret;
	do {
		ret = SYNC_CONS_LOCK([&route, this] {
			if (m_status == HJSTATUS_Done) {
				route += "_1";
				return HJErrAlreadyDone;
			}
			return HJ_OK;
		});
		if (ret != HJ_OK) {
			break;
		}

		auto flag = m_flag.load();
		auto inputAudioKeyHash = m_inputAudioKeyHash.load();
		auto inputVideoKeyHash = m_inputVideoKeyHash.load();

		size_t audioSize = -1;
		HJMediaFrame::Ptr previewAudio{};
		if (flag & HJMEDIA_TYPE_AUDIO) {
			route += "_2";
			previewAudio = preview(inputAudioKeyHash, &audioSize);
		}
		size_t videoSize = -1;
		HJMediaFrame::Ptr previewVideo{};
		if (flag & HJMEDIA_TYPE_VIDEO) {
			route += "_3";
			previewVideo = preview(inputVideoKeyHash, &videoSize);
		}

		HJMediaFrame::Ptr outFrame{};
		do {
			if (previewAudio != nullptr) {
				route += HJFMT("_ADTS({})", previewAudio->getDTS());
				if (!(flag & HJMEDIA_TYPE_VIDEO) || ((previewVideo != nullptr) && (previewAudio->getDTS() <= previewVideo->getDTS()))) {
					receive(inputAudioKeyHash, &audioSize);
					route += HJFMT("<=VDTS({})_ASize({})", previewVideo->getDTS(), audioSize);
					outFrame = previewAudio;
					break;
				}
			}

			if (previewVideo != nullptr) {
				route += previewAudio ? ">=" : "_";
				route += HJFMT("VDTS({})", previewVideo->getDTS());
				if (!(flag & HJMEDIA_TYPE_AUDIO) || ((previewAudio != nullptr) && (previewVideo->getDTS() <= previewAudio->getDTS()))) {
					receive(inputVideoKeyHash, &videoSize);
					route += HJFMT("_VSize({})", videoSize);
					outFrame = previewVideo;
					break;
				}
			}
		} while (false);
		if (outFrame == nullptr) {
			route += "_4";
			ret = HJ_WOULD_BLOCK;
			break;
		}

		route += "_5";
		deliverToOutputs(outFrame);
	} while (false);

	RUNTASKLog("{}, leave, route({}), duration({}), ret({})", getName(), route, (HJCurrentSteadyMS() - enter), ret);
	return ret;
}

void HJPluginAVInterleave::deliverToOutputs(HJMediaFrame::Ptr& i_mediaFrame)
{
	auto now = HJCurrentSteadyMS();
	if (start_time > 0) {
		int rate{};
		if (i_mediaFrame->isAudio()) {
			auto info = i_mediaFrame->getAudioInfo();
			sample_size += info->getSampleCnt();
			if ((now - start_time) / 1000 >= duration_count * 5) {
				auto samples = duration_count * 5 * info->getSampleRate();
				HJFLogi("{}, ({})s, samples({}), sample_size diff({}), delay({})",
					getName(), duration_count * 5, samples, sample_size - samples, now - i_mediaFrame->getPTS());
				duration_count++;
			}
		}
		else if (i_mediaFrame->isVideo()) {
			auto info = i_mediaFrame->getVideoInfo();
			frame_size++;
			if ((now - start_time) / 1000 >= v_duration_count * 5) {
				auto frames = v_duration_count * 5 * info->m_frameRate;
				HJFLogi("{}, ({})s, frames({}), sample_size diff({}), delay({})",
					getName(), v_duration_count * 5, frames, frame_size - frames, now - i_mediaFrame->getPTS());
				v_duration_count++;
			}
		}
	}
	else {
		start_time  = now;
	}

	HJPlugin::deliverToOutputs(i_mediaFrame);
}

NS_HJ_END
