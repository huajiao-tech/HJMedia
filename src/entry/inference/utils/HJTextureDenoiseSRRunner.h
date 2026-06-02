#pragma once

#include <cstdint>
#include <memory>

namespace HJ
{
    class HJOGFBOCtrl;
    class HJRteComDrawDenoiseFilter;
    class HJRteComDrawSRFilter;
}

class HJTextureDenoiseSRRunner
{
public:
    HJTextureDenoiseSRRunner();
    ~HJTextureDenoiseSRRunner();

    int init(float denoiseStrength = 0.2f, float sharpness = 1.0f, int scaleFactor = 2,
        bool enableExtraSharpen = true, float extraSharpenBoost = 0.55f);
    int process(
        uint32_t inputTextureId,
        int inputWidth,
        int inputHeight,
        uint32_t& outTextureId,
        int& outWidth,
        int& outHeight,
        int64_t* outElapsedMs = nullptr);
    void done();

private:
    int priEnsureFbo(std::shared_ptr<HJ::HJOGFBOCtrl>& io_fbo, int width, int height);

    std::shared_ptr<HJ::HJRteComDrawDenoiseFilter> m_denoiseFilter;
    std::shared_ptr<HJ::HJRteComDrawSRFilter> m_srFilter;
    std::shared_ptr<HJ::HJOGFBOCtrl> m_denoiseFbo;
    std::shared_ptr<HJ::HJOGFBOCtrl> m_outputFbo;
    int m_scaleFactor = 2;
    bool m_inited = false;
};
