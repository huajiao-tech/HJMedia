#pragma once
#include "HJMacros.h"
#include "HJFLog.h"
#include <multimedia/image_framework/image/image_native.h>
#include <multimedia/image_framework/image/image_source_native.h>
#include <multimedia/image_framework/image/pixelmap_native.h>
#include <multimedia/image_framework/image/image_receiver_native.h>

NS_HJ_BEGIN

class HJImageNative {
public:
    HJImageNative();
    ~HJImageNative();

    uint64_t getSurfaceId();

private:
    static void onCallback(OH_ImageReceiverNative *receiver);

    void imageReceiverRelease();

    OH_ImageReceiverNative *m_receiver = nullptr;
    OH_ImageReceiverOptions *m_options = nullptr;
    uint64_t m_surfaceId{123};
};

NS_HJ_END
