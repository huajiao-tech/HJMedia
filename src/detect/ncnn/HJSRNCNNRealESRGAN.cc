#include "HJSRNCNNRealESRGAN.h"

#include "HJFLog.h"
#include "HJError.h"
#include "HJBaseUtils.h"
#include "HJUtilitys.h"
#include "HJTransferMediaData.h"
#include "HJSPBuffer.h"
#include "ncnn/net.h"
#include "libyuv.h"
#include <algorithm>

NS_HJ_BEGIN

HJSRNCNNRealESRGAN::HJSRNCNNRealESRGAN()
{

}

HJSRNCNNRealESRGAN::~HJSRNCNNRealESRGAN()
{
}

int HJSRNCNNRealESRGAN::process(std::shared_ptr<HJTransferMediaData> i_inputData, std::shared_ptr<HJTransferMediaData>& o_data, HJSRRet& o_ret)
{
    int i_err = HJ_OK;
    if (!i_inputData)
    {
        return HJErrInvalidParams;
    }
    if (!m_net)
    {
        return HJErrNotInited;
    }

    int i_width = 0;
    int i_height = 0;
    int64_t i_timestamp = 0;
    const unsigned char* inputRgb = nullptr;

    do
    {
        i_err = prepareInputRgb(i_inputData, inputRgb, i_width, i_height, i_timestamp, "HJSRNCNNRealESRGAN");
        if (i_err < 0)
        {
            break;
        }

        int64_t t0 = HJCurrentSteadyMS();

        int targetScale = (std::max)(1, m_option ? m_option->ncnnScale : 1);
        targetScale = (std::min)(targetScale, 4);
        int targetWidth = i_width * targetScale;
        int targetHeight = i_height * targetScale;

        HJSPBuffer::Ptr outRgb = nullptr;
        int outWidth = 0;
        int outHeight = 0;
        int ret = HJBaseVideoSR::inference(m_inputName, m_outputName, inputRgb, i_width, i_height, outRgb, outWidth, outHeight);
        if (ret != 0 || !outRgb || outWidth <= 0 || outHeight <= 0)
        {
            i_err = HJErrFatal;
            HJFLoge("HJSRNCNNRealESRGAN process failed ret:{}", ret);
            break;
        }
        int64_t t1 = HJCurrentSteadyMS();
        o_ret.m_elapseMs = t1 - t0;
        o_ret.m_systemTime = t0;
        o_ret.m_timeStamp = i_timestamp;
        i_err = HJBaseVideoSR::finalizeSROutput(outRgb, outWidth, outHeight, targetWidth, targetHeight, i_timestamp, i_inputData, o_data, "HJSRNCNNRealESRGAN");
        if (i_err < 0)
        {
            break;
        }
        int64_t t2 = HJCurrentSteadyMS();
        HJFPERLog5i("HJSRNCNNRealESRGAN process type:{} in:{}x{} out:{}x{} elapseMs:{}",
            (int)i_inputData->getConvertType(), i_width, i_height, outWidth, outHeight, (t2 - t0));
    } while (false);
    return i_err;
}

int HJSRNCNNRealESRGAN::init(HJBaseParam::Ptr params)
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

        std::string name = "";
        if (m_option->ncnnRealESRGANType == "realesr-general-x4v3")
        {
            name = "realesr-general-x4v3-dn0500-opt";//"realesr-general-x4v3-dn0800";//"realesr-general-x4v3-dn0200";//"realesr-general-x4v3-dn0500-opt";
            if (m_option->ncnnRealESRGANDenose == 0.f)
            {
                name = "realesr-general-x4v3";
            }
            else if (m_option->ncnnRealESRGANDenose == 0.2f)
            {
                name = "realesr-general-x4v3-dn0200";
            }
            else if (m_option->ncnnRealESRGANDenose == 0.5f)
            {
                name = "realesr-general-x4v3-dn0500-opt";
            }
            else if (m_option->ncnnRealESRGANDenose == 0.8f)
            {
                name = "realesr-general-x4v3-dn0800";
            }
            else if (m_option->ncnnRealESRGANDenose == 1.f)
            {
                name = "realesr-general-wdn-x4v3";
            }
            m_modelScale = 4;

            HJFLogi("realesr-general init denoise:{} model:{} scale:{}", m_option->ncnnRealESRGANDenose, name, m_modelScale);
        }
        else if (m_option->ncnnRealESRGANType == "realesr-animevideov3-x2")
        {
            name = "realesr-animevideov3-x2";
            m_modelScale = 2;
        }
        else if (m_option->ncnnRealESRGANType == "realesrgan-x2plus")
        {
            name = "realesrgan-x2plus";
            m_modelScale = 2;
        }
        else
        {
            HJFLoge("invalid ncnnRealESRGANType:{}", m_option->ncnnRealESRGANType);
            i_err = HJErrInvalidParams;
            break;
        }
 		HJFLogi("HJSRNCNNRealESRGAN init model:{} scale:{}", name, m_modelScale);
        std::string param_path = HJUtilitys::concatenatePath(m_modelUrls, "NCNN/realesr/" + name + ".param");
        if (!HJBaseUtils::isFileExists(param_path))
        {
            HJFLoge("realesrgan param not exists, path:{}", param_path);
            i_err = HJErrInvalidFile;
            break;
        }

        std::string model_path = HJUtilitys::concatenatePath(m_modelUrls, "NCNN/realesr/" + name + ".bin");
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
        int64_t t0 = HJCurrentSteadyMS();
        if (m_net->load_param(param_path.c_str()))
        {
            HJFLoge("realesrgan load param failed");
            i_err = HJErrNotInited;
            break;
        }
        int64_t t1 = HJCurrentSteadyMS();
        if (m_net->load_model(model_path.c_str()))
        {
            HJFLoge("realesrgan load model failed");
            i_err = HJErrNotInited;
            break;
        }
        int64_t t2 = HJCurrentSteadyMS();
        HJFLogi("realesrgan load param:{} and model:{} total elapse:{}", t1 - t0, t2 - t1, t2 - t0);



        const std::vector<const char*>& in_names = m_net->input_names();
        const std::vector<const char*>& out_names = m_net->output_names();
        if (in_names.empty() || out_names.empty())
        {
            HJFLoge("realesrgan input/output names empty");
            i_err = HJErrNotInited;
            break;
        }
        m_inputName = in_names[0];
        m_outputName = out_names[0];
    } while (false);
    return i_err;
}

NS_HJ_END
