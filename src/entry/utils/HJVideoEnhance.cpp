#include "HJVideoEnhance.h"

#include "HJFLog.h"
#include "HJError.h"
#include "HJComUtils.h"
#include "HJOGFBOCtrl.h"
#include "HJRteComDraw.h"
#if defined(WIN32_LIB)
#include "render/HJRenderCoreSingleton.h"
#endif

#include <algorithm>
#include <cmath>

NS_HJ_USING

HJVideoEnhance::HJVideoEnhance()
{
}

HJVideoEnhance::~HJVideoEnhance()
{
    done();
}

int HJVideoEnhance::init(const HJVideoEnhanceOption& i_option)
{
    return priInitWithOption(i_option);
}

int HJVideoEnhance::adjustParam(const HJVideoEnhanceOption& i_option)
{
    HJFLogi("HJVideoEnhance adjustParam enter enableDenoise:{} enableSR:{} denoiseStrength:{} sharpness:{} extraSharpen:{} extraSharpenBoost:{} match:{}",
        i_option.enableDenoise,
        i_option.enableSR,
        i_option.denoiseStrength,
        i_option.sharpness,
        i_option.enableExtraSharpen,
        i_option.extraSharpenBoost,
        i_option.match);
    m_option = i_option;

    if (m_srFilter)
    {
        HJBaseParam::Ptr srParam = HJBaseParam::Create();
        HJ_CatchMapPlainSetVal(srParam, float, "sharpness", m_option.sharpness);
        HJ_CatchMapPlainSetVal(srParam, bool, "enableExtraSharpen", m_option.enableExtraSharpen);
        HJ_CatchMapPlainSetVal(srParam, float, "extraSharpenBoost", m_option.extraSharpenBoost);
        HJ_CatchMapPlainSetVal(srParam, float, "match", m_option.match);
        const int i_err = m_srFilter->setParam(srParam);
        if (i_err < 0)
        {
            HJFLoge("HJVideoEnhance adjustParam sr setParam failed ret:{} match:{}", i_err, m_option.match);
            return i_err;
        }
    }

    HJFLogi("HJVideoEnhance adjustParam end");
    return HJ_OK;
}

int HJVideoEnhance::priInitWithOption(const HJVideoEnhanceOption& i_option)
{
    int i_err = HJ_OK;
    HJFLogi("HJVideoEnhance init enter enableDenoise:{} enableSR:{} denoiseStrength:{} sharpness:{} extraSharpen:{} extraSharpenBoost:{} match:{}",
        i_option.enableDenoise,
        i_option.enableSR,
        i_option.denoiseStrength,
        i_option.sharpness,
        i_option.enableExtraSharpen,
        i_option.extraSharpenBoost,
        i_option.match);
    do
    {
#if !defined(WIN32_LIB) && !defined(ANDROID_LIB) && !defined(IOS_LIB) && !defined(MACOS_LIB)
        i_err = HJErrNotSupport;
        HJFLoge("HJVideoEnhance init unsupported platform ret:{}", i_err);
        break;
#else
        done();

        if (!i_option.enableDenoise && !i_option.enableSR)
        {
            i_err = HJErrInvalidParams;
            HJFLoge("HJVideoEnhance init invalid params both denoise and sr are disabled ret:{}", i_err);
            break;
        }

#if defined(WIN32_LIB)
        i_err = HJRenderCoreSingleton::Instance().init();
        if (i_err < 0)
        {
            HJFLoge("HJVideoEnhance init render core error:{}", i_err);
            break;
        }
#endif

        m_option = i_option;

        if (m_option.enableDenoise)
        {
            HJBaseParam::Ptr denoiseParam = HJBaseParam::Create();
            HJ_CatchMapPlainSetVal(denoiseParam, float, "denoiseStrength", m_option.denoiseStrength);
            HJRteComDrawDenoiseFilter::Ptr denoiseFilter = HJRteComDrawDenoiseFilter::Create();
            i_err = denoiseFilter->init(denoiseParam);
            if (i_err < 0)
            {
                HJFLoge("HJVideoEnhance init denoise filter error:{}", i_err);
                break;
            }
            m_denoiseFilter = denoiseFilter;
        }

        if (m_option.enableSR)
        {
            HJBaseParam::Ptr srParam = HJBaseParam::Create();
            HJ_CatchMapPlainSetVal(srParam, float, "sharpness", m_option.sharpness);
            HJ_CatchMapPlainSetVal(srParam, bool, "enableExtraSharpen", m_option.enableExtraSharpen);
            HJ_CatchMapPlainSetVal(srParam, float, "extraSharpenBoost", m_option.extraSharpenBoost);
            HJ_CatchMapPlainSetVal(srParam, float, "match", m_option.match);
            HJRteComDrawSRFilter::Ptr srFilter = HJRteComDrawSRFilter::Create();
            i_err = srFilter->init(srParam);
            if (i_err < 0)
            {
                HJFLoge("HJVideoEnhance init sr filter error:{}", i_err);
                break;
            }
            srFilter->setDynamicScaleRatio(false);
            m_srFilter = srFilter;
        }

        m_inited = true;
#endif
    } while (false);

    if (i_err < 0)
    {
        done();
    }
    HJFLogi("HJVideoEnhance init end ret:{} inited:{}", i_err, m_inited);
    return i_err;
}

