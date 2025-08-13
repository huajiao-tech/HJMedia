#include "HJPusherBridge.h"

NS_HJ_BEGIN

HJPusherBridge::HJPusherBridge() = default;

HJPusherBridge::~HJPusherBridge() = default;


void HJPusherBridge::setThreadSafeFunction(std::unique_ptr<ThreadSafeFunctionWrapper> i_tsf) {
    m_tsf = std::move(i_tsf);
}

bool HJPusherBridge::isThreadSafeFunctionVaild() {
    return m_tsf != nullptr;
}

void HJPusherBridge::Call(HJYJsonDocument* data) {
    m_tsf->Call(data);
}

NS_HJ_END