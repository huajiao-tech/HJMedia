#pragma once

#include "HJUIItemPlayerPlugin.h"
#include "HJFLog.h"
#include "imgui.h"
#include <algorithm>
#include <cstdio>
#include "HJGraphLivePlayer.h"
#include "HJGraphVodPlayer.h"
#include "HJNotify.h"
#include "HJMediaInfo.h"
#include "HJComEvent.h"
#include "HJMediaFrame.h"
#include "HJYuvTexCvt.h"
#include "HJFFHeaders.h"
#include "HJStatContext.h"

NS_HJ_BEGIN

static std::string FormatMsToHMS(int64_t ms)
{
	int64_t totalSeconds = ms / 1000;
	int hours = static_cast<int>(totalSeconds / 3600);
	int minutes = static_cast<int>((totalSeconds / 60) % 60);
	int seconds = static_cast<int>(totalSeconds % 60);
	char buf[32] = {};
	std::snprintf(buf, sizeof(buf), "%02d:%02d:%02d", hours, minutes, seconds);
	return std::string(buf);
}

static HJGraphType GetGraphTypeByPlayerType(int playerType)
{
	switch (playerType) {
	case 0:
		return HJGraphType_VOD;
	case 1:
		return HJGraphType_LIVESTREAM;
	case 2:
		return HJGraphType_MUSIC;
	default:
		return HJGraphType_NONE;
	}
}

HJUIItemPlayerPlugin::HJUIItemPlayerPlugin()
{
	m_yuvTexCvt = HJYuvTexCvt::Create();
}

HJUIItemPlayerPlugin::~HJUIItemPlayerPlugin()
{
	priDone();
}

int HJUIItemPlayerPlugin::priResume()
{
	HJFLogi("resume");
	if (m_player) {
		return m_player->resume();
	}
	return HJErrNotInited;
}

int HJUIItemPlayerPlugin::priPause()
{
	HJFLogi("pause");
	if (m_player) {
		return m_player->pause();
	}
	return HJErrNotInited;
}

int HJUIItemPlayerPlugin::priStartRecording()
{
	HJFLogi("start recording");
	if (m_player) {
		auto param = std::make_shared<HJKeyStorage>();
		HJMediaUrl::Ptr mediaUrl = std::make_shared<HJMediaUrl>(m_recordUrl);
		(*param)["mediaUrl"] = mediaUrl;
		return m_player->openRecorder(param);
	}
	return HJErrNotInited;
}

int HJUIItemPlayerPlugin::priStopRecording()
{
	HJFLogi("stop recording");
	if (m_player) {
		m_player->closeRecorder();
		return HJ_OK;
	}
	return HJErrNotInited;
}

int HJUIItemPlayerPlugin::priSeek(int64_t i_curTime)
{
	HJFLogi("seek: {} ", i_curTime);
	if (m_player) {
		return m_player->seek(i_curTime);
	}
	return HJErrNotInited;
}

int HJUIItemPlayerPlugin::priRepeats(int i_repeats)
{
	HJFLogi("repeats");
	if (m_player) {
		return m_player->setRepeats(i_repeats);
	}
	return HJErrNotInited;
}