int HJVideoEnhance::process(
    uint32_t inputTextureId,
    int inputWidth,
    int inputHeight,
    int outputWidth,
    int outputHeight,
    uint32_t& outTextureId,
    int64_t& outElapsedMs)
{
    outTextureId = 0;
    outElapsedMs = 0;
    HJFLogi("HJVideoEnhance process enter inTex:{} input:{}x{} output:{}x{} enableDenoise:{} enableSR:{}",
        inputTextureId,
        inputWidth,
        inputHeight,
        outputWidth,
        outputHeight,
        m_option.enableDenoise,
        m_option.enableSR);

    if (!m_inited)
    {
        HJFLoge("HJVideoEnhance process not inited");
        return HJErrNotInited;
    }
    if (inputTextureId == 0 || inputWidth <= 0 || inputHeight <= 0 || outputWidth <= 0 || outputHeight <= 0)
    {
        HJFLoge("HJVideoEnhance process invalid params inTex:{} input:{}x{} output:{}x{}",
            inputTextureId, inputWidth, inputHeight, outputWidth, outputHeight);
        return HJErrInvalidParams;
    }
    if (!m_option.enableDenoise && !m_option.enableSR)
    {
        outTextureId = inputTextureId;
        outElapsedMs = 0;
        HJFLogi("HJVideoEnhance process bypass both denoise and sr are disabled outTex:{}", outTextureId);
        return HJ_OK;
    }

    const int64_t startMs = HJCurrentSteadyMS();

    int i_err = priPrepareOutputFbo(outputWidth, outputHeight);
    if (i_err < 0)
    {
        HJFLoge("HJVideoEnhance process prepare output fbo failed ret:{} output:{}x{}", i_err, outputWidth, outputHeight);
        return i_err;
    }

    uint32_t textureToProcess = inputTextureId;
    int textureWidth = inputWidth;
    int textureHeight = inputHeight;

    i_err = priRunDenoiseStage(
        textureToProcess,
        textureWidth,
        textureHeight,
        outputWidth,
        outputHeight,
        textureToProcess);
    if (i_err == HJ_OK && (!m_option.enableSR || !m_srFilter))
    {
        outTextureId = m_outputFbo->getTextureId();
        outElapsedMs = HJCurrentSteadyMS() - startMs;
        HJFLogi("HJVideoEnhance process end denoise-only ret:{} outTex:{} elapsedMs:{}",
            HJ_OK, outTextureId, outElapsedMs);
        return HJ_OK;
    }
    if (i_err < 0)
    {
        HJFLoge("HJVideoEnhance process denoise stage failed ret:{}", i_err);
        return i_err;
    }

    i_err = priRunSRStage(textureToProcess, textureWidth, textureHeight, outputWidth, outputHeight);
    if (i_err < 0)
    {
        HJFLoge("HJVideoEnhance process sr stage failed ret:{}", i_err);
        return i_err;
    }

    outTextureId = m_outputFbo->getTextureId();
    outElapsedMs = HJCurrentSteadyMS() - startMs;
    HJFLogi("HJVideoEnhance process end ret:{} outTex:{} elapsedMs:{}",
        HJ_OK, outTextureId, outElapsedMs);
    return HJ_OK;
}

void HJVideoEnhance::done()
{
    HJFLogi("HJVideoEnhance done enter");
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
    m_workFbo = nullptr;
    m_outputFbo = nullptr;
    HJFLogi("HJVideoEnhance done end");
}

int HJVideoEnhance::priPrepareOutputFbo(int outputWidth, int outputHeight)
{
    int i_err = priEnsureFbo(m_outputFbo, outputWidth, outputHeight);
    if (i_err < 0)
    {
        HJFLoge("HJVideoEnhance prepare output fbo failed ret:{} output:{}x{}", i_err, outputWidth, outputHeight);
    }
    return i_err;
}

int HJVideoEnhance::priRunDenoiseStage(
    uint32_t inputTextureId,
    int inputWidth,
    int inputHeight,
    int outputWidth,
    int outputHeight,
    uint32_t& outTextureId)
{
    outTextureId = inputTextureId;
    if (!m_option.enableDenoise || !m_denoiseFilter)
    {
        return HJ_WOULD_BLOCK;
    }

    if (m_option.enableSR && m_srFilter)
    {
        return priRunDenoiseToWorkFbo(inputTextureId, inputWidth, inputHeight, outTextureId);
    }

    return priRunDenoiseToOutputFbo(inputTextureId, inputWidth, inputHeight, outputWidth, outputHeight);
}

