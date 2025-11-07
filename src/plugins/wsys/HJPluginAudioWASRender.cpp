#include "HJPluginAudioWASRender.h"
#include "HJFLog.h"
#include "HJFFHeaders.h"

NS_HJ_BEGIN

#define RUNTASKLog		HJFLogi

HJPluginAudioWASRender::HJPluginAudioWASRender(const std::string& i_name, HJKeyStorage::Ptr i_graphInfo)
    : HJPluginAudioRender(i_name, i_graphInfo)
{
    m_audioEvent[0] = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    m_audioEvent[1] = CreateEvent(nullptr, TRUE, FALSE, nullptr);
}

HJPluginAudioWASRender::~HJPluginAudioWASRender()
{
    HJPluginAudioWASRender::done();

    CloseHandle(m_audioEvent[0]);
    CloseHandle(m_audioEvent[1]);
}

int HJPluginAudioWASRender::done()
{
    SetEvent(m_audioEvent[1]);

    return HJPluginAudioRender::done();
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

    // 获取默认音频输出设备
    hr = m_deviceEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &m_device);
    if (FAILED(hr)) {
        HJFLoge("{}, GetDefaultAudioEndpoint failed with HRESULT: 0x{:x}", getName(), hr);
        return HJErrFatal;
    }

    // 激活音频客户端
    hr = m_device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&m_audioClient);
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

bool HJPluginAudioWASRender::releaseRender()
{
    if (m_audioClient) {
        m_audioClient->Stop();
    }

    if (m_renderClient) {
        m_renderClient->Release();
        m_renderClient = nullptr;
    }

    if (m_audioClient) {
        m_audioClient->Release();
        m_audioClient = nullptr;
    }

    if (m_device) {
        m_device->Release();
        m_device = nullptr;
    }

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
    log = true;
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
        DWORD waitResult = WaitForMultipleObjects(2, m_audioEvent, FALSE, INFINITE);
        if (waitResult == WAIT_OBJECT_0) {
            route += HJFMT("_0({})", HJCurrentSteadyMS() - enter);
        }
        else if (waitResult == WAIT_OBJECT_0 + 1) {
            route += "_1";
            ret = HJ_STOP;
            break;
        }
        else {
            route += "_2";
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
                return HJErrFatal;
            }

            // 获取数据缓冲区
            numFramesRequested = m_bufferFrameCount - numPaddingFrames;
            if (numFramesRequested > 0) {
                route += "_23";
                hr = m_renderClient->GetBuffer(numFramesRequested, &data);
                if (FAILED(hr)) {
                    route += "_24";
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
            route += "_3";
            break;
        }

        size_t inputKeyHash = m_inputKeyHash.load();
        size_t dataSize = static_cast<size_t>(numFramesRequested * m_blockAlign);
        size_t bufferSize{};
        while (dataSize > 0) {
            route += "_4";
            if (!kernalBuffer) {
                route += "_5";
                kernalBuffer = receive(inputKeyHash, &size, &audioDuration, nullptr, &audioSamples);
                if (!kernalBuffer) {
                    route += "_6";
                    if (!m_buffering) {
                        route += "_7";
                        m_buffering = true;
                        if (m_pluginListener) {
                            route += "_8";
                            m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_AUDIORENDER_START_BUFFERING)));
                        }
                    }

                    memset(data, 0, dataSize);
                    break;
                }

                setInfoAudioDuration(audioSamples);

                if (m_pluginListener) {
                    route += "_9";
                    auto notify = HJMakeNotification(HJ_PLUGIN_NOTIFY_AUDIORENDER_FRAME);
                    (*notify)["frame"] = kernalBuffer;
                    m_pluginListener(std::move(notify));
                }

                if (m_buffering) {
                    route += "_10";
                    m_buffering = false;
                    if (m_pluginListener) {
                        route += "_11";
                        m_pluginListener(std::move(HJMakeNotification(HJ_PLUGIN_NOTIFY_AUDIORENDER_STOP_BUFFERING)));
                    }
                }

                if (kernalBuffer->isEofFrame()) {
                    route += "_12";
                    if (m_pluginListener) {
                        route += "_13";
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
                route += "_14";
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

NS_HJ_END
