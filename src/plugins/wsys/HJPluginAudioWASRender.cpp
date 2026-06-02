#include "HJPluginAudioWASRender.h"
#include "HJGraph.h"
#include "HJFLog.h"

NS_HJ_BEGIN

#define RUNTASKLog		HJFLogi

HJPluginAudioWASRender::DeviceNotificationClient::DeviceNotificationClient(HJPluginAudioWASRender* parent)
    : m_refCount(1), m_parent(parent)
{
}

STDMETHODIMP_(ULONG) HJPluginAudioWASRender::DeviceNotificationClient::AddRef()
{
    return InterlockedIncrement(&m_refCount);
}

STDMETHODIMP_(ULONG) HJPluginAudioWASRender::DeviceNotificationClient::Release()
{
    LONG refCount = InterlockedDecrement(&m_refCount);
    if (refCount == 0) {
        delete this;
    }
    return refCount;
}

STDMETHODIMP HJPluginAudioWASRender::DeviceNotificationClient::QueryInterface(REFIID riid, void** ppvObject)
{
    if (riid == IID_IUnknown || riid == __uuidof(IMMNotificationClient)) {
        *ppvObject = static_cast<IMMNotificationClient*>(this);
        AddRef();
        return S_OK;
    }
    *ppvObject = nullptr;
    return E_NOINTERFACE;
}

STDMETHODIMP HJPluginAudioWASRender::DeviceNotificationClient::OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState)
{
    return S_OK;
}

STDMETHODIMP HJPluginAudioWASRender::DeviceNotificationClient::OnDeviceAdded(LPCWSTR pwstrDeviceId)
{
    return S_OK;
}

STDMETHODIMP HJPluginAudioWASRender::DeviceNotificationClient::OnDeviceRemoved(LPCWSTR pwstrDeviceId)
{
    return S_OK;
}

STDMETHODIMP HJPluginAudioWASRender::DeviceNotificationClient::OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDefaultDeviceId)
{
    (void)pwstrDefaultDeviceId;
    if (flow == eRender && role == eMultimedia && m_parent) {
        SetEvent(m_parent->m_audioEvent[2]);
    }
    return S_OK;
}

STDMETHODIMP HJPluginAudioWASRender::DeviceNotificationClient::OnPropertyValueChanged(LPCWSTR pwstrDeviceId, const PROPERTYKEY key)
{
    return S_OK;
}

HJPluginAudioWASRender::HJPluginAudioWASRender(const std::string& i_name, size_t i_identify, HJKeyStorage::Ptr i_graphInfo)
    : HJPluginAudioRender(i_name, i_identify, i_graphInfo)
{
    // Audio system event
    m_audioEvent[0] = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    // done event
    m_audioEvent[1] = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    // device changed event
    m_audioEvent[2] = CreateEvent(nullptr, TRUE, FALSE, nullptr);
}

HJPluginAudioWASRender::~HJPluginAudioWASRender()
{
    done();

    if (m_audioEvent[0]) {
        CloseHandle(m_audioEvent[0]);
    }
    if (m_audioEvent[1]) {
        CloseHandle(m_audioEvent[1]);
    }
    if (m_audioEvent[2]) {
        CloseHandle(m_audioEvent[2]);
    }
}

int HJPluginAudioWASRender::internalInit(HJKeyStorage::Ptr i_param)
{
    int ret = HJPluginAudioRender::internalInit(i_param);
    if (ret != HJ_OK) {
        return ret;
    }
    if (!m_audioEvent[0] || !m_audioEvent[1] || !m_audioEvent[2]) {
        HJFLoge("{}, create event failed", getName());
        return HJErrFatal;
    }

    m_eventThread = HJLooperThread::quickStart(getName() + "_event");
    if (m_eventThread == nullptr) {
        HJFLoge("{}, create event thread failed", getName());
        return HJErrFatal;
    }

    m_eventHandler = m_eventThread->createHandler();
    if (m_eventHandler == nullptr) {
        HJFLoge("{}, create event handler failed", getName());
        return HJErrFatal;
    }

    m_eventHandler->async([this] {
        while (true) { 
            DWORD waitResult = WaitForMultipleObjects(3, m_audioEvent, FALSE, INFINITE);
            if (waitResult == WAIT_OBJECT_0 + 1) {
                // done event
                break;
            }
            else if (waitResult == WAIT_OBJECT_0) {
                // Audio buffer event
                postEvent();
            }
            else if (waitResult == WAIT_OBJECT_0 + 2) {
                // device changed event
                ResetEvent(m_audioEvent[2]);
                resetDevice();
            }
            else {
                break;
            }
        }
    });

    return HJ_OK;
}

