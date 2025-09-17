#include "HJFLog.h"
#include "HJTLSLinkManager.h"
#include "HJTLSLinkManagerInterface.h"
#include "HJException.h"
#include "HJGlobalSettings.h"

#if defined( __cplusplus )
extern "C" {
#endif
    #include "libavformat/avformat.h"
    #include "libavformat/internal.h"
    #include "libavformat/network.h"
    #include "libavformat/os_support.h"
    #include "libavformat/url.h"
    #include "libavformat/tls.h"
    //#include "libavcodec/internal.h"
    #include "libavutil/avstring.h"
    #include "libavutil/avutil.h"
    #include "libavutil/opt.h"
    #include "libavutil/parseutils.h"
    #include "libavutil/thread.h"

    #include <openssl/bio.h>
    #include <openssl/ssl.h>
    #include <openssl/err.h>
#if defined( __cplusplus )
}
#endif

#define JPMakeUrlLinkName(url, flags)   HJFMT("{}&flags={}", url, flags)

NS_HJ_BEGIN
/////////////////////////////////////////////////////////////////////////////
HJTLSOpenSSLLink::HJTLSOpenSSLLink(const std::string& name, HJOptions::Ptr opts)
{
    setName(name);
    //
    //m_uc = (URLContext*)av_mallocz(sizeof(URLContext) + 1);
    //if (!m_uc) {
    //    HJ_EXCEPT(HJException::ERR_INVALID_CALL, "malloc URLContext failed");
    //    return;
    //}
    //m_uc->interrupt_callback.callback = onInterruptCB;
    //m_uc->interrupt_callback.opaque = (void*)this;

    tls_shared = (TLSShared*)av_mallocz(sizeof(TLSShared));
    if (!tls_shared) {
        HJ_EXCEPT(HJException::ERR_INVALID_CALL, "malloc TLSShared failed");
        return;
    }
}

HJTLSOpenSSLLink::~HJTLSOpenSSLLink()
{
    //if (m_uc) {
    //    av_freep(&m_uc);
    //    m_uc = NULL;
    //}
    ffurl_closep(&m_uc);
    if (tls_shared) {
        av_freep(&tls_shared->ca_file);
        av_freep(&tls_shared->cert_file);
        av_freep(&tls_shared->key_file);
        av_freep(&tls_shared->host);
        //
        av_freep(&tls_shared);
        tls_shared = NULL;
    }
}

