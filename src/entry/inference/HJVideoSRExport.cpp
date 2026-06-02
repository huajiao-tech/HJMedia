#include "HJVideoSRExport.h"

#include "HJFLog.h"
#include "HJError.h"
#include "HJBaseVideoSR.h"
#include "HJThreadPool.h"
#include "HJTransferMediaData.h"

NS_HJ_USING

HJVideoSRWrapper::HJVideoSRWrapper()
{
    m_threadPool = std::make_shared<HJThreadPool>();
}

HJVideoSRWrapper::~HJVideoSRWrapper()
{
    HJFLogi("HJVideoSRWrapper ~HJVideoSRWrapper enter");
    if (m_threadPool)
    {
        m_threadPool->done();
        m_threadPool = nullptr;
    }
    HJFLogi("HJVideoSRWrapper ~HJVideoSRWrapper end");
}

int HJVideoSRWrapper::init(HJVideoSROutputCb i_cb, HJVideoSRWrapperType i_type, const std::string& i_modelUrl, const HJVideoSRWrapperOption& i_option)
{
    int i_err = HJ_OK;
    HJFLogi("HJVideoSRWrapper init type:{} modelUrl:{}", (int)i_type, i_modelUrl);
    do
    {
        m_outputCb = i_cb;

        HJVideoSRType type = HJVideoSRType_NCNN_RealESRGAN;
        switch (i_type)
        {
        case HJVideoSRWrapperType_NCNNREALESRGAN:
            type = HJVideoSRType_NCNN_RealESRGAN;
            break;
        case HJVideoSRWrapperType_NCNNREALCUGAN:
            type = HJVideoSRType_NCNN_RealCUGAN;
            break;
        case HJVideoSRWrapperType_NCNNPLAINUSR:
			type = HJVideoSRType_NCNN_PlainUSR;
            break;
        case HJVideoSRWrapperType_MINDSPOREREALESRGAN:
            type = HJVideoSRType_MindSpore_RealESRGAN;
            break;
        case HJVideoSRWrapperType_COREMLREALESRGAN:
            type = HJVideoSRType_CoreML_RealESRGAN;
            break;
        case HJVideoSRWrapperType_VTFRAMEPROCESSOR:
            type = HJVideoSRType_VTFrameProcessor;
            break;
        default:
            break;
        }

        m_videoSR = HJBaseVideoSR::createVideoSR(type);
        if (!m_videoSR)
        {
            HJFLoge("HJVideoSRWrapper createVideoSR failed");
            i_err = HJErrNotInited;
            break;
        }

        HJBaseParam::Ptr param = HJBaseParam::Create();
        HJ_CatchMapPlainSetVal(param, std::string, HJBaseVideoSR::s_parammodelPath, i_modelUrl);

        HJVideoSROption::Ptr optionPtr = HJVideoSROption::Create();
#define COPY_OPTION(var) \
        optionPtr->var = i_option.var;\
        HJFLogi("copy option {}:{}", #var, optionPtr->var);

        COPY_OPTION(ncnnRealESRGANType);
        COPY_OPTION(ncnnRealESRGANDenose);
        COPY_OPTION(ncnnRealCUGANType);
        COPY_OPTION(ncnnUseGPU);
        COPY_OPTION(ncnnThreadNums);
        COPY_OPTION(ncnnScale);
        COPY_OPTION(coreMLModelName);
        COPY_OPTION(coreMLComputeMode);
#undef COPY_OPTION

        HJ_CatchMapSetVal(param, HJVideoSROption::Ptr, optionPtr);
        i_err = m_videoSR->init(param);
        if (i_err < 0)
        {
            HJFLoge("HJVideoSRWrapper init error:{}", i_err);
            break;
        }

        i_err = m_threadPool->start();
        if (i_err < 0)
        {
            HJFLoge("HJVideoSRWrapper thread start error:{}", i_err);
            break;
        }
    } while (false);

    HJFLogi("HJVideoSRWrapper init i_err:{}", i_err);
    return i_err;
}

int HJVideoSRWrapper::priProcess(std::shared_ptr<HJ::HJTransferMediaData> i_mediaData)
{
    int i_err = HJ_OK;
    do
    {
        HJSRRet o_ret;
        HJTransferMediaData::Ptr o_data = nullptr;
        i_err = m_videoSR->process(i_mediaData, o_data, o_ret);
        if (i_err < 0)
        {
            HJFLoge("HJVideoSRWrapper process error:{}", i_err);
            break;
        }

        if (m_outputCb)
        {
            m_outputCb(o_data, o_ret);
        }
    } while (false);

    return i_err;
}

int HJVideoSRWrapper::process(std::shared_ptr<HJUnifyWrapperData> i_input, bool i_bSync)
{
    int i_err = HJ_OK;
    do
    {
        if (!m_videoSR)
        {
            i_err = HJErrAlreadyDone;
            break;
        }
        if (!i_input)
        {
            i_err = HJErrInvalidParams;
            break;
        }
        if (m_bError)
        {
            i_err = HJErrInvalid;
            break;
        }

        HJTransferMediaData::Ptr data = nullptr;
        switch (i_input->dataType)
        {
        case HJUnifyWrapperDataType_NV12:
            data = i_bSync
                ? HJTransferMediaDataYUVNV12::CreateView(
                    i_input->data, i_input->nPitch, i_input->width, i_input->height, i_input->timeStamp)
                : HJTransferMediaDataYUVNV12::Create<HJTransferMediaDataYUVNV12>(
                    i_input->data, i_input->nPitch, i_input->width, i_input->height, i_input->timeStamp);
            break;
        case HJUnifyWrapperDataType_RGB:
            data = i_bSync
                ? HJTransferMediaDataRGB::CreateView(
                    i_input->data, i_input->nPitch, i_input->width, i_input->height, i_input->timeStamp)
                : HJTransferMediaDataRGB::Create<HJTransferMediaDataRGB>(
                    i_input->data, i_input->nPitch, i_input->width, i_input->height, i_input->timeStamp);
            break;
        case HJUnifyWrapperDataType_RGBA:
            data = i_bSync
                ? HJTransferMediaDataRGBA::CreateView(
                    i_input->data, i_input->nPitch, i_input->width, i_input->height, i_input->timeStamp)
                : HJTransferMediaDataRGBA::Create<HJTransferMediaDataRGBA>(
                    i_input->data, i_input->nPitch, i_input->width, i_input->height, i_input->timeStamp);
            break;
        default:
            i_err = HJErrInvalidParams;
            break;
        }
        if (i_err < 0)
        {
            break;
        }
        if (!data)
        {
            i_err = HJErrInvalidParams;
            break;
        }

        if (i_bSync)
        {
            i_err = priProcess(data);
            if (i_err < 0)
            {
                HJFLoge("HJVideoSRWrapper process error:{}", i_err);
                break;
            }
        }
        else
        {
            if (m_threadPool)
            {
                m_threadPool->asyncClear([this, data]()
                    {
                        int ret = priProcess(data);
                        if (ret < 0)
                        {
                            HJFLoge("HJVideoSRWrapper process error:{}", ret);
                            m_bError = true;
                        }
                        return ret;
                    }, 889);
            }
        }
    } while (false);

    return i_err;
}
