#include "HJNCNNRealCUGANUtils.h"
#include "ncnn/pipeline.h"
#include "HJFLog.h"
#include <algorithm>
#include <cstring>
#include <mutex>
#include <unordered_map>
#include <vector>
#include "HJSPBuffer.h"
#include "libyuv.h"
#include "HJDataConvert.h"
#include "HJUtilitys.h"

NS_HJ_BEGIN

#if NCNN_VULKAN
static std::unordered_map<const ncnn::VulkanDevice*, ncnn::Pipeline*> g_realcuganPostPipelines;
static std::mutex g_realcuganPostMutex;

static int fusedPostprocessGpu(const ncnn::VulkanDevice* vkdev, const ncnn::VkMat& src, ncnn::VkMat& dst,
    int targetW, int targetH, ncnn::VkCompute& cmd, const ncnn::Option& opt)
{
    if (!vkdev || src.empty() || targetW <= 0 || targetH <= 0)
    {
        return -1;
    }

    ncnn::Pipeline* pipeline = 0;
    {
        std::lock_guard<std::mutex> lock(g_realcuganPostMutex);
        std::unordered_map<const ncnn::VulkanDevice*, ncnn::Pipeline*>::iterator it = g_realcuganPostPipelines.find(vkdev);
        if (it != g_realcuganPostPipelines.end())
        {
            pipeline = it->second;
        }
    }

    if (!pipeline)
    {
        std::lock_guard<std::mutex> lock(g_realcuganPostMutex);
        std::unordered_map<const ncnn::VulkanDevice*, ncnn::Pipeline*>::iterator it = g_realcuganPostPipelines.find(vkdev);
        if (it != g_realcuganPostPipelines.end())
        {
            pipeline = it->second;
        }

        if (!pipeline)
        {
            static const char* shader = R"(
#version 450
layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
layout (binding = 0) readonly buffer src_blob { float src_data[]; };
layout (binding = 1) writeonly buffer dst_blob { float dst_data[]; };
layout (push_constant) uniform parameter
{
    int src_w;
    int src_h;
    int src_c;
    int src_cstep;
    int dst_w;
    int dst_h;
    int dst_c;
    int dst_cstep;
} p;
float sample_c(int c, float fx, float fy)
{
    int x0 = int(floor(fx));
    int y0 = int(floor(fy));
    int x1 = min(x0 + 1, p.src_w - 1);
    int y1 = min(y0 + 1, p.src_h - 1);
    x0 = clamp(x0, 0, p.src_w - 1);
    y0 = clamp(y0, 0, p.src_h - 1);
    float ax = fx - float(x0);
    float ay = fy - float(y0);
    int base = c * p.src_cstep;
    float v00 = src_data[base + y0 * p.src_w + x0];
    float v01 = src_data[base + y0 * p.src_w + x1];
    float v10 = src_data[base + y1 * p.src_w + x0];
    float v11 = src_data[base + y1 * p.src_w + x1];
    float v0 = mix(v00, v01, ax);
    float v1 = mix(v10, v11, ax);
    return mix(v0, v1, ay);
}
void main()
{
    int x = int(gl_GlobalInvocationID.x);
    int y = int(gl_GlobalInvocationID.y);
    int c = int(gl_GlobalInvocationID.z);
    if (x >= p.dst_w || y >= p.dst_h || c >= p.dst_c) return;
    float sx = (float(x) + 0.5) * float(p.src_w) / float(p.dst_w) - 0.5;
    float sy = (float(y) + 0.5) * float(p.src_h) / float(p.dst_h) - 0.5;
    int sc = c;
    if (p.src_c <= 1) sc = 0;
    else if (p.src_c == 2 && c == 2) sc = 1;
    else sc = min(c, p.src_c - 1);
    float v = sample_c(sc, sx, sy);
    v = clamp(v, 0.0, 1.0) * 255.0;
    dst_data[c * p.dst_cstep + y * p.dst_w + x] = v;
}
)";

            ncnn::Option sopt = opt;
            sopt.use_vulkan_compute = true;
            std::vector<uint32_t> spv;
            if (ncnn::compile_spirv_module(shader, sopt, spv) != 0)
            {
                return -1;
            }

            pipeline = new ncnn::Pipeline(vkdev);
            pipeline->set_optimal_local_size_xyz(8, 8, 1);
            std::vector<ncnn::vk_specialization_type> specs;
            if (pipeline->create(spv.data(), spv.size() * sizeof(uint32_t), specs) != 0)
            {
                delete pipeline;
                pipeline = 0;
                return -1;
            }

            g_realcuganPostPipelines[vkdev] = pipeline;
        }
    }

    ncnn::VkMat srcPack1;
    vkdev->convert_packing(src, srcPack1, 1, 1, cmd, opt);
    if (srcPack1.empty())
    {
        return -1;
    }

    dst.create(targetW, targetH, 3, (size_t)4u, 1, opt.blob_vkallocator);
    if (dst.empty())
    {
        return -1;
    }

    std::vector<ncnn::VkMat> bindings(2);
    bindings[0] = srcPack1;
    bindings[1] = dst;

    std::vector<ncnn::vk_constant_type> constants(8);
    constants[0].i = srcPack1.w;
    constants[1].i = srcPack1.h;
    constants[2].i = srcPack1.c;
    constants[3].i = (int)srcPack1.cstep;
    constants[4].i = dst.w;
    constants[5].i = dst.h;
    constants[6].i = dst.c;
    constants[7].i = (int)dst.cstep;

    cmd.record_pipeline(pipeline, bindings, constants, dst);
    return 0;
}
#endif

