#pragma once

#include <Audioclient.h>
#include <mmdeviceapi.h>
//#include <atomic>
#include "HJPluginAudioRender.h"

NS_HJ_BEGIN

class HJPluginAudioWASRender : public HJPluginAudioRender
{
public:
    class DeviceNotificationClient : public IMMNotificationClient
    {
    private:
        LONG m_refCount;
        HJPluginAudioWASRender* m_parent;

    public:
        DeviceNotificationClient(HJPluginAudioWASRender* parent);

        // IUnknown methods
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();
        STDMETHOD(QueryInterface)(REFIID riid, void** ppvObject);

        // IMMNotificationClient methods
        STDMETHOD(OnDeviceStateChanged)(LPCWSTR pwstrDeviceId, DWORD dwNewState);
        STDMETHOD(OnDeviceAdded)(LPCWSTR pwstrDeviceId);
        STDMETHOD(OnDeviceRemoved)(LPCWSTR pwstrDeviceId);
        STDMETHOD(OnDefaultDeviceChanged)(EDataFlow flow, ERole role, LPCWSTR pwstrDefaultDeviceId);
        STDMETHOD(OnPropertyValueChanged)(LPCWSTR pwstrDeviceId, const PROPERTYKEY key);
    };

    DeviceNotificationClient* m_notificationClient;

public:
    HJ_DEFINE_CREATE(HJPluginAudioWASRender);

    HJPluginAudioWASRender(const std::string& i_name = "HJPluginAudioWASRender", HJKeyStorage::Ptr i_graphInfo = nullptr);
    virtual ~HJPluginAudioWASRender();
    virtual int done() override;

    int resetDevice(const std::string& i_deviceName = "");
    void onDefaultDeviceChanged();

protected:
    virtual void onInputUpdated() override { }
    virtual int initRender(const HJAudioInfo::Ptr& i_audioInfo) override;
    virtual bool releaseRender() override;
    virtual int runTask(int64_t* o_delay) override;
    virtual void internalSetMute() override { }

    virtual int initDevice(const HJAudioInfo::Ptr& i_audioInfo);
    virtual void releaseDevice();

private:
    HANDLE m_audioEvent[3];

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
