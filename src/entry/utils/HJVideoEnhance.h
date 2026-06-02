#pragma once

#include "HJExport.h"

#include <cstdint>
#include <memory>

namespace HJ
{
    class HJOGFBOCtrl;
    class HJRteComDrawDenoiseFilter;
    class HJRteComDrawSRFilter;
}

struct HJVideoEnhanceOption
{
    float denoiseStrength = 1.f;
    float sharpness = 1.0f;
    bool enableExtraSharpen = true;
    float extraSharpenBoost = 0.55f;
    float match = 1.0f;
    bool enableDenoise = true;
    bool enableSR = true;
};

class HJExport HJVideoEnhance
{
public:
    HJVideoEnhance();
    ~HJVideoEnhance();

    int init(const HJVideoEnhanceOption& i_option = HJVideoEnhanceOption());
    int adjustParam(const HJVideoEnhanceOption& i_option);
    int process(
        uint32_t inputTextureId,
        int inputWidth,
        int inputHeight,
        int outputWidth,
        int outputHeight,
        uint32_t& outTextureId,
        int64_t& outElapsedMs);
    void done();

private:
    int priInitWithOption(const HJVideoEnhanceOption& i_option);
    int priPrepareOutputFbo(int outputWidth, int outputHeight);
    int priRunDenoiseStage(
        uint32_t inputTextureId,
        int inputWidth,
        int inputHeight,
        int outputWidth,
        int outputHeight,
        uint32_t& outTextureId);
    int priRunSRStage(
        uint32_t inputTextureId,
        int inputWidth,
        int inputHeight,
        int outputWidth,
        int outputHeight);
    int priEnsureFbo(std::shared_ptr<HJ::HJOGFBOCtrl>& io_fbo, int width, int height);
    int priRunDenoiseToWorkFbo(
        uint32_t inputTextureId,
        int inputWidth,
        int inputHeight,
        uint32_t& outTextureId);
    int priRunDenoiseToOutputFbo(
        uint32_t inputTextureId,
        int inputWidth,
        int inputHeight,
        int outputWidth,
        int outputHeight);
    int priProcessSR(
        uint32_t inputTextureId,
        int inputWidth,
        int inputHeight,
        int outputWidth,
        int outputHeight);
    int priProcessDenoise(
        uint32_t inputTextureId,
        int inputWidth,
        int inputHeight,
        int outputWidth,
        int outputHeight);

    HJVideoEnhanceOption m_option;
    std::shared_ptr<HJ::HJRteComDrawDenoiseFilter> m_denoiseFilter;
    std::shared_ptr<HJ::HJRteComDrawSRFilter> m_srFilter;
    std::shared_ptr<HJ::HJOGFBOCtrl> m_workFbo;
    std::shared_ptr<HJ::HJOGFBOCtrl> m_outputFbo;
    bool m_inited = false;
};

