#pragma once

#include "HJPrerequisites.h"
#include "HJJsonBase.h"

NS_HJ_BEGIN

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

class HJTransferRenderEnvInfo : public HJJsonBase
{
public:
    virtual int deserialInfo(const HJYJsonObject::Ptr &i_obj = nullptr)
    {
        int i_err = HJ_OK;
        do
        {
            HJ_JSON_BASE_DESERIAL(m_obj, i_obj, bUseRenderEnv, renderFps);
        } while (false);
        return i_err;
    }
    
    virtual int serialInfo(const HJYJsonObject::Ptr &i_obj = nullptr)
    {
        int i_err = HJ_OK;
        do
        {
            HJ_JSON_BASE_SERIAL(m_obj, i_obj, bUseRenderEnv, renderFps);
        } while (false);
        return i_err;
    }
    
 	bool bUseRenderEnv = true;
 	int renderFps = 30;
};

class HJTransferRenderModeInfo : public HJJsonBase
{
public:
    virtual int deserialInfo(const HJYJsonObject::Ptr &i_obj = nullptr)
    {
        int i_err = HJ_OK;
        do
        {
            HJ_JSON_BASE_DESERIAL(m_obj, i_obj, color, cropMode, bAlphaVideoLeftRight, viewOffx, viewOffy, viewWidth, viewHeight);
        } while (false);
        return i_err;
    }
    
    virtual int serialInfo(const HJYJsonObject::Ptr &i_obj = nullptr)
    {
        int i_err = HJ_OK;
        do
        {
            HJ_JSON_BASE_SERIAL(m_obj, i_obj, color, cropMode, bAlphaVideoLeftRight, viewOffx, viewOffy, viewWidth, viewHeight);
        } while (false);
        return i_err;
    }

	std::string color = "BT601";      //BT601  BT709; only soft decode support
 	std::string cropMode = "fit";     //fit clip origin full 

    bool bAlphaVideoLeftRight = false;

    double viewOffx = 0.0;             //left
    double viewOffy = 0.0;            //top
    double viewWidth = 1.0;
    double viewHeight = 1.0;
};

class HJTransferRenderTargetInfo : public HJJsonBase
{
public:
    virtual int deserialInfo(const HJYJsonObject::Ptr &i_obj = nullptr)
    {
        int i_err = HJ_OK;
        do
        {
            HJ_JSON_BASE_DESERIAL(m_obj, i_obj, nativeWindow, width, height, state, type, fps);  
        } while (false);
        return i_err;
    }
    
    virtual int serialInfo(const HJYJsonObject::Ptr &i_obj = nullptr)
    {
        int i_err = HJ_OK;
        do
        {
            HJ_JSON_BASE_SERIAL(m_obj, i_obj, nativeWindow, width, height, state, type, fps);
        } while (false);
        return i_err;
    }
    
    int64_t nativeWindow = 0;
    int width = 0;
    int height = 0;   
    int state = HJTargetState_Create;
    int type = 0;
    int fps = 30;
};


NS_HJ_END
