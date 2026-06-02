#pragma once

#include "HJCommonInterface.h"
#include "HJExport.h"
#include <memory>
#include <mutex>
#include <deque>

typedef enum HJFaceDetectWrapperType
{
    HJFaceDetectWrapperType_Unknown        = -1,
    HJFaceDetectWrapperType_TNNBLAZE       = 0,
    HJFaceDetectWrapperType_TNNALIGNER     = 1,
    HJFaceDetectWrapperType_NCNNRETINAFACE = 3,
    HJFaceDetectWrapperType_NCNNSCRFD      = 4,
    HJFaceDetectWrapperType_IOSVISIONRECT  = 5,
    HJFaceDetectWrapperType_COREMLRETINAFACE = 6,

} HJFaceDetectWrapperType;

typedef struct HJFaceDetectWrapperOption
{ 
    float tnnFaceAlignThreshold = 0.75f;
    int   tnnFaceAlignMinFaceSize = 20;

    float ncnnRetinaFaceProbThreshold = 0.8f;
    float ncnnRetinaFaceNmsThreshold = 0.4f;
    int ncnnRetinaFaceThreadNums = 1;// 1 or 2
    int retinaFaceTargetSize = 320;
    bool ncnnRetinaFaceUseGPU = false;

    //fit and padding is good effect, if not equal scale, the face rect is not good, and landmark is not good
    bool ncnnScrfdEqualScale = true;
    int ncnnScrfdTargetSize = 320;
    float ncnnScrfdProbThreshold = 0.3f;
    float ncnnScrfdNmsThreshold = 0.45f;
    int ncnnScrfdThreadNums = 1; // 1-2
    bool ncnnScrfdUseGPU = false;

    int coreMLRetinaFaceComputeMode = 0; // 0 All, 1 CPU+GPU, 2 CPU+ANE, 3 CPUOnly
    int visionRectTargetSize = 0; // 0 keeps original resolution; >0 enables matched-scale Vision profiling
    int visionRectComputeMode = 0; // 0 auto, 1 ANE, 2 CPU, 3 GPU
} HJFaceDetectWrapperOption;


namespace HJ
{
    class HJBaseFaceDetect;
    class HJThreadPool;
    class HJTransferMediaData;
}
using HJFaceDetectConciseCb = std::function<void(const std::string& i_faceInfo)>;

class HJExport HJFaceDetectWrapper
{
public:
    HJFaceDetectWrapper();
    ~HJFaceDetectWrapper();

    int init(HJFaceDetectConciseCb i_cb, HJFaceDetectWrapperType i_type, const std::string &i_modelUrl, const HJFaceDetectWrapperOption &i_option);
    int detect(std::shared_ptr<HJUnifyWrapperData> i_input, bool i_bSmooth = false, bool i_bSync = false, bool i_bDebugPoints = false, bool i_bDebugImg = false);

private:
    int priDetect(std::shared_ptr<HJ::HJTransferMediaData> i_mediaData, bool i_bSmooth, bool i_bDebugImg);
    std::shared_ptr<HJ::HJBaseFaceDetect> m_faceDetect = nullptr;
    HJFaceDetectConciseCb m_conciseOutCb = nullptr;
    std::shared_ptr<HJ::HJThreadPool> m_threadPool = nullptr;
    bool m_bError = false;
};
