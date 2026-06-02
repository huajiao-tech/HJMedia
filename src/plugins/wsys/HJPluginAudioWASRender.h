#pragma once

#include <Audioclient.h>
#include <mmdeviceapi.h>
#include <vector>
#include "HJPluginAudioRender.h"

NS_HJ_BEGIN

// See doc/HJPluginAudioWASRender.md for details.

class HJPluginAudioWASRender : public HJPluginAudioRender
{
public:
    using Wtr = std::weak_ptr<HJPluginAudioWASRender>;

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

    HJPluginAudioWASRender(const std::string& i_name = "HJPluginAudioWASRender", size_t i_identify = 0, HJKeyStorage::Ptr i_graphInfo = nullptr);
    virtual ~HJPluginAudioWASRender();

    int resetDevice(const std::string& i_deviceName = "");

protected:
    virtual int internalInit(HJKeyStorage::Ptr i_param) override;
    virtual void internalRelease() override;
    virtual int runTask(int64_t* o_delay) override;
    virtual void postTask(int64_t i_delay = 0) {}
    virtual int64_t getRenderTimestamp() override;
    virtual int initRender(const HJAudioInfo::Ptr& i_audioInfo) override;
    virtual void releaseRender() override;

    virtual int initDevice(const HJAudioInfo::Ptr& i_audioInfo);
    virtual void releaseDevice();
    virtual void runReset();
    virtual void postEvent();

private:
    HANDLE m_audioEvent[3];

    IMMDeviceEnumerator* m_deviceEnumerator{};
    IMMDevice* m_device{};
    IAudioClient* m_audioClient{};
    IAudioRenderClient* m_renderClient{};
    IAudioClock* m_audioClock{};

    UINT32 m_bufferFrameCount{};
    REFERENCE_TIME m_hnsRequestedDuration{};
    UINT64 m_clockFreq{};
    std::vector<uint8_t> m_submittedPCMCache{};

    HJLooperThread::Handler::Ptr m_eventHandler{};
    HJLooperThread::Ptr m_eventThread{};
    bool m_beCoInitialize{};

private:
    virtual int setStreamRunning(bool i_running, bool i_eofStop = false);
    virtual std::tuple<int, UINT32, BYTE*> getAudioBuffer(std::string& route);
    virtual int releaseAudioBuffer(std::string& route, int numFramesRequested);
};

NS_HJ_END
