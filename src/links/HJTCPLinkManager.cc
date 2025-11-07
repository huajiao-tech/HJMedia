//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include <ctime>
#include "HJFLog.h"
#include "HJException.h"
#include "HJTCPLinkManager.h"
#include "HJTCPLinkManagerInterface.h"
#include "HJGlobalSettings.h"

#if defined( __cplusplus )
extern "C" {
#endif
#include "libavformat/avformat.h"
#include "libavutil/avassert.h"
#include "libavutil/parseutils.h"
#include "libavutil/opt.h"
//#include "libavutil/time.h"

//#include "libavutil/internal.h"
#include "libavformat/network.h"
#include "libavformat/os_support.h"
#include "libavformat/url.h"
#if HAVE_POLL_H
#include <poll.h>
#endif
//#include <sys/time.h>
#if defined( __cplusplus )
}
#endif

//#define JP_HAVE_AICACHE     1
//#define JP_HAVE_NETWORK_LIMITED     1
//#define JP_HAVE_BITRATE_MEASURE     1
typedef struct timeval timeval;

NS_HJ_BEGIN
/////////////////////////////////////////////////////////////////////////////
HJLinkAddInfo::HJLinkAddInfo(addrinfo* ai, int64_t time)
    : m_ai(ai)
    , m_time(time)
{

}

HJLinkAddInfo::~HJLinkAddInfo()
{
    if (m_ai) {
        freeaddrinfo(m_ai);
        m_ai = NULL;
    }
}

/////////////////////////////////////////////////////////////////////////////
HJ_INSTANCE(HJLongLinkManager)
HJLongLinkManager::HJLongLinkManager()
{
}

HJLongLinkManager::~HJLongLinkManager()
{
}

void HJLongLinkManager::done()
{
    HJ_AUTO_LOCK(m_mutex);
    m_tcps.clear();
    for (const auto& it : m_tcpids)
    {
        const auto& tcp = it.second;
        tcp->tcpshutdown(AVIO_FLAG_WRITE/*flags*/);
        tcp->lclose();
    }
    m_tcpids.clear();
    return;
}

size_t HJLongLinkManager::initLLink(const std::string url, int flags)
{
    HJ_AUTO_LOCK(m_mutex);
    std::string name = HJFMT("{}&flags={}", url, flags);
    auto tcp = getLLink(name);
    if (tcp) {
        tcp->setUsed(true);
        return tcp->getID();
    }
    tcp = std::make_shared<HJLongLinkTcp>(name);
    size_t gid = HJLongLinkManager::getGlobalID();
    tcp->setID(gid);
    //
    m_tcps.emplace(name, tcp);
    m_tcpids[gid] = tcp;
    HJFLogi("init llink name:{}, id:{}", name, gid);
    //
    return tcp->getID();
}

int HJLongLinkManager::unusedLLink(size_t handle)
{
    HJ_AUTO_LOCK(m_mutex);
    auto it = m_tcpids.find(handle);
    if (it != m_tcpids.end()) {
        const auto& tcp = it->second;
        tcp->setUsed(false);
    }
    return HJ_OK;
}

int HJLongLinkManager::freeLLink(size_t handle)
{
    int ret = HJ_OK;
    HJ_AUTO_LOCK(m_mutex);
    for (auto it = m_tcps.begin(); it != m_tcps.end(); it++)
    {
        const auto& tcp = it->second;
        if (handle == tcp->getID()) {
            m_tcps.erase(it);
            break;
        }
    }
    auto it = m_tcpids.find(handle);
    if (it != m_tcpids.end()) {
        const auto& tcp = it->second;
        HJFLogi("free llink url:{}, id:{}", tcp->getName(), tcp->getID());
        ret = tcp->lclose();
        //
        m_tcpids.erase(it);
    }
    return ret;
}

