//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include <set>
#include "HJUtilitys.h"
#include "HJMediaUtils.h"

typedef struct AVFrame AVFrame;
//***********************************************************************************//
NS_HJ_BEGIN
class HJAVUtils : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJAVUtils>;

    static std::vector<float> calculateAudioPeaks(const AVFrame* i_avf);

};

NS_HJ_END