void HJPluginAudioWASRender::internalRelease()
{
    HJFLogi("{}, internalRelease() begin", getName());

    HJPluginAudioRender::internalRelease();

    if (m_eventThread) {
        SetEvent(m_audioEvent[1]);
        m_eventThread->done();
        m_eventThread = nullptr;
    }
    m_eventHandler = nullptr;

    HJFLogi("{}, internalRelease() end", getName());
}

#include <Functiondiscoverykeys_devpkey.h>
#define SAFE_RELEASE(p) { if ((p) != NULL) { (p)->Release(); (p) = NULL; } }

IMMDevice* getAudioDevice(IMMDeviceEnumerator* pEnumerator, const char* deviceName) {
    HRESULT hr = S_OK;
    IMMDevice* pDevice = NULL;

    if (!deviceName) {
        // Get the default audio rendering device
        hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &pDevice);
        if (FAILED(hr)) {
            return nullptr;
        }
        return pDevice;
    }

    IMMDeviceCollection* pCollection = NULL;
    IPropertyStore* pProps = NULL;

    // 2. Get all active audio rendering (playback) devices
    hr = pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pCollection);
    if (FAILED(hr)) goto Exit;

    // 3. Get device count
    UINT count;
    hr = pCollection->GetCount(&count);
    if (FAILED(hr)) goto Exit;

    // 4. Iterate through devices
    for (UINT i = 0; i < count; i++) {
        // Get device pointer
        hr = pCollection->Item(i, &pDevice);
        if (FAILED(hr)) continue;
/*
        // Get device ID
        LPWSTR pwszID = NULL;
        hr = pDevice->GetId(&pwszID);
        if (SUCCEEDED(hr)) {
            wprintf(L"Device %d ID: %s\n", i, pwszID);
            CoTaskMemFree(pwszID);
        }
*/
        // Open property store
        hr = pDevice->OpenPropertyStore(STGM_READ, &pProps);
        if (SUCCEEDED(hr)) {
            PROPVARIANT varName;
            PropVariantInit(&varName);

            // Get device friendly name
            hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
            if (SUCCEEDED(hr)) {
//                setlocale(LC_ALL, ""); // Use system locale
//                wprintf(L"  Name: %s\n", varName.pwszVal);
                int utf8Size = WideCharToMultiByte(
                    CP_UTF8,                // Target encoding: UTF-8
                    0,                      // Flags, typically 0
                    varName.pwszVal,        // Source wide character string
                    -1,                     // Auto-detect length, -1 means null-terminated
                    NULL,                   // Output buffer, NULL means calculate required size
                    0,                      // Buffer size, 0 means calculate
                    NULL,                   // Default character, usually NULL
                    NULL                    // Whether to use default character, usually NULL
                );

                if (utf8Size > 0) {
                    // Allocate sufficient memory to store UTF-8 string
                    char* utf8Name = (char*)malloc(utf8Size);
                    if (utf8Name) {
                        WideCharToMultiByte(
                            CP_UTF8, 0, varName.pwszVal, -1,
                            utf8Name, utf8Size, NULL, NULL
                        );

                        // Process UTF-8 string
                        printf("  Name (UTF-8): %s\n", utf8Name);

                        int result = std::strcmp(utf8Name, deviceName);
                        // Free memory
                        free(utf8Name);
                        if (result == 0) {
                            VariantClear((VARIANTARG*)(&varName));
                            PropVariantClear(&varName);
                            goto Exit;
                        }
                    }
                }

                // Free VARIANT variable which needs to be cleared
                VariantClear((VARIANTARG*)(&varName));
            }
/*
            // Get device interface name
            hr = pProps->GetValue(PKEY_DeviceInterface_FriendlyName, &varName);
            if (SUCCEEDED(hr)) {
                wprintf(L"  Interface: %s\n", varName.pwszVal);
            }
*/
            PropVariantClear(&varName);
            pProps->Release();
            pProps = NULL;
        }

        pDevice->Release();
        pDevice = NULL;
    }