HJLongLinkTcp::Ptr HJLongLinkManager::getLLinkPtr(size_t handle)
{
    HJ_AUTO_LOCK(m_mutex);
    auto it = m_tcpids.find(handle);
    if (it != m_tcpids.end()) {
        return it->second;
    }
    return nullptr;
}

HJLongLinkTcp::Ptr HJLongLinkManager::getLLink(const std::string& name)
{
    for (auto it = m_tcps.begin(); it != m_tcps.end(); it++) {
        const auto& tcp = it->second;
        if ((tcp->getName() == name) && !tcp->getUsed()) {
            return tcp;
        }
    }
    //auto it = m_tcps.find(key);
    //if (it != m_tcps.end()) {
    //    return it->second;
    //}
    return nullptr;
}

const size_t HJLongLinkManager::getGlobalID()
{
    static std::atomic<size_t> s_globalLongLinkIDCounter(0);
    return s_globalLongLinkIDCounter.fetch_add(1, std::memory_order_relaxed);
}

HJLinkAddInfo::Ptr HJLongLinkManager::getAddrInfo(const std::string uri)
{
    HJ_AUTO_LOCK(m_addrMutex);
    int ret = 0;
    HJLinkAddInfo::Ptr addInfo = nullptr;
    do
    {
        const auto& it = m_addInfos.find(uri);
        if (it != m_addInfos.end()) {
            addInfo = it->second;
            HJFLogi("get addr info, uri:{}", uri);
            break;
        }
        //
        int64_t t0 = HJCurrentSteadyMS();
        char hostname[1024], proto[1024], path[1024];
        int port = 0;
        av_url_split(proto, sizeof(proto), NULL, 0, hostname, sizeof(hostname), &port, path, sizeof(path), uri.c_str());
        if (port <= 0 || port >= 65536) {
            return nullptr;
        }
        struct addrinfo* ai = NULL;
        struct addrinfo hints = { 0 };
        hints.ai_family = AF_UNSPEC;        //AF_INET, ipv4
        hints.ai_socktype = SOCK_STREAM;
        std::string portstr = HJFMT("{}", port);
        if (!hostname[0]) {
            ret = getaddrinfo(NULL, portstr.c_str(), &hints, &ai);
        } else {
            ret = getaddrinfo(hostname, portstr.c_str(), &hints, &ai);
        }
        if (ret) {
            HJFLoge("error, getaddrinfo failed, ret:{}", ret);
            break;
        }
        int64_t dns_time = HJCurrentSteadyMS() - t0;
        //
        std::string hostInfo = "";
        addrinfo* pai = ai;
        char hostbuf[100], portbuf[20];
        while (pai)
        {
            getnameinfo(pai->ai_addr, pai->ai_addrlen,
                hostbuf, sizeof(hostbuf), portbuf, sizeof(portbuf),
                NI_NUMERICHOST | NI_NUMERICSERV);
            hostInfo += HJFMT("[{}:{}] ", hostbuf, portbuf);
            pai = pai->ai_next;
        }
        addInfo = std::make_shared<HJLinkAddInfo>(ai, dns_time);
        addInfo->setHostInfo(hostInfo);
        //
        m_addInfos.emplace(uri, addInfo);
        HJFLogi("end, get addr info, dns time:{}, uri{}", dns_time, uri);
    } while (false);

    return addInfo;
}

void HJLongLinkManager::removeAddrInfo(const std::string uri)
{
    HJ_AUTO_LOCK(m_addrMutex);
    const auto& it = m_addInfos.find(uri);
    if (it != m_addInfos.end()) {
        m_addInfos.erase(it);
    }
    return;
}

