#pragma once

#include "HJPrerequisites.h"
#include "ncnn/net.h"

#include <string>

NS_HJ_BEGIN
class HJSPBuffer;
class HJNCNNRealCUGANUtils
{
public:
    int process(ncnn::Net& net, const std::string& inputName, const std::string& outputName,
        const unsigned char* rgb, int width, int height, int targetWidth, int targetHeight,
        std::shared_ptr<HJSPBuffer>& outRgb, int& outWidth, int& outHeight);

private:
    std::shared_ptr<HJSPBuffer> m_padBuffer = nullptr;
};

NS_HJ_END
