#include "HJBaseVideoSR.h"
#include "HJFLog.h"
#include "HJError.h"
#include "HJComUtils.h"
#include "HJBaseUtils.h"
#include "HJDataConvert.h"
#include "libyuv.h"
#include "libyuv/scale_rgb.h"
#include "HJTransferMediaData.h"
#include "HJSPBuffer.h"
#include "HJSRNCNNRealESRGAN.h"
#include "HJSRNCNNRealCUGAN.h"
#include "HJSRNCNNPlainUSR.h"
#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif
#if defined(HJ_HAS_MINDSPORE_LITE) && defined(HarmonyOS)
#include "hsys/mindspore/HJSRMindSporeRealESRGAN.h"
#endif
#if defined(__APPLE__) && TARGET_OS_IOS
#include "osys/coreml/HJSRCoreMLRealESRGAN.h"
#include "osys/VTFrameProcessor/HJSRVTFrameProcessor.h"
#endif
#include "ncnn/net.h"
#include "ncnn/cpu.h"

NS_HJ_BEGIN

std::string HJBaseVideoSR::s_parammodelPath = "VIDEO_SR_MODEL_PATH";

int HJBaseVideoSR::init(HJBaseParam::Ptr params)
{
	int i_err = HJ_OK;
	do
	{
        HJFLogi("HJBaseVideoSR::init, modelUrls:{}", m_modelUrls);
		HJ_CatchMapPlainGetVal(params, std::string, s_parammodelPath, m_modelUrls);
		if (m_modelUrls.empty())
		{
			i_err = HJErrInvalidParams;
            HJLoge("HJBaseVideoSR::init, modelUrls is empty");
            break;
		}
		if (!HJBaseUtils::isDirectoryExists(m_modelUrls))
		{
            i_err = HJErrInvalidFile;
            HJLoge("HJBaseVideoSR::init, modelUrls is not exists");
            break;
		}
        HJ_CatchMapGetVal(params, HJVideoSROption::Ptr, m_option);
        if (!m_option)
        {
            m_option = HJVideoSROption::Create();
        }
	} while (false);
    HJFLogi("HJBaseFaceDetect::init, end i_err:{}", i_err);
	return i_err;
}

int HJBaseVideoSR::inference(const std::string& inputName, const std::string& outputName,
    const unsigned char* rgb, int width, int height,
    HJSPBuffer::Ptr& outRgb, int& outWidth, int& outHeight) const
{
	if (!rgb || width <= 0 || height <= 0 || inputName.empty() || outputName.empty())
	{
		return -1;
	}

	ncnn::Mat in = ncnn::Mat::from_pixels(rgb, ncnn::Mat::PIXEL_RGB, width, height);
	const float norm_vals[3] = { 1 / 255.f, 1 / 255.f, 1 / 255.f };
	in.substract_mean_normalize(0, norm_vals);

	ncnn::Extractor ex = m_net->create_extractor();
	int ret = ex.input(inputName.c_str(), in);
	if (ret != 0)
	{
		return -1;
	}

	ncnn::Mat outMat;
	ret = ex.extract(outputName.c_str(), outMat);

	if (ret != 0 || outMat.empty() || outMat.w <= 0 || outMat.h <= 0 || outMat.c != 3)
	{
		return -1;
	}

	const float to255[3] = { 255.f, 255.f, 255.f };
	outMat.substract_mean_normalize(0, to255);

	outWidth = outMat.w;
	outHeight = outMat.h;
	const int outSize = outWidth * outHeight * 3;
	if (!outRgb || outRgb->getSize() != outSize)
	{
		outRgb = HJSPBuffer::create(outSize);
	}
	outMat.to_pixels(outRgb->getBuf(), ncnn::Mat::PIXEL_RGB);

	return 0;
}
int HJBaseVideoSR::configNet()
{
	int i_err = HJ_OK;
    do 
    {
		m_net = std::make_shared<ncnn::Net>();
		m_net->opt.use_vulkan_compute = m_option->ncnnUseGPU;
		m_net->opt.num_threads = m_option->ncnnThreadNums;
		m_blobPoolAllocator.clear();
		m_workspacePoolAllocator.clear();
		m_net->opt.blob_allocator = &m_blobPoolAllocator;
		m_net->opt.workspace_allocator = &m_workspacePoolAllocator;
		m_net->opt.use_winograd_convolution = true;
#if defined(ANDROID)
		//if (!m_net->opt.use_vulkan_compute)
		//{
		ncnn::set_cpu_powersave(2);
		//}
#endif

		if (m_net->opt.use_vulkan_compute)
		{
			m_net->opt.use_fp16_packed = true;
			m_net->opt.use_fp16_storage = true;
			m_net->opt.use_fp16_arithmetic = true;
		}
		else
		{
			m_net->opt.use_fp16_packed = true;
			m_net->opt.use_fp16_storage = true;
			m_net->opt.use_fp16_arithmetic = true;
			m_net->opt.use_int8_inference = false;
		}
    } while (false);
    return i_err;
}
int HJBaseVideoSR::process(std::shared_ptr<HJTransferMediaData> i_inputData, std::shared_ptr<HJTransferMediaData>& o_data, HJSRRet& o_ret)
{
    int i_err = HJ_OK;
    (void)i_inputData;
    (void)o_data;
    return i_err;
}

