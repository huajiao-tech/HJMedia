#include "HJRetinaFaceDecodeUtils.h"

#include <algorithm>
#include <cmath>

NS_HJ_BEGIN

namespace
{
static float intersectionArea(const HJRetinaFaceObject& a, const HJRetinaFaceObject& b)
{
    const float x0 = (std::max)(a.x, b.x);
    const float y0 = (std::max)(a.y, b.y);
    const float x1 = (std::min)(a.x + a.w, b.x + b.w);
    const float y1 = (std::min)(a.y + a.h, b.y + b.h);
    const float w = x1 - x0;
    const float h = y1 - y0;
    if (w <= 0.0f || h <= 0.0f)
    {
        return 0.0f;
    }
    return w * h;
}

static void sortByProb(std::vector<HJRetinaFaceObject>& faceobjects)
{
    std::sort(faceobjects.begin(), faceobjects.end(), [](const HJRetinaFaceObject& lhs, const HJRetinaFaceObject& rhs)
    {
        return lhs.prob > rhs.prob;
    });
}

static void nmsSortedBBoxes(const std::vector<HJRetinaFaceObject>& faceobjects,
                            float nmsThreshold,
                            std::vector<int>& picked)
{
    picked.clear();
    const int n = (int)faceobjects.size();
    std::vector<float> areas((size_t)n, 0.0f);
    for (int i = 0; i < n; ++i)
    {
        areas[(size_t)i] = faceobjects[(size_t)i].w * faceobjects[(size_t)i].h;
    }

    for (int i = 0; i < n; ++i)
    {
        const HJRetinaFaceObject& a = faceobjects[(size_t)i];
        bool keep = true;
        for (int pickedIndex : picked)
        {
            const HJRetinaFaceObject& b = faceobjects[(size_t)pickedIndex];
            const float interArea = intersectionArea(a, b);
            const float unionArea = areas[(size_t)i] + areas[(size_t)pickedIndex] - interArea;
            if (unionArea > 0.0f && interArea / unionArea > nmsThreshold)
            {
                keep = false;
                break;
            }
        }
        if (keep)
        {
            picked.push_back(i);
        }
    }
}

static std::vector<float> generateAnchors(int baseSize, const std::vector<float>& ratios, const std::vector<float>& scales)
{
    std::vector<float> anchors;
    anchors.resize(ratios.size() * scales.size() * 4);
    const float cx = baseSize * 0.5f;
    const float cy = baseSize * 0.5f;
    size_t anchorIndex = 0;
    for (float ratio : ratios)
    {
        const int ratioWidth = (int)std::round(baseSize / std::sqrt(ratio));
        const int ratioHeight = (int)std::round(ratioWidth * ratio);
        for (float scale : scales)
        {
            const float anchorW = ratioWidth * scale;
            const float anchorH = ratioHeight * scale;
            anchors[anchorIndex++] = cx - anchorW * 0.5f;
            anchors[anchorIndex++] = cy - anchorH * 0.5f;
            anchors[anchorIndex++] = cx + anchorW * 0.5f;
            anchors[anchorIndex++] = cy + anchorH * 0.5f;
        }
    }
    return anchors;
}

static void generateProposals(const std::vector<float>& anchors,
                              int featStride,
                              const HJRetinaFaceTensor& score,
                              const HJRetinaFaceTensor& bbox,
                              const HJRetinaFaceTensor* landmark,
                              float probThreshold,
                              std::vector<HJRetinaFaceObject>& faceobjects)
{
    const int numAnchors = (int)(anchors.size() / 4);
    const int width = score.width;
    const int height = score.height;
    if (!score.valid() || !bbox.valid() || width <= 0 || height <= 0 || numAnchors <= 0)
    {
        return;
    }
    if (score.channels < numAnchors * 2 || bbox.channels < numAnchors * 4)
    {
        return;
    }
    const bool hasLandmark = landmark && landmark->valid() && landmark->channels >= numAnchors * 10 &&
        landmark->width == width && landmark->height == height;

    for (int anchorIndex = 0; anchorIndex < numAnchors; ++anchorIndex)
    {
        const float* anchor = anchors.data() + (size_t)anchorIndex * 4;
        const float* scorePtr = score.channel(anchorIndex + numAnchors);
        const float* bboxDx = bbox.channel(anchorIndex * 4 + 0);
        const float* bboxDy = bbox.channel(anchorIndex * 4 + 1);
        const float* bboxDw = bbox.channel(anchorIndex * 4 + 2);
        const float* bboxDh = bbox.channel(anchorIndex * 4 + 3);
        const float* landmarkPtr[10] = {};
        if (hasLandmark)
        {
            for (int k = 0; k < 10; ++k)
            {
                landmarkPtr[k] = landmark->channel(anchorIndex * 10 + k);
            }
        }

        const float anchorW = anchor[2] - anchor[0];
        const float anchorH = anchor[3] - anchor[1];
        float anchorY = anchor[1];
        for (int y = 0; y < height; ++y)
        {
            float anchorX = anchor[0];
            for (int x = 0; x < width; ++x)
            {
                const int index = y * width + x;
                const float prob = scorePtr[index];
                if (prob < probThreshold)
                {
                    anchorX += featStride;
                    continue;
                }

                const float cx = anchorX + anchorW * 0.5f;
                const float cy = anchorY + anchorH * 0.5f;
                const float dx = bboxDx[index];
                const float dy = bboxDy[index];
                const float dw = bboxDw[index];
                const float dh = bboxDh[index];

                const float pbCx = cx + anchorW * dx;
                const float pbCy = cy + anchorH * dy;
                const float pbW = anchorW * std::exp(dw);
                const float pbH = anchorH * std::exp(dh);

                HJRetinaFaceObject obj;
                obj.x = pbCx - pbW * 0.5f;
                obj.y = pbCy - pbH * 0.5f;
                obj.w = pbW + 1.0f;
                obj.h = pbH + 1.0f;
                obj.prob = prob;
                obj.hasLandmark = hasLandmark;
                if (hasLandmark)
                {
                    for (int p = 0; p < 5; ++p)
                    {
                        obj.landmark[p][0] = cx + (anchorW + 1.0f) * landmarkPtr[p * 2 + 0][index];
                        obj.landmark[p][1] = cy + (anchorH + 1.0f) * landmarkPtr[p * 2 + 1][index];
                    }
                }
                faceobjects.push_back(obj);
                anchorX += featStride;
            }
            anchorY += featStride;
        }
    }
}

static bool minSizesForStride(int stride, float& min0, float& min1)
{
    switch (stride)
    {
        case 8:
            min0 = 16.0f;
            min1 = 32.0f;
            return true;
        case 16:
            min0 = 64.0f;
            min1 = 128.0f;
            return true;
        case 32:
            min0 = 256.0f;
            min1 = 512.0f;
            return true;
        default:
            return false;
    }
}

static void generatePriorProposals(const HJRetinaFaceFeatureSet& feature,
                                   float probThreshold,
                                   int imgWidth,
                                   int imgHeight,
                                   std::vector<HJRetinaFaceObject>& faceobjects)
{
    if (!feature.score.valid() || !feature.bbox.valid() || feature.score.width <= 0 || feature.score.height <= 0)
    {
        return;
    }

    const int numAnchors = feature.score.channels / 2;
    if (numAnchors <= 0 || feature.bbox.channels < numAnchors * 4)
    {
        return;
    }

    float minSize0 = 0.0f;
    float minSize1 = 0.0f;
    if (!minSizesForStride(feature.stride, minSize0, minSize1))
    {
        return;
    }

    const bool hasLandmark = feature.landmark.valid() &&
        feature.landmark.channels >= numAnchors * 10 &&
        feature.landmark.width == feature.score.width &&
        feature.landmark.height == feature.score.height;
    const float minSizes[2] = { minSize0, minSize1 };
    const float varianceCenter = 0.1f;
    const float varianceSize = 0.2f;

    for (int y = 0; y < feature.score.height; ++y)
    {
        for (int x = 0; x < feature.score.width; ++x)
        {
            const int index = y * feature.score.width + x;
            for (int anchorIndex = 0; anchorIndex < numAnchors && anchorIndex < 2; ++anchorIndex)
            {
                const float prob = feature.score.channel(anchorIndex + numAnchors)[index];
                if (prob < probThreshold)
                {
                    continue;
                }

                const float priorCx = ((float)x + 0.5f) * (float)feature.stride / (float)imgWidth;
                const float priorCy = ((float)y + 0.5f) * (float)feature.stride / (float)imgHeight;
                const float priorW = minSizes[anchorIndex] / (float)imgWidth;
                const float priorH = minSizes[anchorIndex] / (float)imgHeight;

                const float dx = feature.bbox.channel(anchorIndex * 4 + 0)[index];
                const float dy = feature.bbox.channel(anchorIndex * 4 + 1)[index];
                const float dw = feature.bbox.channel(anchorIndex * 4 + 2)[index];
                const float dh = feature.bbox.channel(anchorIndex * 4 + 3)[index];

                const float boxCx = priorCx + dx * varianceCenter * priorW;
                const float boxCy = priorCy + dy * varianceCenter * priorH;
                const float boxW = priorW * std::exp(dw * varianceSize);
                const float boxH = priorH * std::exp(dh * varianceSize);

                HJRetinaFaceObject obj;
                obj.x = (boxCx - boxW * 0.5f) * (float)imgWidth;
                obj.y = (boxCy - boxH * 0.5f) * (float)imgHeight;
                obj.w = boxW * (float)imgWidth;
                obj.h = boxH * (float)imgHeight;
                obj.prob = prob;
                obj.hasLandmark = hasLandmark;

                if (hasLandmark)
                {
                    for (int p = 0; p < 5; ++p)
                    {
                        const float lx = feature.landmark.channel(anchorIndex * 10 + p * 2 + 0)[index];
                        const float ly = feature.landmark.channel(anchorIndex * 10 + p * 2 + 1)[index];
                        obj.landmark[p][0] = (priorCx + lx * varianceCenter * priorW) * (float)imgWidth;
                        obj.landmark[p][1] = (priorCy + ly * varianceCenter * priorH) * (float)imgHeight;
                    }
                }
                faceobjects.push_back(obj);
            }
        }
    }
}
}

