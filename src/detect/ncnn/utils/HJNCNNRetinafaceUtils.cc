#include "HJNCNNRetinafaceUtils.h"

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

ncnn::Mat generate_anchors(int base_size, const ncnn::Mat& ratios, const ncnn::Mat& scales)
{
    int num_ratio = ratios.w;
    int num_scale = scales.w;

    ncnn::Mat anchors;
    anchors.create(4, num_ratio * num_scale);

    const float cx = base_size * 0.5f;
    const float cy = base_size * 0.5f;

    for (int i = 0; i < num_ratio; i++)
    {
        float ar = ratios[i];

        int r_w = static_cast<int>(round(base_size / sqrt(ar)));
        int r_h = static_cast<int>(round(r_w * ar));

        for (int j = 0; j < num_scale; j++)
        {
            float scale = scales[j];

            float rs_w = r_w * scale;
            float rs_h = r_h * scale;

            float* anchor = anchors.row(i * num_scale + j);

            anchor[0] = cx - rs_w * 0.5f;
            anchor[1] = cy - rs_h * 0.5f;
            anchor[2] = cx + rs_w * 0.5f;
            anchor[3] = cy + rs_h * 0.5f;
        }
    }

    return anchors;
}

void generate_proposals(const ncnn::Mat& anchors, int feat_stride, const ncnn::Mat& score_blob,
                        const ncnn::Mat& bbox_blob, const ncnn::Mat& landmark_blob,
                        float prob_threshold, std::vector<HJNCNNFaceObject>& faceobjects)
{
    int w = score_blob.w;
    int h = score_blob.h;

    const int num_anchors = anchors.h;

    for (int q = 0; q < num_anchors; q++)
    {
        const float* anchor = anchors.row(q);

        const ncnn::Mat score = score_blob.channel(q + num_anchors);
        const ncnn::Mat bbox = bbox_blob.channel_range(q * 4, 4);
        const ncnn::Mat landmark = landmark_blob.channel_range(q * 10, 10);

        float anchor_y = anchor[1];

        float anchor_w = anchor[2] - anchor[0];
        float anchor_h = anchor[3] - anchor[1];

        for (int i = 0; i < h; i++)
        {
            float anchor_x = anchor[0];

            for (int j = 0; j < w; j++)
            {
                int index = i * w + j;

                float prob = score[index];

                if (prob >= prob_threshold)
                {
                    float dx = bbox.channel(0)[index];
                    float dy = bbox.channel(1)[index];
                    float dw = bbox.channel(2)[index];
                    float dh = bbox.channel(3)[index];

                    float cx = anchor_x + anchor_w * 0.5f;
                    float cy = anchor_y + anchor_h * 0.5f;

                    float pb_cx = cx + anchor_w * dx;
                    float pb_cy = cy + anchor_h * dy;

                    float pb_w = anchor_w * exp(dw);
                    float pb_h = anchor_h * exp(dh);

                    float x0 = pb_cx - pb_w * 0.5f;
                    float y0 = pb_cy - pb_h * 0.5f;
                    float x1 = pb_cx + pb_w * 0.5f;
                    float y1 = pb_cy + pb_h * 0.5f;

                    HJNCNNFaceObject obj;
                    obj.x = x0;
                    obj.y = y0;
                    obj.w = x1 - x0 + 1;
                    obj.h = y1 - y0 + 1;
                    obj.landmark[0][0] = cx + (anchor_w + 1) * landmark.channel(0)[index];
                    obj.landmark[0][1] = cy + (anchor_h + 1) * landmark.channel(1)[index];
                    obj.landmark[1][0] = cx + (anchor_w + 1) * landmark.channel(2)[index];
                    obj.landmark[1][1] = cy + (anchor_h + 1) * landmark.channel(3)[index];
                    obj.landmark[2][0] = cx + (anchor_w + 1) * landmark.channel(4)[index];
                    obj.landmark[2][1] = cy + (anchor_h + 1) * landmark.channel(5)[index];
                    obj.landmark[3][0] = cx + (anchor_w + 1) * landmark.channel(6)[index];
                    obj.landmark[3][1] = cy + (anchor_h + 1) * landmark.channel(7)[index];
                    obj.landmark[4][0] = cx + (anchor_w + 1) * landmark.channel(8)[index];
                    obj.landmark[4][1] = cy + (anchor_h + 1) * landmark.channel(9)[index];
                    obj.prob = prob;

                    faceobjects.push_back(obj);
                }

                anchor_x += feat_stride;
            }

            anchor_y += feat_stride;
        }
    }
}
} // namespace