int HJVideoEnhance::priRunSRStage(
    uint32_t inputTextureId,
    int inputWidth,
    int inputHeight,
    int outputWidth,
    int outputHeight)
{
    if (!m_option.enableSR || !m_srFilter)
    {
        return HJ_WOULD_BLOCK;
    }

    return priProcessSR(inputTextureId, inputWidth, inputHeight, outputWidth, outputHeight);
}

int HJVideoEnhance::priEnsureFbo(std::shared_ptr<HJOGFBOCtrl>& io_fbo, int width, int height)
{
    if (width <= 0 || height <= 0)
    {
        HJFLoge("HJVideoEnhance ensure fbo invalid params {}x{}", width, height);
        return HJErrInvalidParams;
    }
    if (io_fbo && io_fbo->getWidth() == width && io_fbo->getHeight() == height)
    {
        return HJ_OK;
    }

    io_fbo = HJOGFBOCtrl::Create();
    if (!io_fbo)
    {
        HJFLoge("HJVideoEnhance ensure fbo create failed {}x{}", width, height);
        return HJErrNewObj;
    }
    int i_err = io_fbo->init(width, height, true);
    if (i_err < 0)
    {
        HJFLoge("HJVideoEnhance ensure fbo init failed ret:{} {}x{}", i_err, width, height);
    }
    return i_err;
}

int HJVideoEnhance::priRunDenoiseToWorkFbo(
    uint32_t inputTextureId,
    int inputWidth,
    int inputHeight,
    uint32_t& outTextureId)
{
    outTextureId = 0;
    if (!m_denoiseFilter)
    {
        HJFLoge("HJVideoEnhance denoise-to-work not inited");
        return HJErrNotInited;
    }

    int i_err = priEnsureFbo(m_workFbo, inputWidth, inputHeight);
    if (i_err < 0)
    {
        HJFLoge("HJVideoEnhance denoise-to-work ensure fbo failed ret:{} input:{}x{}", i_err, inputWidth, inputHeight);
        return i_err;
    }

    m_workFbo->attach();
    i_err = m_denoiseFilter->draw(
        inputTextureId,
        HJWindowRenderMode_CLIP,
        inputWidth,
        inputHeight,
        inputWidth,
        inputHeight,
        nullptr,
        false,
        false);
    m_workFbo->detach();
    if (i_err < 0)
    {
        HJFLoge("HJVideoEnhance denoise-to-work draw failed ret:{} input:{}x{}", i_err, inputWidth, inputHeight);
        return i_err;
    }

    outTextureId = m_workFbo->getTextureId();
    HJFLogi("HJVideoEnhance denoise-to-work ok outTex:{} input:{}x{}", outTextureId, inputWidth, inputHeight);
    return HJ_OK;
}

int HJVideoEnhance::priRunDenoiseToOutputFbo(
    uint32_t inputTextureId,
    int inputWidth,
    int inputHeight,
    int outputWidth,
    int outputHeight)
{
    int i_err = priProcessDenoise(inputTextureId, inputWidth, inputHeight, outputWidth, outputHeight);
    if (i_err < 0)
    {
        HJFLoge("HJVideoEnhance denoise-to-output failed ret:{} input:{}x{} output:{}x{}",
            i_err, inputWidth, inputHeight, outputWidth, outputHeight);
    }
    return i_err;
}

int HJVideoEnhance::priProcessSR(
    uint32_t inputTextureId,
    int inputWidth,
    int inputHeight,
    int outputWidth,
    int outputHeight)
{
    if (!m_srFilter || !m_outputFbo)
    {
        HJFLoge("HJVideoEnhance sr not inited");
        return HJErrNotInited;
    }

    m_outputFbo->attach();
    int i_err = m_srFilter->draw(
        inputTextureId,
        HJWindowRenderMode_CLIP,
        inputWidth,
        inputHeight,
        outputWidth,
        outputHeight,
        nullptr,
        false,
        false);
    m_outputFbo->detach();
    if (i_err < 0)
    {
        HJFLoge("HJVideoEnhance sr draw failed ret:{} input:{}x{} output:{}x{}",
            i_err, inputWidth, inputHeight, outputWidth, outputHeight);
    }
    return i_err;
}

int HJVideoEnhance::priProcessDenoise(
    uint32_t inputTextureId,
    int inputWidth,
    int inputHeight,
    int outputWidth,
    int outputHeight)
{
    if (!m_denoiseFilter || !m_outputFbo)
    {
        HJFLoge("HJVideoEnhance denoise not inited");
        return HJErrNotInited;
    }

    m_outputFbo->attach();
    int i_err = m_denoiseFilter->draw(
        inputTextureId,
        HJWindowRenderMode_CLIP,
        inputWidth,
        inputHeight,
        outputWidth,
        outputHeight,
        nullptr,
        false,
        false);
    m_outputFbo->detach();
    if (i_err < 0)
    {
        HJFLoge("HJVideoEnhance denoise draw failed ret:{} input:{}x{} output:{}x{}",
            i_err, inputWidth, inputHeight, outputWidth, outputHeight);
    }
    return i_err;
}



