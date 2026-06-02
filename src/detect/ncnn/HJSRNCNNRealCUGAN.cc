#include "HJSRNCNNRealCUGAN.h"

#include "HJFLog.h"
#include "HJError.h"
#include "HJBaseUtils.h"
#include "HJUtilitys.h"
#include "HJTransferMediaData.h"
#include "HJSPBuffer.h"

#include "ncnn/utils/HJNCNNRealCUGANUtils.h"
#include "ncnn/net.h"
#include "ncnn/cpu.h"
#include "ncnn/gpu.h"
#include "libyuv.h"
#include <algorithm>

NS_HJ_BEGIN

HJSRNCNNRealCUGAN::HJSRNCNNRealCUGAN()
{
}

HJSRNCNNRealCUGAN::~HJSRNCNNRealCUGAN()
{
}

//int HJSRNCNNRealCUGAN::parseNoiseLevel(const std::string& type) const
//{
//    if (type.find("noise0") != std::string::npos || type.find("denoise0") != std::string::npos || type.find("no-denoise") != std::string::npos)
//    {
//        return 0;
//    }
//    if (type.find("noise1") != std::string::npos || type.find("denoise1") != std::string::npos)
//    {
//        return 1;
//    }
//    if (type.find("noise2") != std::string::npos || type.find("denoise2") != std::string::npos)
//    {
//        return 2;
//    }
//    if (type.find("noise3") != std::string::npos || type.find("denoise3") != std::string::npos)
//    {
//        return 3;
//    }
//    return -1;
//}
//
//std::string HJSRNCNNRealCUGAN::parseModelFolder(const std::string& type) const
//{
//    if (type.find("models-nose") != std::string::npos || type.find("nose") != std::string::npos)
//    {
//        return "models-nose";
//    }
//    if (type.find("models-pro") != std::string::npos || type.find("pro") != std::string::npos)
//    {
//        return "models-pro";
//    }
//    if (type.find("models-se") != std::string::npos || type.find("realcugan") != std::string::npos)
//    {
//        return "models-se";
//    }
//    return "models-se";
//}

const unsigned char* HJSRNCNNRealCUGAN::priPadBuffer(const unsigned char* rgb, int width, int height)
{
    int prepadding = 18;
    const int inputW = width + prepadding * 2;
    const int inputH = height + prepadding * 2;
    const unsigned char* inputRgb = rgb;
    if (prepadding > 0)
    {
        const int paddedSize = inputW * inputH * 3;
        if (!m_padBuffer || m_padBuffer->getSize() != paddedSize)
        {
            m_padBuffer = HJSPBuffer::create(paddedSize);
        }
        if (!m_padBuffer || !m_padBuffer->getBuf())
        {
            return nullptr;
        }

        unsigned char* rgbPadded = m_padBuffer->getBuf();
        const int srcStride = width * 3;
        const int dstStride = inputW * 3;
        const int sideBytes = prepadding * 3;
        const int centerOffset = prepadding * dstStride + prepadding * 3;

        // Copy center valid region in one shot.
        libyuv::CopyPlane(rgb, srcStride, rgbPadded + centerOffset, dstStride, srcStride, height);
#if 0           
        {
            int pitches[4] = { inputW * 3, 0, 0, 0 };
            unsigned char* planes[4] = { rgbPadded, nullptr, nullptr, nullptr };
            HJDataConvert::cvtSaveToImage(HJConvertDataType_RGB, planes, pitches, inputW, inputH, HJUtilitys::concatenatePath(HJUtilitys::exeDir(), "PADRGB1.png"));
        }
#endif
        // Fill left/right borders for middle rows.
        for (int y = 0; y < height; y++)
        {
            const unsigned char* srcRow = rgb + (size_t)y * srcStride;
            unsigned char* dstRow = rgbPadded + (size_t)(prepadding + y) * dstStride;
            unsigned char* dstLeft = dstRow;
            unsigned char* dstRight = dstRow + (size_t)(prepadding + width) * 3;

            // Left: replicate first source pixel.
            std::memcpy(dstLeft, srcRow, 3);
            for (int filled = 3; filled < sideBytes;)
            {
                const int copyBytes = (std::min)(filled, sideBytes - filled);
                std::memcpy(dstLeft + filled, dstLeft, (size_t)copyBytes);
                filled += copyBytes;
            }

            // Right: replicate last source pixel.
            const unsigned char* srcLast = srcRow + (size_t)(width - 1) * 3;
            std::memcpy(dstRight, srcLast, 3);
            for (int filled = 3; filled < sideBytes;)
            {
                const int copyBytes = (std::min)(filled, sideBytes - filled);
                std::memcpy(dstRight + filled, dstRight, (size_t)copyBytes);
                filled += copyBytes;
            }
        }

        // Fill top/bottom borders by copying already prepared first/last middle rows.
        unsigned char* firstValidRow = rgbPadded + (size_t)prepadding * dstStride;
        unsigned char* lastValidRow = rgbPadded + (size_t)(prepadding + height - 1) * dstStride;
        libyuv::CopyPlane(firstValidRow, dstStride, rgbPadded, dstStride, dstStride, prepadding);
        libyuv::CopyPlane(lastValidRow, dstStride,
            rgbPadded + (size_t)(prepadding + height) * dstStride, dstStride, dstStride, prepadding);
#if 0           
        {
            int pitches[4] = { inputW * 3, 0, 0, 0 };
            unsigned char* planes[4] = { rgbPadded, nullptr, nullptr, nullptr };
            HJDataConvert::cvtSaveToImage(HJConvertDataType_RGB, planes, pitches, inputW, inputH, HJUtilitys::concatenatePath(HJUtilitys::exeDir(), "PADRGB2.png"));
        }
#endif
        inputRgb = rgbPadded;
    }
    return inputRgb;
}

