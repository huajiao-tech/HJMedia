#pragma once

//#include "HJPrerequisites.h"

typedef enum HJPrioEffectType
{
    HJPrioEffect_UNKNOWN = 0,
    HJPrioEffect_Gray = 1,
	HJPrioEffect_Blur = 2,
} HJPrioEffectType;


typedef enum HJPrioComSourceType
{
    HJPrioComSourceType_UNKNOWN     = 0,
    HJPrioComSourceType_Bridge      = 1,
	HJPrioComSourceType_SPLITSCREEN = 2,
    HJPrioComSourceType_SERIES      = 3,
} HJPrioComSourceType;

