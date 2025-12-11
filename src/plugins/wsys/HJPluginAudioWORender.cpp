#include "HJPluginAudioWORender.h"
#include "HJFLog.h"
#include "HJFFHeaders.h"

NS_HJ_BEGIN

#define RUNTASKLog		HJFLogi

#define MAX_KERNAL_BUFFER_SIZE      3

void callback(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	if (uMsg == WOM_DONE) {
		WAVEHDR* header = (WAVEHDR*)dwParam1;
		if (header) {
			delete header;
		}
		((HJPluginAudioWORender*)dwInstance)->onOutputUpdated();
	}
}

void HJPluginAudioWORender::onOutputUpdated()
{
	auto mediaFrame = m_kernalFrames.receive();
	if (mediaFrame) {
		auto err = SYNC_CONS_LOCK([=] {
			if (m_status == HJSTATUS_Done) {
				return HJErrAlreadyDone;
			}

			m_timeline->setTimestamp(mediaFrame->m_streamIndex, mediaFrame->getPTS(), mediaFrame->getSpeed());
			return HJ_OK;
		});
		if (err == HJErrAlreadyDone) {
			return;
		}
	}

	HJPluginAudioRender::onOutputUpdated();
}

int HJPluginAudioWORender::runTask(int64_t* o_delay)
{
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
	int64_t audioSamples = 0;
	int ret = HJ_WOULD_BLOCK;
	while (m_kernalFrames.size() < MAX_KERNAL_BUFFER_SIZE) {
		route += "_0";
		auto inFrame = receive(m_inputKeyHash.load(), &size, &audioDuration, nullptr, &audioSamples);
		if (inFrame == nullptr) {
			route += "_1";
			if (!m_buffering) {
				m_buffering = true;
				if (m_pluginListener) {
					m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_AUDIORENDER_START_BUFFERING)));
				}
			}

			ret = HJ_WOULD_BLOCK;
			break;
		}

		setInfoAudioDuration(audioSamples);

		if (m_buffering) {
			m_buffering = false;
			if (m_pluginListener) {
				m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_AUDIORENDER_STOP_BUFFERING)));
			}
		}

		if (inFrame->isEofFrame()) {
			if (m_pluginListener) {
				m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_AUDIORENDER_EOF)));
			}
			ret = HJ_WOULD_BLOCK;
			break;
		}

		auto err = SYNC_CONS_LOCK([&route, inFrame, this] {
			if (m_status == HJSTATUS_Done) {
				route += "_3";
				return HJErrAlreadyDone;
			}
			if (m_status < HJSTATUS_Ready) {
				route += "_4";
				return HJ_WOULD_BLOCK;
			}
			if (m_status >= HJSTATUS_EOF) {
				route += "_5";
				return HJ_WOULD_BLOCK;
			}

			if (m_render) {
				route += "_7";
				WAVEHDR* header = new WAVEHDR();
				ZeroMemory(header, sizeof(WAVEHDR));
				HJAVFrame::Ptr avFrame = inFrame->getMFrame();
				AVFrame* frame = avFrame->getAVFrame();

				header->lpData = (LPSTR)(frame->data[0]);
				header->dwBufferLength = (DWORD)(frame->nb_samples * m_audioInfo->m_channels * m_audioInfo->m_bytesPerSample);
				MMRESULT result = waveOutPrepareHeader(m_render, header, sizeof(WAVEHDR));
				if (result == MMSYSERR_NOERROR) {
					result = waveOutWrite(m_render, header, sizeof(WAVEHDR));
					if (result == MMSYSERR_NOERROR) {
						m_kernalFrames.deliver(inFrame);
						return HJ_OK;
					}
				}
				delete header;
			}

			return HJErrFatal;
		});
		if (err != HJ_OK) {
			if (err == HJErrAlreadyDone) {
				ret = HJErrAlreadyDone;
			}
			break;
		}
	}

	if (log) {
		RUNTASKLog("{}, leave, route({}), size({}), task duration({}), ret({})", getName(), route, size, (HJCurrentSteadyMS() - enter), ret);
	}
	return ret;
}

void HJPluginAudioWORender::internalSetMute()
{
	if (m_render) {
		DWORD dwVolume = m_muted ? 0x00000000 : 0xFFFFFFFF;
		MMRESULT result = waveOutSetVolume(m_render, dwVolume);
		if (result != MMSYSERR_NOERROR) {
			HJFLoge("{}, waveOutSetVolume error({})", getName(), result);
		}
	}
}

int HJPluginAudioWORender::initRender(const HJAudioInfo::Ptr& i_audioInfo)
{
	WAVEFORMATEX format_pcm;
	format_pcm.wFormatTag = WAVE_FORMAT_PCM;
	format_pcm.nChannels = i_audioInfo->m_channels;
	format_pcm.nSamplesPerSec = i_audioInfo->m_samplesRate;
	format_pcm.wBitsPerSample = i_audioInfo->m_bytesPerSample * 8;
	format_pcm.nBlockAlign = (WORD)((format_pcm.nChannels * format_pcm.wBitsPerSample) >> 3);
	format_pcm.nAvgBytesPerSec = format_pcm.nSamplesPerSec * format_pcm.nBlockAlign;
	format_pcm.cbSize = 0;
	MMRESULT result = waveOutOpen((LPHWAVEOUT)&m_render, WAVE_MAPPER,
		(LPCWAVEFORMATEX)&format_pcm, (DWORD_PTR)callback,
		(DWORD_PTR)this, CALLBACK_FUNCTION);
	if (result != MMSYSERR_NOERROR) {
		HJFLoge("{}, waveOutOpen error({})", getName(), result);
		releaseRender();
		return HJErrFatal;
	}

//	waveOutPause(m_render);
	internalSetMute();
/*
	if (m_pluginListener) {
		m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_AUDIOORENDER_START_PLAYING)));
	}
*/
	return HJ_OK;
}

bool HJPluginAudioWORender::releaseRender()
{
	bool ret = false;
	if (m_render) {
//		if (m_pluginListener) {
//			m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_AUDIOORENDER_STOP_PLAYING)));
//		}

		waveOutReset(m_render);
		waveOutClose(m_render);
		m_render = nullptr;

		ret = true;
	}

	m_kernalFrames.flush();
	return ret;
}

NS_HJ_END