int HJBaseVideoSR::prepareInputRgb(
    const std::shared_ptr<HJTransferMediaData>& i_inputData,
    const unsigned char*& o_rgb,
    int& o_width,
    int& o_height,
    int64_t& o_timestamp,
    const char* i_logTag)
{
    if (!i_inputData)
    {
        return HJErrInvalidParams;
    }

    o_rgb = nullptr;
    o_width = i_inputData->getWidth();
    o_height = i_inputData->getHeight();
    o_timestamp = i_inputData->getTimeStamp();

    if (o_width <= 0 || o_height <= 0)
    {
        HJFLoge("{} invalid input wh:{}x{}", i_logTag ? i_logTag : "HJBaseVideoSR", o_width, o_height);
        return HJErrInvalidParams;
    }

    const HJConvertDataType inputType = i_inputData->getConvertType();
    const int rgbStride = o_width * 3;
    const int inputRgbSize = rgbStride * o_height;

    if (inputType == HJConvertDataType_YUVNV12)
    {
        unsigned char* i_data0 = i_inputData->getData(0);
        unsigned char* i_data1 = i_inputData->getData(1);
        const int i_stride0 = i_inputData->getStride(0);
        const int i_stride1 = i_inputData->getStride(1);
        if (!i_data0 || !i_data1 || i_stride0 <= 0 || i_stride1 <= 0)
        {
            HJFLoge("{} invalid NV12 planes/strides", i_logTag ? i_logTag : "HJBaseVideoSR");
            return HJErrInvalidParams;
        }

        if (!m_inputRgb || m_inputRgb->getSize() != inputRgbSize)
        {
            m_inputRgb = HJSPBuffer::create(inputRgbSize);
        }
        if (!m_inputRgb || !m_inputRgb->getBuf())
        {
            HJFLoge("{} alloc input rgb failed size:{}", i_logTag ? i_logTag : "HJBaseVideoSR", inputRgbSize);
            return HJErrFatal;
        }

        const int ret = libyuv::NV12ToRAW(i_data0, i_stride0, i_data1, i_stride1,
            m_inputRgb->getBuf(), rgbStride, o_width, o_height);
        if (ret != 0)
        {
            HJFLoge("{} NV12ToRAW failed ret:{}", i_logTag ? i_logTag : "HJBaseVideoSR", ret);
            return HJErrFatal;
        }
        o_rgb = m_inputRgb->getBuf();
        return HJ_OK;
    }

    if (inputType == HJConvertDataType_RGB)
    {
        unsigned char* srcRgb = i_inputData->getData(0);
        const int srcStride = i_inputData->getStride(0);
        if (!srcRgb || srcStride <= 0)
        {
            HJFLoge("{} invalid RGB plane/stride", i_logTag ? i_logTag : "HJBaseVideoSR");
            return HJErrInvalidParams;
        }

        if (srcStride == rgbStride)
        {
            o_rgb = srcRgb;
            return HJ_OK;
        }

        if (!m_inputRgb || m_inputRgb->getSize() != inputRgbSize)
        {
            m_inputRgb = HJSPBuffer::create(inputRgbSize);
        }
        if (!m_inputRgb || !m_inputRgb->getBuf())
        {
            HJFLoge("{} alloc input rgb failed size:{}", i_logTag ? i_logTag : "HJBaseVideoSR", inputRgbSize);
            return HJErrFatal;
        }

        if (srcStride == rgbStride)
        {
            memcpy(m_inputRgb->getBuf(), srcRgb, inputRgbSize);
        }
        else
        {
            for (int y = 0; y < o_height; ++y)
            {
                memcpy(m_inputRgb->getBuf() + (size_t)y * (size_t)rgbStride,
                    srcRgb + (size_t)y * (size_t)srcStride,
                    (size_t)rgbStride);
            }
        }
        o_rgb = m_inputRgb->getBuf();
        return HJ_OK;
    }

    HJFLoge("{} unsupported input type:{}", i_logTag ? i_logTag : "HJBaseVideoSR", (int)inputType);
    return HJErrInvalidParams;
}

