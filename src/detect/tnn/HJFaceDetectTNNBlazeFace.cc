#include "HJFaceDetectTNNBlazeFace.h"
#include "HJFLog.h"
#include "HJError.h"
#include "tnn/base/utils/utils.h"
#include "sample_timer.h"
#include "blazeface_detector.h"
#include "macro.h"
#include "HJBaseUtils.h"
#include "HJUtilitys.h"
#include "HJSPBuffer.h"
#include "libyuv.h"
#include "HJDataConvert.h"
#include "HJTransferMediaData.h"

using namespace TNN_NS;
NS_HJ_BEGIN

HJFaceDetectTNNBlazeFace::HJFaceDetectTNNBlazeFace()
{
    HJBaseFaceDetect::setTargetWidth(128);
    HJBaseFaceDetect::setTargetHeight(128);
}

int HJFaceDetectTNNBlazeFace::detect(std::shared_ptr<HJTransferMediaData> i_inputData, HJFaceDetectRet& o_ret)
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

        if (output && dynamic_cast<BlazeFaceDetectorOutput*>(output.get()))
        {
            auto faceInfo = dynamic_cast<BlazeFaceDetectorOutput*>(output.get())->face_list;
            o_ret.m_bContainFace = !faceInfo.empty();
            if (o_ret.m_bContainFace)
            {
                for (int i = 0; i < faceInfo.size(); ++i)
                {
                    auto& face = faceInfo[i];
                    std::vector<std::pair<float, float>> mapped_kps;
                    mapped_kps.reserve(face.key_points.size());
                    for (auto& kp : face.key_points) {
                        float kx = (kp.first - offx) * i_width / scaleW;
                        float ky = (kp.second - offy) * i_height / scaleH;
                        mapped_kps.push_back({ kx, ky });
                    }
                    if (!mapped_kps.empty())
                    {
                        float rectx1 = (face.x1 - offx) * i_width / scaleW;
                        float recty1 = (face.y1 - offy) * i_height / scaleH;
                        float rectx2 = (face.x2 - offx) * i_width / scaleW;
                        float recty2 = (face.y2 - offy) * i_height / scaleH;

                        std::shared_ptr<HJSingleFaceDetectRet> singleFace = std::make_shared<HJSingleFaceDetectRet>();
                        singleFace->setRect(rectx1, recty1, rectx2 - rectx1, recty2 - recty1);
                        for (int idx = 0; idx < mapped_kps.size(); ++idx)
                        {
                            singleFace->addKeyPoint(mapped_kps[idx].first, mapped_kps[idx].second);
                        }
                        o_ret.add(singleFace);
                    }
                }
                
            }
        }
	} while (false);
    return i_err;
}

int HJFaceDetectTNNBlazeFace::init(HJBaseParam::Ptr params)
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
        m_detector = std::dynamic_pointer_cast<BlazeFaceDetector>(initBlazeFace(params));
        if (!m_detector)
        {
            HJFLoge("priInitBlazeFace error");
            i_err = HJErrNotInited;
            break;
        }
	} while (false);
    return i_err;
}

std::shared_ptr<TNNSDKSample> HJFaceDetectTNNBlazeFace::initBlazeFace(HJBaseParam::Ptr params)
{
    std::shared_ptr<TNNSDKSample> detect_sdk = nullptr;
    do
    { 
        std::string blaceproto = HJUtilitys::concatenatePath(m_modelUrls, "TNN/blazeface/blazeface.tnnproto");
        if (!HJBaseUtils::isFileExists(blaceproto))
        {
            HJFLoge("blazeface.tnnproto not exists, blaceproto:{}", blaceproto);
            break;
        }

        auto proto_content = fdLoadFile(blaceproto);
        if (proto_content.empty())
        {
            HJFLoge("load proto error");
            break;
        }

        std::string blacemodel = HJUtilitys::concatenatePath(m_modelUrls, "TNN/blazeface/blazeface.tnnmodel");
        if (!HJBaseUtils::isFileExists(blacemodel))
        {
            HJFLoge("blazeface.tnnproto not exists, blacemodel:{}", blacemodel);
            break;
        }
        auto model_content = fdLoadFile(blacemodel);
        if (model_content.empty())
        {
            HJFLoge("load model error");
            break;
        }

        std::string blaceAnchors = HJUtilitys::concatenatePath(m_modelUrls, "TNN/blazeface/blazeface_anchors.txt");
        if (!HJBaseUtils::isFileExists(blaceAnchors))
        {
            HJFLoge("blazeface_anchors.txt not exists, blaceAnchors:{}", blaceAnchors);
            break;
        }

        auto option = std::make_shared<BlazeFaceDetectorOption>();
        option->proto_content = proto_content;
        option->model_content = model_content;
        option->anchor_path = blaceAnchors;
        option->library_path = "";
        option->compute_units = TNN_NS::TNNComputeUnitsCPU;
#ifdef _CUDA_
        option->compute_units = TNN_NS::TNNComputeUnitsGPU;
#endif
        option->min_score_threshold = 0.75f;
        option->min_suppression_threshold = 0.3f;
        
        detect_sdk = std::make_shared<BlazeFaceDetector>();
        TNN_NS::Status status = detect_sdk->Init(option);
        if (status != TNN_NS::TNN_OK) 
        {
            HJFLoge("BlazeFaceDetector init error");
            detect_sdk = nullptr;
            break;
        }       
    } while (false);
    return detect_sdk;
}
NS_HJ_END
