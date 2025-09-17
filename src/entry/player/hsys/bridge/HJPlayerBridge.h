#pragma once

#include "hsys/verify/HJNAPIPlayer.h"
#include "ThreadSafeFunctionWrapper.h"
#include "HJJson.h"

NS_HJ_BEGIN

class HJPlayerBridge final : public HJNAPIPlayer {
public:
    HJPlayerBridge();

    ~HJPlayerBridge();

    void setThreadSafeFunction(std::unique_ptr<ThreadSafeFunctionWrapper> i_tsf);

    void call(HJYJsonDocument* data);

    void setStateCall(std::unique_ptr<ThreadSafeFunctionWrapper> i_tsf);

    void stateCall(HJYJsonDocument* data);

private:
    std::unique_ptr<ThreadSafeFunctionWrapper> m_tsf = nullptr;
    std::unique_ptr<ThreadSafeFunctionWrapper> m_stateCall = nullptr;
};

NS_HJ_END