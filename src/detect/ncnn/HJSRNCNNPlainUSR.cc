#include "HJSRNCNNPlainUSR.h"

#include "HJFLog.h"
#include "HJError.h"
#include "HJBaseUtils.h"
#include "HJUtilitys.h"
#include "HJTransferMediaData.h"
#include "HJSPBuffer.h"

#include "ncnn/net.h"
#include "ncnn/cpu.h"
#include "libyuv.h"
#include <algorithm>

NS_HJ_BEGIN

HJSRNCNNPlainUSR::HJSRNCNNPlainUSR()
{

}

HJSRNCNNPlainUSR::~HJSRNCNNPlainUSR()
{
}

int HJSRNCNNPlainUSR::process(std::shared_ptr<HJTransferMediaData> i_inputData, std::shared_ptr<HJTransferMediaData>& o_data, HJSRRet& o_ret)
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
        i_err = prepareInputRgb(i_inputData, inputRgb, i_width, i_height, i_timestamp, "HJSRNCNNPlainUSR");
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
            HJFLoge("process failed ret:{}", ret);
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
#if 0
        // --- Debug: Set top-left nxn area to red ---
        if (o_data && o_data->getConvertType() == HJConvertDataType_RGBA) {
            unsigned char* rgba = o_data->getData(0);
            int stride = o_data->getStride(0);
            int w = o_data->getWidth();
            int h = o_data->getHeight();
            int rectSize = 30;
            if (rgba && w >= rectSize && h >= rectSize) {
                for (int y = 0; y < rectSize; ++y) {
                    unsigned char* row = rgba + (size_t)y * (size_t)stride;
                    for (int x = 0; x < rectSize; ++x) {
                        row[x * 4 + 0] = 255; // R
                        row[x * 4 + 1] = 0;   // G
                        row[x * 4 + 2] = 0;   // B
                        row[x * 4 + 3] = 255; // A
                    }
                }
            }
        }
        // --- End Debug ---
#endif

        int64_t t2 = HJCurrentSteadyMS();
        HJFPERLog5i("HJSRNCNNPlainUSR process type:{} in:{}x{} out:{}x{} elapseMs:{}",
            (int)i_inputData->getConvertType(), i_width, i_height, outWidth, outHeight, (t2 - t0));
    } while (false);
    return i_err;
}

int HJSRNCNNPlainUSR::init(HJBaseParam::Ptr params)
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

        m_modelScale = 2;
        name = "PlainUSRU_Rep_x2-opt";

        //if (m_option->ncnnRealESRGANType == "realesr-general-x4v3")
        //{

        //}

        std::string param_path = HJUtilitys::concatenatePath(m_modelUrls, "NCNN/plainusr/" + name + ".param");
        if (!HJBaseUtils::isFileExists(param_path))
        {
            HJFLoge("param not exists, path:{}", param_path);
            i_err = HJErrInvalidFile;
            break;
        }

        std::string model_path = HJUtilitys::concatenatePath(m_modelUrls, "NCNN/plainusr/" + name + ".bin");
        if (!HJBaseUtils::isFileExists(model_path))
        {
            HJFLoge("model not exists, path:{}", model_path);
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
            HJFLoge("load param failed");
            i_err = HJErrNotInited;
            break;
        }
        int64_t t1 = HJCurrentSteadyMS();
        if (m_net->load_model(model_path.c_str()))
        {
            HJFLoge("load model failed");
            i_err = HJErrNotInited;
            break;
        }
        int64_t t2 = HJCurrentSteadyMS();
        HJFLogi("load param:{} and model:{} total elapse:{}", t1 - t0, t2 - t1, t2 - t0);

        const std::vector<const char*>& in_names = m_net->input_names();
        const std::vector<const char*>& out_names = m_net->output_names();
        if (in_names.empty() || out_names.empty())
        {
            HJFLoge("input/output names empty");
            i_err = HJErrNotInited;
            break;
        }
        m_inputName = in_names[0];
        m_outputName = out_names[0];
    } while (false);
    return i_err;
}

NS_HJ_END
