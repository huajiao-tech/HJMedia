#include "HJPluginAudioWASRender.h"
#include "HJFLog.h"
#include "HJFFHeaders.h"

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
/*
    if (riid == IID_IUnknown || riid == __uuidof(IMMNotificationClient)) {
        *ppvObject = static_cast<IMMNotificationClient*>(this);
        AddRef();
        return S_OK;
    }
*/
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
    // 当默认音频渲染设备发生变化时
    if (flow == eRender && role == eMultimedia) {
        // 触发重新初始化流程
        if (m_parent) {
            // 可能需要通过消息机制通知主线程重新初始化音频设备
            // 因为这个回调是在系统线程中执行的
            m_parent->onDefaultDeviceChanged();
        }
    }
    return S_OK;
}

STDMETHODIMP HJPluginAudioWASRender::DeviceNotificationClient::OnPropertyValueChanged(LPCWSTR pwstrDeviceId, const PROPERTYKEY key)
{
    return S_OK;
}

HJPluginAudioWASRender::HJPluginAudioWASRender(const std::string& i_name, HJKeyStorage::Ptr i_graphInfo)
    : HJPluginAudioRender(i_name, i_graphInfo)
{
    m_audioEvent[0] = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    m_audioEvent[1] = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    m_audioEvent[2] = CreateEvent(nullptr, TRUE, FALSE, nullptr);
}

HJPluginAudioWASRender::~HJPluginAudioWASRender()
{
    HJPluginAudioWASRender::done();

    CloseHandle(m_audioEvent[0]);
    CloseHandle(m_audioEvent[1]);
    CloseHandle(m_audioEvent[2]);
}

int HJPluginAudioWASRender::done()
{
    SetEvent(m_audioEvent[1]);

    return HJPluginAudioRender::done();
}

#include <Functiondiscoverykeys_devpkey.h>
#define SAFE_RELEASE(p) { if ((p) != NULL) { (p)->Release(); (p) = NULL; } }

