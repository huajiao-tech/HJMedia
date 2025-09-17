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

    void Call(HJYJsonDocument* data);

    void setStateCall(std::unique_ptr<ThreadSafeFunctionWrapper> i_tsf);

    void stateCall(HJYJsonDocument* data);

    void setAudioCall(std::unique_ptr<ThreadSafeFunctionWrapper> i_tsf);

    void audioCall(std::pair<std::unique_ptr<uint8_t[]>, int>* data);

private:
    std::unique_ptr<ThreadSafeFunctionWrapper> m_tsf = nullptr;
    std::unique_ptr<ThreadSafeFunctionWrapper> m_stateCall = nullptr;
    std::unique_ptr<ThreadSafeFunctionWrapper> m_audioCall = nullptr;
};

NS_HJ_END