int HJUIItemPlayerPlugin::priInit()
{
	int i_err = HJ_OK;
	do
	{
		const auto graphType = GetGraphTypeByPlayerType(m_playerType);
		if (graphType == HJGraphType_NONE) {
			i_err = HJErrInvalidParams;
			HJFLoge("invalid player type: {}", m_playerType);
			break;
		}
		auto param = std::make_shared<HJKeyStorage>();
		if (graphType == HJGraphType_LIVESTREAM)
		{
			m_statCtx = HJStatContext::Create();
			i_err = m_statCtx->init(123, "windowsDevice", "sn", 10, 0, [](const std::string& i_name, int i_nType, const std::string& i_info)
				{
					HJFLogi("StatStart callback name:{}, type:{}, info:{}", i_name, i_nType, i_info);
				});
			if (i_err < 0) {
				break;
			}
			std::weak_ptr<HJStatContext> statCtx = m_statCtx;
			(*param)["HJStatContext"] = statCtx;
		}
		else
		{
			(*param)["repeats"] = m_loopCount;
		}
		auto audioInfo = std::make_shared<HJAudioInfo>();
		audioInfo->m_samplesRate = 48000;
		audioInfo->setChannels(2);
		audioInfo->m_sampleFmt = 1;
		audioInfo->m_bytesPerSample = 2;
		(*param)["audioInfo"] = audioInfo;
#if defined (WINDOWS)
		LPWSTR deviceName = L"扬声器 (Realtek(R) Audio)";
		int utf8Size = WideCharToMultiByte(
			CP_UTF8,                // 目标编码：UTF-8
			0,                      // 标志（一般填 0）
			deviceName,			    // 输入的宽字符串
			-1,                     // 自动计算长度（-1 表示以 NULL 结尾）
			NULL,                   // 输出缓冲区（NULL 表示计算所需大小）
			0,                      // 输出缓冲区大小（0 表示计算）
			NULL,                   // 默认字符（一般 NULL）
			NULL                    // 是否使用默认字符（一般 NULL）
		);
		if (utf8Size > 0) {
			char* utf8Name = (char*)malloc(utf8Size);
			if (utf8Name) {
				WideCharToMultiByte(
					CP_UTF8, 0, deviceName, -1,
					utf8Name, utf8Size, NULL, NULL
				);

				std::string audioDeviceName = utf8Name;
				free(utf8Name);
//				(*param)["audioDeviceName"] = audioDeviceName;
			}
		}
#endif
		(*param)["pcmCallbackIntervalMs"] = static_cast<int64_t>(40);

		if (m_player == nullptr) {
			m_player = HJGraphPlayer::createGraph(graphType, 0);
			if (!m_player) {
				i_err = HJErrInvalidParams;
				HJFLoge("create graph failed, type: {}", static_cast<int>(graphType));
				break;
			}
		}

		registerHandlers(m_player->eventBus());
		i_err = m_player->init(param);
		if (i_err < 0) {
//			m_player.reset();
			break;
		}
//		m_isStreamOpened.store(false);
//		m_trackIds.clear();
//		m_durationMs = 0;
//		m_curTrackId = 0;
//		m_curTimeMs = 0;
//		m_seekTimeMs = 0;
//		m_isPaused = false;
//		m_isSeeking = false;
//		m_firstTime = 0;
//		m_state = HJUIItemState_idle;
	} while (false);
	return i_err;
}

int HJUIItemPlayerPlugin::priOpen()
{
	int i_err = HJ_OK;
	do
	{
		if (m_url.empty())
		{
			i_err = HJErrInvalidParams;
			HJFLoge("url is empty");
			break;
		}
		if (!m_player) {
			i_err = priInit();
			if (i_err < 0) {
				break;
			}
		}
		HJMediaUrl::Ptr mediaUrl = std::make_shared<HJMediaUrl>(m_url);
//		(*mediaUrl)["startTimestamp"] = static_cast<int64_t>(60000);
		i_err = m_player->openURL(mediaUrl);
		if (i_err < 0) {
			HJFLoge("open url failed, ret:{}, url:{}", i_err, m_url);
			break;
		}
		m_player->setVolume(m_volume);
//		m_isStreamOpened.store(false);
//		m_trackIds.clear();
//		m_durationMs = 0;
//		m_curTrackId = 0;
//		m_curTimeMs = 0;
//		m_seekTimeMs = 0;
//		m_isPaused = false;
//		m_isSeeking = false;
//		m_state = HJUIItemState_ready;
	} while (false);
	return i_err;
}

int HJUIItemPlayerPlugin::priReset()
{
	priDone();
	int i_err = priInit();
	if (i_err < 0) {
		return i_err;
	}
	return priOpen();
}

int HJUIItemPlayerPlugin::priDone()
{
	if (m_player)
	{
		m_player->done();
		m_player.reset();
	}
	if (m_vq) {
		video_queue_close(m_vq);
		m_vq = nullptr;
		m_vqRun = false;
	}
	m_statCtx.reset();
	m_prevState = SHARED_QUEUE_STATE_INVALID;
//	m_state = HJUIItemState_idle;
	m_isStreamOpened.store(false);
	m_isPaused = false;
	m_curTimeMs = 0;
	m_seekTimeMs = 0;
	m_isSeeking = false;
	m_firstTime = 0;
	m_trackIds.clear();
	m_durationMs = 0;
	m_curTrackId = 0;
	return HJ_OK;
}

