#pragma once
#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include <string.h>

#include "HJUtilitys.h"
#include "HJStatisticalTools.h"

struct URLContext;
struct AVIOInterruptCB;
typedef struct addrinfo addrinfo;
typedef struct AVDictionary AVDictionary;

NS_HJ_BEGIN
/////////////////////////////////////////////////////////////////////////////
class HJLinkAddInfo : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJLinkAddInfo>;
    HJLinkAddInfo(addrinfo *ai, int64_t time = 0);
    virtual ~HJLinkAddInfo();

    addrinfo* getAInfo() {
        return m_ai;
    }
    void setHostInfo(const std::string& info) {
        m_hostInfo = info;
    }
    std::string& getHostInfo() {
        return m_hostInfo;
    }
private:
    addrinfo*   m_ai{ nullptr };
    int64_t     m_time{ 0 };
    std::string m_hostInfo{ "" };
};

class HJLongLinkTcp : public HJObject
{
public:
	using Ptr = std::shared_ptr<HJLongLinkTcp>;
	using WPtr = std::weak_ptr<HJLongLinkTcp>;
	HJLongLinkTcp(const std::string& name, HJOptions::Ptr opts = nullptr);
	virtual ~HJLongLinkTcp();

    int open(const char* uri, int flags, AVDictionary** options = NULL);
    int accept();
    int read(uint8_t* buf, int size);
    int write(const uint8_t* buf, int size);
    int tcpshutdown(int flags);
    int lclose();
    int get_file_handle();
    int get_window_size();

    virtual void setQuit(const bool isQuit = true) {
        m_isQuit = isQuit;
    }
    virtual bool getIsQuit() {
        return m_isQuit;
    }
    bool isAlive();
    void setUsed(bool used) {
        m_used = used;
    }
    bool getUsed() {
        return m_used;
    }
    void setObjName(const std::string name) {
        m_objName = name;
    }
    const std::string& getObjName() {
        return m_objName;
    }
private:
    int procOptions(HJOptions::Ptr opts);
    //
    static int customize_fd(void* ctx, int fd, int family);
    static int onInterruptCB(void* ctx);
private:
    int fd{ -1 };
    //
    int listen{ 0 };
    char* local_port{ NULL };
    char* local_addr{ NULL };
    int open_timeout{ 5000000 };
    int rw_timeout{ -1 };
    int listen_timeout{ -1 };
    int recv_buffer_size{ -1 };
    int send_buffer_size{ -1 };
    bool tcp_nodelay{ false };
    int tcp_mss{ -1 };
    //
    char* str_sid{ NULL };
    char* str_url{ NULL };
    int   use_ipv4{ false };
    char str_ip[400];
    unsigned short port{ 0 };
    int ecode{ 0 };

    std::string         m_objName{ "" };
    std::string         m_filename{ "" };
    int                 flags;
    URLContext*         m_uc{ NULL };
    std::atomic<bool>   m_isQuit = { false };
    bool                m_isAlive{ false };
    std::atomic<bool>   m_used{ true };
	HJNetworkSimulator::Ptr m_simulator{ nullptr };
    HJBitrateTracker::Ptr m_bitrateTracker{ nullptr };
};

/////////////////////////////////////////////////////////////////////////////
class HJLongLinkManager : public HJObject
{
public:
	using Ptr = std::shared_ptr<HJLongLinkManager>;
	using WPtr = std::weak_ptr<HJLongLinkManager>;
	HJLongLinkManager();
	virtual ~HJLongLinkManager();

    HJ_INSTANCE_DECL(HJLongLinkManager);

    void done();

    size_t initLLink(const std::string url, int flags);
    int unusedLLink(size_t handle);
    int freeLLink(size_t handle);

    HJLongLinkTcp::Ptr getLLinkPtr(size_t handle);
    //
    HJLinkAddInfo::Ptr getAddrInfo(const std::string uri);
    void removeAddrInfo(const std::string uri);
private:
    HJLongLinkTcp::Ptr getLLink(const std::string& name);
    static const size_t getGlobalID();
private:
    std::unordered_multimap<std::string, HJLongLinkTcp::Ptr> m_tcps;
    std::unordered_map<size_t, HJLongLinkTcp::Ptr>           m_tcpids;
    int                                                      m_linksMax{ 16 };
    std::recursive_mutex                                     m_mutex;
    std::unordered_multimap<std::string, HJLinkAddInfo::Ptr> m_addInfos;
    std::recursive_mutex                                     m_addrMutex;
};
#define HJLongLinkManagerInstance()  HJ::HJLongLinkManager::getInstance()
#define HJLongLinkManagerDestroy()	 HJ::HJLongLinkManager::destoryInstance()
//
#define JPGetAddrInfo(url)  HJLongLinkManagerInstance()->getAddrInfo(url);
#define JPFreeAddrInfo(url) HJLongLinkManagerInstance()->removeAddrInfo(url);

/////////////////////////////////////////////////////////////////////////////
NS_HJ_END