int HJBaseVideoSR::finalizeSROutput(
    const std::shared_ptr<HJSPBuffer>& i_outRgb,
    int i_outWidth,
    int i_outHeight,
    int i_targetWidth,
    int i_targetHeight,
    int64_t i_timestamp,
    const std::shared_ptr<HJTransferMediaData>& i_inputData,
    std::shared_ptr<HJTransferMediaData>& o_data,
    const char* i_logTag)
{
    if (!i_outRgb || i_outWidth <= 0 || i_outHeight <= 0 || i_targetWidth <= 0 || i_targetHeight <= 0)
    {
        return HJErrInvalidParams;
    }

    const unsigned char* finalRgb = i_outRgb->getBuf();
    int finalWidth = i_outWidth;
    int finalHeight = i_outHeight;
    int finalStride = i_outWidth * 3;

    if (i_targetWidth != i_outWidth || i_targetHeight != i_outHeight)
    {
        const int targetSize = i_targetWidth * i_targetHeight * 3;
        if (!m_scaledRgb || m_scaledRgb->getSize() != targetSize)
        {
            m_scaledRgb = HJSPBuffer::create(targetSize);
        }

        int ret = libyuv::RGBScale(i_outRgb->getBuf(), i_outWidth * 3, i_outWidth, i_outHeight,
            m_scaledRgb->getBuf(), i_targetWidth * 3, i_targetWidth, i_targetHeight,
            libyuv::kFilterBilinear);
        if (ret != 0)
        {
            HJFLoge("{} RGBScale failed ret:{} in:{}x{} out:{}x{}",
                i_logTag ? i_logTag : "HJBaseVideoSR", ret, i_outWidth, i_outHeight, i_targetWidth, i_targetHeight);
            return HJErrFatal;
        }

        finalRgb = m_scaledRgb->getBuf();
        finalWidth = i_targetWidth;
        finalHeight = i_targetHeight;
        finalStride = i_targetWidth * 3;
    }

    unsigned char* rgbData[4] = { const_cast<unsigned char*>(finalRgb), nullptr, nullptr, nullptr };
    int rgbPitch[4] = { finalStride, 0, 0, 0 };
    o_data = HJTransferMediaData::create(
        HJConvertDataType_RGB,
        rgbData,
        rgbPitch,
        finalWidth,
        finalHeight,
        i_timestamp,
        HJConvertDataType_RGBA);
    if (!o_data)
    {
        HJFLoge("{} output convert failed", i_logTag ? i_logTag : "HJBaseVideoSR");
        return HJErrFatal;
    }

    if (i_inputData)
    {
        o_data->copymap(*i_inputData);
    }
    return HJ_OK;
}

