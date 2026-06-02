#pragma once

typedef enum HJRteComType
{
	HJRteComType_Source,
	HJRteComType_Filter,
	HJRteComType_Target,
} HJRteComType;


typedef enum HJRteComPriority
{
    HJRteComPriority_Default     = 0,
    HJRteComPriority_VideoSource = 1,
    HJRteComPriority_VideoSourceCustomFilter = 2,
    HJRteComPriority_ImageSource = 4,

    HJRteComPriority_Beauty      = 10,

    HJRteComPriority_VideoGray   = 20,
    HJRteComPriority_VideoDenoise = 25,

    HJRteComPriority_VideoBlur   = 40,
    HJRteComPriority_Mirror      = 50,
    HJRteComPriority_FaceU2D     = 80,
    HJRteComPriority_VideoSR     = 98,
    HJRteComPriority_Gift        = 100,
    HJRteComPriority_Target      = 1000,
} HJRteComPriority;


typedef enum HJRteEffectType
{
    HJRteEffect_UNKNOWN = 0,
    HJRteEffect_Gray = 1,
	HJRteEffect_Blur = 2,
} HJRteEffectType;

typedef enum HJRteTextureType
{
    HJRteTextureType_2D,
    HJRteTextureType_OES,
} HJRteTextureType;

typedef enum HJRteGraphProcType
{
    HJRteGraphProcType_Test = -1,
    HJRteGraphProcType_NONE = 0,
    HJRteGraphProcType_PLACEHOLDER_Default = 1,
    HJRteGraphProcType_SPLITSCREEN = 2,
    HJRteGraphProcType_CONFIG_SETUP = 3,
} HJRteGraphProcType;

typedef enum HJRteGraphConstructorType
{
    HJRteGraphConstructorType_Test = -1,
    HJRteGraphConstructorType_None = 0,
    HJRteGraphConstructorType_PlaceHolder,
    HJRteGraphConstructorType_SplictScreen,
    HJRteGraphConstructorType_Image,
    HJRteGraphConstructorType_ImageSeq,
} HJRteGraphConstructorType;

class HJRteUtils
{ 
public:
    static std::string ParamUrlFaceu;
    static std::string ParamUrlImgSeq;
    static std::string ParamUrlImg;
    static std::string ParamFaceInfoSource;
};
