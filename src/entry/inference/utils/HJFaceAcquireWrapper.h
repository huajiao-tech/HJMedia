#pragma once

#include "HJExport.h"
#include "HJFaceDetectExport.h"
#include <memory>
#include <mutex>
#include <vector>

namespace HJ
{
    class HJTransferMediaData;
}


struct HJFaceAcquireWrapperItem
{
    std::shared_ptr<HJ::HJTransferMediaData> faceData = nullptr;
    HJUnifyWrapperRect faceRect = HJUnifyWrapperRect{ 0, 0, 0, 0 }; // relative to original frame
    int64_t m_elapsedTime = 0;
    int64_t m_systemTime = 0;
};

class HJExport HJFaceAcquireWrapper
{
public:
    HJFaceAcquireWrapper();
    virtual ~HJFaceAcquireWrapper();

    int init(
        const std::string& modelDir,
        HJFaceDetectWrapperType faceDetectType,
        const HJFaceDetectWrapperOption& option = HJFaceDetectWrapperOption());

    int acquireFaces(
        const std::shared_ptr<HJ::HJTransferMediaData>& input,
        std::vector<HJFaceAcquireWrapperItem>& outFaces);

    void done();

private:
    int ensureInit();
    static int clampPositiveRect(int frameW, int frameH, int& x, int& y, int& w, int& h);

    std::shared_ptr<::HJFaceDetectWrapper> m_faceDetect = nullptr;
    HJFaceDetectWrapperType m_faceDetectType = HJFaceDetectWrapperType_NCNNSCRFD;
    HJFaceDetectWrapperOption m_option;
    std::string m_modelDir;
    bool m_inited = false;

    std::mutex m_faceInfoMutex;
    std::string m_faceInfo;
};