/////////////////////////////////////////////////////////////////////////////
HJLongLinkTcp::HJLongLinkTcp(const std::string& name, HJOptions::Ptr opts)
{
    int res = HJ_OK;
    setName(name);
	if (opts) {
		res = procOptions(opts);
        if (HJ_OK != res) {
            HJ_EXCEPT(HJException::ERR_INVALIDPARAMS, "parse options failed");
            return;
        }
	}
    m_uc = (URLContext *)av_mallocz(sizeof(URLContext) + 1);
    if (!m_uc) {
        HJ_EXCEPT(HJException::ERR_INVALID_CALL, "malloc URLContext failed");
        return;
    }
    m_uc->interrupt_callback.callback = onInterruptCB;
    m_uc->interrupt_callback.opaque = (void*)this;
    //
#if JP_HAVE_NETWORK_LIMITED
    m_simulator = std::make_shared<HJNetworkSimulator>(1.0 * 1000 * 1000);
#endif
    m_bitrateTracker = std::make_shared<HJBitrateTracker>();
}

HJLongLinkTcp::~HJLongLinkTcp()
{
    //if (m_uc) {
    //    av_freep(&m_uc);
    //    m_uc = NULL;
    //}
    ffurl_closep(&m_uc);
}

int HJLongLinkTcp::open(const char* uri, int flags, AVDictionary** options)
{
    struct addrinfo hints = { 0 }, * ai, * cur_ai;
    int port, fd = -1;
    const char* p;
    char buf[256];
    int ret;
    char hostname[1024], proto[1024], path[1024];
    char portstr[10];
    //
    ret = ffurl_alloc(&m_uc, uri, flags, NULL);
    if (ret < 0) {
        HJFLoge("error, url alloc uc failed, m_uri:{}", uri ? uri : "");
        return ret;
    }
    if ((ret = av_opt_set_dict(m_uc, options)) < 0) {
        HJFLoge("error, url set dict failed, m_uri:{}", uri ? uri : "");
        return ret;
    }
    m_uc->interrupt_callback.callback = onInterruptCB;
    m_uc->interrupt_callback.opaque = (void*)this;
    //
    this->m_filename = std::string(uri);
    this->flags = flags;
    this->open_timeout = 5000000;
    if (options) {
        AVDictionaryEntry* e = av_dict_get(*options, "obj_name", NULL, 0);
        if (e && e->value) {
            m_objName = std::string((const char*)e->value); //av_strdup((const char*)e->value);
        }
        e = av_dict_get(*options, "timeout", NULL, 0);
        if (e && e->value) {
            this->rw_timeout = strtol(e->value, NULL, 0);
        }
        e = av_dict_get(*options, "send_buffer_size", NULL, 0);
        if (e && e->value) {
            this->send_buffer_size = strtol(e->value,NULL, 0);
        }
        e = av_dict_get(*options, "recv_buffer_size", NULL, 0);
        if (e && e->value) {
            this->recv_buffer_size = strtol(e->value, NULL, 0);
        }
        e = av_dict_get(*options, "tcp_nodelay", NULL, 0);
        if (e && e->value) {
            this->tcp_nodelay = strtol(e->value, NULL, 0);
        }
    }
    HJFLogi("tcp id:{}, obj name:{}, open uri:{}, flags:{}, str_sid:{}, str_url:{}, timeout:{}", getID(), m_objName, uri, flags, this->str_sid ? this->str_sid : "", this->str_url ? this->str_url : "", this->rw_timeout);

    av_url_split(proto, sizeof(proto), NULL, 0, hostname, sizeof(hostname),
        &port, path, sizeof(path), uri);
    if (strcmp(proto, "tcp")) {
        return AVERROR(EINVAL);
    }
    if (port <= 0 || port >= 65536) {
        HJFLoge("error, Port missing in uri");
        return AVERROR(EINVAL);
    }
    this->port = port;

    p = strchr(uri, '?');
    if (p) {
        if (av_find_info_tag(buf, sizeof(buf), "listen", p)) {
            char* endptr = NULL;
            this->listen = strtol(buf, &endptr, 10);
            /* assume if no digits were found it is a request to enable it */
            if (buf == endptr)
                this->listen = 1;
        }
        if (av_find_info_tag(buf, sizeof(buf), "local_port", p)) {
            av_freep(&this->local_port);
            this->local_port = av_strdup(buf);
            if (!this->local_port)
                return AVERROR(ENOMEM);
        }
        if (av_find_info_tag(buf, sizeof(buf), "local_addr", p)) {
            av_freep(&this->local_addr);
            this->local_addr = av_strdup(buf);
            if (!this->local_addr)
                return AVERROR(ENOMEM);
        }
        if (av_find_info_tag(buf, sizeof(buf), "timeout", p)) {
            this->rw_timeout = strtol(buf, NULL, 10);
        }
        if (av_find_info_tag(buf, sizeof(buf), "listen_timeout", p)) {
            this->listen_timeout = strtol(buf, NULL, 10);
        }
        if (av_find_info_tag(buf, sizeof(buf), "tcp_nodelay", p)) {
            this->tcp_nodelay = strtol(buf, NULL, 10);
        }
    }
    if (this->rw_timeout >= 0) {
        this->open_timeout = this->rw_timeout;
        //h->rw_timeout = this->rw_timeout;
    }
#if defined(JP_HAVE_AICACHE)
    uint64_t dns_start = HJCurrentSteadyMS();
    auto addInfo = JPGetAddrInfo(uri);
    if (!addInfo) {
        HJFLoge("error, Failed to resolve uri:{}", uri);
        return AVERROR(EIO);
    }
    ai = addInfo->getAInfo();
    if (!ai) {
        JPFreeAddrInfo(uri);
        HJFLoge("error, Failed to get add info uri:{}", uri);
        return AVERROR(EIO);
    }
    uint64_t dns_time = HJCurrentSteadyMS() - dns_start;
    JPTMSetProcInfo(m_objName, "dns", dns_time);
    HJFLogi("addr {} dns time:{}, host ip info:{}", m_objName, dns_time, addInfo->getHostInfo());
#else
    uint64_t dns_start = HJCurrentSteadyMS();//current_time();
    hints.ai_family = AF_UNSPEC;
    if (this->use_ipv4) {
        hints.ai_family = AF_INET;
        //JPLAYER_LOG_INFO("use ipv4 for addrinfo");
    }
    hints.ai_socktype = SOCK_STREAM;
    snprintf(portstr, sizeof(portstr), "%d", port);
    if (this->listen)
        hints.ai_flags |= AI_PASSIVE;
    if (!hostname[0])
        ret = getaddrinfo(NULL, portstr, &hints, &ai);
    else
        ret = getaddrinfo(hostname, portstr, &hints, &ai);
    this->ecode = ret;
    uint64_t dns_time = HJCurrentSteadyMS() - dns_start;
    if (ret) {
        //JPLAYER_LOG_ERROR("Failed to resolve hostname %s: %s\n", hostname, gai_strerror(ret));
        return AVERROR(EIO);
    }
    struct addrinfo* pai = ai;
//    int maxcnt = 5;
    this->str_ip[0] = 0;
    while (/*maxcnt-- &&*/ pai && pai->ai_addr) {
        char str_ip[100] = { 0 };
        getnameinfo((pai->ai_addr), pai->ai_addrlen, str_ip, sizeof(str_ip), NULL, 0, NI_NUMERICHOST | NI_NUMERICSERV);
        strcat(this->str_ip, str_ip);
        strcat(this->str_ip, "-");
        if (pai->ai_next == NULL) {
            break;
        }
        else {
            pai = pai->ai_next;
        }
    }
    //JPTMSetProcInfo(m_objName, "dns", dns_time);
    HJFLogi("addr {} dns time:{}, str_ip:{}", m_objName, dns_time, this->str_ip);
#endif

    cur_ai = ai;
#if HAVE_STRUCT_SOCKADDR_IN6
    // workaround for IOS9 getaddrinfo in IPv6 only network use hardcode IPv4 address can not resolve port number.
    if (cur_ai->ai_family == AF_INET6) {
        struct sockaddr_in6* sockaddr_v6 = (struct sockaddr_in6*)cur_ai->ai_addr;
        if (!sockaddr_v6->sin6_port) {
            sockaddr_v6->sin6_port = htons(port);
        }
    }
#endif
    if (this->listen > 0) {
        while (cur_ai && fd < 0) {
            fd = ff_socket(cur_ai->ai_family,
                cur_ai->ai_socktype,
                cur_ai->ai_protocol, m_uc);
            if (fd < 0) {
                ret = ff_neterrno();
                cur_ai = cur_ai->ai_next;
            }
        }
        if (fd < 0)
            goto fail1;
        customize_fd(this, fd, cur_ai->ai_family);
    }

    if (this->listen == 2) {
        // multi-client
        if ((ret = ff_listen(fd, cur_ai->ai_addr, cur_ai->ai_addrlen, m_uc)) < 0)
            goto fail1;
    }
    else if (this->listen == 1) {
        // single client
        if ((ret = ff_listen_bind(fd, cur_ai->ai_addr, cur_ai->ai_addrlen,
            this->listen_timeout, m_uc)) < 0)
            goto fail1;
        // Socket descriptor already closed here. Safe to overwrite to client one.
        fd = ret;
    }
    else {
        ret = ff_connect_parallel(ai, this->open_timeout / 1000, 3, m_uc, &fd, customize_fd, this);
        if (ret < 0)
            goto fail1;
    }
    //h->is_streamed = 1;
    this->fd = fd;
    this->m_isAlive = true;

#if !defined(JP_HAVE_AICACHE)
    freeaddrinfo(ai);
#endif
    return 0;

fail1:
    HJFLoge("error id:{}, obj name:{}, open uri:{} failed:{}", getID(), m_objName, uri, ret);
    if (fd >= 0)
        closesocket(fd);
#if defined(JP_HAVE_AICACHE)
    JPFreeAddrInfo(uri);
#else
    freeaddrinfo(ai);
#endif
    this->ecode = ret;
    return ret;
}

