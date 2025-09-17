#pragma once
#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include <string.h>

#include "HJUtilitys.h"
#include "HJExecutor.h"
#include "HJHLinksManagerInterface.h"

typedef struct URLContext URLContext;
typedef struct AVDictionary AVDictionary;
typedef struct AVIOInterruptCB AVIOInterruptCB;
typedef struct AVBPrint AVBPrint;

NS_HJ_BEGIN
typedef struct JPHTTPShared JPHTTPShared;
/////////////////////////////////////////////////////////////////////////////
class HJHLink : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJHLink>;
    using WPtr = std::weak_ptr<HJHLink>;
    HJHLink(const std::string& name, HJOptions::Ptr opts = nullptr);
    virtual ~HJHLink();
    
    int http_open(const char* uri, int flags, AVDictionary** options, AVDictionary* params);
    int http_accept(URLContext** c);
    int http_handshake();
    int http_read(uint8_t* buf, int size);
    int http_pre_read(int size);
    int http_write(const uint8_t* buf, int size);
    int64_t http_seek(int64_t off, int whence);
    int http_close();
    int http_get_file_handle();
    int http_get_short_seek();
    int http_shutdown(int flags);
    //
    const std::string& getUrl() {
        return m_uri;
    }
private:
    void procParams(AVDictionary* params);
    void freeParams(JPHTTPShared* s);
    static int onInterruptCB(void* ctx);

    void ff_http_init_auth_state(URLContext* dest, const URLContext* src);
    int http_listen(URLContext* h, const char* uri, int flags, AVDictionary** options);
    int http_open_cnx(URLContext* h, AVDictionary** options);
    int ff_http_do_new_request(URLContext* h, const char* uri);
    int ff_http_do_new_request2(URLContext* h, const char* uri, AVDictionary** opts);
    int ff_http_averror(int status_code, int default_averror);
    int http_open_cnx_internal(URLContext* h, AVDictionary** options);
    int http_should_reconnect(JPHTTPShared* s, int err);
    char* redirect_cache_get(JPHTTPShared* s);
    int redirect_cache_set(JPHTTPShared* s, const char* source, const char* dest, int64_t expiry);
    void bprint_escaped_path(AVBPrint* bp, const char* path);
    int http_connect(URLContext* h, const char* path, const char* local_path,
        const char* hoststr, const char* auth,
        const char* proxyauth);
    int has_header(const char* str, const char* header);
    int http_read_header(URLContext* h);
    void handle_http_errors(URLContext* h, int error);
    int http_write_reply(URLContext* h, int status_code);

    int get_cookies(char** cookies, const char* path, const char* domain);
    int parse_cookie(const char* p, AVDictionary** cookies);
    int cookie_string(AVDictionary* dict, char** cookies);
    void parse_expires(JPHTTPShared* s, const char* p);
    void parse_cache_control(JPHTTPShared* s, const char* p);
    int parse_set_cookie(const char* set_cookie, AVDictionary** dict);
    int parse_set_cookie_expiry_time(const char* exp_str, struct tm* buf);

    int http_get_line(JPHTTPShared* s, char* line, int line_size);
    int http_getc(JPHTTPShared* s);
    int process_line(URLContext* h, char* line, int line_count);
    int check_http_code(URLContext* h, int http_code, const char* end);
    int parse_location(JPHTTPShared* s, const char* p);
    void parse_content_range(URLContext* h, const char* p);
    int parse_content_encoding(URLContext* h, const char* p);
    int parse_icy(JPHTTPShared* s, const char* tag, const char* p);

    int http_buf_read(URLContext* h, uint8_t* buf, int size);
    int http_buf_read_compressed(URLContext* h, uint8_t* buf, int size);
    int64_t http_seek_internal(URLContext* h, int64_t off, int whence, int force_reconnect);
    int http_read_stream(URLContext* h, uint8_t* buf, int size);
    int http_read_stream_all(URLContext* h, uint8_t* buf, int size);
    void update_metadata(char* data);
    int store_icy(URLContext* h, int size);
