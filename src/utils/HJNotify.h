//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJUtilitys.h"

NS_HJ_BEGIN
//***********************************************************************************//
typedef enum HJNotifyID
{
    HJNotify_NONE = 0,
    HJNotify_Prepared,
    HJNotify_Already,
    HJNotify_NeedWindow,
    HJNotify_WindowChanged,
    HJNotify_PlaySuccess,
    HJNotify_DemuxEnd,
    HJNotify_Complete,
    HJNotify_LoadingStart,
    HJNotify_LoadingEnd,
    HJNotify_ProgressStatus,
    HJNotify_MediaChanged,
    HJNotify_Error,
} HJNotifyID;
HJEnumToStringFuncDecl(HJNotifyID);       //HJNotifyIDToString

typedef enum HJMessageID
{
    HJ_MSG_NONE = 0,
    HJ_MSG_Seek,
    
} HJMessageID;

#define HJ_NOTIFY_IO_INTERRUPT_NET    0x1000

class HJNotification : public HJKeyStorage, public HJObject
{
public:
    using Ptr = std::shared_ptr<HJNotification>;
    HJNotification(const size_t identify, const std::string& msg)
        : HJObject("", identify)
        , m_msg(msg) {
    }
    HJNotification(const size_t identify, const std::string& msg, HJRunnable run)
        : HJObject("", identify)
        , m_msg(msg) 
        , m_run(run) {
    }
    HJNotification(const size_t identify, const int64_t val, const std::string& msg)
        : HJObject("", identify)
        , m_val(val)
        , m_msg(msg) {
    }
    void setVal(const int64_t val) {
        m_val = val;
    }
    const int64_t getVal() const {
        return m_val;
    }
    void setMsg(const std::string& msg) {
        m_msg = msg;
    }
    const std::string& getMsg() const {
        return m_msg;
    }
    //const std::any* getStorage(const std::string& key) {
    //    auto it = find(key);
    //    if (it != end()) {
    //        return &it->second;
    //    }
    //    return nullptr;
    //}
//    template<typename T>
//    T getStoreObj(const std::string& key) {
//
//    }
    void setRunnable(HJRunnable run) {
        m_run = std::move(run);
    }
    void operator()() {
        if (m_run) {
            m_run();
        }
    }
private:
    int64_t     m_val = 0;
    std::string m_msg = "";
    HJRunnable m_run = nullptr;
};
//using HJMessage = HJNotification;
//
inline HJNotification::Ptr HJMakeNotification(const size_t identify, const int64_t val = 0, const std::string& msg = "") {
    return std::make_shared<HJNotification>(identify, val, msg);
}
inline HJNotification::Ptr HJMakeNotification(const size_t identify, const std::string& msg) {
    return std::make_shared<HJNotification>(identify, msg);
}
/*
inline HJMessage::Ptr HJMakeMessage(const size_t identify, const std::string& msg = "") {
    return std::make_shared<HJMessage>(identify, msg);
}
*/
//***********************************************************************************//
using HJListener = std::function<int(const HJNotification::Ptr)>;

NS_HJ_END