IMMDevice* getAudioDevice(IMMDeviceEnumerator* pEnumerator, const char* deviceName) {
    HRESULT hr = S_OK;
    IMMDevice* pDevice = NULL;

    if (!deviceName) {
        // 获取默认音频输出设备
        hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &pDevice);
        if (FAILED(hr)) {
            return nullptr;
        }
        return pDevice;
    }

    IMMDeviceCollection* pCollection = NULL;
    IPropertyStore* pProps = NULL;

    // 2. 获取所有音频渲染(输出)设备
    hr = pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pCollection);
    if (FAILED(hr)) goto Exit;

    // 3. 获取设备数量
    UINT count;
    hr = pCollection->GetCount(&count);
    if (FAILED(hr)) goto Exit;

    // 4. 遍历设备
    for (UINT i = 0; i < count; i++) {
        // 获取设备指针
        hr = pCollection->Item(i, &pDevice);
        if (FAILED(hr)) continue;
/*
        // 获取设备ID
        LPWSTR pwszID = NULL;
        hr = pDevice->GetId(&pwszID);
        if (SUCCEEDED(hr)) {
            wprintf(L"Device %d ID: %s\n", i, pwszID);
            CoTaskMemFree(pwszID);
        }
*/
        // 打开属性存储
        hr = pDevice->OpenPropertyStore(STGM_READ, &pProps);
        if (SUCCEEDED(hr)) {
            PROPVARIANT varName;
            PropVariantInit(&varName);

            // 获取设备友好名称
            hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
            if (SUCCEEDED(hr)) {
//                setlocale(LC_ALL, ""); // 使用系统默认区域设置
//                wprintf(L"  Name: %s\n", varName.pwszVal);
                int utf8Size = WideCharToMultiByte(
                    CP_UTF8,                // 目标编码：UTF-8
                    0,                      // 标志（一般填 0）
                    varName.pwszVal,        // 输入的宽字符串
                    -1,                     // 自动计算长度（-1 表示以 NULL 结尾）
                    NULL,                   // 输出缓冲区（NULL 表示计算所需大小）
                    0,                      // 输出缓冲区大小（0 表示计算）
                    NULL,                   // 默认字符（一般 NULL）
                    NULL                    // 是否使用默认字符（一般 NULL）
                );

                if (utf8Size > 0) {
                    // 分配足够的内存存储 UTF-8 字符串
                    char* utf8Name = (char*)malloc(utf8Size);
                    if (utf8Name) {
                        WideCharToMultiByte(
                            CP_UTF8, 0, varName.pwszVal, -1,
                            utf8Name, utf8Size, NULL, NULL
                        );

                        // 输出 UTF-8 字符串
                        printf("  Name (UTF-8): %s\n", utf8Name);

                        int result = std::strcmp(utf8Name, deviceName);
                        // 释放内存
                        free(utf8Name);
                        if (result == 0) {
                            VariantClear((VARIANTARG*)(&varName));
                            PropVariantClear(&varName);
                            goto Exit;
                        }
                    }
                }

                // 释放 VARIANT（如果不再需要）
                VariantClear((VARIANTARG*)(&varName));
            }
/*
            // 获取设备接口名称
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
    // 初始化COM
    HRESULT hr = CoInitialize(nullptr);
    if (FAILED(hr)) {
        HJFLoge("{}, CoInitialize failed with HRESULT: 0x{:x}", getName(), hr);
        return HJErrFatal;
    }

    // 创建设备枚举器
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator), (void**)&m_deviceEnumerator);
    if (FAILED(hr)) {
        HJFLoge("{}, CoCreateInstance for MMDeviceEnumerator failed with HRESULT: 0x{:x}", getName(), hr);
        return HJErrFatal;
    }

    // 创建并注册设备通知客户端
    m_notificationClient = new DeviceNotificationClient(this);
    m_deviceEnumerator->RegisterEndpointNotificationCallback(m_notificationClient);

    return initDevice(i_audioInfo);
}

bool HJPluginAudioWASRender::releaseRender()
{
    // 注销设备通知客户端
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

    CoUninitialize();

    return true;
}

int HJPluginAudioWASRender::runTask(int64_t* o_delay)
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
    int ret = HJ_OK;
    do {
        // 等待音频事件
        DWORD waitResult = WaitForMultipleObjects(3, m_audioEvent, FALSE, INFINITE);
        if (waitResult == WAIT_OBJECT_0) {
            route += HJFMT("_0({})", HJCurrentSteadyMS() - enter);
        }
        else if (waitResult == WAIT_OBJECT_0 + 1) {
            route += "_1";
            ret = HJ_STOP;
            break;
        }
        else if (waitResult == WAIT_OBJECT_0 + 2) {
            route += "_2";
            ret = HJ_STOP;
            break;
        }
        else {
            route += "_3";
            break;
        }

        HJMediaFrame::Ptr kernalBuffer{};
        UINT32 numFramesRequested{};
        BYTE* data{};
        ret = SYNC_CONS_LOCK([&route, &kernalBuffer, &numFramesRequested, &data, this] {
            if (m_status == HJSTATUS_Done) {
                route += "_20";
                return HJErrAlreadyDone;
            }
            if (m_status >= HJSTATUS_EOF) {
                route += "_21";
                return HJ_WOULD_BLOCK;
            }

            UINT32 numPaddingFrames;
            auto hr = m_audioClient->GetCurrentPadding(&numPaddingFrames);
            if (FAILED(hr)) {
                route += "_22";
                if (m_pluginListener) {
                    route += "_23";
                    m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_ERROR_AUDIORENDER_RUNTASK)));
                }
                return HJErrFatal;
            }

            // 获取数据缓冲区
            numFramesRequested = m_bufferFrameCount - numPaddingFrames;
            if (numFramesRequested > 0) {
                route += "_24";
                hr = m_renderClient->GetBuffer(numFramesRequested, &data);
                if (FAILED(hr)) {
                    route += "_25";
                    if (m_pluginListener) {
                        route += "_26";
                        m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_ERROR_AUDIORENDER_RUNTASK)));
                    }
                    return HJErrFatal;
                }
            }

            kernalBuffer = m_kernalFrame;
            return HJ_OK;
        });
        if (ret != HJ_OK) {
            break;
        }
        if (numFramesRequested <= 0) {
            route += "_4";
            break;
        }

        size_t inputKeyHash = m_inputKeyHash.load();
        size_t dataSize = static_cast<size_t>(numFramesRequested * m_blockAlign);
        size_t bufferSize{};
        while (dataSize > 0) {
            route += "_5";
            if (!kernalBuffer) {
                route += "_6";
                kernalBuffer = receive(inputKeyHash, &size, &audioDuration, nullptr, &audioSamples);
                if (!kernalBuffer) {
                    route += "_7";
                    if (!m_buffering) {
                        route += "_8";
                        m_buffering = true;
                        if (m_pluginListener) {
                            route += "_9";
                            m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_AUDIORENDER_START_BUFFERING)));
                        }
                    }

                    memset(data, 0, dataSize);
                    break;
                }

                setInfoAudioDuration(audioSamples);

                if (m_pluginListener) {
                    route += "_10";
                    auto notify = HJMakeNotification(HJ_PLUGIN_NOTIFY_AUDIORENDER_FRAME);
                    (*notify)["frame"] = kernalBuffer;
                    m_pluginListener(std::move(notify));
                }

                if (m_buffering) {
                    route += "_11";
                    m_buffering = false;
                    if (m_pluginListener) {
                        route += "_12";
                        m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_AUDIORENDER_STOP_BUFFERING)));
                    }
                }

                if (kernalBuffer->isEofFrame()) {
                    route += "_13";
                    if (m_pluginListener) {
                        route += "_14";
                        m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_AUDIORENDER_EOF)));
                    }
                    setStatus(HJSTATUS_EOF);
                    break;
                }
            }

            HJAVFrame::Ptr avFrame = kernalBuffer->getMFrame();
            AVFrame* frame = avFrame->getAVFrame();

            bufferSize = static_cast<size_t>(frame->nb_samples * m_blockAlign);
            size_t copySize = std::min<size_t>(dataSize, bufferSize - kernalBuffer->m_bufferPos);
            memcpy(data, frame->data[0] + kernalBuffer->m_bufferPos, copySize);
            data += copySize;
            dataSize -= copySize;
            kernalBuffer->m_bufferPos += copySize;

            if (kernalBuffer->m_bufferPos >= bufferSize) {
                route += "_15";
                ret = SYNC_CONS_LOCK([&route, kernalBuffer, this] {
                    if (m_status == HJSTATUS_Done) {
                        route += "_30";
                        return HJErrAlreadyDone;
                    }

                    m_timeline->setTimestamp(kernalBuffer->m_streamIndex, kernalBuffer->getPTS(), kernalBuffer->getSpeed());
                    return HJ_OK;
                    });
                if (ret != HJ_OK) {
                    break;
                }

                kernalBuffer = nullptr;
            }
        }

        ret = SYNC_CONS_LOCK([&route, numFramesRequested, kernalBuffer, this] {
            if (m_status == HJSTATUS_Done) {
                route += "_40";
                return HJErrAlreadyDone;
            }
            if (m_status >= HJSTATUS_EOF) {
                route += "_41";
                return HJ_WOULD_BLOCK;
            }

            DWORD flags = m_muted ? AUDCLNT_BUFFERFLAGS_SILENT : 0;
            auto hr = m_renderClient->ReleaseBuffer(numFramesRequested, flags);
            if (FAILED(hr)) {
                route += "_42";
                if (m_pluginListener) {
                    route += "_43";
                    m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_ERROR_AUDIORENDER_RUNTASK)));
                }
                return HJErrFatal;
            }

            m_kernalFrame = kernalBuffer;
            return HJ_OK;
        });
    } while (false);

    if (log) {
        RUNTASKLog("{}, leave, route({}), size({}), task duration({}), ret({})",
            getName(), route, size, (HJCurrentSteadyMS() - enter), ret);
    }
    return ret;
}

int HJPluginAudioWASRender::resetDevice(const std::string& i_deviceName)
{
    return SYNC_PROD_LOCK([=] {
        CHECK_DONE_STATUS(HJErrAlreadyDone);

        m_audioDeviceName = i_deviceName;

        SetEvent(m_audioEvent[2]);
        if (m_handler->async([=] {
            auto ret = SYNC_PROD_LOCK([=] {
                CHECK_DONE_STATUS(HJErrAlreadyDone);

                releaseDevice();
                ResetEvent(m_audioEvent[2]);

                auto err = initDevice(m_audioInfo);
                if (err < 0) {
                    setStatus(HJSTATUS_Exception, false);
                }
                else if (err == HJ_OK) {
                    setStatus(HJSTATUS_Ready, false);
                }
                return err;
             });
            if (ret == HJ_OK) {
                postTask();
            }
            else if (ret < 0 && ret != HJErrAlreadyDone) {
                if (m_pluginListener) {
                    m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_ERROR_AUDIORENDER_INIT)));
                }
            }
        })) {
            return HJ_OK;
        }

        return HJErrFatal;
    });
}

void HJPluginAudioWASRender::onDefaultDeviceChanged() {
    std::string deviceName;
    SYNC_CONS_LOCK([&deviceName, this] {
        deviceName = m_audioDeviceName;
    });

    if (deviceName.empty()) {
        HJFLogd("{}, default device changed, reset device", getName());
        resetDevice();
    }
}

int HJPluginAudioWASRender::initDevice(const HJAudioInfo::Ptr& i_audioInfo)
{
    const char* deviceName = m_audioDeviceName.empty() ? nullptr : m_audioDeviceName.c_str();
    m_device = getAudioDevice(m_deviceEnumerator, deviceName);
    if (!m_device) {
        HJFLoge("{}, getAudioDevice failed", getName());
        return HJErrFatal;
    }
/*
    // 获取默认音频输出设备
    hr = m_deviceEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &m_device);
    if (FAILED(hr)) {
        HJFLoge("{}, GetDefaultAudioEndpoint failed with HRESULT: 0x{:x}", getName(), hr);
        return HJErrFatal;
    }
*/
    // 激活音频客户端
    HRESULT hr = m_device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&m_audioClient);
    if (FAILED(hr)) {
        HJFLoge("{}, Activate IAudioClient failed with HRESULT: 0x{:x}", getName(), hr);
        return HJErrFatal;
    }

    // 设置音频格式
    m_blockAlign = static_cast<WORD>(i_audioInfo->m_channels * i_audioInfo->m_bytesPerSample);

    WAVEFORMATEX* pwfx = (WAVEFORMATEX*)CoTaskMemAlloc(sizeof(WAVEFORMATEX));
    if (!pwfx) {
        HJFLoge("{}, Failed to allocate memory for WAVEFORMATEX", getName());
        return HJErrFatal;
    }

    pwfx->wFormatTag = WAVE_FORMAT_PCM;
    pwfx->nChannels = static_cast<WORD>(i_audioInfo->m_channels);
    pwfx->nSamplesPerSec = static_cast<DWORD>(i_audioInfo->m_samplesRate);
    pwfx->wBitsPerSample = static_cast<WORD>(i_audioInfo->m_bytesPerSample * 8);
    pwfx->nBlockAlign = m_blockAlign;
    pwfx->nAvgBytesPerSec = pwfx->nSamplesPerSec * m_blockAlign;
    pwfx->cbSize = 0;
/*
    WAVEFORMATEX* pwfx2 = NULL;
    hr = m_audioClient->GetMixFormat(&pwfx2);
    if (FAILED(hr)) {
        HJFLoge("{}, GetMixFormat IAudioClient failed with HRESULT: 0x{:x}", getName(), hr);
        return HJErrFatal;
    }

    // 计算缓冲区大小 (约100ms)
    m_hnsRequestedDuration = 1000000; // 100ms

    // 检查是否支持该格式
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
    // 初始化音频客户端
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
    // 获取缓冲区大小
    hr = m_audioClient->GetBufferSize(&m_bufferFrameCount);
    if (FAILED(hr)) {
        HJFLoge("{}, IAudioClient::GetBufferSize failed with HRESULT: 0x{:x}", getName(), hr);
        return HJErrFatal;
    }

    // 获取渲染客户端
    hr = m_audioClient->GetService(__uuidof(IAudioRenderClient), (void**)&m_renderClient);
    if (FAILED(hr)) {
        HJFLoge("{}, IAudioClient::GetService failed with HRESULT: 0x{:x}", getName(), hr);
        return HJErrFatal;
    }

    // 设置事件回调
    hr = m_audioClient->SetEventHandle(m_audioEvent[0]);
    if (FAILED(hr)) {
        HJFLoge("{}, IAudioClient::SetEventHandle failed with HRESULT: 0x{:x}", getName(), hr);
        return HJErrFatal;
    }

    // 启动音频流
    hr = m_audioClient->Start();
    if (FAILED(hr)) {
        HJFLoge("{}, IAudioClient::Start failed with HRESULT: 0x{:x}", getName(), hr);
        return HJErrFatal;
    }

    return HJ_OK;
}

void HJPluginAudioWASRender::releaseDevice()
{
    // 停止当前音频流
    if (m_audioClient) {
        m_audioClient->Stop();
    }

    // 释放现有资源
    SAFE_RELEASE(m_renderClient);
    SAFE_RELEASE(m_audioClient);
    SAFE_RELEASE(m_device);
}

NS_HJ_END