int HJLongLinkTcp::accept()
{
    int ret;
    av_assert0(this->listen);
    HJFLogi("entry");
    if ((ret = ffurl_alloc(&m_uc, this->m_filename.c_str(), this->flags, &m_uc->interrupt_callback)) < 0) {
        HJFLoge("error, url alloc failed:{}", ret);
        return ret;
    }
    ret = ff_accept(this->fd, this->listen_timeout, m_uc);
    if (ret < 0) {
        ffurl_closep(&m_uc);
        HJFLoge("error, accept failed:{}", ret);
        return ret;
    }
    this->fd = ret;
    return 0;
}

int HJLongLinkTcp::read(uint8_t* buf, int size)
{
    int ret;
    
    if (!(this->flags & AVIO_FLAG_NONBLOCK)) {
        ret = ff_network_wait_fd_timeout(this->fd, 0, this->rw_timeout, &m_uc->interrupt_callback);
        if (ret) {
            HJFLogi("network wait timeout ret:{}", ret);
            return ret;
        }
    }
    ret = recv(this->fd, (char *)buf, size, 0);
    if (ret < 0) {
        this->ecode = ret;
    }
    if (ret == 0) {
        HJFLogw("warning, AVERROR_EOF");
        return AVERROR_EOF;
    }
    ret = ret < 0 ? ff_neterrno() : ret;

    //while (HJGlobalSettingsManager::getNetBlockEnable()) {
    //    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    //};
#if JP_HAVE_NETWORK_LIMITED
    if (ret > 0) {
        m_simulator->waitData(ret);
    }
#endif
#if JP_HAVE_BITRATE_MEASURE
    if (ret > 0) {
        float bps = m_bitrateTracker->addData(ret);
        HJFLogi("end, read out bps:{}", bps);
    }
#endif

    //HJFLogi("end, read size:{}, ret:{}", size, ret);
    return ret;
}