int HJUIItemPlayerPlugin::priTryGetParam()
{
	int i_err = 0;
	do
	{
		ImGui::Begin("PlayerParams");
		ImGui::SetWindowSize(ImVec2(700, 220));
		char urls[512] = {};
		if (!m_url.empty())
		{
			memcpy(urls, m_url.c_str(), m_url.length());
		}
//		int playType = m_isLive ? 1 : 0;

		if (ImGui::InputText("url", urls, IM_ARRAYSIZE(urls)))
		{
			m_url = urls;
		}

		if (ImGui::RadioButton("VOD", &m_playerType, 0))
		{
//			m_isLive = false;
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("Live", &m_playerType, 1))
		{
//			m_isLive = true;
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("Music", &m_playerType, 2))
		{
//			m_isLive = true;
		}
		ImGui::SameLine();
		ImGui::PushItemWidth(80);
		if (ImGui::InputInt("##loop", &m_loopCount))
		{
			if (m_loopCount < 0)
			{
				m_loopCount = 0;
			}
		}
		ImGui::PopItemWidth();
		ImGui::SameLine();
		if (ImGui::Button("Repeats"))
		{
			m_ret = priRepeats(m_loopCount);
		}

		if (ImGui::Button("Init"))
		{
			m_ret = priInit();
		}
		ImGui::SameLine();
		if (ImGui::Button("Open"))
		{
			m_ret = priOpen();
		}
		ImGui::SameLine();
		if (ImGui::Button("Reset"))
		{
			m_ret = priReset();
/*
			m_url = urls;
			if (m_url.empty())
			{
				HJFLoge("param error");
				ImGui::End();
				return 0;
			}

			priDone();

			m_state = HJUIItemState_ready;
*/
		}
		ImGui::SameLine();
		const char* pauseLabel = m_isPaused ? "Resume" : "Pause";
		if (ImGui::Button(pauseLabel))
		{
			if (m_isPaused)
			{
				m_ret = priResume();
				m_isPaused = false;
			}
			else
			{
				m_ret = priPause();
				m_isPaused = true;
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Done"))
		{
			m_ret = priDone();
		}

		char retBuf[30]{};
		std::snprintf(retBuf, sizeof(retBuf), "%d", m_ret);
		std::string retString(retBuf);
		ImGui::SameLine();
		ImGui::Text("ret = %s", retString.c_str());

		if (m_isStreamOpened.load()) {
			ImGui::NewLine();
			auto size = m_trackIds.size();
			for (int i = 0; i < size; i++) {
				ImGui::SameLine();
				std::string str = "Track" + std::to_string(i);
				if (ImGui::RadioButton(str.c_str(), &m_curTrackId, i)) {
					m_player->switchAudioTrack(m_curTrackId);
				}
			}

			float minVolume = 0.0f;
			float maxVolume = 1.0f;
			ImGui::SameLine();
			ImGui::SetCursorPosX(300.0f);
			ImGui::PushItemWidth(160.0f);
			ImGui::SliderScalar("Volume", ImGuiDataType_Float, &m_volume, &minVolume, &maxVolume);
			if (ImGui::IsItemActive()) {
				if (m_player) {
					m_player->setVolume(m_volume);
				}
			}
			ImGui::PopItemWidth();

//			ImGui::NewLine();
			m_curTimeMs = m_player ? m_player->getCurrentTimestamp() : 0;
			if (!m_isSeeking)
			{
				m_seekTimeMs = m_curTimeMs;
			}
			m_seekTimeMs = (std::min)(m_seekTimeMs, m_durationMs);
	//		m_maxTimeMs = (std::min)(m_durationMs, (std::max)(m_maxTimeMs, m_seekTimeMs + 1000));
			int64_t minTime = 0;
			bool sliderChanged = ImGui::SliderScalar("##TimeSlider", ImGuiDataType_S64, &m_seekTimeMs, &minTime, &m_durationMs, " ");
			bool sliderActive = ImGui::IsItemActive();
			bool sliderDeactivated = ImGui::IsItemDeactivatedAfterEdit();
			std::string timeLabel = FormatMsToHMS(m_seekTimeMs);
			ImGui::SameLine();
			ImGui::Text("Time %s", timeLabel.c_str());
			std::string durationLabel = FormatMsToHMS(m_durationMs);
			ImGui::SameLine();
			ImGui::Text("Duration %s", durationLabel.c_str());
			if (sliderActive)
			{
				m_isSeeking = true;
				priSeek(m_seekTimeMs);
			}
			if (sliderDeactivated || sliderChanged)
			{
				m_isSeeking = false;
			}

			// temp
//			ImGui::NewLine();
			const char* pauseLabel = m_isRecording ? "Stop recording" : "Start recording";
			if (ImGui::Button(pauseLabel))
			{
				if (m_isRecording)
				{
					m_ret = priStopRecording();
					m_isRecording = false;
				}
				else
				{
					m_ret = priStartRecording();
					m_isRecording = true;
				}
			}
		}

		ImGui::End();
	} while (false);
	return i_err;
}

int HJUIItemPlayerPlugin::run()
{
	int i_err = 0;
	do
	{
		priTryGetParam();

//		if ((m_state == HJUIItemState_ready) && !m_player)
//		{
//			if (m_vq) {
//				video_queue_close(m_vq);
//				m_vq = nullptr;
//			}
//
//			auto param = std::make_shared<HJKeyStorage>();
//			HJGraphType graphType{};
//			switch (m_playerType) {
//			case 0:
//				graphType = HJGraphType_VOD;
//				break;
//			case 1:
//				graphType = HJGraphType_LIVESTREAM;
//				break;
//			case 2:
//				graphType = HJGraphType_MUSIC;
//				break;
//			default:
//				break;
//			}
//
//			if (graphType == HJGraphType_LIVESTREAM)
//			{
//				m_statCtx = HJStatContext::Create();
//				i_err = m_statCtx->init(123, "windowsDevice", "sn", 10, 0, [](const std::string& i_name, int i_nType, const std::string& i_info)
//					{
//						HJFLogi("StatStart callback name:{}, type:{}, info:{}", i_name, i_nType, i_info);
//					});
//				std::weak_ptr<HJStatContext> statCtx = m_statCtx;
//				(*param)["HJStatContext"] = statCtx;
//			}
//			else
//			{
//				(*param)["repeats"] = m_loopCount;
//			}
//
//			m_player = HJGraphPlayer::createGraph(graphType, 0);
//
//			registerHandlers(m_player->eventBus());
//
//			auto audioInfo = std::make_shared<HJAudioInfo>();
//			audioInfo->m_samplesRate = 48000;
//			audioInfo->setChannels(2);
//			audioInfo->m_sampleFmt = 1;
//			audioInfo->m_bytesPerSample = 2;
//			(*param)["audioInfo"] = audioInfo;
//#if defined (WINDOWS)
//			LPWSTR deviceName = L"扬声器 (Realtek(R) Audio)";
//			int utf8Size = WideCharToMultiByte(
//				CP_UTF8,                // 目标编码：UTF-8
//				0,                      // 标志（一般填 0）
//				deviceName,			    // 输入的宽字符串
//				-1,                     // 自动计算长度（-1 表示以 NULL 结尾）
//				NULL,                   // 输出缓冲区（NULL 表示计算所需大小）
//				0,                      // 输出缓冲区大小（0 表示计算）
//				NULL,                   // 默认字符（一般 NULL）
//				NULL                    // 是否使用默认字符（一般 NULL）
//			);
//			if (utf8Size > 0) {
//				char* utf8Name = (char*)malloc(utf8Size);
//				if (utf8Name) {
//					WideCharToMultiByte(
//						CP_UTF8, 0, deviceName, -1,
//						utf8Name, utf8Size, NULL, NULL
//					);
//
//					std::string audioDeviceName = utf8Name;
//					free(utf8Name);
////					(*param)["audioDeviceName"] = audioDeviceName;
//				}
//			}
//#endif
//			(*param)["pcmCallbackIntervalMs"] = static_cast<int64_t>(40);
//
//			m_player->init(param);
//			HJMediaUrl::Ptr mediaUrl = std::make_shared<HJMediaUrl>(m_url);
//			m_player->openURL(mediaUrl);
//			m_player->setVolume(m_volume);
//
//			m_vq = video_queue_open();
//			m_state = HJUIItemState_run;
//		}
//	    //fixme lfs; If the resolution increases, it can be restarted
//		else if (m_state == HJUIItemState_ready)
//		{
//			m_vq = video_queue_open();
//			if (m_vq)
//			{
//				m_state = HJUIItemState_run;
//			}
//		}

		if (m_player && !m_vqRun) {
			m_vq = video_queue_open();
			if (m_vq)
			{
				m_vqRun = true;
			}
		}

		if (m_vq) {
			auto state = video_queue_state(m_vq);
			if (state != m_prevState)
			{
				m_prevState = state;
				if (state == SHARED_QUEUE_STATE_STOPPING)
				{
					if (m_vq) {
						video_queue_close(m_vq);
						m_vq = nullptr;
						//fixme lfs; If the resolution increases, it can be restarted
//						m_state = HJUIItemState_ready;
						m_vqRun = false;
					}
				}
				else if (state == SHARED_QUEUE_STATE_INVALID)
				{
				}
			}

			if (state == SHARED_QUEUE_STATE_READY)
			{
				void* data{};
				uint64_t ts;
				int width{}, height{};
				if (video_queue_read2(m_vq, &data, &ts, &width, &height)) {
//					m_curTimeMs = m_player ? m_player->getCurrentTimestamp() : 0;

					size_t size = width * height;
					HJYuvInfo::Ptr yuvInfo = HJYuvInfo::Create();
					yuvInfo->m_y = (uint8_t*)data;
					yuvInfo->m_u = (uint8_t*)data + size;
					yuvInfo->m_v = (uint8_t*)data + size * 5 / 4;
					yuvInfo->m_width = width;
					yuvInfo->m_height = height;
					yuvInfo->m_yLineSize = width;
					yuvInfo->m_uLineSize = width / 2;
					yuvInfo->m_vLineSize = width / 2;
					m_yuvTexCvt->update(yuvInfo);

					ImGui::Begin("DrawImg");
					ImGui::SetWindowSize(ImVec2(yuvInfo->m_width, yuvInfo->m_height));
					ImGui::Image((void*)(intptr_t)m_yuvTexCvt->getTextureId(), ImVec2(yuvInfo->m_width, yuvInfo->m_height));
					ImGui::End();
				}
				else {
					int a = 0;
				}
			}
			else {
				int a = 0;
			}
		}
	} while (false);
	return i_err;
}

void HJUIItemPlayerPlugin::registerHandlers(const std::shared_ptr<HJEventBus> i_bus)
{
	i_bus->registerHandler(EVENT_GRAPH_STREAM_OPENED_ID, [this]() {
		m_isStreamOpened.store(true);
		m_trackIds = m_player->getAudioTrackIds();
		m_durationMs = m_player->getDuration();

//		m_player->seek(60000);
	});

	i_bus->registerHandler(EVENT_GRAPH_EOF_ID, [this]() {
		int a = 0;
	});

	i_bus->registerHandler(EVENT_GRAPH_VIDEO_FRAME_ID, [this](const HJMediaFrame::Ptr& frame) {
		m_asyncCache.enqueue(frame);
	});

	i_bus->registerHandler(EVENT_GRAPH_RENDERED_PCM_ID, [this](const HJGraphRenderedPCM& pcm) {
		auto audioInfo = pcm.m_audioInfo;
		auto pcmData = pcm.m_pcmData;
		HJFLogi("EVENT_GRAPH_RENDERED_PCM channels:{}, bytesPerSample:{}, sampleRate:{}, PCM data size:{}", audioInfo->m_channels, audioInfo->m_bytesPerSample, audioInfo->m_samplesRate, pcmData->size());
	});

	i_bus->registerHandler(EVENT_GRAPH_SEI_INFOS_ID, [this](const std::vector<HJSEIData>& userSEIDatas, bool keyFrame) {

	});

	i_bus->registerHandler(EVENT_GRAPH_FIRST_FRAME_RENDERED_ID, [this](const std::string& pluginName) {

	});

	i_bus->registerHandler(EVENT_GRAPH_AUTODELAY_PARAMS_ID, [this](const std::shared_ptr<HJParams>& params) {

	});
}

NS_HJ_END
