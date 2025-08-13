#include "HJGraphPusher.h"

NS_HJ_BEGIN

HJGraphPusher::HJGraphPusher(const std::string& i_name, size_t i_identify)
    : HJGraph(i_name, i_identify)
{
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
        GET_PARAMETER(HJOHSurfaceCb, surfaceCb);
        IF_FALSE_BREAK(videoInfo == nullptr || surfaceCb != nullptr, HJErrInvalidParams);
        GET_PARAMETER(HJListener, rtmpListener);
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
            IF_FALSE_BREAK(m_audioCapturer = HJPluginAudioOHCapturer::Create(), HJErrFatal);
            addPlugin(m_audioCapturer);
            IF_FALSE_BREAK(m_audioResampler = HJPluginAudioResampler::Create(), HJErrFatal);
            addPlugin(m_audioResampler);
            IF_FALSE_BREAK(m_audioEncoder = HJPluginFDKAACEncoder::Create(), HJErrFatal);
            addPlugin(m_audioEncoder);
            IF_FAIL_BREAK(ret = connectPlugins(m_audioCapturer, m_audioResampler, HJMEDIA_TYPE_AUDIO), ret);
            IF_FAIL_BREAK(ret = connectPlugins(m_audioResampler, m_audioEncoder, HJMEDIA_TYPE_AUDIO), ret);
            IF_FAIL_BREAK(ret = connectPlugins(m_audioEncoder, m_avInterleave, HJMEDIA_TYPE_AUDIO), ret);
        }
        if (videoInfo != nullptr) {
            m_videoInfo = videoInfo;
            IF_FALSE_BREAK(m_videoEncoder = HJPluginVideoOHEncoder::Create(), HJErrFatal);
            addPlugin(m_videoEncoder);
            IF_FAIL_BREAK(ret = connectPlugins(m_videoEncoder, m_avInterleave, HJMEDIA_TYPE_VIDEO), ret);
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
            IF_FAIL_BREAK(ret = m_audioResampler->init(param), ret);
            param->erase("thread");
            IF_FAIL_BREAK(ret = m_audioCapturer->init(param), ret);
        }

        if (videoInfo != nullptr) {
            param = std::make_shared<HJKeyStorage>();
            (*param)["surfaceCb"] = surfaceCb;
            (*param)["videoInfo"] = videoInfo;
            if (pusherListener) {
                (*param)["pluginListener"] = pusherListener;
            }
            IF_FAIL_BREAK(ret = m_videoEncoder->init(param), ret);
        }

        return HJ_OK;
    } while (false);

    internalRelease();
    return ret;
}

void HJGraphPusher::internalRelease()
{
    m_videoEncoder = nullptr;
    m_audioEncoder = nullptr;
    m_audioResampler = nullptr;
    m_audioCapturer = nullptr;
    m_avInterleave = nullptr;
    m_rtmp = nullptr;
    m_muxer = nullptr;

    m_audioThread = nullptr;

    m_audioInfo = nullptr;
    m_videoInfo = nullptr;
    m_pusherListener = nullptr;
    m_inRecording = false;

    HJGraph::internalRelease();
}

void HJGraphPusher::setMute(bool i_mute)
{
    SYNC_CONS_LOCK([=] {
        CHECK_DONE_STATUS();
        if (m_audioCapturer) {
            m_audioCapturer->setMute(i_mute);
        }
    });
}

int HJGraphPusher::adjustBitrate(int i_newBitrate)
{
    if (i_newBitrate <= 0) {
		return HJErrInvalidParams;
	}

    return SYNC_CONS_LOCK([=] {
        CHECK_DONE_STATUS(HJErrAlreadyDone);
        if (m_videoEncoder == nullptr) {
            return HJErrFatal;
        }

        return m_videoEncoder->adjustBitrate(i_newBitrate);
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
        if (m_muxer != nullptr) {
            removePlugin(m_muxer);
            m_muxer = nullptr;
            m_inRecording = false;
        }
    });
}

NS_HJ_END