int HJLongLinkTcp::write(const uint8_t* buf, int size)
{
    int ret;

    if (!(this->flags & AVIO_FLAG_NONBLOCK)) {
        ret = ff_network_wait_fd_timeout(this->fd, 1,this->rw_timeout, &m_uc->interrupt_callback);
        if (ret) {
            HJFLogi("network wait timeout ret:{}", ret);
            return ret;
        }
    }
    ret = send(this->fd, (char *)buf, size, MSG_NOSIGNAL);
    if (ret < 0) {
        this->ecode = ret;
    }
    ret = ret < 0 ? ff_neterrno() : ret;

    //HJFLogi("end, write size:{}, ret:{}", size, ret);
    return ret;
}

int HJLongLinkTcp::tcpshutdown(int flags)
{
    int how;
    HJFLogi("entry id:{}, obj name:{}", getID(), m_objName);
    if (flags & AVIO_FLAG_WRITE && flags & AVIO_FLAG_READ) {
        how = SHUT_RDWR;
    }
    else if (flags & AVIO_FLAG_WRITE) {
        how = SHUT_WR;
    }
    else {
        how = SHUT_RD;
    }
    m_isAlive = false;

    return shutdown(this->fd, how);
}

int HJLongLinkTcp::lclose()
{
    HJFLogi("entry id:{}, obj name:{}", getID(), m_objName);
    m_isQuit = true;
    m_isAlive = false;
    closesocket(this->fd);
    HJFLogi("end id:{}, obj name:{}", getID(), m_objName);

    return 0;
}

