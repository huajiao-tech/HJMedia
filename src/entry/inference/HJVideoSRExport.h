#pragma once

#include "HJCommonInterface.h"
#include "HJExport.h"
#include <memory>

typedef enum HJVideoSRWrapperType
{
    HJVideoSRWrapperType_Unknown = -1,
    HJVideoSRWrapperType_NCNNREALESRGAN = 0,
	HJVideoSRWrapperType_NCNNREALCUGAN = 1,
    HJVideoSRWrapperType_NCNNPLAINUSR  = 2,
    HJVideoSRWrapperType_MINDSPOREREALESRGAN = 3,
    HJVideoSRWrapperType_COREMLREALESRGAN = 4,
    HJVideoSRWrapperType_VTFRAMEPROCESSOR = 5,
    HJVideoSRWrapperType_TEXTUREFSR = 6,
} HJVideoSRWrapperType;

typedef struct HJVideoSRWrapperOption
{ 
    std::string ncnnRealESRGANType = "realesr-general-x4v3";
    float ncnnRealESRGANDenose = 0.5f;
    std::string ncnnRealCUGANType = "conservative";
    bool ncnnUseGPU = true;
    int ncnnThreadNums = 1;
    int ncnnScale = 2;
    float textureFsrDenoiseStrength = 1.0f;
    float textureFsrSharpness = 1.0f;
    bool textureFsrEnableExtraSharpen = true;
    float textureFsrExtraSharpenBoost = 0.55f;
    int textureFsrScale = 2;
    std::string coreMLModelName = "";
    int coreMLComputeMode = 0;
} HJVideoSRWrapperOption;

namespace HJ
{
    class HJBaseVideoSR;
    class HJThreadPool;
    class HJTransferMediaData;
    struct HJSRRet;
}
using HJVideoSROutputCb = std::function<void(std::shared_ptr<HJ::HJTransferMediaData> i_output, const HJ::HJSRRet& i_ret)>;

class HJExport HJVideoSRWrapper
{
public:
    HJVideoSRWrapper();
    ~HJVideoSRWrapper();

    int init(HJVideoSROutputCb i_cb, HJVideoSRWrapperType i_type, const std::string& i_modelUrl, const HJVideoSRWrapperOption& i_option);
    int process(std::shared_ptr<HJUnifyWrapperData> i_input, bool i_bSync = false);

private:
    int priProcess(std::shared_ptr<HJ::HJTransferMediaData> i_mediaData);
    std::shared_ptr<HJ::HJBaseVideoSR> m_videoSR = nullptr;
    HJVideoSROutputCb m_outputCb = nullptr;
    std::shared_ptr<HJ::HJThreadPool> m_threadPool = nullptr;
    bool m_bError = false;
};
