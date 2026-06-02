#include "HJRteComDraw.h"

#include <algorithm>
#include <array>
#include <cmath>

#include "HJFLog.h"
#include "HJOGBaseShader.h"
#include "HJOGCommon.h"

NS_HJ_BEGIN

namespace {

constexpr int HJRteDenoiseKernelCount = 9;
constexpr float HJRteDenoiseSpaceSigma = 1.25f;
constexpr float HJRteDenoiseMinRangeSigma = 0.08f;
constexpr float HJRteDenoiseMaxRangeSigma = 0.24f;

float HJRteDenoiseGaussianWeight(float i_distanceSquared, float i_sigma)
{
    return std::exp(-i_distanceSquared / (2.0f * i_sigma * i_sigma));
}

class HJOGShaderFilterDenoiseBilateral : public HJOGCopyShaderStrip
{
public:
    HJ_DEFINE_CREATE(HJOGShaderFilterDenoiseBilateral);

    HJOGShaderFilterDenoiseBilateral()
    {
        HJ_SetInsName(HJOGShaderFilterDenoiseBilateral);
    }

    void setTexelSize(float i_x, float i_y)
    {
        m_texelSize[0] = i_x;
        m_texelSize[1] = i_y;
    }

    void setOffsets(const std::array<float, HJRteDenoiseKernelCount * 2>& i_offsets)
    {
        m_offsets = i_offsets;
    }

    void setSpatialWeights(const std::array<float, HJRteDenoiseKernelCount>& i_weights)
    {
        m_spatialWeights = i_weights;
    }

    void setRangeFactor(float i_rangeFactor)
    {
        m_rangeFactor = i_rangeFactor;
    }

    virtual int shaderGetHandle(GLuint i_program) override
    {
        m_texelSizeHandle = glGetUniformLocation(i_program, "uTexelSize");
        m_offsetsHandle = glGetUniformLocation(i_program, "uOffsets[0]");
        m_spatialWeightsHandle = glGetUniformLocation(i_program, "uSpatialWeights[0]");
        m_rangeFactorHandle = glGetUniformLocation(i_program, "uRangeFactor");
        return (m_texelSizeHandle == -1 || m_offsetsHandle == -1 || m_spatialWeightsHandle == -1 || m_rangeFactorHandle == -1) ? HJErrFatal : HJ_OK;
    }

    virtual void shaderDrawUpdate() override
    {
        glUniform2f(m_texelSizeHandle, m_texelSize[0], m_texelSize[1]);
        glUniform2fv(m_offsetsHandle, HJRteDenoiseKernelCount, m_offsets.data());
        glUniform1fv(m_spatialWeightsHandle, HJRteDenoiseKernelCount, m_spatialWeights.data());
        glUniform1f(m_rangeFactorHandle, m_rangeFactor);
    }

    virtual std::string shaderGetFragment() override
    {
        return std::string(HJOGCommon::HJ_GLSL_VERSION) +
            std::string(HJOGCommon::HJ_GLSL_PRECISION) +
            R"(
in vec2 v_texcood;
uniform sampler2D sTexture;
uniform vec2 uTexelSize;
uniform vec2 uOffsets[9];
uniform float uSpatialWeights[9];
uniform float uRangeFactor;
out vec4 FragColor;
void main()
{
    vec4 center = texture(sTexture, v_texcood);
    vec3 accum = vec3(0.0);
    float totalWeight = 0.0;
    for (int i = 0; i < 9; ++i)
    {
        vec3 sampleColor = texture(sTexture, v_texcood + uOffsets[i] * uTexelSize).rgb;
        vec3 diff = sampleColor - center.rgb;
        float rangeWeight = exp(dot(diff, diff) * uRangeFactor);
        float weight = uSpatialWeights[i] * rangeWeight;
        accum += sampleColor * weight;
        totalWeight += weight;
    }
    FragColor = vec4(accum / max(totalWeight, 1e-5), center.a);
}
)";
    }

private:
    GLint m_texelSizeHandle = -1;
    GLint m_offsetsHandle = -1;
    GLint m_spatialWeightsHandle = -1;
    GLint m_rangeFactorHandle = -1;
    std::array<float, 2> m_texelSize = {0.0f, 0.0f};
    std::array<float, HJRteDenoiseKernelCount * 2> m_offsets = {};
    std::array<float, HJRteDenoiseKernelCount> m_spatialWeights = {};
    float m_rangeFactor = 0.0f;
};

} // namespace

