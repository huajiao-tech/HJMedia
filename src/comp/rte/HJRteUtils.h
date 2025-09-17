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

    HJRteComPriority_Beauty      = 10,

    HJRteComPriority_VideoGray   = 20,
    HJRteComPriority_VideoBlur   = 40,
    HJRteComPriority_Mirror      = 50,
    HJRteComPriority_FaceU2D     = 80,
    HJRteComPriority_Gift        = 100,
    HJRteComPriority_Target      = 1000,
} HJRteComPriority;

typedef enum HJRteTextureType
{
    HJRteTextureType_2D,
    HJRteTextureType_OES,
} HJRteTextureType;