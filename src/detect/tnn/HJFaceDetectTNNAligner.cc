#include "HJFaceDetectTNNAligner.h"
#include "HJFLog.h"
#include "HJError.h"
#include "tnn/base/utils/utils.h"
#include "blazeface_detector.h"
#include "macro.h"
#include "youtu_face_align.h"
#include "face_detect_aligner.h"
#include "HJBaseUtils.h"
#include "HJUtilitys.h"
#include "HJSPBuffer.h"
#include "libyuv.h"
#include "HJDataConvert.h"
#include "HJTransferMediaData.h"

using namespace TNN_NS;
NS_HJ_BEGIN

HJFaceDetectTNNAligner::HJFaceDetectTNNAligner()
{
    HJBaseFaceDetect::setTargetWidth(128);
    HJBaseFaceDetect::setTargetHeight(128);
}

int HJFaceDetectTNNAligner::detect(std::shared_ptr<HJTransferMediaData> i_inputData, HJFaceDetectRet& o_ret)
{
    int i_err = HJ_OK;
    if (!i_inputData)
    {
        i_err = HJErrInvalidParams;
        return i_err;
    }
    HJConvertDataType i_type = i_inputData->getConvertType();
    unsigned char* i_data[4] = {
        i_inputData->getData(0),
        i_inputData->getData(1),
        i_inputData->getData(2),
        i_inputData->getData(3)
    };
    int i_nPitch[4] = {
        i_inputData->getStride(0),
        i_inputData->getStride(1),
        i_inputData->getStride(2),
        i_inputData->getStride(3)
    };
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


        i_err = HJBaseFaceDetect::convert(i_inputData, HJDataScaleType::Fit, vec);
        if (i_err < 0)
        {
            i_err = HJErrFatal;
            HJFLoge("HJBaseFaceDetect init error:{}", i_err);
            break;
        }
        scaleW = vec.x;
        scaleH = vec.y;
        offx = vec.z;
        offy = vec.w;

        DimsVector input_dims = { 1, 3, m_targetHeight, m_targetWidth };
        auto image_mat = std::make_shared<TNN_NS::Mat>(TNN_NS::DEVICE_NAIVE, TNN_NS::N8UC3, input_dims, m_scaleRgb->getBuf());
        if (!image_mat)
        {
            i_err = HJErrFatal;
            HJFLoge("HJBaseFaceDetect init error:{}", i_err);
            break;
        }

        std::shared_ptr<TNNSDKOutput> output = nullptr;
        TNN_NS::Status status = m_detector->Predict(std::make_shared<TNNSDKInput>(image_mat), output);
        if (status != TNN_NS::TNN_OK)
        {
            HJFLoge("predict error");
            i_err = HJErrFatal;
            break;
        }
        status = m_detector->ProcessSDKOutput(output);
        if (status != TNN_NS::TNN_OK)
        {
            HJFLoge("process error");
            i_err = HJErrFatal;
            break;
        }
        int64_t t1 = HJCurrentSteadyMS();

        o_ret.reset();
        o_ret.setSize(i_width, i_height);
        o_ret.m_elapseMs = (t1 - t0);
        o_ret.m_systemTime = t0;
        o_ret.m_timeStamp = i_timestamp;

        if (output && dynamic_cast<YoutuFaceAlignOutput*>(output.get())) 
        {
            auto face = dynamic_cast<YoutuFaceAlignOutput*>(output.get())->face;

            std::vector<std::pair<float, float>> mapped_kps;
            mapped_kps.reserve(face.key_points.size());
            for (auto& kp : face.key_points) {
                float kx = (kp.first - offx) * i_width / scaleW;
                float ky = (kp.second - offy) * i_height / scaleH;
                mapped_kps.push_back({ kx, ky });
            }
            if (!mapped_kps.empty())
            {
                float min_x = mapped_kps[0].first, max_x = mapped_kps[0].first;
                float min_y = mapped_kps[0].second, max_y = mapped_kps[0].second;
                for (auto& kp : mapped_kps) {
                    min_x = std::min(min_x, kp.first);
                    max_x = std::max(max_x, kp.first);
                    min_y = std::min(min_y, kp.second);
                    max_y = std::max(max_y, kp.second);
                }

                o_ret.m_bContainFace = true;

                //not more than one face
                std::shared_ptr<HJSingleFaceDetectRet> singleFace = std::make_shared<HJSingleFaceDetectRet>();
                singleFace->setRect(min_x, min_y, max_x - min_x, max_y - min_y);
                for (int idx = 0; idx < mapped_kps.size(); ++idx)
                {
                    singleFace->addKeyPoint(mapped_kps[idx].first, mapped_kps[idx].second);
                }
                o_ret.add(singleFace);
            }
        }
	} while (false);
    return i_err;
}