int HJNCNNRealCUGANUtils::process(ncnn::Net& net, const std::string& inputName, const std::string& outputName,
    const unsigned char* rgb, int width, int height, int targetWidth, int targetHeight,
    std::shared_ptr<HJSPBuffer>& outRgb, int& outWidth, int& outHeight)
{
    if (!rgb || width <= 0 || height <= 0 || inputName.empty() || outputName.empty())
    {
        return -1;
    }

    //int modelScale = 1;
    //if (targetWidth > 0 && width > 0)
    //{
    //    modelScale = targetWidth / width;
    //}
    //modelScale = 2;
    //int prepadding = 0;
    //if (modelScale == 2) prepadding = 18;
    //if (modelScale == 3) prepadding = 14;
    //if (modelScale == 4) prepadding = 19;
    int prepadding = 18;
    const int inputW = width + prepadding * 2;
    const int inputH = height + prepadding * 2;
    const unsigned char* inputRgb = rgb;
    if (prepadding > 0)
    {
        const int paddedSize = inputW * inputH * 3;
        if (!m_padBuffer || m_padBuffer->getSize() != paddedSize)
        {
            m_padBuffer = HJSPBuffer::create(paddedSize);
        }
        if (!m_padBuffer || !m_padBuffer->getBuf())
        {
            return -1;
        }

        unsigned char* rgbPadded = m_padBuffer->getBuf();
        const int srcStride = width * 3;
        const int dstStride = inputW * 3;
        const int sideBytes = prepadding * 3;
        const int centerOffset = prepadding * dstStride + prepadding * 3;

        // Copy center valid region in one shot.
        libyuv::CopyPlane(rgb, srcStride, rgbPadded + centerOffset, dstStride, srcStride, height);
#if 0           
        {
            int pitches[4] = { inputW * 3, 0, 0, 0};
            unsigned char* planes[4] = { rgbPadded, nullptr, nullptr, nullptr };
            HJDataConvert::cvtSaveToImage(HJConvertDataType_RGB, planes, pitches, inputW, inputH, HJUtilitys::concatenatePath(HJUtilitys::exeDir(), "PADRGB1.png"));
        }
#endif
        // Fill left/right borders for middle rows.
        for (int y = 0; y < height; y++)
        {
            const unsigned char* srcRow = rgb + (size_t)y * srcStride;
            unsigned char* dstRow = rgbPadded + (size_t)(prepadding + y) * dstStride;
            unsigned char* dstLeft = dstRow;
            unsigned char* dstRight = dstRow + (size_t)(prepadding + width) * 3;

            // Left: replicate first source pixel.
            std::memcpy(dstLeft, srcRow, 3);
            for (int filled = 3; filled < sideBytes;)
            {
                const int copyBytes = (std::min)(filled, sideBytes - filled);
                std::memcpy(dstLeft + filled, dstLeft, (size_t)copyBytes);
                filled += copyBytes;
            }

            // Right: replicate last source pixel.
            const unsigned char* srcLast = srcRow + (size_t)(width - 1) * 3;
            std::memcpy(dstRight, srcLast, 3);
            for (int filled = 3; filled < sideBytes;)
            {
                const int copyBytes = (std::min)(filled, sideBytes - filled);
                std::memcpy(dstRight + filled, dstRight, (size_t)copyBytes);
                filled += copyBytes;
            }
        }

        // Fill top/bottom borders by copying already prepared first/last middle rows.
        unsigned char* firstValidRow = rgbPadded + (size_t)prepadding * dstStride;
        unsigned char* lastValidRow = rgbPadded + (size_t)(prepadding + height - 1) * dstStride;
        libyuv::CopyPlane(firstValidRow, dstStride, rgbPadded, dstStride, dstStride, prepadding);
        libyuv::CopyPlane(lastValidRow, dstStride,
            rgbPadded + (size_t)(prepadding + height) * dstStride, dstStride, dstStride, prepadding);
#if 0           
        {
            int pitches[4] = { inputW * 3, 0, 0, 0 };
            unsigned char* planes[4] = { rgbPadded, nullptr, nullptr, nullptr };
            HJDataConvert::cvtSaveToImage(HJConvertDataType_RGB, planes, pitches, inputW, inputH, HJUtilitys::concatenatePath(HJUtilitys::exeDir(), "PADRGB2.png"));
        }
#endif
        inputRgb = rgbPadded;
    }

    ncnn::Mat in = ncnn::Mat::from_pixels(inputRgb, ncnn::Mat::PIXEL_RGB, inputW, inputH);
    const float norm_vals[3] = { 1 / 255.f, 1 / 255.f, 1 / 255.f };
    in.substract_mean_normalize(0, norm_vals);

    ncnn::Extractor ex = net.create_extractor();
    if (net.opt.blob_allocator)
    {
        ex.set_blob_allocator(net.opt.blob_allocator);
    }
    if (net.opt.workspace_allocator)
    {
        ex.set_workspace_allocator(net.opt.workspace_allocator);
    }

    int ret = ex.input(inputName.c_str(), in);
    if (ret != 0)
    {
        return -1;
    }

    ncnn::Mat outMat;
#if NCNN_VULKAN
    bool gpuPostDone = false;
    const ncnn::VulkanDevice* vkdev = 0;
    ncnn::VkAllocator* blobVkAllocator = 0;
    ncnn::VkAllocator* stagingVkAllocator = 0;
    auto cleanupVk = [&]() {
        if (!vkdev)
        {
            return;
        }
        if (blobVkAllocator)
        {
            vkdev->reclaim_blob_allocator(blobVkAllocator);
            blobVkAllocator = 0;
        }
        if (stagingVkAllocator)
        {
            vkdev->reclaim_staging_allocator(stagingVkAllocator);
            stagingVkAllocator = 0;
        }
    };

    if (net.opt.use_vulkan_compute)
    {
        vkdev = net.vulkan_device();
        if (vkdev)
        {
            blobVkAllocator = vkdev->acquire_blob_allocator();
            stagingVkAllocator = vkdev->acquire_staging_allocator();
            if (blobVkAllocator && stagingVkAllocator)
            {
                ex.set_blob_vkallocator(blobVkAllocator);
                ex.set_workspace_vkallocator(blobVkAllocator);
                ex.set_staging_vkallocator(stagingVkAllocator);

                ncnn::Option vkopt = net.opt;
                vkopt.blob_vkallocator = blobVkAllocator;
                vkopt.workspace_vkallocator = blobVkAllocator;
                vkopt.staging_vkallocator = stagingVkAllocator;

                do
                {
                    ncnn::VkCompute cmd(vkdev);
                    ncnn::VkMat outGpu;
                    ret = ex.extract(outputName.c_str(), outGpu, cmd);
                    if (ret != 0 || outGpu.empty())
                    {
                        break;
                    }

                    ncnn::VkMat postGpu;
                    const int finalTargetW = targetWidth > 0 ? targetWidth : outGpu.w;
                    const int finalTargetH = targetHeight > 0 ? targetHeight : outGpu.h;
                    ret = fusedPostprocessGpu(vkdev, outGpu, postGpu, finalTargetW, finalTargetH, cmd, vkopt);
                    if (ret != 0 || postGpu.empty())
                    {
                        break;
                    }

                    cmd.record_download(postGpu, outMat, vkopt);
                    ret = cmd.submit_and_wait();
                    if (ret != 0 || outMat.empty())
                    {
                        break;
                    }
                    gpuPostDone = true;
                } while (false);
            }
        }
    }

    if (!gpuPostDone)
#endif
    {
        ret = ex.extract(outputName.c_str(), outMat);
    }

    if (ret != 0 || outMat.empty() || outMat.w <= 0 || outMat.h <= 0 || outMat.c != 3)
    {
#if NCNN_VULKAN
        cleanupVk();
#endif
        return -1;
    }

#if NCNN_VULKAN
    if (!gpuPostDone)
#endif
    {
        const float to255[3] = { 255.f, 255.f, 255.f };
        outMat.substract_mean_normalize(0, to255);
    }

    outWidth = outMat.w;
    outHeight = outMat.h;
    const int outSize = outWidth * outHeight * 3;
    if (!outRgb || outRgb->getSize() != outSize)
    {
        outRgb = HJSPBuffer::create(outSize);
    }
    outMat.to_pixels(outRgb->getBuf(), ncnn::Mat::PIXEL_RGB);

#if NCNN_VULKAN
    cleanupVk();
#endif
    return 0;
}

NS_HJ_END
