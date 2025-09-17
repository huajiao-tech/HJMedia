#include "HJPusherBridge.h"

NS_HJ_BEGIN

HJPusherBridge::HJPusherBridge() = default;

HJPusherBridge::~HJPusherBridge() = default;


void HJPusherBridge::setThreadSafeFunction(std::unique_ptr<ThreadSafeFunctionWrapper> i_tsf) {
    m_tsf = std::move(i_tsf);
}

void HJPusherBridge::Call(HJYJsonDocument* data) {
    if (m_tsf) {
        m_tsf->Call(data);
    }
}

void HJPusherBridge::setStateCall(std::unique_ptr<ThreadSafeFunctionWrapper> i_tsf) {
    m_stateCall = std::move(i_tsf);
}

void HJPusherBridge::stateCall(HJYJsonDocument* data) {
    if (m_stateCall) {
        m_stateCall->Call(data);
    }
}

void HJPusherBridge::setAudioCall(std::unique_ptr<ThreadSafeFunctionWrapper> i_tsf) {
    m_audioCall = std::move(i_tsf);
}

void HJPusherBridge::audioCall(std::pair<std::unique_ptr<uint8_t[]>, int>* data) {
    if (m_audioCall) {
        m_audioCall->Call(data);
    }
}

NS_HJ_END