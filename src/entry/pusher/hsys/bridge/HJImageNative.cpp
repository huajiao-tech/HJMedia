#include "HJImageNative.h"

NS_HJ_BEGIN
    HJImageNative::HJImageNative() {
        Image_ErrorCode errCode = IMAGE_SUCCESS;
        errCode = OH_ImageReceiverOptions_Create(&m_options);
        if (errCode != IMAGE_SUCCESS) {
            HJFLoge("HJImageNative OH_ImageReceiverOptions_Create failed, errCode: {}.", errCode);
        }
        Image_Size imgSize;
        imgSize.width = 1080; // 创建预览流的宽。
        imgSize.height = 1080; // 创建预览流的高。
        int32_t capacity = 8; // BufferQueue里最大Image数量，推荐填写8。
        errCode = OH_ImageReceiverOptions_SetSize(m_options, imgSize);
        if (errCode != IMAGE_SUCCESS) {
            HJFLoge("HJImageNative OH_ImageReceiverOptions_SetSize failed, errCode: {}.", errCode);
        }
        errCode = OH_ImageReceiverOptions_SetCapacity(m_options, capacity);
        if (errCode != IMAGE_SUCCESS) {
            HJFLoge("HJImageNative OH_ImageReceiverOptions_SetCapacity failed, errCode: {}.", errCode);
        }
        // 创建OH_ImageReceiverNative对象。
        errCode = OH_ImageReceiverNative_Create(m_options, &m_receiver);
        if (errCode != IMAGE_SUCCESS) {
            HJFLoge("HJImageNative OH_ImageReceiverNative_Create failed, errCode: {}.", errCode);
        }
        errCode = OH_ImageReceiverNative_On(m_receiver, onCallback);
        if (errCode != IMAGE_SUCCESS) {
            HJFLoge("HJImageNative OH_ImageReceiverNative_On failed, errCode: {}.", errCode);
        }
        errCode = OH_ImageReceiverNative_GetReceivingSurfaceId(m_receiver, &m_surfaceId);
        if (errCode != IMAGE_SUCCESS) {
            HJFLoge("HJImageNative OH_ImageReceiverNative_GetReceivingSurfaceId failed, errCode: {}.", errCode);
        }
        HJFLogi("HJImageNative constructor surfaceId: {}", m_surfaceId);
    }

    HJImageNative::~HJImageNative() {
        imageReceiverRelease();
    }

    uint64_t HJImageNative::getSurfaceId() {
        HJFLogi("HJImageNative getSurfaceId surfaceId: {}", m_surfaceId);
        return m_surfaceId;
    }

    void HJImageNative::onCallback(OH_ImageReceiverNative *receiver) {
        HJFLogi("HJImageNative buffer available.");
        uint32_t *types = nullptr;
        OH_NativeBuffer* imageBuffer = nullptr;
        OH_ImageNative *image = nullptr;
        OH_PixelmapNative *pixelmap = nullptr;
        do {
            HJFLogi("HJImageNative buffer available.");
            Image_ErrorCode errCode = OH_ImageReceiverNative_ReadNextImage(receiver, &image);
            if (errCode != IMAGE_SUCCESS) {
                HJFLoge("ImageReceiverNativeCTest get image receiver next image failed, errCode: {}.", errCode);
                break;
            }
            Image_Size size;
            errCode = OH_ImageNative_GetImageSize(image, &size);
            if (errCode != IMAGE_SUCCESS) {
                HJFLoge("HJImageNative get image receiver next image failed, errCode: {}.", errCode);
                break;
            }
            HJFLogi("HJImageNative OH_ImageNative_GetImageSize {} {}", size.width, size.height);

            // 获取图像ComponentType。
            size_t typeSize = 0;
            errCode = OH_ImageNative_GetComponentTypes(image, nullptr, &typeSize);
            if (errCode != IMAGE_SUCCESS) {
                HJFLoge("HJImageNative OH_ImageNative_GetComponentTypes failed, errCode: {}.", errCode);
                break;
            }
            types = new uint32_t[typeSize];
            errCode = OH_ImageNative_GetComponentTypes(image, &types, &typeSize);
            if (errCode != IMAGE_SUCCESS) {
                HJFLoge("HJImageNative OH_ImageNative_GetComponentTypes failed, errCode: {}.", errCode);
                break;
            }
            uint32_t component = types[0];

            // 获取图像buffer。
            errCode = OH_ImageNative_GetByteBuffer(image, component, &imageBuffer);
            if (errCode != IMAGE_SUCCESS) {
                HJFLoge("HJImageNative OH_ImageNative_GetByteBuffer failed, errCode: {}.", errCode);
                break;
            }
            size_t bufferSize = 0;
            errCode = OH_ImageNative_GetBufferSize(image, component, &bufferSize);
            if (errCode != IMAGE_SUCCESS) {
                HJFLoge("HJImageNative OH_ImageNative_GetBufferSize failed, errCode: {}.", errCode);
                break;
            }

            HJFLogi("HJImageNative buffer component: {} size: {}", component, bufferSize); // 12 NATIVEBUFFER_PIXEL_FMT_RGBA_8888
            //
            // // 获取图像行距。
            // int32_t stride = 0;
            // errCode = OH_ImageNative_GetRowStride(image, component, &stride);
            // if (errCode != IMAGE_SUCCESS) {
            //     HJFLoge("HJImageNative OH_ImageNative_GetRowStride failed, errCode: {}.", errCode);
            //     break;
            // }
            // HJLogi("ImageReceiverNativeCTest buffer stride：{}.", stride);
            void *srcVir = nullptr;
            int32_t err = OH_NativeBuffer_Map(imageBuffer, &srcVir);
            if (err != 0) {
                HJFLoge("HJImageNative OH_NativeBuffer_Map failed, errCode: {}.", errCode);
                break;
            }
            auto srcBuffer = static_cast<uint8_t *>(srcVir);

            OH_Pixelmap_InitializationOptions *createOpts;
            OH_PixelmapInitializationOptions_Create(&createOpts);
            OH_PixelmapInitializationOptions_SetWidth(createOpts, 720);
            OH_PixelmapInitializationOptions_SetHeight(createOpts, 1280);
            OH_PixelmapInitializationOptions_SetPixelFormat(createOpts, PIXEL_FORMAT_RGBA_8888);
            OH_PixelmapInitializationOptions_SetAlphaType(createOpts, PIXELMAP_ALPHA_TYPE_UNKNOWN);

            size_t dstBufferSize = size.width * size.height * 4;
            // OH_PixelmapNative_CreatePixelmap(srcBuffer, dstBufferSize, createOpts, &pixelmap);
//            OH_PixelmapNative_ConvertPixelmapNativeToNapi();

        } while (false);
        if (imageBuffer) {
            OH_NativeBuffer_Unmap(imageBuffer);
        }
        if (image) {
            OH_ImageNative_Release(image);
        }
        if (pixelmap) {
            OH_PixelmapNative_Release(pixelmap);
        }
        delete[] types;
    }

    void HJImageNative::imageReceiverRelease() {
        // 关闭被 OH_ImageReceiverNative_On 开启的回调事件。
        Image_ErrorCode errCode = OH_ImageReceiverNative_Off(m_receiver);
        if (errCode != IMAGE_SUCCESS) {
            HJFLoge("ImageReceiverNativeCTest image receiver off failed, errCode: {}.", errCode);
        }

        // 释放 OH_ImageReceiverOptions 实例。
        errCode = OH_ImageReceiverNative_Release(m_receiver);
        if (errCode != IMAGE_SUCCESS) {
            HJFLoge("ImageReceiverNativeCTest release image receiver failed, errCode: {}.", errCode);
        }
    }

NS_HJ_END