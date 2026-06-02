#pragma once

#include "HJPrerequisites.h"
#include "HJComUtils.h"
#include "HJConvertUtils.h"
#include "HJSRUtils.h"
#include "HJMediaUtils.h"
#include "HJDataConvert.h"
#include "ncnn/allocator.h"

namespace ncnn
{
	class Net;
}

NS_HJ_BEGIN

class HJTransferMediaData;
class HJSPBuffer;

typedef enum HJVideoSRType
{
    HJVideoSRType_None = 0,
    HJVideoSRType_NCNN_RealESRGAN,
	HJVideoSRType_NCNN_RealCUGAN,
	HJVideoSRType_NCNN_PlainUSR,
    HJVideoSRType_MindSpore_RealESRGAN,
    HJVideoSRType_CoreML_RealESRGAN,
    HJVideoSRType_VTFrameProcessor,
} HJVideoSRType;

class HJVideoSROption
{
public:
    HJ_DEFINE_CREATE(HJVideoSROption);
    HJVideoSROption()
	{

	}
    std::string ncnnRealESRGANType = "realesr-general-x4v3";  //realesr-general-x4v3   realesr-animevideov3-x2  realesrgan-x2plus
    float ncnnRealESRGANDenose = 0.5f;
    std::string ncnnRealCUGANType = "conservative";  //"conservative"; //"no-denoise";//
    bool ncnnUseGPU = true;
    int ncnnThreadNums = 1;
    int ncnnScale = 2;
    std::string coreMLModelName = "";
    int coreMLComputeMode = 0;
};

class HJBaseVideoSR : public HJBaseSharedObject
{
public:
    HJ_DEFINE_CREATE(HJBaseVideoSR);
    HJBaseVideoSR() = default;
    virtual ~HJBaseVideoSR() = default;

    virtual int init(HJBaseParam::Ptr params);
    virtual int process(std::shared_ptr<HJTransferMediaData> i_data, std::shared_ptr<HJTransferMediaData>& o_data, HJSRRet &o_ret);

    void setType(HJVideoSRType i_type)
    {
        m_type = i_type;
    }
    HJVideoSRType getType() const
    {
        return m_type;
    }

    static HJBaseVideoSR::Ptr createVideoSR(HJVideoSRType i_type);

	static std::string s_parammodelPath;

protected:
    int configNet();
	int inference(const std::string& inputName, const std::string& outputName,
		const unsigned char* rgb, int width, int height,
		HJSPBuffer::Ptr& outRgb, int& outWidth, int& outHeight) const;
    int prepareInputRgb(
        const std::shared_ptr<HJTransferMediaData>& i_inputData,
        const unsigned char*& o_rgb,
        int& o_width,
        int& o_height,
        int64_t& o_timestamp,
        const char* i_logTag);
    int finalizeSROutput(
        const std::shared_ptr<HJSPBuffer>& i_outRgb,
        int i_outWidth,
        int i_outHeight,
        int i_targetWidth,
        int i_targetHeight,
        int64_t i_timestamp,
        const std::shared_ptr<HJTransferMediaData>& i_inputData,
        std::shared_ptr<HJTransferMediaData>& o_data,
        const char* i_logTag);
    int finalizeSROutputRGBA(
        const std::shared_ptr<HJSPBuffer>& i_outRgba,
        int i_outWidth,
        int i_outHeight,
        int i_targetWidth,
        int i_targetHeight,
        int64_t i_timestamp,
        const std::shared_ptr<HJTransferMediaData>& i_inputData,
        std::shared_ptr<HJTransferMediaData>& o_data,
        const char* i_logTag);

	std::string m_modelUrls = "";
    HJVideoSROption::Ptr m_option = nullptr;
    ncnn::PoolAllocator m_blobPoolAllocator;
    ncnn::PoolAllocator m_workspacePoolAllocator;
    std::shared_ptr<HJSPBuffer> m_scaledRgb = nullptr;  
    std::shared_ptr<ncnn::Net> m_net;

    std::string m_inputName = "";
    std::string m_outputName = "";
    int m_modelScale = 2;
    std::shared_ptr<HJSPBuffer> m_inputRgb = nullptr;
    std::shared_ptr<HJSPBuffer> m_scaledRgba = nullptr;
private:

    HJVideoSRType m_type = HJVideoSRType_None;
};



NS_HJ_END