Exit:
    SAFE_RELEASE(pProps);
    SAFE_RELEASE(pCollection);

    return pDevice;
}

int HJPluginAudioWASRender::initRender(const HJAudioInfo::Ptr& i_audioInfo)
{
    int ret{ HJ_OK };
    do {
        // Initialize COM
        HRESULT hr = CoInitialize(nullptr);
        if (FAILED(hr)) {
            HJFLoge("{}, CoInitialize failed with HRESULT: 0x{:x}", getName(), hr);
            ret = HJErrFatal;
            break;
        }
        m_beCoInitialize = true;

        // Create device enumerator
        hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
            __uuidof(IMMDeviceEnumerator), (void**)&m_deviceEnumerator);
        if (FAILED(hr)) {
            HJFLoge("{}, CoCreateInstance for MMDeviceEnumerator failed with HRESULT: 0x{:x}", getName(), hr);
            ret = HJErrFatal;
            break;
        }

        // Register device notification client
        m_notificationClient = new DeviceNotificationClient(this);
        hr = m_deviceEnumerator->RegisterEndpointNotificationCallback(m_notificationClient);
        if (FAILED(hr)) {
            HJFLoge("{}, CoCreateInstance RegisterEndpointNotificationCallback failed with HRESULT: 0x{:x}", getName(), hr);
            ret = HJErrFatal;
            break;
        }

        ret = initDevice(i_audioInfo);
    } while (false);
    if (ret != HJ_OK) {
        releaseRender();
    }

    return ret;
}

void HJPluginAudioWASRender::releaseRender()
{
    // Unregister device notification client
    if (m_deviceEnumerator && m_notificationClient) {
        m_deviceEnumerator->UnregisterEndpointNotificationCallback(m_notificationClient);
        m_notificationClient->Release();
        m_notificationClient = nullptr;
    }

    releaseDevice();

    if (m_deviceEnumerator) {
        m_deviceEnumerator->Release();
        m_deviceEnumerator = nullptr;
    }

    if (m_beCoInitialize) {
        CoUninitialize();
        m_beCoInitialize = false;
    }
}

int HJPluginAudioWASRender::runTask(int64_t* o_delay)
{
    auto log = logRunTask();
    if (log) {
        RUNTASKLog("{}, enter", getName());
    }

    std::string route{};
    int64_t size{ -1 };
    int ret{ HJ_OK };
    do {
        BYTE* data{};
        UINT32 numFramesRequested{};
        std::tie(ret, numFramesRequested, data) = getAudioBuffer(route);
        if (ret != HJ_OK) {
            route += "_0";
            break;
        }
        if (numFramesRequested <= 0) {
            ret = HJ_WOULD_BLOCK;
            route += "_1";
            break;
        }

        int32_t dataSize = static_cast<int32_t>(numFramesRequested * m_blockAlign);
        int32_t validSize{};
        HJMediaFrame::Ptr kernalFrame{};
        std::tie(ret, validSize, kernalFrame) = fillAudioBuffer(route, static_cast<void*>(data), dataSize, size);
        if (ret != HJ_OK) {
            route += "_2";
            releaseAudioBuffer(route, 0);
            break;
        }
        if (validSize < dataSize) {
            route += "_3";
            memset(data + validSize, 0, dataSize - validSize);
        }
        else if (kernalFrame != nullptr) {
            route += "_4";
            store(m_inputKeyHash.load(), kernalFrame);
        }

        applyOutputVolume(data, dataSize);

        if (m_pcmCallback) {
            route += "_5";
            if (m_submittedPCMCache.size() < static_cast<size_t>(dataSize)) {
                route += "_6";
                m_submittedPCMCache.resize(static_cast<size_t>(dataSize));
            }
            memcpy(m_submittedPCMCache.data(), data, static_cast<size_t>(dataSize));
        }

        ret = releaseAudioBuffer(route, static_cast<int>(numFramesRequested));
        if (ret == HJ_OK && m_pcmCallback) {
            route += "_7";
            appendPCMCallbackData(m_submittedPCMCache.data(), dataSize);
        }

        route += "_8";
    } while (false);

    if (log) {
        RUNTASKLog("{}, leave, route({}), size({}), task duration({}), ret({})", getName(), route, size, (HJCurrentSteadyMS() - m_enterTimestamp), ret);
    }
    return ret;
}

