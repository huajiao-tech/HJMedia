#include "HJGPUToRAM.h"
#include "HJFLog.h"
#include "HJFileData.h"
#include "libyuv.h"

#define TEST_DIRECT_RELEASE 0
#define TEST_WRITE_YUV 0

NS_HJ_BEGIN

HJRGBAMediaData::Ptr HJBaseGPUToRAM::getMediaRGBAData()
{
    HJRGBAMediaData::Ptr ptr = nullptr;
    HJ_LOCK(m_mediaMutex)
    if (!m_mediaQueue.empty())
    {
        ptr = *m_mediaQueue.begin();
        m_mediaQueue.pop_front();
    }    
    return ptr;
}
void HJBaseGPUToRAM::setMediaData(HJSPBuffer::Ptr i_buffer, int width, int height)
{
    HJRGBAMediaData::Ptr ptr = HJRGBAMediaData::Create<HJRGBAMediaData>(i_buffer, width, height);

    HJ_LOCK(m_mediaMutex)
    m_mediaQueue.clear();
    m_mediaQueue.push_back(std::move(ptr));
}
void HJBaseGPUToRAM::setMediaData(int i_width, int i_height, unsigned char *i_data, int i_stride, int bufferSize)
{
    HJRGBAMediaData::Ptr data = HJRGBAMediaData::Create<HJRGBAMediaData>(i_width, i_height, i_data, i_stride, bufferSize);
    HJ_LOCK(m_mediaMutex)
    m_mediaQueue.clear();
    m_mediaQueue.push_back(std::move(data));
}
HJBaseGPUToRAM::Ptr HJBaseGPUToRAM::CreateGPUToRAM(HJGPUToRAMType i_type)
{
    HJBaseGPUToRAM::Ptr ptr = nullptr;
    switch (i_type)
    {
    case HJGPUToRAMType_ImageReceiver:
        ptr = HJGPUToRAMImageReceiver::Create();
        break;
    case HJGPUToRAMType_PBO:
        ptr = HJGPUToRAMPBO::Create();
        break;
    default:
        break;
    }
    return ptr;
}

HJNativeImagWrapper::~HJNativeImagWrapper()
{
    if (m_image)
    {
        Image_ErrorCode errCode = OH_ImageNative_Release(m_image);
        if (errCode != IMAGE_SUCCESS)
        {
            HJFLoge("OH_ImageNative_Release error :{} ", (int)errCode);
        }
        HJGPUToRAMImageReceiver::m_releaseIdx++;
        HJFLogi("OH_ImageNative_Release success receive:{} release:{} map:{} unmap:{}  rrd:{} image:{}", HJGPUToRAMImageReceiver::m_receiveIdx, HJGPUToRAMImageReceiver::m_releaseIdx, HJGPUToRAMImageReceiver::m_unmapIdx, HJGPUToRAMImageReceiver::m_mapIdx, (HJGPUToRAMImageReceiver::m_receiveIdx - HJGPUToRAMImageReceiver::m_releaseIdx), size_t(m_image));
        m_image = nullptr;
    }
}

////////////////////////////

HJGPUToRAMImageReceiver *HJGPUToRAMImageReceiver::the = nullptr;
std::atomic<int64_t> HJGPUToRAMImageReceiver::m_mapIdx{0};
std::atomic<int64_t> HJGPUToRAMImageReceiver::m_unmapIdx{0};
std::atomic<int64_t> HJGPUToRAMImageReceiver::m_receiveIdx{0};
std::atomic<int64_t> HJGPUToRAMImageReceiver::m_releaseIdx{0};

HJGPUToRAMImageReceiver::HJGPUToRAMImageReceiver()
{
    HJ_SetInsName(HJGPUToRAMImageReceiver)
} 

HJGPUToRAMImageReceiver::~HJGPUToRAMImageReceiver()
{
    priDone();
}