int HJFaceDetectTNNAligner::init(HJBaseParam::Ptr params)
{
	int i_err = HJ_OK;
	do
	{
		i_err = HJFaceDetectTNNBlazeFace::init(params);
        if (i_err != HJ_OK)
        {
			HJFLoge("HJBaseFaceDetect init error:{}", i_err);
            break;
        }
        auto detect_sdk = HJFaceDetectTNNBlazeFace::initBlazeFace(params);
        if (!detect_sdk)
        {
            HJFLoge("priInitBlazeFace error");
            i_err = HJErrNotInited;
            break;
        }
        auto align_sdk1 = priInitAlignSDK(params, 1);
        if (!align_sdk1)
        {
            HJFLoge("priInitAlignSDK1 error");
            i_err = HJErrNotInited;
            break;
        }
        auto align_sdk2 = priInitAlignSDK(params, 2);
        if (!align_sdk2)
        {
            HJFLoge("priInitAlignSDK2 error");
            i_err = HJErrNotInited;
            break;
        }
        m_detector = std::make_shared<FaceDetectAligner>();
        TNN_NS::Status status = m_detector->Init({ detect_sdk, align_sdk1, align_sdk2 });
        if (status != TNN_NS::TNN_OK) 
        {
            HJFLoge("FaceDetectAligner init error");
            m_detector = nullptr;
            break;
        }
	} while (false);
    return i_err;
}

std::shared_ptr<TNNSDKSample> HJFaceDetectTNNAligner::priInitAlignSDK(HJBaseParam::Ptr params, int phase)
{
    std::shared_ptr<TNNSDKSample> sdk = nullptr;
    do
    {
        std::string proto = (phase == 1 ? "youtu_face_alignment_phase1.tnnproto" : "youtu_face_alignment_phase2.tnnproto");
        std::string model = (phase == 1 ? "youtu_face_alignment_phase1.tnnmodel" : "youtu_face_alignment_phase2.tnnmodel");
        std::string mean_pts = (phase == 1 ? "youtu_mean_pts_phase1.txt" : "youtu_mean_pts_phase2.txt");

        proto = HJUtilitys::concatenatePath(m_modelUrls, "TNN/youtu_face_alignment/" + proto);
        if (!HJBaseUtils::isFileExists(proto))
        {
            HJFLoge("proto not exists, proto:{}", proto);
            break;
        }
        model = HJUtilitys::concatenatePath(m_modelUrls, "TNN/youtu_face_alignment/" + model);
        if (!HJBaseUtils::isFileExists(model))
        {
            HJFLoge("model not exists, model:{}", model);
            break;
        }
        mean_pts = HJUtilitys::concatenatePath(m_modelUrls, "TNN/youtu_face_alignment/" + mean_pts);
        if (!HJBaseUtils::isFileExists(mean_pts))
        {
            HJFLoge("mean_pts not exists, mean_pts:{}", mean_pts);
            break;
        }

        auto option = std::make_shared<YoutuFaceAlignOption>();
        option->proto_content = fdLoadFile(proto);
        if (option->proto_content.empty())
        {
            HJFLoge("load proto error");
            break;
        }

        option->model_content = fdLoadFile(model);
        if (option->model_content.empty())
        {
            HJFLoge("load model error");
            break;
        }

        option->library_path = "";
        option->compute_units = TNN_NS::TNNComputeUnitsCPU;
#ifdef _CUDA_
        option->compute_units = TNN_NS::TNNComputeUnitsGPU;
#endif
        option->input_width = m_targetWidth;
        option->input_height = m_targetHeight;
        option->face_threshold = m_option->tnnFaceAlignThreshold;
        option->min_face_size = m_option->tnnFaceAlignMinFaceSize;
        option->phase = phase;
        option->net_scale = phase == 1 ? 1.2f : 1.3f;
        option->mean_pts_path = mean_pts;
        sdk = std::make_shared<YoutuFaceAlign>();
        TNN_NS::Status ret = sdk->Init(option);
        if (ret != TNN_NS::TNN_OK) 
        {
            HJFLoge("YoutuFaceAlign init error");
            sdk = nullptr;
            break;
        }
    } while (false);
    return sdk;
}


NS_HJ_END
