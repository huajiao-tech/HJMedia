#pragma once

#include "HJPrerequisites.h"
#include "HJReflCppJson.h"
#include "HJComUtils.h"

NS_HJ_BEGIN

typedef enum HJRenderSurfaceType
{
    HJOGEGLSurfaceType_Default   = 0,
    HJOGEGLSurfaceType_UI,
    HJOGEGLSurfaceType_EncoderPusher,
    HJOGEGLSurfaceType_EncoderRecord,
    HJOGEGLSurfaceType_ImageReceiver,
} HJRenderSurfaceType;

typedef enum HJTransferCommandType
{
	HJTRANSFER_NONE             = 0,
	HJTRANSFER_SUSPEND          = 1,
	
    HJTRANSFER_RENDERENV        = 2,
	HJTRANSFER_RENDERMODE       = 3,
    HJTRANSFER_RENDERTARGET     = 4,

} HJTransferCommandType;

typedef enum HJTransferRenderTargetState
{
	HJTargetState_Create  = 0,
    HJTargetState_Change  = 1,
    HJTargetState_Destroy = 2,
} HJTransferRenderTargetState;

using HJTransferFun = std::function<int (const std::string& i_info)>;

class HJTransferRenderEnvInfo : public HJReflCppJsonInterpreter<HJTransferRenderEnvInfo>
{
public:    
 	bool bUseRenderEnv = true;
 	int renderFps = 30;
};

class HJTransferRenderViewPortInfo 
{
public:
    HJ_DEFINE_CREATE(HJTransferRenderViewPortInfo);
    HJTransferRenderViewPortInfo()
    {
    
    }
    static std::shared_ptr<HJTransferRenderViewPortInfo> Create(int i_x, int i_y, int i_w, int i_h)
    {
        std::shared_ptr<HJTransferRenderViewPortInfo> viewport = HJTransferRenderViewPortInfo::Create();
        viewport->x = i_x;
        viewport->y = i_y;
        viewport->width = i_w;
        viewport->height = i_h;
        return viewport;
    }
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

class HJTransferRenderModeInfo : public HJReflCppJsonInterpreter<HJTransferRenderModeInfo>
{
public:
    HJ_DEFINE_CREATE(HJTransferRenderModeInfo);
    HJTransferRenderModeInfo()
    {
        
    }    
    static HJTransferRenderViewPortInfo::Ptr compute(HJTransferRenderModeInfo::Ptr i_renderModeInfo, int i_targetWidth, int i_targetHeight)
    {
        int x = i_targetWidth * i_renderModeInfo->viewOffx;
        int y = (1.f - i_renderModeInfo->viewHeight - i_renderModeInfo->viewOffy) * i_targetHeight;
        int w = i_targetWidth * i_renderModeInfo->viewWidth;
        int h = i_targetHeight * i_renderModeInfo->viewHeight;
        return HJTransferRenderViewPortInfo::Create(x, y, w, h);
    }
    static HJTransferRenderViewPortInfo::Ptr compute(HJTransferRenderModeInfo::Ptr i_renderModeInfo, HJTransferRenderModeInfo::Ptr i_configModeInfo, int i_targetWidth, int i_targetHeight)
    {
        int x = i_targetWidth * i_renderModeInfo->viewOffx + i_targetWidth * i_renderModeInfo->viewWidth * i_configModeInfo->viewOffx;
        int topy = i_targetHeight * i_renderModeInfo->viewOffy + i_targetHeight * i_renderModeInfo->viewHeight * i_configModeInfo->viewOffy;
        int w = i_targetWidth * i_renderModeInfo->viewWidth * i_configModeInfo->viewWidth;
        int h = i_targetHeight * i_renderModeInfo->viewHeight * i_configModeInfo->viewHeight;
        int y = i_targetHeight - topy - h;
        return HJTransferRenderViewPortInfo::Create(x, y, w, h);
    }
	std::string color = "BT601";      //BT601  BT709; only soft decode support
 	int cropMode = (int)HJWindowRenderMode_CLIP;     //fit clip origin full 

    bool bAlphaVideoLeftRight = false;

    double viewOffx = 0.0;             //left
    double viewOffy = 0.0;            //top
    double viewWidth = 1.0;
    double viewHeight = 1.0;
};

class HJTransferRenderTargetInfo : public HJReflCppJsonInterpreter<HJTransferRenderTargetInfo>
{
public:    
    int64_t nativeWindow = 0;
    int width = 0;
    int height = 0;   
    int state = HJTargetState_Create;
    int type = HJOGEGLSurfaceType_Default;
    int fps = 30;
};


NS_HJ_END

REFL_AUTO_SIMPLE(HJ::HJTransferRenderEnvInfo, bUseRenderEnv, renderFps);
REFL_AUTO_SIMPLE(HJ::HJTransferRenderModeInfo, color, cropMode, bAlphaVideoLeftRight, viewOffx, viewOffy, viewWidth, viewHeight);
REFL_AUTO_SIMPLE(HJ::HJTransferRenderTargetInfo, nativeWindow, width, height, state, type, fps);