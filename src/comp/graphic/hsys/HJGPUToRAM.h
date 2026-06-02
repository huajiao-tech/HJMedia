#pragma once

#include "HJComUtils.h"
#include "HJPrerequisites.h"
#include "HJOGBaseShader.h"
#include "HJThreadPool.h"
#include "HJTransferInfo.h"
#include "HJOGBaseShader.h"
#include "HJMediaData.h"

#if defined(HarmonyOS)
#include <native_window/external_window.h>
#include <multimedia/image_framework/image/image_native.h>
#include <multimedia/image_framework/image/image_receiver_native.h>
#endif

NS_HJ_BEGIN

typedef enum HJGPUToRAMType
{
    HJGPUToRAMType_Unknown = 0,
    HJGPUToRAMType_ImageReceiver,
    HJGPUToRAMType_PBO,
} HJGPUToRAMType;
class HJNativeImagWrapper
{
public:
    HJ_DEFINE_CREATE(HJNativeImagWrapper);
    HJNativeImagWrapper()
    {
        
    }
    HJNativeImagWrapper(OH_ImageNative *i_image):
        m_image(i_image)
    {
        
    }
    virtual ~HJNativeImagWrapper();
    OH_ImageNative *m_image = nullptr;
};


class HJBaseGPUToRAM : public HJBaseSharedObject
{
public:
    HJ_DEFINE_CREATE(HJBaseGPUToRAM);
    HJBaseGPUToRAM()
    {
        
    }
    virtual ~HJBaseGPUToRAM()
    {
        
    }
    virtual int init(HJBaseParam::Ptr i_param)
    {
        return HJ_OK;
    }
    virtual void done() 
    {

    }
    HJRGBAMediaData::Ptr getMediaRGBAData();
    void setMediaData(HJSPBuffer::Ptr i_buffer, int width, int height);
    void setMediaData(int i_width, int i_height, unsigned char *i_data, int i_stride, int bufferSize);
    
    static HJBaseGPUToRAM::Ptr CreateGPUToRAM(HJGPUToRAMType i_type);

protected:
    std::mutex m_mediaMutex;
    HJMediaDataRGBAQueue m_mediaQueue;
};

class HJGPUToRAMImageReceiver : public HJBaseGPUToRAM
{
public:
    using HOImageReceiverSurfaceCb = std::function<int(void *i_window, int i_width, int i_height, bool i_bCreate)>;
    HJ_DEFINE_CREATE(HJGPUToRAMImageReceiver);
    HJGPUToRAMImageReceiver();
    virtual ~HJGPUToRAMImageReceiver();

    int init(HJBaseParam::Ptr i_param) override;
    void done() override;
    
    static std::atomic<int64_t> m_mapIdx;
    static std::atomic<int64_t> m_unmapIdx;
    static std::atomic<int64_t> m_receiveIdx;
    static std::atomic<int64_t> m_releaseIdx;   
    
private:
    void priWrite();
    void priDone();
    int priProcCallback(OH_ImageReceiverNative* receiver);
    int priImageProc(HJNativeImagWrapper::Ptr i_wrapper);
    static void priOnCallback(OH_ImageReceiverNative* receiver);

    
    bool m_bDone = false;
    OHNativeWindow* m_nativeWindow = nullptr;
    OH_ImageReceiverOptions* m_options = nullptr;
    OH_ImageReceiverNative* m_receiver = nullptr;
    HOImageReceiverSurfaceCb m_surfaceCb = nullptr;
    

    int m_catchWidth = 0;
    int m_catchHeight = 0;
    
    HJThreadPool::Ptr m_thread = nullptr;
    
    //fixme after modify
    static HJGPUToRAMImageReceiver* the;
};

class HJGPUToRAMPBO : public HJBaseGPUToRAM
{
public:    
    HJ_DEFINE_CREATE(HJGPUToRAMPBO);
    HJGPUToRAMPBO();
    virtual ~HJGPUToRAMPBO();

    int init(HJBaseParam::Ptr i_param) override;
    void done() override;
};


NS_HJ_END