HJRteComDrawDenoiseFilter::HJRteComDrawDenoiseFilter()
{
    HJ_SetInsName(HJRteComDrawDenoiseFilter);
    setPriority(HJRteComPriority_VideoDenoise);
}

int HJRteComDrawDenoiseFilter::init(HJBaseParam::Ptr i_param)
{
    int i_err = HJ_OK;
    do
    {
        i_err = HJRteComDrawFBO::init(i_param);
        if (i_err < 0)
        {
            break;
        }

        if (i_param)
        {
            HJ_CatchMapPlainGetVal(i_param, float, "denoiseStrength", m_denoiseStrength);
        }
        HJFLogi("denoise init effect param denoiseStrength:{}", m_denoiseStrength);
        m_denoiseStrength = priClamp01(m_denoiseStrength);
        priUpdateDenoiseParameters();

        HJOGShaderFilterDenoiseBilateral::Ptr denoiseShader = HJOGShaderFilterDenoiseBilateral::Create();
        i_err = denoiseShader->init(OGCopyShaderStripFlag_2D, false);
        if (i_err < 0)
        {
            HJFLoge("{} init denoise shader error:{}", getInsName(), i_err);
            break;
        }

        m_denoiseShader = denoiseShader;
    } while (false);
    return i_err;
}

void HJRteComDrawDenoiseFilter::done()
{
    m_denoiseShader = nullptr;
    HJRteComDrawFBO::done();
}

int HJRteComDrawDenoiseFilter::setParam(HJBaseParam::Ptr i_param)
{
    if (!i_param)
    {
        return HJ_OK;
    }
    HJ_CatchMapPlainGetVal(i_param, float, "denoiseStrength", m_denoiseStrength);
    m_denoiseStrength = priClamp01(m_denoiseStrength);
    priUpdateDenoiseParameters();
    HJFLogi("denoise setparam effect param denoiseStrength:{}", m_denoiseStrength);
    return HJ_OK;
}

int HJRteComDrawDenoiseFilter::draw(GLuint textureId, int i_fitMode, int srcw, int srch, int dstw, int dsth, float* texMat, bool i_bYFlip, bool i_bXFlip)
{
    int i_err = HJ_OK;
    do
    {
        if (textureId == 0 || srcw <= 0 || srch <= 0 || dstw <= 0 || dsth <= 0)
        {
            i_err = HJErrInvalidParams;
            break;
        }

        HJOGShaderFilterDenoiseBilateral::Ptr denoiseShader = std::dynamic_pointer_cast<HJOGShaderFilterDenoiseBilateral>(m_denoiseShader);
        if (!denoiseShader)
        {
            i_err = HJErrFatal;
            break;
        }

        denoiseShader->setTexelSize(1.0f / static_cast<float>(srcw), 1.0f / static_cast<float>(srch));
        denoiseShader->setOffsets(m_denoiseOffsets);
        denoiseShader->setSpatialWeights(m_denoiseSpatialWeights);
        denoiseShader->setRangeFactor(m_denoiseRangeFactor);

        glViewport(0, 0, dstw, dsth);
        i_err = denoiseShader->draw(textureId, i_fitMode, srcw, srch, dstw, dsth, texMat, i_bYFlip, i_bXFlip);
    } while (false);
    return i_err;
}

float HJRteComDrawDenoiseFilter::priClamp01(float i_value) const
{
    return std::max(0.0f, std::min(1.0f, i_value));
}

void HJRteComDrawDenoiseFilter::priUpdateDenoiseParameters()
{
    int kernelIndex = 0;
    float spatialWeightSum = 0.0f;
    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            m_denoiseOffsets[kernelIndex * 2] = static_cast<float>(x);
            m_denoiseOffsets[kernelIndex * 2 + 1] = static_cast<float>(y);
            float weight = HJRteDenoiseGaussianWeight(static_cast<float>(x * x + y * y), HJRteDenoiseSpaceSigma);
            m_denoiseSpatialWeights[kernelIndex] = weight;
            spatialWeightSum += weight;
            ++kernelIndex;
        }
    }

    if (spatialWeightSum > 0.0f)
    {
        for (float& weight : m_denoiseSpatialWeights)
        {
            weight /= spatialWeightSum;
        }
    }

    const float clampedStrength = priClamp01(m_denoiseStrength);
    const float rangeSigma = HJRteDenoiseMaxRangeSigma - clampedStrength * (HJRteDenoiseMaxRangeSigma - HJRteDenoiseMinRangeSigma);
    m_denoiseRangeFactor = -1.0f / (2.0f * rangeSigma * rangeSigma);
}

NS_HJ_END