int HJLongLinkTcp::get_file_handle()
{
    return this->fd;
}

int HJLongLinkTcp::get_window_size()
{
    int avail = 0;
    socklen_t avail_len = sizeof(avail);

#if HAVE_WINSOCK2_H
    /* SO_RCVBUF with winsock only reports the actual TCP window size when
    auto-tuning has been disabled via setting SO_RCVBUF */
    if (this->recv_buffer_size < 0) {
        return AVERROR(ENOSYS);
    }
#endif

    if (getsockopt(this->fd, SOL_SOCKET, SO_RCVBUF, &avail, &avail_len)) {
        return ff_neterrno();
    }
    HJFLogi("end, SO_RCVBUF avail:{}", avail);
    return avail;
}

int HJLongLinkTcp::customize_fd(void* ctx, int fd, int family)
{
    HJLongLinkTcp* s = (HJLongLinkTcp *)ctx;
    if (s->local_addr || s->local_port) {
        struct addrinfo hints = { 0 }, * ai, * cur_ai;
        int ret;

        hints.ai_family = family;
        hints.ai_socktype = SOCK_STREAM;

        ret = getaddrinfo(s->local_addr, s->local_port, &hints, &ai);
        if (ret) {
            av_log(ctx, AV_LOG_ERROR,
                "Failed to getaddrinfo local addr: %s port: %s err: %s\n",
                s->local_addr, s->local_port, gai_strerror(ret));
            return ret;
        }

        cur_ai = ai;
        while (cur_ai) {
            ret = bind(fd, (struct sockaddr*)cur_ai->ai_addr, (int)cur_ai->ai_addrlen);
            if (ret)
                cur_ai = cur_ai->ai_next;
            else
                break;
        }
        freeaddrinfo(ai);

        if (ret) {
            ff_log_net_error(ctx, AV_LOG_ERROR, "bind local failed");
            return ret;
        }
    }
    /* Set the socket's send or receive buffer sizes, if specified.
   If unspecified or setting fails, system default is used. */
    if (s->recv_buffer_size > 0) {
        if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &s->recv_buffer_size, sizeof(s->recv_buffer_size))) {
            HJFLogi("setsockopt(SO_RCVBUF) size:{}", s->recv_buffer_size);
            //ff_log_net_error(ctx, AV_LOG_WARNING, "setsockopt(SO_RCVBUF)");
        }
    }
    if (s->send_buffer_size > 0) {
        if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &s->send_buffer_size, sizeof(s->send_buffer_size))) {
            HJFLogi("setsockopt(SO_SNDBUF) size:{}, s->send_buffer_size");
            //ff_log_net_error(ctx, AV_LOG_WARNING, "setsockopt(SO_SNDBUF)");
        }
    }
    if (s->tcp_nodelay) {
        if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &s->tcp_nodelay, sizeof(s->tcp_nodelay))) {
            HJFLogi("setsockopt(TCP_NODELAY) nodelay:{}", s->tcp_nodelay);
            //ff_log_net_error(ctx, AV_LOG_WARNING, "setsockopt(TCP_NODELAY)");
        }
    }
