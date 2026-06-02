#pragma once

#include "HJPrerequisites.h"
#include "ncnn/net.h"

#include <vector>

NS_HJ_BEGIN

typedef struct HJNCNNFaceObject
{
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
    float prob = 0.0f;
    float landmark[5][2] = {};
} HJNCNNFaceObject;

class HJNCNNRetinafaceUtils
{
public:
    int detect(ncnn::Net& net, const unsigned char* rgb, int width, int height,
               std::vector<HJNCNNFaceObject>& faceobjects, float prob_threshold, float nms_threshold) const;
};

NS_HJ_END
