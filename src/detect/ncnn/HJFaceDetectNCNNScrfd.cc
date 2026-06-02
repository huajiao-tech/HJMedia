#include "HJFaceDetectNCNNScrfd.h"

#include "HJFLog.h"
#include "HJError.h"
#include "HJBaseUtils.h"
#include "HJUtilitys.h"
#include "HJTransferMediaData.h"

#include "ncnn/utils/HJNCNNScrfdUtils.h"
#include "ncnn/net.h"

NS_HJ_BEGIN

HJFaceDetectNCNNScrfd::HJFaceDetectNCNNScrfd()
{

}

HJFaceDetectNCNNScrfd::~HJFaceDetectNCNNScrfd()
{
}

int HJFaceDetectNCNNScrfd::detect(std::shared_ptr<HJTransferMediaData> i_inputData, HJFaceDetectRet& o_ret)
{
    int i_err = HJ_OK;
    if (!i_inputData)
    {
        return HJErrInvalidParams;
    }
    if (!m_net || !m_scrfd)
    {
        return HJErrNotInited;
    }

    int i_width = i_inputData->getWidth();
    int i_height = i_inputData->getHeight();
    int64_t i_timestamp = i_inputData->getTimeStamp();

    if ((i_width <= 0) || (i_height <= 0))
    {
        HJFLoge("HJBaseFaceDetect detect i_width:{}, i_height:{}", i_width, i_height);
        return HJErrInvalidParams;
    }

    do
    {
        int scaleW = 0;
        int scaleH = 0;
        int offx = 0;
        int offy = 0;

        int64_t t0 = HJCurrentSteadyMS();
        HJVec4i vec = { 0, 0, 0, 0 };

        //int target_size = m_ncnnRetinaFaceTargetSize;
        int targetWidth = m_option->ncnnScrfdTargetSize;
        int targetHeight = m_option->ncnnScrfdTargetSize;
        
        HJBaseFaceDetect::setTargetWidth(targetWidth);
		HJBaseFaceDetect::setTargetHeight(targetHeight);

        HJDataScaleType scaleType = m_option->ncnnScrfdEqualScale ? HJDataScaleType::Fit : HJDataScaleType::Full;
        i_err = HJBaseFaceDetect::convert(i_inputData, scaleType, vec);
        if (i_err < 0)
        {
            i_err = HJErrFatal;
            HJFLoge("HJBaseFaceDetect convert error:{}", i_err);
            break;
        }
        HJFPERLog5i("detect type:{} src:{}x{} target:{}x{} scale:{}x{} off:{}x{}",
            (int)i_inputData->getConvertType(), i_width, i_height, m_targetWidth, m_targetHeight, vec.x, vec.y, vec.z, vec.w);
        scaleW = vec.x;
        scaleH = vec.y;
        offx = vec.z;
        offy = vec.w;

        std::vector<HJNCNNFaceObject> faceobjects;
        int ret = m_scrfd->detect(*m_net, m_scaleRgb->getBuf(), m_targetWidth, m_targetHeight, faceobjects,
            m_option->ncnnScrfdProbThreshold, m_option->ncnnScrfdNmsThreshold);
        if (ret != 0)
        {
            i_err = HJErrFatal;
            HJFLoge("NCNN scrfd detect error:{}", ret);
            break;
        }
        int64_t t1 = HJCurrentSteadyMS();

        o_ret.reset();
        o_ret.setSize(i_width, i_height);
        o_ret.m_elapseMs = (t1 - t0);
        o_ret.m_systemTime = t0;
        o_ret.m_timeStamp = i_timestamp;
        o_ret.m_bContainFace = !faceobjects.empty();
        if (o_ret.m_bContainFace)
        {
            for (size_t i = 0; i < faceobjects.size(); i++)
            {
                const auto& face = faceobjects[i];
                float x0 = (face.x - offx) * i_width / scaleW;
                float y0 = (face.y - offy) * i_height / scaleH;
                float x1 = (face.x + face.w - offx) * i_width / scaleW;
                float y1 = (face.y + face.h - offy) * i_height / scaleH;

                std::shared_ptr<HJSingleFaceDetectRet> singleFace = std::make_shared<HJSingleFaceDetectRet>();
                singleFace->setRect(x0, y0, (x1 - x0), (y1 - y0));
                for (int idx = 0; idx < 5; ++idx)
                {
                    float kx = (face.landmark[idx][0] - offx) * i_width / scaleW;
                    float ky = (face.landmark[idx][1] - offy) * i_height / scaleH;
                    singleFace->addKeyPoint(kx, ky);
                }
                o_ret.add(singleFace);
            }     
        }
    } while (false);
    return i_err;
}

int HJFaceDetectNCNNScrfd::init(HJBaseParam::Ptr params)
{
    int i_err = HJ_OK;
    do
    {
        i_err = HJBaseFaceDetect::init(params);
        if (i_err != HJ_OK)
        {
            HJFLoge("HJBaseFaceDetect init error:{}", i_err);
            break;
        }

        std::string param_path = HJUtilitys::concatenatePath(m_modelUrls, "NCNN/scrfd/scrfd_500m_bnkps_shape320x320.opt.param");
        if (!HJBaseUtils::isFileExists(param_path))
        {
            HJFLoge("scrfd param not exists, path:{}", param_path);
            i_err = HJErrInvalidFile;
            break;
        }

        std::string model_path = HJUtilitys::concatenatePath(m_modelUrls, "NCNN/scrfd/scrfd_500m_bnkps_shape320x320.opt.bin");
        if (!HJBaseUtils::isFileExists(model_path))
        {
            HJFLoge("scrfd model not exists, path:{}", model_path);
            i_err = HJErrInvalidFile;
            break;
        }

        m_net = std::make_unique<ncnn::Net>();
        m_net->opt.use_vulkan_compute = m_option->ncnnScrfdUseGPU;
        m_net->opt.num_threads = m_option->ncnnScrfdThreadNums;

        if (m_net->load_param(param_path.c_str()))
        {
            HJFLoge("scrfd load param failed");
            i_err = HJErrNotInited;
            break;
        }
        if (m_net->load_model(model_path.c_str()))
        {
            HJFLoge("scrfd load model failed");
            i_err = HJErrNotInited;
            break;
        }

        m_scrfd = std::make_unique<HJNCNNScrfdUtils>();
    } while (false);
    return i_err;
}

NS_HJ_END
