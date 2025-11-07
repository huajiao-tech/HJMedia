#pragma once

#include <Audioclient.h>
#include <mmdeviceapi.h>
//#include <atomic>
#include "HJPluginAudioRender.h"

NS_HJ_BEGIN

class HJPluginAudioWASRender : public HJPluginAudioRender
{
public:
    HJ_DEFINE_CREATE(HJPluginAudioWASRender);

    HJPluginAudioWASRender(const std::string& i_name = "HJPluginAudioWASRender", HJKeyStorage::Ptr i_graphInfo = nullptr);
    virtual ~HJPluginAudioWASRender();
    virtual int done() override;

protected:
    virtual void onInputUpdated() override { }
    virtual int initRender(const HJAudioInfo::Ptr& i_audioInfo) override;
    virtual bool releaseRender() override;
    virtual int runTask(int64_t* o_delay) override;
    virtual void internalSetMute() override { }

private:
    HANDLE m_audioEvent[2];

    IMMDeviceEnumerator* m_deviceEnumerator{};
    IMMDevice* m_device{};
    IAudioClient* m_audioClient{};
    IAudioRenderClient* m_renderClient{};

    UINT32 m_bufferFrameCount{};
    REFERENCE_TIME m_hnsRequestedDuration{};
    WORD m_blockAlign{};

    HJMediaFrame::Ptr m_kernalFrame{};
};

NS_HJ_END
