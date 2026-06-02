#include "HJFLog.h"

#include "HJRteGraphProc.h"
#include "HJRteComSource.h"
#include "HJRteComDraw.h"
#include "HJOGBaseShader.h"

NS_HJ_BEGIN

int HJRteGraphProcSplictScreen::constructGraph(HJBaseParam::Ptr i_param)
{
    int i_err = HJ_OK;
    do
    { 
        bool bSplitScreenMirror = false;

        HJRteComSourceBridge::Ptr videoSource = nullptr;
#if defined(HarmonyOS)
        videoSource = HJRteComSourceSplitScreen::Create();
#elif defined(WIN32_LIB)
        videoSource = HJRteComSourceSplitScreenMediaData::Create();
#endif
        HJ_CatchMapPlainGetVal(i_param, bool, "IsMirror", bSplitScreenMirror);

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

        HJRteComSourceSplitScreenMediaData::Ptr bridgeMediaData = std::dynamic_pointer_cast<HJRteComSourceSplitScreenMediaData>(videoSource);
        HJOGBaseShader::Ptr shaderSplitScreenLR = nullptr;
        if (bridgeMediaData)
        {
            shaderSplitScreenLR = HJOGBaseShader::createShader(HJOGBaseShaderType_SplitScreenLR_2D);
        }
        else
        {
            shaderSplitScreenLR = HJOGBaseShader::createShader(HJOGBaseShaderType_SplitScreenLR_OES);
        }

        HJOGCopyShaderStrip::Ptr copyShader = std::dynamic_pointer_cast<HJOGCopyShaderStrip>(shaderSplitScreenLR);
        if (copyShader)
        {
            copyShader->setForceXMirror(bSplitScreenMirror);
        }
        connectCom(videoSource, uicom, shaderSplitScreenLR);

        if (bridgeMediaData)
        {
            HJRteComDrawPBOFBOTarget_0::Ptr targetPBO = HJRteComDrawPBOFBOTarget_0::Create();
            targetPBO->setEnable(true);
            targetPBO->init(i_param);
            addTarget(targetPBO);
            connectCom(videoSource, targetPBO, shaderSplitScreenLR);
        }
    } while (false);
    return i_err;
}

NS_HJ_END



