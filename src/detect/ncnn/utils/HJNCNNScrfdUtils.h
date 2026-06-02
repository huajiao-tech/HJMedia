#pragma once

#include "HJPrerequisites.h"
#include "ncnn/net.h"
#include "ncnn/utils/HJNCNNRetinafaceUtils.h"

#include <vector>

NS_HJ_BEGIN

class HJNCNNScrfdUtils
{
public:
    int detect(ncnn::Net& net, const unsigned char* rgb, int width, int height,
               std::vector<HJNCNNFaceObject>& faceobjects, float prob_threshold, float nms_threshold) const;
};

NS_HJ_END