int HJBaseVideoSR::finalizeSROutputRGBA(
    const std::shared_ptr<HJSPBuffer>& i_outRgba,
    int i_outWidth,
    int i_outHeight,
    int i_targetWidth,
    int i_targetHeight,
    int64_t i_timestamp,
    const std::shared_ptr<HJTransferMediaData>& i_inputData,
    std::shared_ptr<HJTransferMediaData>& o_data,
    const char* i_logTag)
{
    if (!i_outRgba || i_outWidth <= 0 || i_outHeight <= 0 || i_targetWidth <= 0 || i_targetHeight <= 0)
    {
        return HJErrInvalidParams;
    }

    HJSPBuffer::Ptr finalBuffer = i_outRgba;
    int finalWidth = i_outWidth;
    int finalHeight = i_outHeight;
    int finalStride = i_outWidth * 4;

    if (i_targetWidth != i_outWidth || i_targetHeight != i_outHeight)
    {
        const int targetSize = i_targetWidth * i_targetHeight * 4;
        if (!m_scaledRgba || m_scaledRgba->getSize() != targetSize)
        {
            m_scaledRgba = HJSPBuffer::create(targetSize);
        }

        int ret = libyuv::ARGBScale(i_outRgba->getBuf(), i_outWidth * 4, i_outWidth, i_outHeight,
            m_scaledRgba->getBuf(), i_targetWidth * 4, i_targetWidth, i_targetHeight,
            libyuv::kFilterBilinear);
        if (ret != 0)
        {
            HJFLoge("{} ARGBScale failed ret:{} in:{}x{} out:{}x{}",
                i_logTag ? i_logTag : "HJBaseVideoSR", ret, i_outWidth, i_outHeight, i_targetWidth, i_targetHeight);
            return HJErrFatal;
        }

        finalBuffer = m_scaledRgba;
        finalWidth = i_targetWidth;
        finalHeight = i_targetHeight;
        finalStride = i_targetWidth * 4;
    }

    auto rgbaData = std::make_shared<HJTransferMediaDataRGBA>();
    rgbaData->setBuffer(finalBuffer);
    rgbaData->setData(0, finalBuffer->getBuf());
    rgbaData->setStride(0, finalStride);
    rgbaData->setWidth(finalWidth);
    rgbaData->setHeight(finalHeight);
    rgbaData->setTimeStamp(i_timestamp);
    o_data = rgbaData;
    if (!o_data)
    {
        HJFLoge("{} output rgba wrap failed", i_logTag ? i_logTag : "HJBaseVideoSR");
        return HJErrFatal;
    }

    if (i_inputData)
    {
        o_data->copymap(*i_inputData);
    }
    return HJ_OK;
}

HJBaseVideoSR::Ptr HJBaseVideoSR::createVideoSR(HJVideoSRType i_type)
{
    HJBaseVideoSR::Ptr sr = nullptr;
    HJFLogi("HJBaseVideoSR createVideoSR type:{}", (int)i_type);
    switch (i_type)
    {
    case HJVideoSRType_NCNN_RealESRGAN:
        sr = HJSRNCNNRealESRGAN::Create();
        break;
    case HJVideoSRType_NCNN_RealCUGAN:
        sr = HJSRNCNNRealCUGAN::Create();
        break;
    case HJVideoSRType_NCNN_PlainUSR:
        sr = HJSRNCNNPlainUSR::Create();
        break;
    case HJVideoSRType_MindSpore_RealESRGAN:
#if defined(HJ_HAS_MINDSPORE_LITE) && defined(HarmonyOS)
        sr = HJSRMindSporeRealESRGAN::Create();
#else
        HJFLoge("HJBaseVideoSR MindSpore backend unavailable: HarmonyOS + HJ_HAS_MINDSPORE_LITE required");
#endif
        break;
    case HJVideoSRType_CoreML_RealESRGAN:
#if defined(__APPLE__) && TARGET_OS_IOS
        sr = HJSRCoreMLRealESRGAN::Create();
#else
        HJFLoge("HJBaseVideoSR CoreML backend unavailable: iOS required");
#endif
        break;
    case HJVideoSRType_VTFrameProcessor:
#if defined(__APPLE__) && TARGET_OS_IOS
        sr = HJSRVTFrameProcessor::Create();
#else
        HJFLoge("HJBaseVideoSR VTFrameProcessor backend unavailable: iOS required");
#endif
        break;
    default:
        break;
    }

    if (sr)
    {
        sr->setType(i_type);
    }

    return sr;
}

NS_HJ_END