#if !HAVE_WINSOCK2_H
    if (s->tcp_mss > 0) {
        if (setsockopt(fd, IPPROTO_TCP, TCP_MAXSEG, &s->tcp_mss, sizeof(s->tcp_mss))) {
            //ff_log_net_error(ctx, AV_LOG_WARNING, "setsockopt(TCP_MAXSEG)");
        }
    }
#endif /* !HAVE_WINSOCK2_H */
    
    int rcv_buf_size = 0;
    socklen_t rcv_buf_len = sizeof(rcv_buf_size);
    if (getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *)& rcv_buf_size, &rcv_buf_len)) {
        HJFLoge("error, get SO_RCVBUF failed:{}", ff_neterrno());
    }
    int snd_buf_size = 0;
    socklen_t snd_buf_len = sizeof(snd_buf_size);
    if (getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char*)&snd_buf_size, &snd_buf_len)) {
        HJFLoge("error, get SO_RCVBUF failed:{}", ff_neterrno());
    }
    timeval rcv_timeout{ 0,0 }, snd_timeout{0,0};
    socklen_t opt_len = sizeof(timeval);
    if (getsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&rcv_timeout, &opt_len) < 0) {
        HJFLoge("error, get SO_RCVTIMEO failed:{}", ff_neterrno());
    }
    if (getsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (char*)&snd_timeout, &opt_len) < 0) {
        HJFLoge("error, get SO_RCVTIMEO failed:{}", ff_neterrno());
    }
    HJFLogi("end, SO_RCVBUF:{}, SO_SNDBUF:{}, SO_RCVTIMEO:{}, SO_SNDTIMEO:{}", rcv_buf_size, snd_buf_size, (rcv_timeout.tv_sec * 1000000 + rcv_timeout.tv_usec)/1000, (snd_timeout.tv_sec * 1000000 + snd_timeout.tv_usec) / 1000);
    
    return HJ_OK;
}

int HJLongLinkTcp::onInterruptCB(void* ctx)
{
    HJLongLinkTcp* xio = (HJLongLinkTcp*)ctx;
    if (xio) {
        return xio->m_isQuit;
    }
    return false;
}

bool HJLongLinkTcp::isAlive()
{
    return m_isAlive;
}

int HJLongLinkTcp::procOptions(HJOptions::Ptr opts)
{
	int res = HJ_OK;
	do
	{
		if (opts->haveValue("listen")) {
			listen = opts->getValue<int>("listen");
		}
		if (opts->haveValue("timeout")) {
			rw_timeout = opts->getValue<int>("timeout");
		}
		if (opts->haveValue("listen_timeout")) {
			listen_timeout = opts->getValue<int>("listen_timeout");
		}
		if (opts->haveValue("recv_buffer_size")) {
			recv_buffer_size = opts->getValue<int>("recv_buffer_size");
		}
		if (opts->haveValue("send_buffer_size")) {
			send_buffer_size = opts->getValue<int>("send_buffer_size");
		}
		if (opts->haveValue("tcp_nodelay")) {
			tcp_nodelay = opts->getValue<bool>("tcp_nodelay");
		}
		if (opts->haveValue("tcp_mss")) {
			tcp_mss = opts->getValue<int>("tcp_mss");
		}
        if (opts->haveValue("flags")) {
            flags = opts->getValue<int>("flags");
        }
	} while (false);

	return res;
}