int HJPluginAudioWASRender::resetDevice(const std::string& i_deviceName)
{
    return SYNC_PROD_LOCK([this, i_deviceName] {
        CHECK_DONE_STATUS(HJErrAlreadyDone);

        m_audioDeviceName = i_deviceName;

        Wtr wRender = SHARED_FROM_THIS;
        if (m_handler && m_handler->async([wRender] {
            auto render = wRender.lock();
            if (render) {
                render->runReset();
            }
        })) {
            return HJ_OK;
        }

        return HJErrFatal;
    });
}

void HJPluginAudioWASRender::runReset()
{
    auto ret = SYNC_PROD_LOCK([this] {
        CHECK_DONE_STATUS(HJErrAlreadyDone);

        releaseDevice();

        auto ret = initDevice(m_audioInfo);
        if (ret != HJ_OK) {
            return ret;
        }
        if (!m_paused && m_running) {
            if (setStreamRunning(true) != HJ_OK) {
                m_running = false;
                return HJErrFatal;
            }
        }

        return HJ_OK;
    });
    if (ret == HJ_OK) {
        IF_FALSE_RETURN(setStatus(HJSTATUS_Ready));
    }
    else if (ret < 0 && ret != HJErrAlreadyDone) {
        IF_FALSE_RETURN(setStatus(HJSTATUS_Exception));

        report(EVENT_PLUGIN_NOTIFY_ID, HJ_PLUGIN_NOTIFY_ERROR_AUDIORENDER_INIT, getID());
    }
}

void HJPluginAudioWASRender::postEvent()
{
    auto [handler, runTaskId] = m_handlerSync.consLock([this] {
        return std::make_tuple(m_handler, m_runTaskId);
    });

    if (handler != nullptr) {
        Wtr wPlugin = SHARED_FROM_THIS;
        handler->asyncAndClear([wPlugin] {
            auto plugin = wPlugin.lock();
            if (plugin) {
                int64_t delay = 0;
                plugin->runTask(&delay);
            }
        }, runTaskId);
    }
}


int HJPluginAudioWASRender::initDevice(const HJAudioInfo::Ptr& i_audioInfo)
{
    m_clockFreq = 0;

    const char* deviceName = m_audioDeviceName.empty() ? nullptr : m_audioDeviceName.c_str();
    m_device = getAudioDevice(m_deviceEnumerator, deviceName);
    if (!m_device) {
        HJFLoge("{}, getAudioDevice failed", getName());
        return HJErrFatal;
    }
/*
    // Get the default audio rendering device
    hr = m_deviceEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &m_device);
    if (FAILED(hr)) {
        HJFLoge("{}, GetDefaultAudioEndpoint failed with HRESULT: 0x{:x}", getName(), hr);
        return HJErrFatal;
    }
*/
    // Activate audio client
    HRESULT hr = m_device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&m_audioClient);
    if (FAILED(hr)) {
        HJFLoge("{}, Activate IAudioClient failed with HRESULT: 0x{:x}", getName(), hr);
        return HJErrFatal;
    }

    // Calculate block alignment
    m_blockAlign = i_audioInfo->m_channels * i_audioInfo->m_bytesPerSample;
    if (m_blockAlign <= 0) {
        return HJErrFatal;
    }

    WAVEFORMATEX* pwfx = (WAVEFORMATEX*)CoTaskMemAlloc(sizeof(WAVEFORMATEX));
    if (!pwfx) {
        HJFLoge("{}, Failed to allocate memory for WAVEFORMATEX", getName());
        return HJErrFatal;
    }

    pwfx->wFormatTag = WAVE_FORMAT_PCM;
    pwfx->nChannels = static_cast<WORD>(i_audioInfo->m_channels);
    pwfx->nSamplesPerSec = static_cast<DWORD>(i_audioInfo->m_samplesRate);
    pwfx->wBitsPerSample = static_cast<WORD>(i_audioInfo->m_bytesPerSample * 8);
    pwfx->nBlockAlign = static_cast<WORD>(m_blockAlign);
    pwfx->nAvgBytesPerSec = static_cast<DWORD>(pwfx->nSamplesPerSec * m_blockAlign);
    pwfx->cbSize = 0;