int HJSRNCNNRealCUGAN::process(std::shared_ptr<HJTransferMediaData> i_inputData, std::shared_ptr<HJTransferMediaData>& o_data, HJSRRet& o_ret)
{
    int i_err = HJ_OK;
    if (!i_inputData)
    {
        return HJErrInvalidParams;
    }
    if (!m_net || !m_realcuganUtils || m_inputName.empty() || m_outputName.empty())
    {
        return HJErrNotInited;
    }

    int i_width = 0;
    int i_height = 0;
    int64_t i_timestamp = 0;
    const unsigned char* inputRgb = nullptr;

    do
    {
        i_err = prepareInputRgb(i_inputData, inputRgb, i_width, i_height, i_timestamp, "HJSRNCNNRealCUGAN");
        if (i_err < 0)
        {
            break;
        }

        const int targetScale = (std::min)((std::max)(m_option ? m_option->ncnnScale : 2, 1), 4);
        const int targetWidth = i_width * targetScale;
        const int targetHeight = i_height * targetScale;

        HJSPBuffer::Ptr outRgb = nullptr;
        int outWidth = 0;
        int outHeight = 0;

        const int64_t t0 = HJCurrentSteadyMS();

#if 0
        const unsigned char *padBuf = priPadBuffer(m_inputRgb->getBuf(), i_width, i_height);
        int ret = HJBaseVideoSR::inference(m_inputName, m_outputName, padBuf, i_width, i_height, outRgb, outWidth, outHeight);
#else
        int ret = m_realcuganUtils->process(*m_net, m_inputName, m_outputName, inputRgb, i_width, i_height,
            targetWidth, targetHeight, outRgb, outWidth, outHeight);
#endif
        if (ret != 0 || !outRgb || outWidth <= 0 || outHeight <= 0)
        {
            i_err = HJErrFatal;
            HJFLoge("HJSRNCNNRealCUGAN process failed ret:{}", ret);
            break;
        }
		int64_t t1 = HJCurrentSteadyMS();
		o_ret.m_elapseMs = t1 - t0;
		o_ret.m_systemTime = t0;
		o_ret.m_timeStamp = i_timestamp;
		i_err = HJBaseVideoSR::finalizeSROutput(outRgb, outWidth, outHeight, targetWidth, targetHeight, i_timestamp, i_inputData, o_data, "HJSRNCNNRealCUGAN");
		if (i_err < 0)
		{
			break;
		}
		int64_t t2 = HJCurrentSteadyMS();
		HJFPERLog5i("HJSRNCNNRealCUGAN process type:{} in:{}x{} out:{}x{} elapseMs:{}",
            (int)i_inputData->getConvertType(), i_width, i_height, outWidth, outHeight, (t2 - t0));
    } while (false);
    return i_err;
}

