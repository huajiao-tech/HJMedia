#include "HJGraphPusher.h"
#include "HJStatContext.h"

NS_HJ_BEGIN

HJGraphPusher::HJGraphPusher(const std::string& i_name, size_t i_identify)
    : HJGraph(i_name, i_identify)
{
}

HJGraphPusher::~HJGraphPusher()
{
    HJGraphPusher::done();
}

int HJGraphPusher::internalInit(HJKeyStorage::Ptr i_param)
{
    int ret = HJGraph::internalInit(i_param);
    if (ret < 0) {
        return ret;
    }

    do {
        GET_PARAMETER(HJMediaUrl::Ptr, mediaUrl);
        IF_FALSE_BREAK(mediaUrl != nullptr, HJErrInvalidParams);
        GET_PARAMETER(HJAudioInfo::Ptr, audioInfo);
        GET_PARAMETER(HJVideoInfo::Ptr, videoInfo);
        IF_FALSE_BREAK(audioInfo != nullptr || videoInfo != nullptr, HJErrInvalidParams);
#if defined(HarmonyOS)
        GET_PARAMETER(HJOHSurfaceCb, surfaceCb);
        IF_FALSE_BREAK(videoInfo == nullptr || surfaceCb != nullptr, HJErrInvalidParams);
#endif
        GET_PARAMETER(HJListener, rtmpListener);
		std::weak_ptr<HJStatContext> statCtx;
        if(i_param) {
			statCtx = i_param->getValueObj<std::weak_ptr<HJStatContext>>("HJStatContext");
        }
        GET_PARAMETER(HJListener, pusherListener);
        m_pusherListener = pusherListener;

        IF_FALSE_BREAK(m_rtmp = HJPluginRTMPMuxer::Create(), HJErrFatal);
        addPlugin(m_rtmp);
        IF_FALSE_BREAK(m_avInterleave = HJPluginAVInterleave::Create(), HJErrFatal);
        addPlugin(m_avInterleave);
        IF_FAIL_BREAK(ret = connectPlugins(m_avInterleave, m_rtmp, HJMEDIA_TYPE_DATA), ret);
        if (audioInfo != nullptr) {
            m_audioInfo = audioInfo;
            IF_FALSE_BREAK(m_audioThread = HJLooperThread::quickStart("audioThread"), HJErrFatal);
            addThread(m_audioThread);
#if defined(HarmonyOS)
            IF_FALSE_BREAK(m_audioCapturer = HJPluginAudioOHCapturer::Create(), HJErrFatal);
            addPlugin(m_audioCapturer);
#endif
            IF_FALSE_BREAK(m_audioResampler = HJPluginAudioResampler::Create(), HJErrFatal);
            addPlugin(m_audioResampler);
            IF_FALSE_BREAK(m_audioEncoder = HJPluginFDKAACEncoder::Create(), HJErrFatal);
            addPlugin(m_audioEncoder);
#if defined(HarmonyOS)
            IF_FAIL_BREAK(ret = connectPlugins(m_audioCapturer, m_audioResampler, HJMEDIA_TYPE_AUDIO), ret);
#endif
            IF_FAIL_BREAK(ret = connectPlugins(m_audioResampler, m_audioEncoder, HJMEDIA_TYPE_AUDIO), ret);
            IF_FAIL_BREAK(ret = connectPlugins(m_audioEncoder, m_avInterleave, HJMEDIA_TYPE_AUDIO), ret);
        }
        if (videoInfo != nullptr) {
            m_videoInfo = videoInfo;
#if defined(HarmonyOS)
            IF_FALSE_BREAK(m_videoEncoder = HJPluginVideoOHEncoder::Create(), HJErrFatal);
            addPlugin(m_videoEncoder);
            IF_FAIL_BREAK(ret = connectPlugins(m_videoEncoder, m_avInterleave, HJMEDIA_TYPE_VIDEO), ret);
#endif
        }

        auto param = std::make_shared<HJKeyStorage>();
        (*param)["mediaUrl"] = mediaUrl;
        if (audioInfo != nullptr) {
            (*param)["audioInfo"] = audioInfo;
        }
        if (videoInfo != nullptr) {
            (*param)["videoInfo"] = videoInfo;
        }
        if(rtmpListener) {
            (*param)["rtmpListener"] = rtmpListener;
        }
        (*param)["HJStatContext"] = statCtx;
        if (pusherListener) {
            (*param)["pluginListener"] = pusherListener;
        }
        IF_FAIL_BREAK(ret = m_rtmp->init(param), ret);

        param = std::make_shared<HJKeyStorage>();
        if (pusherListener) {
            (*param)["pluginListener"] = pusherListener;
        }
        IF_FAIL_BREAK(ret = m_avInterleave->init(param), ret);

        if (audioInfo != nullptr) {
            param = std::make_shared<HJKeyStorage>();
            (*param)["audioInfo"] = audioInfo;
            (*param)["thread"] = m_audioThread;
            if (pusherListener) {
                (*param)["pluginListener"] = pusherListener;
            }
            IF_FAIL_BREAK(ret = m_audioEncoder->init(param), ret);
            (*param)["fifo"] = true;
            IF_FAIL_BREAK(ret = m_audioResampler->init(param), ret);

            param->erase("fifo");
            param->erase("thread");
#if defined(HarmonyOS)
            IF_FAIL_BREAK(ret = m_audioCapturer->init(param), ret);
#endif
        }

        if (videoInfo != nullptr) {
            param = std::make_shared<HJKeyStorage>();
#if defined(HarmonyOS)
            (*param)["surfaceCb"] = surfaceCb;
#endif
            (*param)["videoInfo"] = videoInfo;
            if (pusherListener) {
                (*param)["pluginListener"] = pusherListener;
            }
#if defined(HarmonyOS)
            IF_FAIL_BREAK(ret = m_videoEncoder->init(param), ret);
#endif
        }

        return HJ_OK;
    } while (false);

    HJGraphPusher::internalRelease();
    return ret;
}

