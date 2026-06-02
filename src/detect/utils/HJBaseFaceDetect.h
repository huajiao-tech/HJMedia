#pragma once

#include "HJPrerequisites.h"
#include "HJComUtils.h"
#include "HJConvertUtils.h"
#include "HJDetectUtils.h"
#include "HJMediaUtils.h"
#include "HJDataConvert.h"

NS_HJ_BEGIN

class HJTransferMediaData;
class HJMorePointSmooth;
class HJMoreFacePointsReal;

typedef enum HJFaceDetectType
{
    HJFaceDetectType_None = 0,
    HJFaceDetectType_TNN_BLAZEFACE,
    HJFaceDetectType_TNN_FACEALIGNER,
	HJFaceDetectType_NCNN_RETINAFACE,
    HJFaceDetectType_NCNN_SCRFD,
    HJFaceDetectType_IOS_VISION_RECT,
    HJFaceDetectType_IOS_COREML_RETINAFACE,
	} HJFaceDetectType;

enum class HJFaceDetectSmoothState
{
    None = 0,
    Enable = 1,
    Disable = 2,
};

class HJFaceDetectOption
{
public:
    HJ_DEFINE_CREATE(HJFaceDetectOption);
	HJFaceDetectOption()
	{

	}
    float tnnFaceAlignThreshold = 0.75f;
    int   tnnFaceAlignMinFaceSize = 20;

    float ncnnRetinaFaceProbThreshold = 0.8f;
    float ncnnRetinaFaceNmsThreshold = 0.4f;
    int ncnnRetinaFaceThreadNums = 1; // 1-2
    int retinaFaceTargetSize = 320;
    bool ncnnRetinaFaceUseGPU = false;

    bool ncnnScrfdEqualScale = true;
    int ncnnScrfdTargetSize = 320;
    float ncnnScrfdProbThreshold = 0.3f;
    float ncnnScrfdNmsThreshold = 0.45f;
    int ncnnScrfdThreadNums = 1; // 1-2
    bool ncnnScrfdUseGPU = false;

    int coreMLRetinaFaceComputeMode = 0; // 0 All, 1 CPU+GPU, 2 CPU+ANE, 3 CPUOnly
    int visionRectTargetSize = 0; // 0 keeps original resolution; >0 clamps max side for matched-scale profiling
    int visionRectComputeMode = 0; // 0 auto, 1 ANE, 2 CPU, 3 GPU
};

class HJBaseFaceDetect : public HJBaseSharedObject
{
public:
    HJ_DEFINE_CREATE(HJBaseFaceDetect);
    HJBaseFaceDetect() = default;
    virtual ~HJBaseFaceDetect() = default;

    virtual int init(HJBaseParam::Ptr params);
    virtual int detect(std::shared_ptr<HJTransferMediaData> i_data, HJFaceDetectRet& o_ret)
    {
        return 0;
    }
    void setTargetWidth(int i_width)
    {
        m_targetWidth = i_width;
    }
    void setTargetHeight(int i_height)
    {
        m_targetHeight = i_height;
    }
    void setType(HJFaceDetectType i_type)
    {
        m_type = i_type;
    }
    HJFaceDetectType getType() const
    {
        return m_type;
    }
    void setDebugPoints(bool i_bDebugPoints)
    {
        m_bDebugPoints = i_bDebugPoints;
    }
    bool getDebugPoints() const
    {
        return m_bDebugPoints;
    }

    int convert(std::shared_ptr<HJTransferMediaData> i_data, HJDataScaleType i_type, HJVec4i& o_vec);

    void dumpImage(std::shared_ptr<HJTransferMediaData> i_data, const HJFaceDetectRet& i_ret, const std::string &i_url);
    
    std::string cvtConcisePoints(HJFaceDetectRet& i_ret, bool i_bSmooth);

    static HJBaseFaceDetect::Ptr createFaceDetect(HJFaceDetectType i_type);

	static std::string s_parammodelPath;
protected:
	std::string m_modelUrls = "";

	int m_targetWidth = 0;
	int m_targetHeight = 0;

	int m_catchWidth = 0;
	int m_catchHeight = 0;

	std::shared_ptr<HJSPBuffer> m_scaleYuv = nullptr;
	std::shared_ptr<HJSPBuffer> m_scaleRgb = nullptr;

	HJFaceDetectOption::Ptr m_option = nullptr;
private:
    std::shared_ptr<HJMoreFacePointsReal> priTrySmoothPoints(std::shared_ptr<HJMoreFacePointsReal> i_points, bool i_bSmooth);

	HJFaceDetectType m_type = HJFaceDetectType_None;
	bool m_bDebugPoints = false;
	std::shared_ptr<HJMorePointSmooth> m_facePointSmooth = nullptr;
    HJFaceDetectSmoothState m_smoothState = HJFaceDetectSmoothState::None;
};



NS_HJ_END