/*
    WAVEFORMATEX* pwfx2 = NULL;
    hr = m_audioClient->GetMixFormat(&pwfx2);
    if (FAILED(hr)) {
        HJFLoge("{}, GetMixFormat IAudioClient failed with HRESULT: 0x{:x}", getName(), hr);
        return HJErrFatal;
    }

    // Calculate buffer size (~100ms)
    m_hnsRequestedDuration = 1000000; // 100ms

    // Check if format is supported
    WAVEFORMATEX* pClosestMatch = nullptr;
    hr = m_audioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, pwfx, &pClosestMatch);
    if (hr == S_FALSE) {
        HJFLoge("{}, Requested format not supported", getName());
        CoTaskMemFree(pwfx);
        if (pClosestMatch) {
            CoTaskMemFree(pClosestMatch);
        }
        return HJErrFatal;
    }
*/
    // Initialize audio client
    hr = m_audioClient->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,
        m_hnsRequestedDuration,
        0,
        pwfx,
        nullptr);
    CoTaskMemFree(pwfx);
    if (FAILED(hr)) {
        HJFLoge("{}, IAudioClient::Initialize failed with HRESULT: 0x{:x}", getName(), hr);
        return HJErrFatal;
    }
/*
    REFERENCE_TIME hnsLatency;
    hr = m_audioClient->GetStreamLatency(&hnsLatency);
    if (FAILED(hr)) {
        HJFLoge("{}, IAudioClient::GetBufferSize failed with HRESULT: 0x{:x}", getName(), hr);
        return HJErrFatal;
    }
*/
    // Get buffer size
    hr = m_audioClient->GetBufferSize(&m_bufferFrameCount);
    if (FAILED(hr)) {
        HJFLoge("{}, IAudioClient::GetBufferSize failed with HRESULT: 0x{:x}", getName(), hr);
        return HJErrFatal;
    }

    // Get render client
    hr = m_audioClient->GetService(__uuidof(IAudioRenderClient), (void**)&m_renderClient);
    if (FAILED(hr)) {
        HJFLoge("{}, IAudioClient::GetService failed with HRESULT: 0x{:x}", getName(), hr);
        return HJErrFatal;
    }

    hr = m_audioClient->GetService(__uuidof(IAudioClock), (void**)&m_audioClock);
    if (FAILED(hr)) {
        HJFLogd("{}, IAudioClient::GetService(IAudioClock) failed with HRESULT: 0x{:x}", getName(), hr);
        m_audioClock = nullptr;
    }
    else {
        hr = m_audioClock->GetFrequency(&m_clockFreq);
        if (FAILED(hr) || m_clockFreq == 0) {
            HJFLogd("{}, IAudioClock::GetFrequency failed with HRESULT: 0x{:x}", getName(), hr);
            SAFE_RELEASE(m_audioClock);
            m_clockFreq = 0;
        }
    }

    // Set event callback
    hr = m_audioClient->SetEventHandle(m_audioEvent[0]);
    if (FAILED(hr)) {
        HJFLoge("{}, IAudioClient::SetEventHandle failed with HRESULT: 0x{:x}", getName(), hr);
        return HJErrFatal;
    }

    return HJ_OK;
}

void HJPluginAudioWASRender::releaseDevice()
{
    discardPendingPCMCallbackData();

    (void)setStreamRunning(false, true);

    // Release all resources
    SAFE_RELEASE(m_renderClient);
    SAFE_RELEASE(m_audioClock);
    SAFE_RELEASE(m_audioClient);
    SAFE_RELEASE(m_device);

    m_clockFreq = 0;
    m_submittedPCMCache.clear();
}

