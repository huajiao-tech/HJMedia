#include "HJNCNNScrfdUtils.h"

#include <algorithm>
#include <cmath>

NS_HJ_BEGIN

namespace
{
float intersection_area(const HJNCNNFaceObject& a, const HJNCNNFaceObject& b)
{
    float x0 = (std::max)(a.x, b.x);
    float y0 = (std::max)(a.y, b.y);
    float x1 = (std::min)(a.x + a.w, b.x + b.w);
    float y1 = (std::min)(a.y + a.h, b.y + b.h);
    float w = x1 - x0;
    float h = y1 - y0;
    if (w <= 0.0f || h <= 0.0f)
        return 0.0f;
    return w * h;
}

void qsort_descent_inplace(std::vector<HJNCNNFaceObject>& faceobjects, int left, int right)
{
    int i = left;
    int j = right;
    float p = faceobjects[(left + right) / 2].prob;

    while (i <= j)
    {
        while (faceobjects[i].prob > p)
            i++;

        while (faceobjects[j].prob < p)
            j--;

        if (i <= j)
        {
            std::swap(faceobjects[i], faceobjects[j]);
            i++;
            j--;
        }
    }

    if (left < j) qsort_descent_inplace(faceobjects, left, j);
    if (i < right) qsort_descent_inplace(faceobjects, i, right);
}

void qsort_descent_inplace(std::vector<HJNCNNFaceObject>& faceobjects)
{
    if (faceobjects.empty())
        return;
    qsort_descent_inplace(faceobjects, 0, static_cast<int>(faceobjects.size() - 1));
}

void nms_sorted_bboxes(const std::vector<HJNCNNFaceObject>& faceobjects, std::vector<int>& picked, float nms_threshold)
{
    picked.clear();

    const int n = static_cast<int>(faceobjects.size());

    std::vector<float> areas(n);
    for (int i = 0; i < n; i++)
    {
        areas[i] = faceobjects[i].w * faceobjects[i].h;
    }

    for (int i = 0; i < n; i++)
    {
        const HJNCNNFaceObject& a = faceobjects[i];

        int keep = 1;
        for (int j = 0; j < static_cast<int>(picked.size()); j++)
        {
            const HJNCNNFaceObject& b = faceobjects[picked[j]];

            float inter_area = intersection_area(a, b);
            float union_area = areas[i] + areas[picked[j]] - inter_area;
            if (inter_area / union_area > nms_threshold)
                keep = 0;
        }

        if (keep)
            picked.push_back(i);
    }
}

int infer_num_anchors(const ncnn::Mat& score_blob, int feat_w, int feat_h)
{
    int score_count = score_blob.w * score_blob.h * score_blob.c;
    int feat_count = feat_w * feat_h;
    if (feat_count <= 0)
        return 0;
    return score_count / feat_count;
}

void generate_proposals_hwc(int num_anchors, int feat_stride, int feat_w, int feat_h,
    const ncnn::Mat& score_blob, const ncnn::Mat& bbox_blob, const ncnn::Mat& kps_blob,
    float prob_threshold, std::vector<HJNCNNFaceObject>& faceobjects)
{
    const float* score_ptr = score_blob;
    const float* bbox_ptr = bbox_blob;
    const float* kps_ptr = kps_blob;

    for (int q = 0; q < num_anchors; q++)
    {
        for (int i = 0; i < feat_h; i++)
        {
            float cy = i * feat_stride;

            for (int j = 0; j < feat_w; j++)
            {
                float cx = j * feat_stride;
                int idx = (i * feat_w + j) * num_anchors + q;

                float prob = score_ptr[idx];
                if (prob < prob_threshold)
                    continue;

                int bbox_idx = (i * feat_w + j) * num_anchors * 4 + q * 4;
                int kps_idx = (i * feat_w + j) * num_anchors * 10 + q * 10;

                float dx = bbox_ptr[bbox_idx + 0] * feat_stride;
                float dy = bbox_ptr[bbox_idx + 1] * feat_stride;
                float dw = bbox_ptr[bbox_idx + 2] * feat_stride;
                float dh = bbox_ptr[bbox_idx + 3] * feat_stride;

                float x0 = cx - dx;
                float y0 = cy - dy;
                float x1 = cx + dw;
                float y1 = cy + dh;

                HJNCNNFaceObject obj;
                obj.x = x0;
                obj.y = y0;
                obj.w = x1 - x0 + 1;
                obj.h = y1 - y0 + 1;
                obj.prob = prob;
                for (int k = 0; k < 5; k++)
                {
                    float kdx = kps_ptr[kps_idx + k * 2] * feat_stride;
                    float kdy = kps_ptr[kps_idx + k * 2 + 1] * feat_stride;
                    obj.landmark[k][0] = cx + kdx;
                    obj.landmark[k][1] = cy + kdy;
                }

                faceobjects.push_back(obj);
            }
        }
    }
}
} // namespace

