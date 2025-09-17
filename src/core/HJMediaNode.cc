//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJMediaNode.h"
#include "HJContext.h"
#include "HJFLog.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJMediaVNode::HJMediaVNode(const HJEnvironment::Ptr& env/* = nullptr*/, const HJScheduler::Ptr& scheduler/* = nullptr*/, const bool isAuto/* = true*/)
{
    setName(HJMakeGlobalName(HJ_TYPE_NAME(HJMediaVNode)));
    setEnviroment(env);
    if(isAuto) {
        setScheduler(scheduler);
    }
}

HJMediaVNode::HJMediaVNode(const HJEnvironment::Ptr& env/* = nullptr*/, const HJExecutor::Ptr& executor/* = nullptr*/, const bool isAuto/* = true*/)
{
    setName(HJMakeGlobalName(HJ_TYPE_NAME(HJMediaVNode)));
    setEnviroment(env);
    if(isAuto) {
        HJScheduler::Ptr scheduler = executor ? std::make_shared<HJScheduler>(executor) : nullptr;
        setScheduler(scheduler);
    }
}

HJMediaVNode::~HJMediaVNode()
{
    //HJLogi("~HJMediaVNode begin");
}

int HJMediaVNode::init(const int capacity/* = HJ_DEFAULT_STORAGE_CAPACITY*/)
{
    m_defaultCapacity = capacity;
    
    return HJ_OK;
}

int HJMediaVNode::start()
{
    int res = HJ_OK;

    m_runState = HJRun_Start;
    res = asyncSelf();
    if (HJ_OK != res) {
        return res;
    }
    for (const auto& it : m_nexts)
    {
        auto node = it.second.lock();
        res = node->start();
        if (HJ_OK != res) {
            break;
        }
    }
    
    return res;
}

int HJMediaVNode::play()
{
    int res = HJ_OK;
    for (const auto& it : m_nexts)
    {
        auto node = it.second.lock();
        res = node->play();
        if (HJ_OK != res) {
            break;
        }
    }    
    return res;
}

int HJMediaVNode::pause()
{
    int res = HJ_OK;
    for (const auto& it : m_nexts)
    {
        auto node = it.second.lock();
        res = node->pause();
        if (HJ_OK != res) {
            break;
        }
    }
    return res;
}

int HJMediaVNode::resume()
{
    int res = HJ_OK;
    for (const auto& it : m_nexts)
    {
        auto node = it.second.lock();
        res = node->resume();
        if (HJ_OK != res) {
            break;
        }
    }
    return res;
}

int HJMediaVNode::flush(HJSeekInfo::Ptr info/* = nullptr*/)
{
    int res = HJ_OK;
    setEOF(false);
    clearInOutStorage();
    //
    for (const auto& it : m_nexts)
    {
        auto node = it.second.lock();
        res = node->flush(info);
        if (HJ_OK != res) {
            break;
        }
    }
    return res;
}

void HJMediaVNode::done()
{
    if (!m_scheduler) {
        return;
    }
    for (const auto& it : m_nexts) {
        auto node = it.second.lock();
        node->done();
    }
    m_nexts.clear();
    m_pres.clear();
    //
    m_inStorages.clear();
    m_outStorage = nullptr;
    m_scheduler = nullptr;

    return;
}

int HJMediaVNode::deliver(HJMediaFrame::Ptr mediaFrame, const std::string& key)
{
    if (!mediaFrame) {
        return HJ_OK;
    }
    HJMediaStorage::Ptr storage = getInStorage(key);
    if (storage) {
        storage->push(std::move(mediaFrame));
        //HJFLogi("node:{}, storage size:{}, capacity:{}", getName(), storage->size(), storage->getCapacity());
    }
    return HJ_OK;
}

const HJMediaFrame::Ptr HJMediaVNode::pop(const std::string& key)
{
    HJMediaStorage::Ptr storage = getInStorage(key);
    if (storage) {
        //HJFLogi("node:{}, storage size:{}, capacity:{}", getName(), storage->size(), storage->getCapacity());
        return storage->pop();
    }
    return nullptr;
}

int HJMediaVNode::recv(HJMediaFrame::Ptr mediaFrame)
{
    if (!m_outStorage || !mediaFrame) {
        return HJErrNotAlready;
    }
    return m_outStorage->push(std::move(mediaFrame));
}

const HJMediaFrame::Ptr HJMediaVNode::popRecv()
{
    if (!m_outStorage) {
        return nullptr;
    }
    return m_outStorage->pop();
}

bool HJMediaVNode::IsNextsFull()
{
    bool isFulls = false;
    do
    {
        HJMediaVNode::Ptr fullNode = nullptr;
        for (const auto& it : m_nexts) {
            auto node = it.second.lock();
            if (node->isFull()) {
                //HJLogw("waning, next node:" + node->getName() + ", type:" + HJMediaType2String(node->getMediaType()) + " storage is full:" + HJ2STR(node->getInStorageSize()));
                fullNode = node;
                break;
            }
        }

        if ((m_nexts.size() > 1) && fullNode && (HJMEDIA_TYPE_AUDIO == fullNode->getMediaType()))
        {
            auto capacity = fullNode->getInStorage()->getCapacity();
            size_t newCapacity = 1.2 * capacity;
            HJFLogi("expansion next node:{} capacity:{} new capacity:{}", fullNode->getName(), capacity, newCapacity);
            fullNode->getInStorage()->setCapacity(newCapacity);
            fullNode = nullptr;
        }
        if (fullNode) {
            isFulls = true;
        }
    } while (false);

    return isFulls;
}

//int HJMediaVNode::sendMessage(const HJMessage::Ptr msg)
//{
//    int res = HJ_OK;
//    res = onMessage(msg);
//    if (HJ_OK != res) {
//        HJLoge("onMessage error:" + HJ2String(res));
//        return res;
//    }
//    for (const auto& it : m_nexts)
//    {
//        auto node = it.second.lock();
//        res = node->sendMessage(msg);
//        if (HJ_OK != res) {
//            break;
//        }
//    }
//    return res;
//}
//
//int HJMediaVNode::onMessage(const HJMessage::Ptr msg)
//{
//    return HJ_OK;
//}

//***********************************************************************************//
const int64_t HJMediaNode::MTASK_DELAY_TIME_DEFAULT = 10;     //ms

NS_HJ_END