/////////////////////////////////////////////////////////////////////////////
NS_HJ_END


/////////////////////////////////////////////////////////////////////////////
size_t hjtcp_link_create(const char* url, int flags)
{
    auto& manager = HJLongLinkManagerInstance();
    if (!manager) {
        return HJ_INVALID_HAND;
    }
    std::string murl{url};
    return manager->initLLink(murl, flags);
}

int hjtcp_link_unused(size_t handle)
{
    auto& manager = HJLongLinkManagerInstance();
    if (!manager) {
        return HJ_INVALID_HAND;
    }
    return manager->unusedLLink(handle);
}

int hjtcp_link_destroy(size_t handle)
{
    auto& manager = HJLongLinkManagerInstance();
    if (!manager) {
        return HJ_INVALID_HAND;
    }
    return manager->freeLLink(handle);
}

void hjtcp_link_done()
{
    auto& manager = HJLongLinkManagerInstance();
    if (!manager) {
        return;
    }
    manager->done();
    manager->destoryInstance();
    return;
}

HJ::HJLongLinkTcp::Ptr hjtcp_link_get_tcp(size_t handle)
{
    auto& manager = HJLongLinkManagerInstance();
    if (!manager) {
        return nullptr;
    }
    auto tcp = manager->getLLinkPtr(handle);
    if (!tcp) {
        HJFLogi("error, llink get tcp is null, id:{}", handle);
        return nullptr;
    }
    return tcp;
}

int hjtcp_link_open(size_t handle, const char* url, int flags, AVDictionary** options)
{
    auto tcp = hjtcp_link_get_tcp(handle);
    if (!tcp) {
        return HJErrNotAlready;
    }
    return tcp->open(url, flags, options);
}

int hjtcp_link_accept(size_t handle)
{
    auto tcp = hjtcp_link_get_tcp(handle);
    if (!tcp) {
        return HJErrNotAlready;
    }
    return tcp->accept();
}

int hjtcp_link_read(size_t handle, uint8_t* buf, int size)
{
    auto tcp = hjtcp_link_get_tcp(handle);
    if (!tcp) {
        return HJErrNotAlready;
    }
    return tcp->read(buf, size);
}

int hjtcp_link_write(size_t handle, const uint8_t* buf, int size)
{
    auto tcp = hjtcp_link_get_tcp(handle);
    if (!tcp) {
        return HJErrNotAlready;
    }
    return tcp->write(buf, size);
}

int hjtcp_link_shutdown(size_t handle, int flags)
{
    auto tcp = hjtcp_link_get_tcp(handle);
    if (!tcp) {
        return HJErrNotAlready;
    }
    return tcp->tcpshutdown(flags);
}
int hjtcp_link_close(size_t handle)
{
    auto tcp = hjtcp_link_get_tcp(handle);
    if (!tcp) {
        return HJErrNotAlready;
    }
    return tcp->lclose();
}
int hjtcp_link_get_file_handle(size_t handle)
{
    auto tcp = hjtcp_link_get_tcp(handle);
    if (!tcp) {
        return HJErrNotAlready;
    }
    return tcp->get_file_handle();
}
int hjtcp_link_get_window_size(size_t handle)
{
    auto tcp = hjtcp_link_get_tcp(handle);
    if (!tcp) {
        return HJErrNotAlready;
    }
    return tcp->get_window_size();
}

int hjtcp_link_is_alive(size_t handle)
{
    auto tcp = hjtcp_link_get_tcp(handle);
    if (!tcp) {
        return HJErrNotAlready;
    }
    return tcp->isAlive();
}
