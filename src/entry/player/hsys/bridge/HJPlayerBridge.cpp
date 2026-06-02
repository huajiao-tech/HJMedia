#include "HJPlayerBridge.h"

NS_HJ_BEGIN

HJPlayerBridge::HJPlayerBridge() = default;

HJPlayerBridge::~HJPlayerBridge() = default;


void HJPlayerBridge::setThreadSafeFunction(std::unique_ptr<ThreadSafeFunctionWrapper> i_tsf) {
    m_tsf = std::move(i_tsf);
}

void HJPlayerBridge::call(HJYJsonDocument* data) {
    if (m_tsf) {
        m_tsf->Call(data);
    }
}

void HJPlayerBridge::setStateCall(std::unique_ptr<ThreadSafeFunctionWrapper> i_tsf) {
    m_stateCall = std::move(i_tsf);
}

void HJPlayerBridge::stateCall(HJYJsonDocument* data) {
    if (m_stateCall) {
        m_stateCall->Call(data);
    }
}

void HJPlayerBridge::setSeiCall(std::unique_ptr<ThreadSafeFunctionWrapper> i_tsf) {
    m_seiCall = std::move(i_tsf);
}

void HJPlayerBridge::seiCall(HJPlayerSeiCallbackData* data) {
    if (m_seiCall) {
        m_seiCall->Call(data);
    }
}

NS_HJ_END