int HJGPUToRAMImageReceiver::init(HJBaseParam::Ptr i_param)
{
    int i_err = HJ_OK;
    do
    {
        i_err = HJBaseGPUToRAM::init(i_param);
        if (i_err < 0)
        {
            break;   
        }
        HJFLogi("{} init enter", getInsName());
        HJ_CatchMapGetVal(i_param, HJGPUToRAMImageReceiver::HOImageReceiverSurfaceCb, m_surfaceCb)

        Image_ErrorCode errCode = OH_ImageReceiverOptions_Create(&m_options);
        if (errCode != IMAGE_SUCCESS || m_options == nullptr)
        {
            HJFLoge("{} OH_ImageReceiverOptions_Create error errCode:{}", getInsName(), (int)errCode);
            i_err = HJErrFatal;
            break;
        }
        Image_Size imgSize;
        imgSize.width = 1;
        imgSize.height = 1;
        int32_t capacity = 8;
        errCode = OH_ImageReceiverOptions_SetSize(m_options, imgSize);
        if (errCode != IMAGE_SUCCESS)
        {
            HJFLoge("{} OH_ImageReceiverOptions_SetSize error errCode:{}", getInsName(), (int)errCode);
            i_err = HJErrFatal;
            break;
        }
        errCode = OH_ImageReceiverOptions_SetCapacity(m_options, capacity);
        if (errCode != IMAGE_SUCCESS)
        {
            HJFLoge("{} OH_ImageReceiverOptions_SetCapacity error errCode:{}", getInsName(), (int)errCode);
            i_err = HJErrFatal;
            break;
        }

        errCode = OH_ImageReceiverNative_Create(m_options, &m_receiver);
        if (errCode != IMAGE_SUCCESS || m_receiver == nullptr)
        {
            HJFLoge("{} OH_ImageReceiverNative_Create error errCode:{}", getInsName(), (int)errCode);
            i_err = HJErrFatal;
            break;
        }
        
        m_thread = HJThreadPool::Create();
        if (m_thread)
        {
            m_thread->start();
        }    
        
        HJFLogi("{} create receiver:{}", getInsName(), size_t(m_receiver));
        uint64_t receiverSurfaceID = 0;
        errCode = OH_ImageReceiverNative_GetReceivingSurfaceId(m_receiver, &receiverSurfaceID);
        if (errCode != IMAGE_SUCCESS)
        {
            HJFLoge("{} OH_ImageReceiverNative_GetReceivingSurfaceId error errCode:{}", getInsName(), (int)errCode);
            i_err = HJErrFatal;
            break;
        }
        else
        {
            int32_t ret = OH_NativeWindow_CreateNativeWindowFromSurfaceId(receiverSurfaceID, &m_nativeWindow);
            if ((ret != 0) || (!m_nativeWindow))
            {
                HJFLoge("{} OH_NativeWindow_CreateNativeWindowFromSurfaceId error errCode:{}", getInsName(), ret);
                i_err = HJErrFatal;
                break;
            }
            OH_NativeWindow_NativeWindowHandleOpt(m_nativeWindow, SET_BUFFER_GEOMETRY, static_cast<int>(imgSize.width), static_cast<int>(imgSize.height));

            the = this;
            // register callback
            Image_ErrorCode errCode = OH_ImageReceiverNative_On(m_receiver, priOnCallback);

            if (m_surfaceCb)
            {
                HJFLogi("{} callback create nativeWindow", getInsName());
                i_err = m_surfaceCb(m_nativeWindow, imgSize.width, imgSize.height, true);
            }
        }
    } while (false);
    HJFLogi("{} init end i_err:{}", getInsName(), i_err);
    return i_err;
}
void HJGPUToRAMImageReceiver::priWrite()
{
    static HJYuvWriter::Ptr yuvFilter = nullptr;

    for (auto media : m_mediaQueue)
    {
        if (!yuvFilter)
        {
            yuvFilter = HJYuvWriter::Create();
            yuvFilter->init("/data/storage/el2/base/haps/entry/files/test.yuv", media->m_width, media->m_height);
        }

        HJSPBuffer::Ptr yuvBuffer = HJSPBuffer::create(media->m_width * media->m_height * 3 / 2);
        libyuv::ABGRToI420(media->m_buffer->getBuf(), media->m_width * 4,
                           yuvBuffer->getBuf(), media->m_width,
                           yuvBuffer->getBuf() + media->m_width * media->m_height, media->m_width / 2,
                           yuvBuffer->getBuf() + media->m_width * media->m_height * 5 / 4, media->m_width / 2,
                           media->m_width, media->m_height);
        yuvFilter->write(yuvBuffer->getBuf());
    }
}