private:
    std::string         m_objname{""};
    std::string			m_uri{""};
    int					flags{3};
    URLContext*         m_uc{NULL};
    std::atomic<bool>   m_isQuit = { false };
    //
    JPHTTPShared*       m_hctx{NULL};
    HJBuffer::Ptr      m_preBuffer{ nullptr };
};

class HJHLinkQueue : public HJObject
{
public:
    using Ptr = std::shared_ptr<HJHLinkQueue>;
    HJHLinkQueue();
    virtual ~HJHLinkQueue();

    void addLink(const HJHLink::Ptr& link);
    HJHLink::Ptr getLink(const std::string& name);
    HJHLink::Ptr removeLink(const std::string& name);
    void freeLink(size_t handle);
    size_t getLinkSize();
    //
    void addUsedLink(const HJHLink::Ptr& link);
    HJHLink::Ptr getUsedLink(size_t handle);
    void freeUsedLink(size_t handle);
private:
    std::recursive_mutex                                m_mutex;
    std::unordered_multimap<std::string, HJHLink::Ptr>  m_links;
    //
    std::unordered_map<size_t, HJHLink::Ptr>            m_usedLinks;
};

class HJHLinkManager : public HJObject
{
public:
    HJ_DECLARE_PUWTR(HJHLinkManager);
    HJHLinkManager();
    virtual ~HJHLinkManager();
    
    HJ_INSTANCE_DECL(HJHLinkManager);
    
    int init();
    void done();
    void terminate();
    
    size_t getLink(URLContext* h, const std::string inUrl, int flags, AVDictionary** options = NULL, AVDictionary* params = NULL);
//    int freeLink(size_t handle);
    
//    HJHLink::Ptr getUsedLinkByID(size_t handle);
    
    void setDefaultUrl(const std::string& url);
    void remDefaultUrl(const std::string& url);
private:
    static const size_t getGlobalID();
    
    HJHLink::Ptr createLink(const std::string url, int flags, AVDictionary** options = NULL, AVDictionary* params = NULL);
    void asyncCreateLink(const std::string& url);
    //void checkLink(size_t handle);
    void asyncCheckLink(size_t handle, int64_t delayTime, const std::string url);

    bool addDefaultUrl(const std::string& url);
    float getHitRate(HJHLink::Ptr& link, std::string objname, bool isHit);

    AVDictionary* getDefaultOptions();
    AVDictionary* getDefaultParams();

    std::string replaceProtocol(const std::string& url);
private:
    std::atomic<bool>       m_security = { false };
    std::recursive_mutex    m_mutex;
    HJHLinkQueue::Ptr       m_links;
    int64_t                 m_linkTotalCount{ 0 };
    int64_t                 m_linkHitCount{ 0 };
    //
    HJ::HJExecutor::Ptr   m_executor{ nullptr };
//    AVDictionary*           m_defaultOptions{ NULL };
//    AVDictionary*           m_defaultParams{ NULL };
    int                     m_defaultFlags{ 1 };
    int                     m_defaultReadSize{ 32768 };
    /*std::list<std::string>	m_tryUrls;*/
    std::map<std::string, std::string>	m_tryUrls;
    std::recursive_mutex    m_tryMutex;
    std::atomic<int64_t>	m_urlLinkAliveTime{ 5 };
    std::atomic<int>        m_urlLinkMax{ 12 };
};

/////////////////////////////////////////////////////////////////////////////
NS_HJ_END

#define HJHLinkManagerInstance()    HJ::HJHLinkManager::getInstance()
#define HJHLinkManagerDestroy()     HJ::HJHLinkManager::destoryInstance()

#define HJHLinkManagerInit()        HJHLinkManagerInstance()->init()
#define HJHLinkManagerDone()        HJHLinkManagerInstance()->done()
#define HJHLinkManagerSetDefaultUrl(url)        HJHLinkManagerInstance()->setDefaultUrl(url)
#define HJHLinkManagerRemDefaultUrl(url)        HJHLinkManagerInstance()->remDefaultUrl(url)