int HJNCNNScrfdUtils::detect(ncnn::Net& scrfd, const unsigned char* rgb, int width, int height,
    std::vector<HJNCNNFaceObject>& faceobjects, float prob_threshold, float nms_threshold) const
{
    faceobjects.clear();
    if (!rgb || width <= 0 || height <= 0)
    {
        return -1;
    }

    const int w = width;
    const int h = height;

    ncnn::Mat in_pad = ncnn::Mat::from_pixels_resize(rgb, ncnn::Mat::PIXEL_RGB, width, height, w, h);

    const float mean_vals[3] = { 127.5f, 127.5f, 127.5f };
    const float norm_vals[3] = { 1.0f / 128.0f, 1.0f / 128.0f, 1.0f / 128.0f };
    in_pad.substract_mean_normalize(mean_vals, norm_vals);

    ncnn::Extractor ex = scrfd.create_extractor();
    ex.input("input.1", in_pad);

    std::vector<HJNCNNFaceObject> faceproposals;

    // stride 8
    {
        ncnn::Mat score_blob, bbox_blob, kps_blob;
        ex.extract("score_8", score_blob);
        ex.extract("bbox_8", bbox_blob);
        ex.extract("kps_8", kps_blob);

        const int feat_stride = 8;
        const int feat_w = w / feat_stride;
        const int feat_h = h / feat_stride;
        int num_anchors = infer_num_anchors(score_blob, feat_w, feat_h);
        if (num_anchors <= 0)
            num_anchors = 1;

        std::vector<HJNCNNFaceObject> faceobjects8;
        generate_proposals_hwc(num_anchors, feat_stride, feat_w, feat_h, score_blob, bbox_blob, kps_blob, prob_threshold, faceobjects8);
        faceproposals.insert(faceproposals.end(), faceobjects8.begin(), faceobjects8.end());
    }

    // stride 16
    {
        ncnn::Mat score_blob, bbox_blob, kps_blob;
        ex.extract("score_16", score_blob);
        ex.extract("bbox_16", bbox_blob);
        ex.extract("kps_16", kps_blob);

        const int feat_stride = 16;
        const int feat_w = w / feat_stride;
        const int feat_h = h / feat_stride;
        int num_anchors = infer_num_anchors(score_blob, feat_w, feat_h);
        if (num_anchors <= 0)
            num_anchors = 1;

        std::vector<HJNCNNFaceObject> faceobjects16;
        generate_proposals_hwc(num_anchors, feat_stride, feat_w, feat_h, score_blob, bbox_blob, kps_blob, prob_threshold, faceobjects16);
        faceproposals.insert(faceproposals.end(), faceobjects16.begin(), faceobjects16.end());
    }

    // stride 32
    {
        ncnn::Mat score_blob, bbox_blob, kps_blob;
        ex.extract("score_32", score_blob);
        ex.extract("bbox_32", bbox_blob);
        ex.extract("kps_32", kps_blob);

        const int feat_stride = 32;
        const int feat_w = w / feat_stride;
        const int feat_h = h / feat_stride;
        int num_anchors = infer_num_anchors(score_blob, feat_w, feat_h);
        if (num_anchors <= 0)
            num_anchors = 1;

        std::vector<HJNCNNFaceObject> faceobjects32;
        generate_proposals_hwc(num_anchors, feat_stride, feat_w, feat_h, score_blob, bbox_blob, kps_blob, prob_threshold, faceobjects32);
        faceproposals.insert(faceproposals.end(), faceobjects32.begin(), faceobjects32.end());
    }

    qsort_descent_inplace(faceproposals);

    std::vector<int> picked;
    nms_sorted_bboxes(faceproposals, picked, nms_threshold);

    int face_count = static_cast<int>(picked.size());
    faceobjects.resize(face_count);
    for (int i = 0; i < face_count; i++)
    {
        faceobjects[i] = faceproposals[picked[i]];

        float x0 = faceobjects[i].x;
        float y0 = faceobjects[i].y;
        float x1 = x0 + faceobjects[i].w;
        float y1 = y0 + faceobjects[i].h;

        x0 = (std::max)((std::min)(x0, static_cast<float>(width - 1)), 0.0f);
        y0 = (std::max)((std::min)(y0, static_cast<float>(height - 1)), 0.0f);
        x1 = (std::max)((std::min)(x1, static_cast<float>(width - 1)), 0.0f);
        y1 = (std::max)((std::min)(y1, static_cast<float>(height - 1)), 0.0f);

        faceobjects[i].x = x0;
        faceobjects[i].y = y0;
        faceobjects[i].w = x1 - x0;
        faceobjects[i].h = y1 - y0;
    }

    return 0;
}

NS_HJ_END