int HJRetinaFaceDecodeUtils::decode(const std::vector<HJRetinaFaceFeatureSet>& features,
                                    float probThreshold,
                                    float nmsThreshold,
                                    int imgWidth,
                                    int imgHeight,
                                    std::vector<HJRetinaFaceObject>& faceobjects)
{
    faceobjects.clear();
    if (features.empty() || imgWidth <= 0 || imgHeight <= 0)
    {
        return -1;
    }

    std::vector<HJRetinaFaceObject> faceProposals;
    for (const auto& feature : features)
    {
        if (feature.stride <= 0)
        {
            continue;
        }
        const std::vector<float> anchors = [&]()
        {
            if (feature.stride == 32)
            {
                return generateAnchors(16, { 1.0f }, { 32.0f, 16.0f });
            }
            if (feature.stride == 16)
            {
                return generateAnchors(16, { 1.0f }, { 8.0f, 4.0f });
            }
            return generateAnchors(16, { 1.0f }, { 2.0f, 1.0f });
        }();
        const HJRetinaFaceTensor* landmark = feature.landmark.valid() ? &feature.landmark : nullptr;
        generateProposals(anchors, feature.stride, feature.score, feature.bbox, landmark, probThreshold, faceProposals);
    }

    sortByProb(faceProposals);
    std::vector<int> picked;
    nmsSortedBBoxes(faceProposals, nmsThreshold, picked);
    faceobjects.resize(picked.size());
    for (size_t i = 0; i < picked.size(); ++i)
    {
        faceobjects[i] = faceProposals[(size_t)picked[i]];
        float x0 = faceobjects[i].x;
        float y0 = faceobjects[i].y;
        float x1 = x0 + faceobjects[i].w;
        float y1 = y0 + faceobjects[i].h;

        x0 = (std::max)((std::min)(x0, (float)(imgWidth - 1)), 0.0f);
        y0 = (std::max)((std::min)(y0, (float)(imgHeight - 1)), 0.0f);
        x1 = (std::max)((std::min)(x1, (float)(imgWidth - 1)), 0.0f);
        y1 = (std::max)((std::min)(y1, (float)(imgHeight - 1)), 0.0f);

        faceobjects[i].x = x0;
        faceobjects[i].y = y0;
        faceobjects[i].w = x1 - x0;
        faceobjects[i].h = y1 - y0;
    }
    return 0;
}