void HJGraphPusher::internalRelease()
{
#if defined(HarmonyOS)
    m_videoEncoder = nullptr;
    m_audioCapturer = nullptr;
#endif
    m_audioEncoder = nullptr;
    m_audioResampler = nullptr;
    m_speechResampler = nullptr;
    m_speechRecognizer = nullptr;
    m_avInterleave = nullptr;
    m_rtmp = nullptr;
    m_muxer = nullptr;

    m_audioThread = nullptr;

    m_audioInfo = nullptr;
    m_videoInfo = nullptr;
    m_pusherListener = nullptr;
    m_inRecording = false;
    m_speechRecognizing = false;

    HJGraph::internalRelease();
}

void HJGraphPusher::setMute(bool i_mute)
{
    SYNC_CONS_LOCK([=] {
        CHECK_DONE_STATUS();
#if defined(HarmonyOS)
        if (m_audioCapturer) {
            m_audioCapturer->setMute(i_mute);
        }
#endif
    });
}

int HJGraphPusher::adjustBitrate(int i_newBitrate)
{
    if (i_newBitrate <= 0) {
		return HJErrInvalidParams;
	}

    return SYNC_CONS_LOCK([=] {
        CHECK_DONE_STATUS(HJErrAlreadyDone);
#if defined(HarmonyOS)
        if (m_videoEncoder == nullptr) {
            return HJErrFatal;
        }

        return m_videoEncoder->adjustBitrate(i_newBitrate);
#else
        return HJErrFatal;
#endif
    });
}