int HJGPUToRAMImageReceiver::priImageProc(HJNativeImagWrapper::Ptr i_wrapper)
{
    int i_err = HJ_OK;
    int64_t t0 = HJCurrentSteadyMS();
    size_t typeSize = 0;
    uint32_t *types = nullptr;
    do 
    {
        OH_ImageNative *image = i_wrapper->m_image;
        Image_Size size;
        Image_ErrorCode errCode = OH_ImageNative_GetImageSize(image, &size);
        if (errCode != IMAGE_SUCCESS)
        {
            HJFLoge("{} OH_ImageNative_GetImageSize error :{} ", getInsName(), (int)errCode);
            i_err = HJErrFatal;
            break;
        }

        errCode = OH_ImageNative_GetComponentTypes(image, nullptr, &typeSize); //NativeImage: S ExtraGet dataSize error 40602000
        if (errCode != IMAGE_SUCCESS || typeSize == 0)
        {
            HJFLoge("{} OH_ImageNative_GetComponentTypes error :{} ", getInsName(), (int)errCode);
            i_err = HJErrFatal;
            break;
        }

        types = new (std::nothrow) uint32_t[typeSize];
        if (!types)
        {
            i_err = HJErrFatal;
            HJFLoge("{} types new error", getInsName());
            break;
        }

        errCode = OH_ImageNative_GetComponentTypes(image, &types, &typeSize); //NativeImage: S ExtraGet dataSize error 40602000
        if (errCode != IMAGE_SUCCESS || typeSize == 0)
        {
            HJFLoge("{} OH_ImageNative_GetComponentTypes error :{} ", getInsName(), (int)errCode);
            i_err = HJErrFatal;
            break;
        }
        uint32_t component = types[0];
        if (component != NATIVEBUFFER_PIXEL_FMT_RGBA_8888)
        {
            i_err = HJErrFatal;
            HJFLoge("{} OH_ImageNative_GetComponentTypes is not NATIVEBUFFER_PIXEL_FMT_RGBA_8888  {}", getInsName(), component);
            break;
        }

        OH_NativeBuffer *imageBuffer = nullptr;
        errCode = OH_ImageNative_GetByteBuffer(image, component, &imageBuffer);
        if (errCode != IMAGE_SUCCESS)
        {
            HJFLoge("{} OH_ImageNative_GetByteBuffer error :{} ", getInsName(), (int)errCode);
            i_err = HJErrFatal;
            break;
        }       
        size_t bufferSize = 0;
        errCode = OH_ImageNative_GetBufferSize(image, component, &bufferSize);
        if (errCode != IMAGE_SUCCESS)
        {
            HJFLoge("{} OH_ImageNative_GetBufferSize error :{} ", getInsName(), (int)errCode);
            i_err = HJErrFatal;
            break;
        }  
        int32_t stride = 0;
        errCode = OH_ImageNative_GetRowStride(image, component, &stride);
        if (errCode != IMAGE_SUCCESS)
        {
            HJFLoge("{} OH_ImageNative_GetRowStride error :{} ", getInsName(), (int)errCode);
            i_err = HJErrFatal;
            break;
        }
        
        if ((m_catchWidth != size.width) || (m_catchHeight != size.height))
        {
            HJFLogi("priOnCallback receiver: old:<{} {}> new:<{} {}> stride:{} component:{}", m_catchWidth, m_catchHeight, size.width, size.height, stride, component);
            m_catchWidth = size.width;
            m_catchHeight = size.height;
        }

        void *srcVir = nullptr;
        int32_t retCode = OH_NativeBuffer_Map(imageBuffer, &srcVir);
        HJGPUToRAMImageReceiver::m_mapIdx++;
        if (retCode != 0)
        {
            HJFLoge("{} OH_NativeBuffer_Map error :{} ", getInsName(), (int)errCode);
            i_err = HJErrFatal;
            break;
        }
        uint8_t *srcBuffer = static_cast<uint8_t *>(srcVir);

        if ((size.width > 1) && (size.height > 1))
        {
//            HJFLogi("{} buffersize:{} imagesize:{} stride:{}", getInsName(), bufferSize, (size.width * size.height * 4), stride);
            HJBaseGPUToRAM::setMediaData(size.width, size.height, srcBuffer, stride, bufferSize);
#if TEST_WRITE_YUV
            int64_t tw0 = HJCurrentSteadyMS();
            priWrite();
            HJFLogi("{} priWrite time is:{} ", getInsName(), (HJCurrentSteadyMS() - tw0));
#endif
        }

        retCode = OH_NativeBuffer_Unmap(imageBuffer); // 释放buffer,保证bufferQueue正常轮转。
        HJGPUToRAMImageReceiver::m_unmapIdx++;
        
        if (retCode != 0)
        {
            HJFLoge("{} OH_NativeBuffer_Unmap error :{} ", getInsName(), (int)errCode);
            i_err = HJErrFatal;
            break;
        }
    } while (false);
    if (types)
    {
        delete[] types;
        types = nullptr;
    }
    int64_t t1 = HJCurrentSteadyMS();
    HJFLogi("{} imagereceiver time is:{} map:{} unmap:{} mmdiff:{}", getInsName(), (t1 - t0), HJGPUToRAMImageReceiver::m_mapIdx, HJGPUToRAMImageReceiver::m_unmapIdx, (HJGPUToRAMImageReceiver::m_unmapIdx - HJGPUToRAMImageReceiver::m_mapIdx));
    return i_err;
}
int HJGPUToRAMImageReceiver::priProcCallback(OH_ImageReceiverNative *receiver)
{
    int i_err = HJ_OK;
    
    OH_ImageNative *image = nullptr;

    do
    {
        int64_t t0 = HJCurrentSteadyMS();
        if (m_receiver != receiver)
        {
            i_err = HJErrFatal;
            HJFLoge("{} receiver is not match :{} {}", getInsName(), size_t(m_receiver), size_t(receiver));
            break;
        }
        //HJFLogi("priOnCallback receiver:{}", size_t(receiver));
        Image_ErrorCode errCode = OH_ImageReceiverNative_ReadNextImage(receiver, &image);
        if (errCode != IMAGE_SUCCESS || image == nullptr)
        {
            HJFLoge("{} OH_ImageReceiverNative_ReadNextImage error :{} ", getInsName(), (int)errCode);
            i_err = HJErrFatal;
            break;
        }
#if TEST_DIRECT_RELEASE
        {
            Image_ErrorCode errCode = OH_ImageNative_Release(image);
            if (errCode != IMAGE_SUCCESS)
            {
                HJFLoge("OH_ImageNative_Release error :{} ", (int)errCode);
            }
            HJGPUToRAMImageReceiver::m_releaseIdx++;
        }
#else
        HJNativeImagWrapper::Ptr wrapper = HJNativeImagWrapper::Create<HJNativeImagWrapper>(image);
//        wrapper = nullptr;
        if (m_thread)
        {
            m_thread->asyncClear([this, wrapper]()
            {
                return priImageProc(wrapper);
            }, 1000);
        }
#endif
                
        HJGPUToRAMImageReceiver::m_receiveIdx++;
        int64_t t1 = HJCurrentSteadyMS();
        HJFLogi("GPUTORAM imgreceiver timediff:{} receive:{} release:{} map:{} unmap:{}  receive-release:{} receive-map:{} image:{}", (t1-t0), HJGPUToRAMImageReceiver::m_receiveIdx, HJGPUToRAMImageReceiver::m_releaseIdx, HJGPUToRAMImageReceiver::m_unmapIdx, HJGPUToRAMImageReceiver::m_mapIdx, (HJGPUToRAMImageReceiver::m_receiveIdx - HJGPUToRAMImageReceiver::m_releaseIdx), (HJGPUToRAMImageReceiver::m_receiveIdx - HJGPUToRAMImageReceiver::m_mapIdx), size_t(image));
    } while (false);
    return i_err;
}
void HJGPUToRAMImageReceiver::priOnCallback(OH_ImageReceiverNative *receiver)
{
    if (the)
    {
        the->priProcCallback(receiver);
    }
}
void HJGPUToRAMImageReceiver::done()
{
    priDone();

    HJBaseGPUToRAM::done();
}