int HJRetinaFaceDecodeUtils::decodeWithPriors(const std::vector<HJRetinaFaceFeatureSet>& features,
                                              float probThreshold,
                                              float nmsThreshold,
                                              int imgWidth,
                                              int imgHeight,
                                              std::vector<HJRetinaFaceObject>& faceobjects)
{
    faceobjects.clear();
    if (features.empty() || imgWidth <= 0 || imgHeight <= 0)
    {
        return -1;
    }

    std::vector<HJRetinaFaceObject> faceProposals;
    for (const auto& feature : features)
    {
        generatePriorProposals(feature, probThreshold, imgWidth, imgHeight, faceProposals);
    }

    sortByProb(faceProposals);
    std::vector<int> picked;
    nmsSortedBBoxes(faceProposals, nmsThreshold, picked);
    faceobjects.resize(picked.size());
    for (size_t i = 0; i < picked.size(); ++i)
    {
        faceobjects[i] = faceProposals[(size_t)picked[i]];
        float x0 = faceobjects[i].x;
        float y0 = faceobjects[i].y;
        float x1 = x0 + faceobjects[i].w;
        float y1 = y0 + faceobjects[i].h;

        x0 = (std::max)((std::min)(x0, (float)(imgWidth - 1)), 0.0f);
        y0 = (std::max)((std::min)(y0, (float)(imgHeight - 1)), 0.0f);
        x1 = (std::max)((std::min)(x1, (float)(imgWidth - 1)), 0.0f);
        y1 = (std::max)((std::min)(y1, (float)(imgHeight - 1)), 0.0f);

        faceobjects[i].x = x0;
        faceobjects[i].y = y0;
        faceobjects[i].w = x1 - x0;
        faceobjects[i].h = y1 - y0;
    }
    return 0;
}

NS_HJ_END