int HJTLSOpenSSLLink::tls_open(const std::string& uri, int flags, AVDictionary** options)
{
    TLSShared* c = this->tls_shared;
    BIO* bio = NULL;
    int ret = 0;

    int64_t t0 = HJCurrentSteadyMS();
    ret = ffurl_alloc(&m_uc, uri.c_str(), flags, NULL);
    if (ret < 0) {
        HJFLoge("error, url alloc uc failed, m_uri:{}", uri);
        return ret;
    }
    if ((ret = av_opt_set_dict(m_uc, options)) < 0) {
        HJFLoge("error, url set dict failed, m_uri:{}", uri);
        return ret;
    }
    m_uc->interrupt_callback.callback = onInterruptCB;
    m_uc->interrupt_callback.opaque = (void*)this;
    //
    //if ((ret = ff_openssl_init()) < 0) {
    //    HJFLoge("error, openssl init failed");
    //    return ret;
    //}
    m_isQuit = false;
    this->m_uri = uri;
    m_uc->flags = this->flags = flags;
    m_objName = ""; //HJFMT("TLSLink_{}", getID());
    long timeout = 0;
    if (options)
    {
        AVDictionaryEntry* e = av_dict_get(*options, "obj_name", NULL, 0);
        if (e && e->value) {
            m_objName = std::string((const char*)e->value); //av_strdup((const char*)e->value);
        }
        e = av_dict_get(*options, "timeout", NULL, 0);
        if (e && e->value) {
            timeout = strtol(e->value, NULL, 0);
        }
    }
    //HJFLogi("entry, uri:{} tls id:{}, obj name:{}, cnt:{}, tls url:{}, flags:{}, timeout:{}", uri, getID(), m_objName, ++m_openCnt, uri, flags, timeout);
    do {
        if ((ret = ff_tls_open_underlying(c, m_uc, this->m_uri.c_str(), options)) < 0) {
            HJFLoge("error, tls open underlying failed:{}", ret);
            break;
        }
        int64_t t1 = HJCurrentSteadyMS();
        // We want to support all versions of TLS >= 1.0, but not the deprecated
        // and insecure SSLv2 and SSLv3.  Despite the name, SSLv23_*_method()
        // enables support for all versions of SSL and TLS, and we then disable
        // support for the old protocols immediately after creating the context.
        this->ctx = SSL_CTX_new(c->listen ? SSLv23_server_method() : SSLv23_client_method());
        if (!this->ctx) {
            HJFLoge("error, ssl ctx creat failed : {}", ERR_error_string(ERR_get_error(), NULL));
            ret = AVERROR(EIO);
            break;
        }
        SSL_CTX_set_options(this->ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
        if (c->ca_file) {
            if (!SSL_CTX_load_verify_locations(this->ctx, c->ca_file, NULL))
                HJFLoge("error, SSL_CTX_load_verify_locations : {}", ERR_error_string(ERR_get_error(), NULL));
        }
        if (c->cert_file && !SSL_CTX_use_certificate_chain_file(this->ctx, c->cert_file)) {
            HJFLoge("error, Unable to load cert file {} : {}", c->cert_file, ERR_error_string(ERR_get_error(), NULL));
            ret = AVERROR(EIO);
            break;
        }
        if (c->key_file && !SSL_CTX_use_PrivateKey_file(this->ctx, c->key_file, SSL_FILETYPE_PEM)) {
            HJFLoge("error, Unable to load key file {}: {}", c->key_file, ERR_error_string(ERR_get_error(), NULL));
            ret = AVERROR(EIO);
            break;
        }
        // Note, this doesn't check that the peer certificate actually matches
        // the requested hostname.
        if (c->verify)
            SSL_CTX_set_verify(this->ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
        this->ssl = SSL_new(this->ctx);
        if (!this->ssl) {
            HJFLoge("error, ssl create failed : {}", ERR_error_string(ERR_get_error(), NULL));
            ret = AVERROR(EIO);
            break;
        }
    #if OPENSSL_VERSION_NUMBER >= 0x1010000fL
        this->url_bio_method = BIO_meth_new(BIO_TYPE_SOURCE_SINK, "urlprotocol bio");
        BIO_meth_set_write(this->url_bio_method, url_bio_bwrite);
        BIO_meth_set_read(this->url_bio_method, url_bio_bread);
        BIO_meth_set_puts(this->url_bio_method, url_bio_bputs);
        BIO_meth_set_ctrl(this->url_bio_method, url_bio_ctrl);
        BIO_meth_set_create(this->url_bio_method, url_bio_create);
        BIO_meth_set_destroy(this->url_bio_method, url_bio_destroy);
        bio = BIO_new(this->url_bio_method);
        BIO_set_data(bio, c->tcp);
        //BIO_set_data(bio, p);
    #else
        bio = BIO_new(&url_bio_method);
        bio->ptr = c->tcp;
        //bio->ptr = p;
    #endif
        SSL_set_bio(this->ssl, bio, bio);
        if (!c->listen && !c->numerichost)
            SSL_set_tlsext_host_name(this->ssl, c->host);
        ret = c->listen ? SSL_accept(this->ssl) : SSL_connect(this->ssl);
        if (ret == 0) {
            HJFLoge("error, Unable to negotiate TLS/SSL session");
            ret = AVERROR(EIO);
            break;
        }
        else if (ret < 0) {
            ret = print_tls_error(ret);
            break;
        }
        m_startTime = HJCurrentSteadyMS();
        //
        int64_t tcp_time = t1 - t0;
        int64_t tls_time = m_startTime - t1;
        if(!m_objName.empty()) {
            HJTMSetProcInfo(m_objName, "tcp", tcp_time);
            HJTMSetProcInfo(m_objName, "tls", tls_time);
        }
//        HJFLogi("end, uri:{} tls id: {}, tcp open time:{}, tls open time:{}", uri, getID(), t1 - t0, tls_time);

        return 0;
    } while (false);
    
    HJFLoge("error, tls id: {}, obj name:{} url:{} tls open failed:{}", getID(), m_objName, uri, ret);
    tls_close();
    return ret;
}

int HJTLSOpenSSLLink::tls_reopen(AVDictionary** options)
{
    return tls_open(m_uri, flags, options);
}

int HJTLSOpenSSLLink::tls_read(uint8_t* buf, int size)
{
    int ret = 0;

    //HJFLogi("entry id:{}, size:{}", getID(), size);
    // Set or clear the AVIO_FLAG_NONBLOCK on c->tls_shared.tcp
    this->tls_shared->tcp->flags &= ~AVIO_FLAG_NONBLOCK;
    this->tls_shared->tcp->flags |= this->flags & AVIO_FLAG_NONBLOCK;
    ret = SSL_read(this->ssl, buf, size);
    if (ret > 0)
        return ret;
    if (ret == 0) {
        HJFLogw("warning, AVERROR_EOF");
        return AVERROR_EOF;
    }
    //HJFLoge("error id:{}, ret:{}", getID(), ret);
    return print_tls_error(ret);
}

int HJTLSOpenSSLLink::tls_write(const uint8_t* buf, int size)
{
    int ret = 0;

    //HJFLogi("entry id:{}, size:{}", getID(), size);
    // Set or clear the AVIO_FLAG_NONBLOCK on c->tls_shared.tcp
    this->tls_shared->tcp->flags &= ~AVIO_FLAG_NONBLOCK;
    this->tls_shared->tcp->flags |= this->flags & AVIO_FLAG_NONBLOCK;
    ret = SSL_write(this->ssl, buf, size);
    if (ret > 0)
        return ret;
    if (ret == 0) {
        HJFLogw("warning, AVERROR_EOF");
        return AVERROR_EOF;
    }
    //HJFLoge("error id:{}, ret:{}", getID(), ret);
    return print_tls_error(ret);
}
int HJTLSOpenSSLLink::tls_get_file_handle()
{
    return ffurl_get_file_handle(this->tls_shared->tcp);
}

int HJTLSOpenSSLLink::tls_get_short_seek()
{
    return ffurl_get_short_seek(this->tls_shared->tcp);
}

int HJTLSOpenSSLLink::tls_close()
{
//    HJFLogi("entry, tls id:{}, obj name:{}", getID(), m_objName);
    m_isQuit = true;
    if (this->ssl) {
        SSL_shutdown(this->ssl);
        SSL_free(this->ssl);
        //this->ssl = NULL;
    }
    if (this->ctx) {
        SSL_CTX_free(this->ctx);
        this->ctx = NULL;
    }
    ffurl_closep(&this->tls_shared->tcp);

#if OPENSSL_VERSION_NUMBER >= 0x1010000fL
    if (this->url_bio_method) {
        BIO_meth_free(this->url_bio_method);
        this->url_bio_method = NULL;
    }
#endif
    //if (this->ssl) {
    //    ff_openssl_deinit();
    //}
    this->ssl = NULL;

    if (tls_shared) {
        av_freep(&tls_shared->ca_file);
        av_freep(&tls_shared->cert_file);
        av_freep(&tls_shared->key_file);
        av_freep(&tls_shared->host);
    }
//    JPTMEraseProcCategory(m_objName);

//    HJFLogi("end, tls id:{}, obj name:{}", getID(), m_objName);
    return 0;
}

int HJTLSOpenSSLLink::onInterruptCB(void* ctx)
{
    //HJFLogi("entry");
    HJTLSOpenSSLLink* xio = (HJTLSOpenSSLLink*)ctx;
    if (xio) {
        return xio->m_isQuit;
    }
    return false;
}

int HJTLSOpenSSLLink::print_tls_error(int ret)
{
    int printed = 0, e, averr = AVERROR(EIO);
    if (this->flags & AVIO_FLAG_NONBLOCK) {
        int err = SSL_get_error(this->ssl, ret);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
            return AVERROR(EAGAIN);
    }
    while ((e = ERR_get_error()) != 0) {
        HJFLoge("error, {}", ERR_error_string(e, NULL));
        printed = 1;
    }
    if (this->io_err) {
        HJFLoge("error, IO error:{}", this->io_err);
        printed = 1;
        averr = this->io_err;
        this->io_err = 0;
    }
    if (!printed) {
        HJFLoge("Unknown error");
    }
    return averr;
}

int HJTLSOpenSSLLink::url_bio_create(BIO* b)
{
#if OPENSSL_VERSION_NUMBER >= 0x1010000fL
    BIO_set_init(b, 1);
    BIO_set_data(b, NULL);
    BIO_set_flags(b, 0);
#else
    b->init = 1;
    b->ptr = NULL;
    b->flags = 0;
#endif
    return 1;
}
int HJTLSOpenSSLLink::url_bio_destroy(BIO* b)
{
    return 1;
}

#if OPENSSL_VERSION_NUMBER >= 0x1010000fL
#define GET_BIO_DATA(x) BIO_get_data(x)
#else
#define GET_BIO_DATA(x) (x)->ptr
#endif

//int HJTLSOpenSSLLink::url_bio_bread(BIO* b, char* buf, int len)
//{
//    URLContext* h = (URLContext *)GET_BIO_DATA(b);
//    int ret = ffurl_read(h, (unsigned char *)buf, len);
//    if (ret >= 0)
//        return ret;
//    BIO_clear_retry_flags(b);
//    if (ret == AVERROR(EAGAIN))
//        BIO_set_retry_read(b);
//    if (ret == AVERROR_EXIT)
//        return 0;
//    return -1;
//}
int HJTLSOpenSSLLink::url_bio_bread(BIO* b, char* buf, int len)
{
    URLContext* h = (URLContext*)GET_BIO_DATA(b);
    //int ret = ffurl_read(this->tls_shared.tcp, (unsigned char*)buf, len);
    int ret = ffurl_read(h, (unsigned char*)buf, len);
    if (ret >= 0)
        return ret;
    BIO_clear_retry_flags(b);
    if (ret == AVERROR_EXIT)
        return 0;
    if (ret == AVERROR(EAGAIN))
        BIO_set_retry_read(b);
    //else
        //c->io_err = ret;
    return -1;
}

//int HJTLSOpenSSLLink::url_bio_bwrite(BIO* b, const char* buf, int len)
//{
//    URLContext* h = (URLContext*)GET_BIO_DATA(b);
//    int ret = ffurl_write(h, (unsigned char*)buf, len);
//    if (ret >= 0)
//        return ret;
//    BIO_clear_retry_flags(b);
//    if (ret == AVERROR(EAGAIN))
//        BIO_set_retry_write(b);
//    if (ret == AVERROR_EXIT)
//        return 0;
//    return -1;
//}
int HJTLSOpenSSLLink::url_bio_bwrite(BIO* b, const char* buf, int len)
{
    URLContext* h = (URLContext*)GET_BIO_DATA(b);
    //int ret = ffurl_write(c->tls_shared.tcp, buf, len);
    int ret = ffurl_write(h, (unsigned char*)buf, len);
    if (ret >= 0)
        return ret;
    BIO_clear_retry_flags(b);
    if (ret == AVERROR_EXIT)
        return 0;
    if (ret == AVERROR(EAGAIN))
        BIO_set_retry_write(b);
    //else
        //c->io_err = ret;
    return -1;
}
long HJTLSOpenSSLLink::url_bio_ctrl(BIO* b, int cmd, long num, void* ptr)
{
    if (cmd == BIO_CTRL_FLUSH) {
        BIO_clear_retry_flags(b);
        return 1;
    }
    return 0;
}
int HJTLSOpenSSLLink::url_bio_bputs(BIO* b, const char* str)
{
    return url_bio_bwrite(b, str, strlen(str));
}

/////////////////////////////////////////////////////////////////////////////
HJ_INSTANCE(HJTLSLinkManager)
HJTLSLinkManager::HJTLSLinkManager()
{
    
}

HJTLSLinkManager::~HJTLSLinkManager()
{
    done();
}

int HJTLSLinkManager::init()
{
    if (!HJGlobalSettingsManager::getUseTLSPool()) {
        return HJErrNotAlready;
    }
    int res = HJ_OK;
//    HJFLogi("entry");
    do
    {
        m_defaultUrls = { "tls://live-pull-1.huajiao.com:443", "tls://live-pull-2.huajiao.com:443", "tls://live-pull-3.huajiao.com:443" };
        
        m_executor = HJExecutorManager::createExecutor(HJUtilitys::generateUUID());
        if (!m_executor) {
            res = HJErrFatal;
            break;
        }
        m_security = true;
        //
        procLinkUrl();
        HJFLogi("tls link manager init end");
    } while (false);

    return res;
}

AVDictionary* HJTLSLinkManager::getDefaultOptions()
{
    AVDictionary* defaultOptions{ NULL };
    av_dict_set_int(&defaultOptions, "use_private_jitter", 1, 0);
    av_dict_set(&defaultOptions, "protocol_whitelist", "http,https,tls,rtp,tcp,udp,crypto,httpproxy", 0);
    //av_dict_set(&m_defaultOptions, "obj_name", "TLSLink", 0);
    // 
    int max_rtmp_reconnection_waittime = HJGlobalSettingsManager::getUrlConnectTimeout();
    av_dict_set_int(&defaultOptions, "timeout", max_rtmp_reconnection_waittime, 0);

    return defaultOptions;
}

void HJTLSLinkManager::done()
{
//    HJFLogi("entry");
    terminate();

    HJ_AUTO_LOCK(m_mutex);
    //
    m_defaultUrls.clear();
    //
//    for (const auto& it : m_usedLinks)
//    {
//        const auto& link = it.second;
//        link->tls_close();
//    }
//    m_usedLinks.clear();
//    HJFLogi("end");
    return;
}

void HJTLSLinkManager::terminate()
{
//    HJFLogi("entry");
    m_security = false;
    //
    if(m_executor) {
        m_executor->stop();
    }
    HJ_AUTO_LOCK(m_mutex);
    for (const auto& it : m_nameLinks)
    {
        const auto& links = it.second;
        for (const auto& link : links) {
            link->tls_close();
        }
    }
    m_nameLinks.clear();
//    HJFLogi("end");
}

std::string HJTLSLinkManager::getTLSUrl(const std::string& url)
{
    char buf[1024];
    int port = 0;
    char hostname[1024], hoststr[1024], proto[10];
    char auth[1024], proxyauth[1024] = "";
    char path[MAX_URL_SIZE];
    char* lower_proto = "tcp";

    av_url_split(proto, sizeof(proto), auth, sizeof(auth),
        hostname, sizeof(hostname), &port,
        path, sizeof(path), url.c_str());

    if (!strcmp(proto, "https")) {
        lower_proto = "tls";
        if (port < 0)
            port = 443;
    }
    if (port < 0)
        port = 80;

    ff_url_join(buf, sizeof(buf), "tls", NULL, hostname, port, NULL);

    return std::string(buf);
}

void HJTLSLinkManager::setDefaultUrl(const std::string& url)
{
    if (!m_security) {
        return;
    }
    
    HJFLogi("set tls url:{}", url);
    {
        HJ_AUTO_LOCK(m_mutex);
        std::string tlsUrl = getTLSUrl(url);
        auto it = std::find(m_defaultUrls.begin(), m_defaultUrls.end(), tlsUrl);
        if (it != m_defaultUrls.end()) {
            HJFLogi("tls url:{} aready exist", url);
            return;
        }
        m_defaultUrls.push_back(tlsUrl);
        m_defaultUrls.unique();
        HJFLogi("add default url:{}", url);
    }
    //
    procLinkUrl();
    return;
}

size_t HJTLSLinkManager::getTLSLink(const std::string url, int flags, AVDictionary** options)
{
    if(!m_security) {
        return 0;
    }
    HJFLogi("entry, url:{}, flags:{}", url, flags);
    HJTLSOpenSSLLink::Ptr link = nullptr;
    int streamtype = 0;
    if (options)
    {
        AVDictionaryEntry* e = av_dict_get(*options, "streamtype", NULL, 0);
        if (e && e->value) {
            streamtype = atoi((const char*)e->value);
        }
    }
    if (2 == streamtype)
    {
        link = getrLinkByName(url);
        if (link) {
            HJFLogi("get tls link pool success, id:{}, url:{}", link->getID(), url);
            return (size_t)new HJObjectHolder<HJTLSOpenSSLLink::Ptr>(link); //link->getID();
        }
    }
    link = createTLSLink(url, flags, options);
    if (!link) {
        return 0;
    }
//    HJ_AUTO_LOCK(m_mutex);
//    m_usedLinks[link->getID()] = link;
    if (2 == streamtype) {
        HJFLogi("get tls link pool null, create success, id:{}, streamtype:{}, url:{}", link->getID(), streamtype, url);
    } else {
        HJFLogi("get tls link create success, id:{}, streamtype:{}, url:{}", link->getID(), streamtype, url);
    }

    return (size_t)new HJObjectHolder<HJTLSOpenSSLLink::Ptr>(link); //link->getID();
}

HJTLSOpenSSLLink::Ptr HJTLSLinkManager::createTLSLink(const std::string url, int flags, AVDictionary** options)
{
    std::string name = url;         //JPMakeUrlLinkName(url, flags);
    auto link = std::make_shared<HJTLSOpenSSLLink>(name);
    size_t gid = HJTLSLinkManager::getGlobalID();
    link->setID(gid);
    //
    int res = link->tls_open(url.c_str(), flags, options);
    if (HJ_OK != res) {
        link->tls_close();
        HJFLoge("error, tls open failed, url:{}", url);
        return nullptr;
    }
    //HJFLogi("create tls link name:{}, id:{}", name, gid);
    //
    return link;
}

void HJTLSLinkManager::addTLSLink(HJTLSOpenSSLLink::Ptr& link)
{
    {
        HJ_AUTO_LOCK(m_mutex);
        const auto& it = m_nameLinks.find(link->getName());
        if (it != m_nameLinks.end()) {
            auto& links = it->second;
            links.push_back(link);
        } else {
            HJTLSOpenSSLLinkList links;
            links.push_back(link);
            m_nameLinks.emplace(link->getName(), links);
        }
//        std::string linksInfo = "";
//        for (const auto& it : m_nameLinks) {
//            linksInfo += HJFMT("links:{}, size:{}, ", it.first, it.second.size());
//        }
//        linksInfo += HJFMT("url:{}", link->getName());
//        HJFLogi("{}", linksInfo);
    }
    //
    asyncDelayTLSTask(link->getID(), HJ_SEC_TO_MS(m_urlLinkAliveTime));

    return;
}

//int HJTLSLinkManager::unusedTLSLink(size_t handle)
//{
//    HJ_AUTO_LOCK(m_mutex);
//    auto it = m_usedLinks.find(handle);
//    if (it != m_usedLinks.end()) {
//        const auto& tcp = it->second;
//        tcp->setUsed(false);
//    }
//    return HJ_OK;
//}

//int HJTLSLinkManager::freeTLSLink(size_t handle)
//{
//    HJ_AUTO_LOCK(m_mutex);
//
//    int ret = HJ_OK;
////    auto it = m_usedLinks.find(handle);
////    if (it != m_usedLinks.end()) {
////        const auto& link = it->second;
////        HJFLogi("free used link url:{}, id:{}", link->getName(), link->getID());
////        ret = link->tls_close();
////        //
////        m_usedLinks.erase(it);
////    }
//    for (auto& it : m_nameLinks) {
//        auto& links = it.second;
//        for (auto linkit = links.begin(); linkit != links.end(); linkit++)
//        {
//            if (handle == (*linkit)->getID()) {
//                const auto& link = *linkit;
//                HJFLogi("free name link url:{}, id:{}", link->getName(), link->getID());
//                ret = link->tls_close();
//                //
//                links.erase(linkit);
//                return ret;
//            }
//        }
//    }
//
//    return ret;
//}

//HJTLSOpenSSLLink::Ptr HJTLSLinkManager::getUsedLinkByID(size_t handle)
//{
//    HJ_AUTO_LOCK(m_mutex);
//    auto it = m_usedLinks.find(handle);
//    if (it != m_usedLinks.end()) {
//        return it->second;
//    }
//    return nullptr;
//}

/**
* get and remove
*/
HJTLSOpenSSLLink::Ptr HJTLSLinkManager::getrLinkByName(const std::string& name)
{
    HJTLSOpenSSLLink::Ptr link = nullptr;
    HJTLSOpenSSLLinkList* links = NULL;
    {
        HJ_AUTO_LOCK(m_mutex);
        auto it = m_nameLinks.find(name);
        if (it != m_nameLinks.end())
        {
            links = &it->second;
            if (!links->empty()) {
                link = links->front();
                links->pop_front();
            }
        }
    }
    //move to used
    if (link) {
//        m_usedLinks[link->getID()] = link;
        //
        if (links && links->size() < m_urlLinkMax) {
//            HJFLogi("getr link by name:{}, link size:{}", name, links->size());
            asyncInitTLSLink(name);
        }
    }
    return link;
}

HJTLSOpenSSLLink::Ptr HJTLSLinkManager::getrLinkByID(size_t handle)
{
    HJ_AUTO_LOCK(m_mutex);
    HJTLSOpenSSLLink::Ptr link = nullptr;
    for (auto& it : m_nameLinks)
    {
        auto& links = it.second;
        for (auto linkit = links.begin(); linkit != links.end(); linkit++)
        {
            if (handle == (*linkit)->getID()) {
                link = *linkit;
                //
                links.erase(linkit);
                return link;
            }
        }
    }
    return link;
}

int HJTLSLinkManager::getLinkCountByName(const std::string& name)
{ 
    const auto& it = m_nameLinks.find(name);
    if (it != m_nameLinks.end()) {
        const auto& links = it->second;
        return links.size();
    }
    return 0;
}

std::string HJTLSLinkManager::getValidLinkUrl()
{
    HJ_AUTO_LOCK(m_mutex);
    if (m_defaultUrls.empty()) {
        return "";
    }
    std::string tlsUrl = "";
    for (const auto& url : m_defaultUrls)
    {
        int cnt = getLinkCountByName(url);
        if (cnt < m_urlLinkMax) {
            tlsUrl = url;
            break;
        }
    }
    if (!tlsUrl.empty()) {
        m_defaultUrls.splice(m_defaultUrls.end(), m_defaultUrls, m_defaultUrls.begin());
    }

    return tlsUrl;
}

void HJTLSLinkManager::asyncInitTLSLink(const std::string& url)
{
    if (!m_executor) {
        return;
    }
//    HJFLogi("async init tls link:{}", url);
    HJTLSLinkManager::Wtr wtr = sharedFrom(this);
    m_executor->async([wtr, url]() {
        HJTLSLinkManager::Ptr mgr = wtr.lock();
        if (!mgr) {
            return;
        }
        AVDictionary* options = mgr->getDefaultOptions();
        auto link = mgr->createTLSLink(url, mgr->m_defaultFlags, &options);
        av_freep(&options);
        if (!link) {
            return;
        }
        mgr->addTLSLink(link);
        //
        mgr->procLinkUrl();
    });
}

void HJTLSLinkManager::procLinkUrl()
{
//    HJFLogi("entry");
    std::string tlsUrl = getValidLinkUrl();
    if (!tlsUrl.empty()) {
//        HJFLogi("proc tls url:{}", tlsUrl);
        asyncInitTLSLink(tlsUrl);
    }
    return;
}

void HJTLSLinkManager::reInitTLSLink(size_t handle)
{
    auto link = getrLinkByID(handle);
    if (!link) {
        return;
    }
    //HJFLogi("HJTLSOpenSSLLink reinit tls link id:{}, elapse time:{}", handle, HJCurrentSteadyMS() - link->getStartTime());
    link->tls_close();
    //
    AVDictionary* options = getDefaultOptions();
    int res = link->tls_reopen(&options);
    av_freep(&options);
    if (HJ_OK != res) {
        link->tls_close();
        HJFLoge("error, tls re open failed, url:{}", link->getUri());
        return;
    }
    addTLSLink(link);

    return;
}

void HJTLSLinkManager::asyncDelayTLSTask(size_t handle, int64_t delayTime)
{
    if (!m_executor) {
        return;
    }
    HJTLSLinkManager::Wtr wtr = sharedFrom(this);
    m_executor->async([wtr, handle]() {
        HJTLSLinkManager::Ptr mgr = wtr.lock();
        if (!mgr) {
            return;
        }
        mgr->reInitTLSLink(handle);
    }, delayTime, false);
}

const size_t HJTLSLinkManager::getGlobalID()
{
    static std::atomic<size_t> s_globalTLSLinkIDCounter(0);
    return s_globalTLSLinkIDCounter.fetch_add(1, std::memory_order_relaxed);
}

HJKeyStorage::Ptr HJTLSLinkManager::getProcInfoCategory(const std::string& category)
{
    auto it = m_procInfos.find(category);
    if (it != m_procInfos.end()) {
        return it->second;
    }
    return nullptr;
}

void HJTLSLinkManager::eraseProcInfoCategory(const std::string& category)
{
    HJ_AUTO_LOCK(m_procMutex);
    auto it = m_procInfos.find(category);
    if (it != m_procInfos.end()) {
        m_procInfos.erase(it);
    }
    return;
}

std::string HJTLSLinkManager::getProcInfoCategoryString(const std::string& category)
{
    if (category.empty()) {
        return "";
    }
    HJ_AUTO_LOCK(m_procMutex);
    std::string value = "";
    const auto dict = getProcInfoCategory(category);
    if (dict) 
    {
        dict->for_each([&](const std::string& key) {
            auto it = dict->find(key);
            if (HJANY_ISTYPE(it->second, int)) {
                value += HJFMT(" [{}: {}] ", it->first, std::any_cast<int>(it->second));
            } else if (HJANY_ISTYPE(it->second, short)) {
                value += HJFMT(" [{}: {}] ", it->first, std::any_cast<short>(it->second));
            } else if (HJANY_ISTYPE(it->second, int64_t)) {
                value += HJFMT(" [{}: {}] ", it->first, std::any_cast<int64_t>(it->second));
            } else if (HJANY_ISTYPE(it->second, uint64_t)) {
                value += HJFMT(" [{}: {}] ", it->first, std::any_cast<uint64_t>(it->second));
            } else if (HJANY_ISTYPE(it->second, float)) {
                value += HJFMT(" [{}: {}] ", it->first, std::any_cast<float>(it->second));
            } else if (HJANY_ISTYPE(it->second, double)) {
                value += HJFMT(" [{}: {}] ", it->first, std::any_cast<float>(it->second));
            } else if (HJANY_ISTYPE(it->second, std::string)) {
                value += HJFMT(" [{}: {}] ", it->first, std::any_cast<std::string>(it->second));
            }
            //value += HJFMT(" [{} : {}] ", it->first, HJ::any_cast<it->second.type()>(it->second));
        });
    }
    return value;
}

void HJTLSLinkManager::setProcInfo(const std::string& category, const std::string& key, const std::any& value)
{
    if (category.empty() || key.empty()) {
        return;
    }
    HJ_AUTO_LOCK(m_procMutex);
    HJKeyStorage::Ptr dict = getProcInfoCategory(category);
    if (!dict) {
        dict = std::make_shared<HJKeyStorage>();
        m_procInfos[category] = dict;
    }
    if (!dict) {
        return;
    }
    (*dict)[key] = value;
    dict->pushOrder(key);
    return;
}

/////////////////////////////////////////////////////////////////////////////
NS_HJ_END


/////////////////////////////////////////////////////////////////////////////
size_t hjtls_link_create(const char* url, int flags, AVDictionary** options)
{
    auto& manager = HJTLSLinkManagerInstance();
    if (!manager) {
        return HJ_INVALID_HAND;
    }
    std::string murl{ url };
    return manager->getTLSLink(murl, flags, options);
}
//int hjtls_link_unused(size_t handle)
//{
//    auto& manager = HJTLSLinkManagerInstance();
//    if (!manager) {
//        return HJ_INVALID_HAND;
//    }
//    return manager->unusedTLSLink(handle);
//}
int hjtls_link_destroy(size_t handle)
{
    if(!handle) {
        return 0;
    }
    HJ::HJObjectHolder<HJ::HJTLSOpenSSLLink::Ptr>* linkHolder = (HJ::HJObjectHolder<HJ::HJTLSOpenSSLLink::Ptr> *)handle;
    HJ::HJTLSOpenSSLLink::Ptr link = linkHolder->get();
    if(link) {
        link->tls_close();
    }
    delete linkHolder;
    
//    auto& manager = HJTLSLinkManagerInstance();
//    if (!manager) {
//        return HJ_INVALID_HAND;
//    }
//    return manager->freeTLSLink(handle);
    return 0;
}
//
void hjtls_link_done()
{
    auto& manager = HJTLSLinkManagerInstance();
    if (!manager) {
        return;
    }
    manager->done();
    manager->destoryInstance();
    return;
}

//HJ::HJTLSOpenSSLLink::Ptr hjtls_link_get_link(size_t handle)
//{
//    auto& manager = HJTLSLinkManagerInstance();
//    if (!manager) {
//        return nullptr;
//    }
//    auto link = manager->getUsedLinkByID(handle);
//    if (!link) {
//        HJFLogi("error, llink get tls is null, id:{}", handle);
//        return nullptr;
//    }
//    return link;
//}

HJ::HJTLSOpenSSLLink::Ptr hjtls_link_get_link(size_t handle)
{
    if(!handle) {
        return nullptr;
    }
    HJ::HJObjectHolder<HJ::HJTLSOpenSSLLink::Ptr>* linkHolder = (HJ::HJObjectHolder<HJ::HJTLSOpenSSLLink::Ptr> *)handle;
    return linkHolder->get();
}

int hjtls_link_open(size_t handle, const char* url, int flags, AVDictionary** options)
{
    auto link = hjtls_link_get_link(handle);
    if (!link) {
        return HJErrNotAlready;
    }
    return link->tls_open(url, flags, options);
}

int hjtls_link_read(size_t handle, uint8_t* buf, int size)
{
    auto link = hjtls_link_get_link(handle);
    if (!link) {
        return HJErrNotAlready;
    }
    return link->tls_read(buf, size);
}

int hjtls_link_write(size_t handle, const uint8_t* buf, int size)
{
    auto link = hjtls_link_get_link(handle);
    if (!link) {
        return HJErrNotAlready;
    }
    return link->tls_write(buf, size);
}

int hjtls_link_close(size_t handle)
{
    auto link = hjtls_link_get_link(handle);
    if (!link) {
        return HJErrNotAlready;
    }
    return link->tls_close();
}

int hjtls_link_get_file_handle(size_t handle)
{
    auto link = hjtls_link_get_link(handle);
    if (!link) {
        return HJErrNotAlready;
    }
    return link->tls_get_file_handle();
}

int hjtls_link_tls_get_short_seek(size_t handle)
{
    auto link = hjtls_link_get_link(handle);
    if (!link) {
        return HJErrNotAlready;
    }
    return link->tls_get_short_seek();
}

void hjtls_link_set_proc_info(const char* category, const char* key, const int64_t value)
{
    if (!category || !key) {
        return;
    }
    HJTMSetProcInfo(category, key, value);
    return;
}

/////////////////////////////////////////////////////////////////////////////
#if defined(HJ_OS_DARWIN)
static int openssl_init = 0;

#if HAVE_THREADS && OPENSSL_VERSION_NUMBER < 0x10100000L
#include <openssl/crypto.h>
pthread_mutex_t *openssl_mutexes;
static void openssl_lock(int mode, int type, const char *file, int line)
{
    if (mode & CRYPTO_LOCK)
        pthread_mutex_lock(&openssl_mutexes[type]);
    else
        pthread_mutex_unlock(&openssl_mutexes[type]);
}
#if !defined(WIN32) && OPENSSL_VERSION_NUMBER < 0x10000000
static unsigned long openssl_thread_id(void)
{
    return (intptr_t) pthread_self();
}
#endif
#endif

int ff_openssl_init(void)
{
    ff_lock_avformat();
    if (!openssl_init) {
        /* OpenSSL 1.0.2 or below, then you would use SSL_library_init. If you are
         * using OpenSSL 1.1.0 or above, then the library will initialize
         * itself automatically.
         * https://wiki.openssl.org/index.php/Library_Initialization
         */
#if OPENSSL_VERSION_NUMBER < 0x10100000L
        SSL_library_init();
        SSL_load_error_strings();
#endif
#if HAVE_THREADS && OPENSSL_VERSION_NUMBER < 0x10100000L
        if (!CRYPTO_get_locking_callback()) {
            int i;
            openssl_mutexes = (pthread_mutex_t *)av_malloc_array(sizeof(pthread_mutex_t), CRYPTO_num_locks());
            if (!openssl_mutexes) {
                ff_unlock_avformat();
                return AVERROR(ENOMEM);
            }

            for (i = 0; i < CRYPTO_num_locks(); i++)
                pthread_mutex_init(&openssl_mutexes[i], NULL);
            CRYPTO_set_locking_callback(openssl_lock);
#if !defined(WIN32) && OPENSSL_VERSION_NUMBER < 0x10000000
            CRYPTO_set_id_callback(openssl_thread_id);
#endif
        }
#endif
    }
    openssl_init++;
    ff_unlock_avformat();

    return 0;
}

void ff_openssl_deinit(void)
{
    ff_lock_avformat();
    openssl_init--;
    if (!openssl_init) {
#if HAVE_THREADS && OPENSSL_VERSION_NUMBER < 0x10100000L
        if (CRYPTO_get_locking_callback() == openssl_lock) {
            int i;
            CRYPTO_set_locking_callback(NULL);
            for (i = 0; i < CRYPTO_num_locks(); i++)
                pthread_mutex_destroy(&openssl_mutexes[i]);
            av_free(openssl_mutexes);
        }
#endif
    }
    ff_unlock_avformat();
}
#endif //HJ_OS_DARWIN
