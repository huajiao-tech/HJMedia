#include "HJTextureDenoiseSRRunner.h"

#include "HJFLog.h"
#include "HJError.h"
#include "HJComUtils.h"
#include "HJOGFBOCtrl.h"
#include "HJRteComDraw.h"
#if defined(WIN32_LIB)
#include "render/HJRenderCoreSingleton.h"
#endif
#include <algorithm>

NS_HJ_USING

HJTextureDenoiseSRRunner::HJTextureDenoiseSRRunner()
{
}

HJTextureDenoiseSRRunner::~HJTextureDenoiseSRRunner()
{
    done();
}

int HJTextureDenoiseSRRunner::init(float denoiseStrength, float sharpness, int scaleFactor,
    bool enableExtraSharpen, float extraSharpenBoost)
{
    int i_err = HJ_OK;
    do
    {
#if !defined(WIN32_LIB) && !defined(ANDROID_LIB)
        i_err = HJErrInvalid;
        break;
#else
        done();

#if defined(WIN32_LIB)
        i_err = HJRenderCoreSingleton::Instance().init();
        if (i_err < 0)
        {
            HJFLoge("HJTextureDenoiseSRRunner init render core error:{}", i_err);
            break;
        }
#endif

        m_scaleFactor = std::max(1, scaleFactor);

        HJBaseParam::Ptr denoiseParam = HJBaseParam::Create();
        HJ_CatchMapPlainSetVal(denoiseParam, float, "denoiseStrength", denoiseStrength);
        HJRteComDrawDenoiseFilter::Ptr denoiseFilter = HJRteComDrawDenoiseFilter::Create();
        i_err = denoiseFilter->init(denoiseParam);
        if (i_err < 0)
        {
            HJFLoge("HJTextureDenoiseSRRunner init denoise filter error:{}", i_err);
            break;
        }

        HJBaseParam::Ptr srParam = HJBaseParam::Create();
        HJ_CatchMapPlainSetVal(srParam, float, "sharpness", sharpness);
        HJ_CatchMapPlainSetVal(srParam, bool, "enableExtraSharpen", enableExtraSharpen);
        HJ_CatchMapPlainSetVal(srParam, float, "extraSharpenBoost", extraSharpenBoost);
        HJRteComDrawSRFilter::Ptr srFilter = HJRteComDrawSRFilter::Create();
        i_err = srFilter->init(srParam);
        if (i_err < 0)
        {
            HJFLoge("HJTextureDenoiseSRRunner init sr filter error:{}", i_err);
            break;
        }
        srFilter->setScaleFactor(m_scaleFactor);
        srFilter->setDynamicScaleRatio(false);

        m_denoiseFilter = denoiseFilter;
        m_srFilter = srFilter;
        m_inited = true;
#endif
    } while (false);

    if (i_err < 0)
    {
        done();
    }
    return i_err;
}

int HJTextureDenoiseSRRunner::process(
    uint32_t inputTextureId,
    int inputWidth,
    int inputHeight,
    uint32_t& outTextureId,
    int& outWidth,
    int& outHeight,
    int64_t* outElapsedMs)
{
    outTextureId = 0;
    outWidth = 0;
    outHeight = 0;

    if (!m_inited || !m_denoiseFilter || !m_srFilter)
    {
        return HJErrNotInited;
    }
    if (inputTextureId == 0 || inputWidth <= 0 || inputHeight <= 0)
    {
        return HJErrInvalidParams;
    }

    const int64_t startMs = HJCurrentSteadyMS();

    const int outputWidth = std::max(1, inputWidth * std::max(1, m_scaleFactor));
    const int outputHeight = std::max(1, inputHeight * std::max(1, m_scaleFactor));

    int i_err = priEnsureFbo(m_denoiseFbo, inputWidth, inputHeight);
    if (i_err < 0)
    {
        return i_err;
    }
    i_err = priEnsureFbo(m_outputFbo, outputWidth, outputHeight);
    if (i_err < 0)
    {
        return i_err;
    }

    m_denoiseFbo->attach();
    i_err = m_denoiseFilter->draw(inputTextureId, HJWindowRenderMode_CLIP, inputWidth, inputHeight, inputWidth, inputHeight, nullptr, false, false);
    m_denoiseFbo->detach();
    if (i_err < 0)
    {
        return i_err;
    }

    m_outputFbo->attach();
    //after w*h<360x640 use denoise, else not use denoise;
    //i_err = m_srFilter->draw(inputTextureId, HJWindowRenderMode_CLIP, inputWidth, inputHeight, outputWidth, outputHeight, nullptr, false, false);
    i_err = m_srFilter->draw(m_denoiseFbo->getTextureId(), HJWindowRenderMode_CLIP, inputWidth, inputHeight, outputWidth, outputHeight, nullptr, false, false);
    m_outputFbo->detach();
    if (i_err < 0)
    {
        return i_err;
    }

    outTextureId = m_outputFbo->getTextureId();
    outWidth = outputWidth;
    outHeight = outputHeight;
    if (outElapsedMs)
    {
        *outElapsedMs = HJCurrentSteadyMS() - startMs;
    }
    return HJ_OK;
}

void HJTextureDenoiseSRRunner::done()
{
    m_inited = false;
    if (m_denoiseFilter)
    {
        m_denoiseFilter->done();
    }
    if (m_srFilter)
    {
        m_srFilter->done();
    }
    m_denoiseFilter = nullptr;
    m_srFilter = nullptr;
    m_denoiseFbo = nullptr;
    m_outputFbo = nullptr;
}

int HJTextureDenoiseSRRunner::priEnsureFbo(std::shared_ptr<HJOGFBOCtrl>& io_fbo, int width, int height)
{
    if (width <= 0 || height <= 0)
    {
        return HJErrInvalidParams;
    }
    if (io_fbo && io_fbo->getWidth() == width && io_fbo->getHeight() == height)
    {
        return HJ_OK;
    }

    io_fbo = HJOGFBOCtrl::Create();
    return io_fbo->init(width, height, true);
}
