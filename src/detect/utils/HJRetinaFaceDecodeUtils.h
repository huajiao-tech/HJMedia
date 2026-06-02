#pragma once

#include "HJPrerequisites.h"

#include <vector>

NS_HJ_BEGIN

typedef struct HJRetinaFaceObject
{
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
    float prob = 0.0f;
    float landmark[5][2] = {};
    bool hasLandmark = false;
} HJRetinaFaceObject;

struct HJRetinaFaceTensor
{
    int channels = 0;
    int height = 0;
    int width = 0;
    std::vector<float> data;

    bool valid() const
    {
        return channels > 0 && height > 0 && width > 0 &&
            data.size() == (size_t)channels * (size_t)height * (size_t)width;
    }

    const float* channel(int index) const
    {
        if (!valid() || index < 0 || index >= channels)
        {
            return nullptr;
        }
        return data.data() + (size_t)index * (size_t)height * (size_t)width;
    }
};

struct HJRetinaFaceFeatureSet
{
    int stride = 0;
    HJRetinaFaceTensor score;
    HJRetinaFaceTensor bbox;
    HJRetinaFaceTensor landmark;
};

class HJRetinaFaceDecodeUtils
{
public:
    static int decode(const std::vector<HJRetinaFaceFeatureSet>& features,
                      float probThreshold,
                      float nmsThreshold,
                      int imgWidth,
                      int imgHeight,
                      std::vector<HJRetinaFaceObject>& faceobjects);
    static int decodeWithPriors(const std::vector<HJRetinaFaceFeatureSet>& features,
                                float probThreshold,
                                float nmsThreshold,
                                int imgWidth,
                                int imgHeight,
                                std::vector<HJRetinaFaceObject>& faceobjects);
};

NS_HJ_END