int HJPluginAudioWASRender::setStreamRunning(bool i_running, bool i_eofStop)
{
    if (!m_audioClient) {
        return HJ_OK;
    }

    if (i_running) {
        const auto hr = m_audioClient->Start();
        if (FAILED(hr)) {
            HJFLoge("{}, IAudioClient::Start failed with HRESULT: 0x{:x}", getName(), hr);
            return HJErrFatal;
        }
        return HJ_OK;
    }

    m_audioClient->Stop();
    if (i_eofStop) {
        const HRESULT hr = m_audioClient->Reset();
        if (FAILED(hr)) {
            HJFLoge("{}, IAudioClient::Reset failed with HRESULT: 0x{:x}", getName(), hr);
            return HJErrFatal;
        }
    }
    return HJ_OK;
}

std::tuple<int, UINT32, BYTE*> HJPluginAudioWASRender::getAudioBuffer(std::string& route)
{
    UINT32 numFramesRequested{};
    BYTE* data{};
    auto ret = SYNC_CONS_LOCK([this, &route, &numFramesRequested, &data] {
        if (m_status == HJSTATUS_Done) {
            route += "_10";
            return HJErrAlreadyDone;
        }
        if (m_status < HJSTATUS_Ready) {
            route += "_11";
            return HJ_WOULD_BLOCK;
        }
        if (m_status >= HJSTATUS_EOF) {
            route += "_12";
            return HJ_WOULD_BLOCK;
        }
        if (!m_audioClient || !m_renderClient) {
            route += "_13";
            return HJErrFatal;
        }

        UINT32 numPaddingFrames;
        if (FAILED(m_audioClient->GetCurrentPadding(&numPaddingFrames))) {
            route += "_14";
            return HJErrFatal;
        }

        // Get available buffer space
        numFramesRequested = m_bufferFrameCount - numPaddingFrames;
        if (numFramesRequested > 0) {
            route += "_15";
            if (FAILED(m_renderClient->GetBuffer(numFramesRequested, &data))) {
                route += "_16";
                return HJErrFatal;
            }
        }

        route += "_17";
        return HJ_OK;
    });
    if (ret == HJErrFatal) {
        report(EVENT_PLUGIN_NOTIFY_ID, HJ_PLUGIN_NOTIFY_ERROR_AUDIORENDER_RUNTASK, getID());
    }
    else if (ret == HJ_WOULD_BLOCK) {
        ret = HJ_OK;
    }

    return std::make_tuple(ret, numFramesRequested, data);
}


int HJPluginAudioWASRender::releaseAudioBuffer(std::string& route, int numFramesRequested)
{
    auto ret = SYNC_CONS_LOCK([this, &route, numFramesRequested] {
        if (m_status == HJSTATUS_Done) {
            route += "_50";
            return HJErrAlreadyDone;
        }
        if (!m_renderClient) {
            route += "_51";
            return HJErrFatal;
        }

        DWORD flags = 0;// m_muted ? AUDCLNT_BUFFERFLAGS_SILENT : 0;
        if (FAILED(m_renderClient->ReleaseBuffer(numFramesRequested, flags))) {
            route += "_52";
            return HJErrFatal;
        }

        route += "_53";
        return HJ_OK;
    });
    if (ret == HJErrFatal) {
        report(EVENT_PLUGIN_NOTIFY_ID, HJ_PLUGIN_NOTIFY_ERROR_AUDIORENDER_RUNTASK, getID());
    }
    else if (ret == HJ_OK && numFramesRequested > 0) {
        // Anchor the render clock on the first successful write.
//        (void)getRenderTimestamp();
    }

    return ret;
}

int64_t HJPluginAudioWASRender::getRenderTimestamp()
{
    if (!m_audioClock || m_clockFreq == 0) {
        return int64_t(0);
    }

    UINT64 position = 0;
    UINT64 qpcPosition = 0;
    HRESULT hr = m_audioClock->GetPosition(&position, &qpcPosition);
    if (FAILED(hr)) {
        return int64_t(0);
    }

    return static_cast<int64_t>((position * 1000ULL) / m_clockFreq);
}

NS_HJ_END
