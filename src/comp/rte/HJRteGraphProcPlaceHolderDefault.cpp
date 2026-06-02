#include "HJFLog.h"

#include "HJRteGraphProc.h"
#include "HJRteComSource.h"
#include "HJRteComDraw.h"
#include "HJOGBaseShader.h"

NS_HJ_BEGIN

int HJRteGraphProcPlaceHolderDefault::constructGraph(HJBaseParam::Ptr i_param)
{
    int i_err = HJ_OK;
    do
    {
		HJRteComSourceBridge::Ptr videoSource = nullptr;
#if defined(HarmonyOS)
		videoSource = HJRteComSourceBridge::Create();
#elif defined(WIN32_LIB)
		videoSource = HJRteComSourceBridgeMediaData::Create();
#endif 
		i_err = videoSource->init(i_param);
		if (i_err < 0)
		{
			HJFLoge("{} m_videoSource init error", getInsName());
			break;
		}
		videoSource->setMainSource(true);
		addSource(videoSource);

        HJRteComDrawEGLUI_0::Ptr uicom = HJRteComDrawEGLUI_0::Create();
        // uicom->setPriority(HJRteComPriority_Target);
        uicom->setSurface(nullptr);
        uicom->setEnable(false);
        addTarget(uicom);

#if 0
		HJRteComSourceBridgeMediaData::Ptr bridgeMediaData = std::dynamic_pointer_cast<HJRteComSourceBridgeMediaData>(m_videoSource);
		HJRteComDrawFBO::Ptr video2D = nullptr;
		if (bridgeMediaData)
		{
			video2D = HJRteComDrawCopy2DFBO::Create();
			HJ_CatchMapPlainSetVal(i_param, bool, "forceYMirror", true);
		}
		else
		{
			video2D = HJRteComDrawCopyOESFBO::Create();
		}
		video2D->init(i_param);
		addFilter(video2D);
		connectCom(m_videoSource, video2D);

		HJOGBaseShader::Ptr shader2D = HJOGBaseShader::createShader(HJOGBaseShaderType_PreMul_Copy_2D);
		connectCom(video2D, uicom, shader2D);
#else
		//OES-2D--Blur--ui
		//       |    |-encoderTarget
		//       |-PBO
		HJRteComSourceBridgeMediaData::Ptr bridgeMediaData = std::dynamic_pointer_cast<HJRteComSourceBridgeMediaData>(videoSource);
		HJRteComDrawFBO::Ptr video2D = nullptr;
		if (bridgeMediaData)
		{
			video2D = HJRteComDrawCopy2DFBO::Create();
		}
		else
		{
			video2D = HJRteComDrawCopyOESFBO::Create();
		}
		video2D->init(i_param);
		addFilter(video2D);
		connectCom(videoSource, video2D);

		HJOGBaseShader::Ptr shader2D = HJOGBaseShader::createShader(HJOGBaseShaderType_PreMul_Copy_2D);
		HJRteComDrawPBOFBODetect::Ptr detectPBO = HJRteComDrawPBOFBODetect::Create();
		detectPBO->setEnable(false);
		detectPBO->init(i_param);
		addTarget(detectPBO); // target is fbo
		connectCom(video2D, detectPBO, shader2D);

		bool bUseCustomFilter = false;
		HJ_CatchMapPlainGetVal(i_param, bool, "useCustomFilter", bUseCustomFilter);

		HJRteComCustomSourceFilter::Ptr customFilter = HJRteComCustomSourceFilter::Create();
		customFilter->setEnable(bUseCustomFilter);
		customFilter->init(i_param);
		addFilter(customFilter);
		connectCom(video2D, customFilter);

		HJRteComDrawBlurCascadeFBO::Ptr blur = HJRteComDrawBlurCascadeFBO::Create();
		blur->setCascadeNum(s_blurCascadeNum);
		blur->init(nullptr);
		blur->setEnable(false);
		addFilter(blur);

		connectCom(customFilter, blur);
		connectCom(blur, uicom, shader2D);

		// faceu
		HJRteComSourceFaceu::Ptr faceu = HJRteComSourceFaceu::Create();
		faceu->init(nullptr);
		faceu->setEnable(false);

		addSource(faceu);
		HJOGBaseShader::Ptr shaderNoPreMul2D = HJOGBaseShader::createShader(HJOGBaseShaderType_Copy_2D);
		connectCom(faceu, uicom, shaderNoPreMul2D);

		if (bridgeMediaData)
		{
			HJRteComDrawPBOFBOTarget_0::Ptr targetPBO = HJRteComDrawPBOFBOTarget_0::Create();
			targetPBO->setEnable(true);
			targetPBO->init(i_param);
			addTarget(targetPBO); // target is fbo
			connectCom(blur, targetPBO, shader2D);
			connectCom(faceu, targetPBO, shaderNoPreMul2D);
		}

		////////encoder
		HJRteComDrawEGLEncoder::Ptr encoderTarget = HJRteComDrawEGLEncoder::Create();
		encoderTarget->setInsName("encoderTarget");
		encoderTarget->setSurface(nullptr);
		encoderTarget->setEnable(false);
		addTarget(encoderTarget);
		connectCom(blur, encoderTarget, shader2D);

		connectCom(faceu, encoderTarget, shaderNoPreMul2D);
#endif
    } while (false);
    return i_err;
}

NS_HJ_END