int HJGraphPusher::openRecorder(HJKeyStorage::Ptr i_param)
{
    GET_PARAMETER(HJMediaUrl::Ptr, mediaUrl);
    if (mediaUrl == nullptr) {
        return HJErrInvalidParams;
    }

    return SYNC_PROD_LOCK([=] {
        CHECK_DONE_STATUS(HJErrAlreadyDone);
            if (m_inRecording) {
                return HJErrAlreadyExist;
            }

        int ret;
        do {
            IF_FALSE_BREAK(m_muxer = HJPluginFFMuxer::Create(), HJErrFatal);
            addPlugin(m_muxer);
            IF_FAIL_BREAK(ret = connectPlugins(m_avInterleave, m_muxer, HJMEDIA_TYPE_DATA), ret);

            auto param = std::make_shared<HJKeyStorage>();
            (*param)["mediaUrl"] = mediaUrl;
            if (m_audioInfo != nullptr) {
                (*param)["audioInfo"] = m_audioInfo;
            }
            if (m_videoInfo != nullptr) {
                (*param)["videoInfo"] = m_videoInfo;
            }
            if (m_pusherListener) {
                (*param)["pluginListener"] = m_pusherListener;
            }
            IF_FAIL_BREAK(ret = m_muxer->init(param), ret);

            m_inRecording = true;
            return HJ_OK;
        } while (false);

        return ret;
    });
}

void HJGraphPusher::closeRecorder()
{
    SYNC_PROD_LOCK([this] {
        CHECK_DONE_STATUS();

        if (m_muxer != nullptr) {
            removePlugin(m_muxer);
            m_muxer = nullptr;
            m_inRecording = false;
        }
    });
}

int HJGraphPusher::openSpeechRecognizer(HJKeyStorage::Ptr i_param)
{
    GET_PARAMETER(HJPluginSpeechRecognizer::Call, speechRecognizerCall);
    if (speechRecognizerCall == nullptr) {
        return HJErrInvalidParams;
    }

    return SYNC_PROD_LOCK([=] {
        CHECK_DONE_STATUS(HJErrAlreadyDone);
            if (m_speechRecognizing) {
                return HJErrAlreadyExist;
            }

        int ret;
        do {
            IF_FALSE_BREAK(m_speechResampler = std::make_shared<HJPluginAudioResampler>("HJPluginSpeechResampler"), HJErrFatal);
            addPlugin(m_speechResampler);
            IF_FALSE_BREAK(m_speechRecognizer = HJPluginSpeechRecognizer::Create(), HJErrFatal);
            m_speechRecognizer->m_call = speechRecognizerCall;
            addPlugin(m_speechRecognizer);
#if defined(HarmonyOS)
            IF_FAIL_BREAK(ret = connectPlugins(m_audioCapturer, m_speechResampler, HJMEDIA_TYPE_AUDIO), ret);
#endif
            IF_FAIL_BREAK(ret = connectPlugins(m_speechResampler, m_speechRecognizer, HJMEDIA_TYPE_AUDIO), ret);

            auto param = std::make_shared<HJKeyStorage>();
            auto speechAudioInfo = std::dynamic_pointer_cast<HJAudioInfo>(m_audioInfo->dup());
            speechAudioInfo->setChannels(1);
            speechAudioInfo->setSampleRate(16000);
            speechAudioInfo->setSampleCnt(320);
            (*param)["audioInfo"] = speechAudioInfo;
            IF_FAIL_BREAK(ret = m_speechResampler->init(param), ret);
            IF_FAIL_BREAK(ret = m_speechRecognizer->init(param), ret);

            m_speechRecognizing = true;
            return HJ_OK;
        } while (false);

        return ret;
    });
}

void HJGraphPusher::closeSpeechRecognizer()
{
    SYNC_PROD_LOCK([this] {
        CHECK_DONE_STATUS();

        if (m_speechRecognizer != nullptr) {
            removePlugin(m_speechRecognizer);
            m_speechRecognizer = nullptr;
        }
        if (m_speechResampler != nullptr) {
            removePlugin(m_speechResampler);
            m_speechResampler = nullptr;
            m_speechRecognizing = false;
        }
    });
}

NS_HJ_END
