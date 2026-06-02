#pragma once

#include "HJExport.h"
#include "HJFaceAcquireWrapper.h"
#include "HJVideoSRExport.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

namespace HJ
{
    class HJTransferMediaData;
}

struct HJFaceSRProcessResult
{
    int faceCount = 0;
    int64_t faceDetectElapsedMs = 0;
    int64_t srElapsedMs = -1;
    int64_t preScaleElapsedMs = 0;
    int64_t postResizeElapsedMs = 0;
    HJUnifyWrapperRect faceRect = HJUnifyWrapperRect{ 0, 0, 0, 0 };
    int faceTargetDisplayWidth = 0;
    int faceTargetDisplayHeight = 0;
    int scaleW = 0;
    int scaleH = 0;
    int preScaleInputWidth = 0;
    int preScaleInputHeight = 0;
    int preScaleOutputWidth = 0;
    int preScaleOutputHeight = 0;
    int postResizeInputWidth = 0;
    int postResizeInputHeight = 0;
    int postResizeOutputWidth = 0;
    int postResizeOutputHeight = 0;
    int padLeft = 0;
    int padRight = 0;
    int padTop = 0;
    int padBottom = 0;
};

typedef enum HJFaceSRProcessMode
{
    HJFaceSRProcessMode_Full = 0,
    HJFaceSRProcessMode_FaceOrigin = 1,
    HJFaceSRProcessMode_FaceScale = 2,
    HJFaceSRProcessMode_FullScale = 3,
} HJFaceSRProcessMode;

typedef enum HJFaceSRProcessPolicy
{
	HJFaceSRProcessPolicy_Mipmap   = 0,
	HJFaceSRProcessPolicy_Bilinear = 1,
	HJFaceSRProcessPolicy_Bicubic  = 2,
	HJFaceSRProcessPolicy_Lanczos3 = 3,
	HJFaceSRProcessPolicy_Lanczos4 = 4,

} HJFaceSRProcessPolicy;

typedef struct HJFaceSRProcessOption
{
    HJFaceSRProcessMode mode = HJFaceSRProcessMode_FaceScale;
    int preScaleWidth = 0;
    int preScaleHeight = 0;
    int composeCanvasWidth = 0;
    int composeCanvasHeight = 0;
    bool bFeather = true;
    bool bEnablePostSRDisplayResize = true;
    bool bMixedEnable = true;
    float mixAlphaRatio = 0.8f;
	HJFaceSRProcessPolicy policy = HJFaceSRProcessPolicy_Lanczos3;
} HJFaceSRProcessOption;

// The HJUnifyWrapperData planes borrow the internal output buffer and are only valid
// during the callback invocation. Retain/cross-thread use must copy out the pixels.
using HJFaceSROutputCb = std::function<void(const std::shared_ptr<HJUnifyWrapperData>& output)>;

class HJExport HJFaceSRWrapper
{
public:
    HJFaceSRWrapper();
    virtual ~HJFaceSRWrapper();

    int init(
        const std::string& modelDir,
        HJFaceDetectWrapperType faceDetectType,
        HJVideoSRWrapperType srType,
        const HJFaceDetectWrapperOption& faceDetectOption = HJFaceDetectWrapperOption(),
        const HJVideoSRWrapperOption& srOption = HJVideoSRWrapperOption());

    int process(
        const std::shared_ptr<HJ::HJTransferMediaData>& input,
        const HJFaceSRProcessOption& i_processOption,
        HJFaceSRProcessResult& outResult);

    int process(
        const std::shared_ptr<HJUnifyWrapperData>& input,
        const HJFaceSRProcessOption& i_processOption,
        HJFaceSRProcessResult& outResult,
        const HJFaceSROutputCb& outCb);

    int process(
        uint32_t inputTextureId,
        int inputWidth,
        int inputHeight,
        uint32_t& outputTextureId,
        int& outputWidth,
        int& outputHeight,
        HJFaceSRProcessResult& outResult);

    std::shared_ptr<HJ::HJTransferMediaData> takeLastOutput();

    void done();

private:
    struct HJFaceSRPreScaleMeta
    {
        int canvasW = 0;
        int canvasH = 0;
        int contentW = 0;
        int contentH = 0;
        int offsetX = 0;
        int offsetY = 0;
        bool applied = false;
    };

    int runSRLocked(
        const std::shared_ptr<HJ::HJTransferMediaData>& input,
        HJFaceSRProcessResult& outResult,
        int64_t& outSrElapsedMs,
        bool applyFeatherAlpha,
        float srAlphaRatio,
        int postTargetWidth,
        int postTargetHeight,
        const HJFaceSRPreScaleMeta* preScaleMeta = nullptr);

    static std::shared_ptr<HJ::HJTransferMediaData> preScaleInputIfNeeded(
        const std::shared_ptr<HJ::HJTransferMediaData>& inputData,
        int preScaleWidth,
        int preScaleHeight,
        HJFaceSRProcessPolicy i_procPolicy,
        bool forceCanvasSize,
        HJFaceSRPreScaleMeta& outMeta);

    std::unique_ptr<HJFaceAcquireWrapper> m_faceAcquireWrapper;
    std::unique_ptr<HJVideoSRWrapper> m_srWrapper;
    std::unique_ptr<class HJTextureDenoiseSRRunner> m_textureDenoiseSR;
    std::mutex m_mutex;
    std::mutex m_srMutex;
    std::shared_ptr<HJ::HJTransferMediaData> m_srOutput = nullptr;
    int64_t m_srElapseMs = -1;
    HJVideoSRWrapperType m_srType = HJVideoSRWrapperType_Unknown;
    bool m_inited = false;
};