int HJSRNCNNRealCUGAN::init(HJBaseParam::Ptr params)
{
    int i_err = HJ_OK;
    do
    {
        i_err = HJBaseVideoSR::init(params);
        if (i_err != HJ_OK)
        {
            HJFLoge("HJBaseVideoSR init error:{}", i_err);
            break;
        }
		m_modelScale = 2;
        //m_modelScale = (std::min)((std::max)(m_option ? m_option->ncnnScale : 2, 1), 4);
        //const std::string type = m_option ? m_option->ncnnRealESRGANType : "";
        //m_noiseLevel = parseNoiseLevel(type);
        //const std::string modelFolder = parseModelFolder(type);

        //const std::string modelBaseName =
        //    m_noiseLevel == -1 ? HJFMT("up{}x-conservative", m_modelScale)
        //    : (m_noiseLevel == 0 ? HJFMT("up{}x-no-denoise", m_modelScale)
        //                         : HJFMT("up{}x-denoise{}x", m_modelScale, m_noiseLevel));
        //const std::string modelRoot = HJUtilitys::concatenatePath(m_modelUrls, "NCNN/realcugan/" + modelFolder);
        //const std::string paramPath = HJUtilitys::concatenatePath(modelRoot, modelBaseName + ".param");
        //const std::string binPath = HJUtilitys::concatenatePath(modelRoot, modelBaseName + ".bin");
        //if (!HJBaseUtils::isFileExists(paramPath) || !HJBaseUtils::isFileExists(binPath))
        //{
        //    HJFLoge("realcugan model not exists param:{} bin:{}", paramPath, binPath);
        //    i_err = HJErrInvalidFile;
        //    break;
        //}
        std::string name = "up2x-conservative";
        if (m_option->ncnnRealCUGANType == "conservative")
        {
            name = "up2x-conservative";
        }
        else if (m_option->ncnnRealCUGANType == "no-denoise")
        {
            name = "up2x-no-denoise";
        }
         //"up2x-no-denoise";// "up2x-conservative";
        std::string param_path = HJUtilitys::concatenatePath(m_modelUrls, "NCNN/realcugan/" + name + ".param");
        if (!HJBaseUtils::isFileExists(param_path))
        {
            HJFLoge("realesrgan param not exists, path:{}", param_path);
            i_err = HJErrInvalidFile;
            break;
        }

        std::string model_path = HJUtilitys::concatenatePath(m_modelUrls, "NCNN/realcugan/" + name + ".bin");
        if (!HJBaseUtils::isFileExists(model_path))
        {
            HJFLoge("realesrgan model not exists, path:{}", model_path);
            i_err = HJErrInvalidFile;
            break;
        }

		i_err = configNet();
		if (i_err < 0)
		{
			HJFLoge("configNet failed i_err:{}", i_err);
			break;
		}

        const int64_t t0 = HJCurrentSteadyMS();
        if (m_net->load_param(param_path.c_str()) != 0)
        {
            HJFLoge("realcugan load param failed:{}", param_path);
            i_err = HJErrNotInited;
            break;
        }
        const int64_t t1 = HJCurrentSteadyMS();
        if (m_net->load_model(model_path.c_str()) != 0)
        {
            HJFLoge("realcugan load model failed:{}", model_path);
            i_err = HJErrNotInited;
            break;
        }
        const int64_t t2 = HJCurrentSteadyMS();

        const std::vector<const char*>& in_names = m_net->input_names();
        const std::vector<const char*>& out_names = m_net->output_names();
        if (in_names.empty() || out_names.empty())
        {
            HJFLoge("realcugan input/output names empty");
            i_err = HJErrNotInited;
            break;
        }
        m_inputName = in_names[0];
        m_outputName = out_names[0];

        m_realcuganUtils = std::make_unique<HJNCNNRealCUGANUtils>();
        HJFLogi("realcugan init done scale:{} param:{}ms model:{}ms total:{}ms", m_modelScale, t1 - t0, t2 - t1, t2 - t0);
    } while (false);
    return i_err;
}

NS_HJ_END