void HJGPUToRAMImageReceiver::priDone()
{
    if (!m_bDone)
    {
        m_bDone = true;

        HJFLogi("{} priDone enter", getInsName());

        // callback
        if (m_surfaceCb)
        {
            HJFLogi("{} surface destroy ", getInsName());
            m_surfaceCb(m_nativeWindow, 0, 0, false);
        }

        if (m_nativeWindow)
        {
            HJFLogi("{} OH_NativeWindow_DestroyNativeWindow ", getInsName());
            OH_NativeWindow_DestroyNativeWindow(m_nativeWindow);
            m_nativeWindow = nullptr;
        }
        if (m_receiver)
        {
            HJFLogi("{} OH_ImageReceiverNative_Release ", getInsName());
            
            Image_ErrorCode errCode = OH_ImageReceiverNative_Off(m_receiver);
            if (errCode != IMAGE_SUCCESS) {
                HJFLoge("{} OH_ImageReceiverNative_Off errCode:{}", getInsName(), (int)errCode);
            }
            OH_ImageReceiverNative_Release(m_receiver);
            m_receiver = nullptr;
        }
        if (m_options)
        {
            HJFLogi("{} OH_ImageReceiverOptions_Release ", getInsName());
            OH_ImageReceiverOptions_Release(m_options);
            m_options = nullptr;
        }
        if (m_thread)
        {
            m_thread->done();
            m_thread = nullptr;
        }
        
        HJFLogi("{} priDone end", getInsName());
    }
}


//////////////////////////////////////////////////////////////////////////////////////////////

HJGPUToRAMPBO::HJGPUToRAMPBO()
{

}
HJGPUToRAMPBO::~HJGPUToRAMPBO()
{

}

int HJGPUToRAMPBO::init(HJBaseParam::Ptr i_param) 
{
    int i_err = HJ_OK;
    do
    {   

    } while (false);
    return i_err;
}

void HJGPUToRAMPBO::done() 
{
    
}

NS_HJ_END