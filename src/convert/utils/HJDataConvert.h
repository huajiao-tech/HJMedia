#pragma once

#include "HJPrerequisites.h"
#include "HJConvertUtils.h"
#include "HJMediaUtils.h"


NS_HJ_BEGIN

enum class HJDataScaleType
{
    Fit,
    Clip,
    Origin,
    Full
};

class HJDataConvert 
{
public:
    static HJVec4i cvtGetScaleOffset(HJDataScaleType i_type, int i_srcW, int i_srcH, int i_screenW, int i_screenH);
    static void cvtSaveToImage(HJConvertDataType i_type, unsigned char* i_data[4], int i_nPitch[4], int i_nWidth, int i_nHeight, const std::string& i_pFilePath);
    
};

NS_HJ_END