int HJNCNNRetinafaceUtils::detect(ncnn::Net& retinaface, const unsigned char* rgb, int img_w, int img_h,
                                  std::vector<HJNCNNFaceObject>& faceobjects, float prob_threshold, float nms_threshold) const
{
    faceobjects.clear();
    if (!rgb || img_w <= 0 || img_h <= 0)
    {
        return -1;
    }

    ncnn::Mat in = ncnn::Mat::from_pixels(rgb, ncnn::Mat::PIXEL_RGB, img_w, img_h);

    ncnn::Extractor ex = retinaface.create_extractor();

    ex.input("data", in);

    std::vector<HJNCNNFaceObject> faceproposals;

    {
        ncnn::Mat score_blob, bbox_blob, landmark_blob;
        ex.extract("face_rpn_cls_prob_reshape_stride32", score_blob);
        ex.extract("face_rpn_bbox_pred_stride32", bbox_blob);
        ex.extract("face_rpn_landmark_pred_stride32", landmark_blob);

        const int base_size = 16;
        const int feat_stride = 32;
        ncnn::Mat ratios(1);
        ratios[0] = 1.f;
        ncnn::Mat scales(2);
        scales[0] = 32.f;
        scales[1] = 16.f;
        ncnn::Mat anchors = generate_anchors(base_size, ratios, scales);

        std::vector<HJNCNNFaceObject> faceobjects32;
        generate_proposals(anchors, feat_stride, score_blob, bbox_blob, landmark_blob, prob_threshold, faceobjects32);

        faceproposals.insert(faceproposals.end(), faceobjects32.begin(), faceobjects32.end());
    }

    {
        ncnn::Mat score_blob, bbox_blob, landmark_blob;
        ex.extract("face_rpn_cls_prob_reshape_stride16", score_blob);
        ex.extract("face_rpn_bbox_pred_stride16", bbox_blob);
        ex.extract("face_rpn_landmark_pred_stride16", landmark_blob);

        const int base_size = 16;
        const int feat_stride = 16;
        ncnn::Mat ratios(1);
        ratios[0] = 1.f;
        ncnn::Mat scales(2);
        scales[0] = 8.f;
        scales[1] = 4.f;
        ncnn::Mat anchors = generate_anchors(base_size, ratios, scales);

        std::vector<HJNCNNFaceObject> faceobjects16;
        generate_proposals(anchors, feat_stride, score_blob, bbox_blob, landmark_blob, prob_threshold, faceobjects16);

        faceproposals.insert(faceproposals.end(), faceobjects16.begin(), faceobjects16.end());
    }

    {
        ncnn::Mat score_blob, bbox_blob, landmark_blob;
        ex.extract("face_rpn_cls_prob_reshape_stride8", score_blob);
        ex.extract("face_rpn_bbox_pred_stride8", bbox_blob);
        ex.extract("face_rpn_landmark_pred_stride8", landmark_blob);

        const int base_size = 16;
        const int feat_stride = 8;
        ncnn::Mat ratios(1);
        ratios[0] = 1.f;
        ncnn::Mat scales(2);
        scales[0] = 2.f;
        scales[1] = 1.f;
        ncnn::Mat anchors = generate_anchors(base_size, ratios, scales);

        std::vector<HJNCNNFaceObject> faceobjects8;
        generate_proposals(anchors, feat_stride, score_blob, bbox_blob, landmark_blob, prob_threshold, faceobjects8);

        faceproposals.insert(faceproposals.end(), faceobjects8.begin(), faceobjects8.end());
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

        x0 = (std::max)((std::min)(x0, static_cast<float>(img_w - 1)), 0.0f);
        y0 = (std::max)((std::min)(y0, static_cast<float>(img_h - 1)), 0.0f);
        x1 = (std::max)((std::min)(x1, static_cast<float>(img_w - 1)), 0.0f);
        y1 = (std::max)((std::min)(y1, static_cast<float>(img_h - 1)), 0.0f);

        faceobjects[i].x = x0;
        faceobjects[i].y = y0;
        faceobjects[i].w = x1 - x0;
        faceobjects[i].h = y1 - y0;
    }

    return 0;
}

NS_HJ_END
