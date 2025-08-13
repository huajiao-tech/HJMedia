#pragma once

#include "HJPusherInterface.h"
#include "ThreadSafeFunctionWrapper.h"
#include "hsys/verify/HJNAPILiveStream.h"

NS_HJ_BEGIN

class HJPusherBridge final : public HJNAPILiveStream {
public:
    HJPusherBridge();

    ~HJPusherBridge() override;

    void setThreadSafeFunction(std::unique_ptr<ThreadSafeFunctionWrapper> i_tsf);

    bool isThreadSafeFunctionVaild();

    void Call(HJYJsonDocument* data);

private:
    std::unique_ptr<ThreadSafeFunctionWrapper> m_tsf = nullptr;
};

NS_HJ_END