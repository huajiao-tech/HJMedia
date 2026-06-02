#include "HJDefines.h"
#include "asys/HJAAudioLoader.h"

#if defined(HJ_OS_ANDROID)

#include <dlfcn.h>

namespace {

template <typename T>
bool loadSymbol(void* handle, const char* name, T& out)
{
    out = reinterpret_cast<T>(dlsym(handle, name));
    return out != nullptr;
}

} // namespace

HJAAudioLoader& HJAAudioLoader::instance()
{
    static HJAAudioLoader loader;
    return loader;
}

HJAAudioLoader::HJAAudioLoader()
{
    m_handle = dlopen("libaaudio.so", RTLD_NOW);
    if (!m_handle) {
        m_ok = false;
        return;
    }

    bool ok = true;
    ok = ok && loadSymbol(m_handle, "AAudio_createStreamBuilder", createStreamBuilder);
    ok = ok && loadSymbol(m_handle, "AAudioStreamBuilder_delete", streamBuilderDelete);
    ok = ok && loadSymbol(m_handle, "AAudioStreamBuilder_openStream", streamBuilderOpenStream);
    ok = ok && loadSymbol(m_handle, "AAudioStreamBuilder_setSampleRate", streamBuilderSetSampleRate);
    ok = ok && loadSymbol(m_handle, "AAudioStreamBuilder_setChannelCount", streamBuilderSetChannelCount);
    ok = ok && loadSymbol(m_handle, "AAudioStreamBuilder_setFormat", streamBuilderSetFormat);
    ok = ok && loadSymbol(m_handle, "AAudioStreamBuilder_setSharingMode", streamBuilderSetSharingMode);
    ok = ok && loadSymbol(m_handle, "AAudioStreamBuilder_setPerformanceMode", streamBuilderSetPerformanceMode);
    ok = ok && loadSymbol(m_handle, "AAudioStreamBuilder_setFramesPerDataCallback", streamBuilderSetFramesPerDataCallback);
    ok = ok && loadSymbol(m_handle, "AAudioStreamBuilder_setDataCallback", streamBuilderSetDataCallback);
    ok = ok && loadSymbol(m_handle, "AAudioStreamBuilder_setErrorCallback", streamBuilderSetErrorCallback);

    ok = ok && loadSymbol(m_handle, "AAudioStream_requestStart", streamRequestStart);
    loadSymbol(m_handle, "AAudioStream_requestPause", streamRequestPause);
    ok = ok && loadSymbol(m_handle, "AAudioStream_requestStop", streamRequestStop);
    loadSymbol(m_handle, "AAudioStream_requestFlush", streamRequestFlush);
    ok = ok && loadSymbol(m_handle, "AAudioStream_close", streamClose);
    ok = ok && loadSymbol(m_handle, "AAudioStream_getFramesRead", streamGetFramesRead);

    if (!ok) {
        m_ok = false;
        dlclose(m_handle);
        m_handle = nullptr;
        return;
    }

    m_ok = true;
}

#endif
