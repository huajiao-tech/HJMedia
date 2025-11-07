#include "HJFLog.h"
#include "HJHLinksManager.h"
#include "HJHLinksManagerInterface.h"
#include "HJException.h"
#include "HJTLSLinkManager.h"
#include "HJGlobalSettings.h"

#if defined( __cplusplus )
extern "C" {
#endif
    #include "config.h"
	#include "config_components.h"

    #if CONFIG_ZLIB
    #include <zlib.h>
    #endif /* CONFIG_ZLIB */

    #include "libavutil/avassert.h"
    #include "libavutil/avstring.h"
	#include "libavutil/bprint.h"
	#include "libavutil/getenv_utf8.h"
    #include "libavutil/opt.h"
    #include "libavutil/time.h"
    #include "libavutil/parseutils.h"
    #include "libavutil/error.h"
//    #include "libavutil/internal.h"

    #include "libavformat/avformat.h"
    #include "libavformat/http.h"
    #include "libavformat/httpauth.h"
    #include "libavformat/internal.h"
    #include "libavformat/network.h"
    #include "libavformat/os_support.h"
    #include "libavformat/url.h"
	#include "libavformat/version.h"
#if defined( __cplusplus )
}
#endif

#if HAVE_LIBC_MSVCRT
#define SIZE_SPECIFIER "Iu"
#else
#define SIZE_SPECIFIER "zu"
#endif

NS_HJ_BEGIN
/////////////////////////////////////////////////////////////////////////////
/* The IO buffer size is unrelated to the max URL size in itself, but needs
 * to be large enough to fit the full request headers (including long
 * path names). */
#define BUFFER_SIZE   (MAX_URL_SIZE + HTTP_HEADERS_SIZE)
#define MAX_REDIRECTS 8
#define MAX_CACHED_REDIRECTS 32
#define HTTP_SINGLE   1
#define HTTP_MUTLI    2
#define MAX_EXPIRY    19
#define WHITESPACES " \n\t\r"
typedef enum {
    LOWER_PROTO,
    READ_HEADERS,
    WRITE_REPLY_HEADERS,
    FINISH
}JPHandshakeState;

typedef struct JPHTTPShared
{
    URLContext *hd;
    unsigned char buffer[BUFFER_SIZE], *buf_ptr, *buf_end;
    int line_count;
    int http_code;
    /* Used if "Transfer-Encoding: chunked" otherwise -1. */
    uint64_t chunksize;
    int chunkend;
    uint64_t off, end_off, filesize;
    char *uri;
    char *location;
    HTTPAuthState auth_state;
    HTTPAuthState proxy_auth_state;
    char *http_proxy;
    char *headers;
    char *mime_type;
    char *http_version;
    char *user_agent;
    char *referer;
    char *content_type;
    /* Set if the server correctly handles Connection: close and will close
     * the connection after feeding us the content. */
    int willclose;
    int seekable;           /**< Control seekability, 0 = disable, 1 = enable, -1 = probe. */
    int chunked_post;
    /* A flag which indicates if the end of chunked encoding has been sent. */
    int end_chunked_post;
    /* A flag which indicates we have finished to read POST reply. */
    int end_header;
    /* A flag which indicates if we use persistent connections. */
    int multiple_requests;
    uint8_t *post_data;
    int post_datalen;
    int is_akamai;
    int is_mediagateway;
    char *cookies;          ///< holds newline (\n) delimited Set-Cookie header field values (without the "Set-Cookie: " field name)
    /* A dictionary containing cookies keyed by cookie name */
    AVDictionary *cookie_dict;
    int icy;
    /* how much data was read since the last ICY metadata packet */
    uint64_t icy_data_read;
    /* after how many bytes of read data a new metadata packet will be found */
    uint64_t icy_metaint;
    char *icy_metadata_headers;
    char *icy_metadata_packet;
    AVDictionary *metadata;
#if CONFIG_ZLIB
    int compressed;
    z_stream inflate_stream;
    uint8_t *inflate_buffer;
#endif /* CONFIG_ZLIB */
    AVDictionary *chained_options;
    /* -1 = try to send if applicable, 0 = always disabled, 1 = always enabled */
    int send_expect_100;
    char *method;
    int reconnect;
    int reconnect_at_eof;
    int reconnect_on_network_error;
    int reconnect_streamed;
    int reconnect_delay_max;
    char *reconnect_on_http_error;
    int listen;
    char *resource;
    int reply_code;
    int is_multi_client;
    JPHandshakeState handshake_step;
    int is_connected_server;
    int short_seek_size;
    int64_t expires;
    char *new_location;
    AVDictionary *redirect_cache;
    uint64_t filesize_from_content_range;
} JPHTTPShared;

#define DEFAULT_USER_AGENT "Lavf/" AV_STRINGIFY(LIBAVFORMAT_VERSION)

/////////////////////////////////////////////////////////////////////////////
HJHLink::HJHLink(const std::string& name, HJOptions::Ptr opts)
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
    //m_uc->protocol_whitelist = av_strdup("http,https,tls,rtp,tcp,udp,crypto,httpproxy");
    //
    m_hctx = (JPHTTPShared*)av_mallocz(sizeof(JPHTTPShared) + 1);
    if (!m_hctx) {
        HJ_EXCEPT(HJException::ERR_INVALID_CALL, "malloc URLContext failed");
        return;
    }
}

HJHLink::~HJHLink()
{
    //av_freep(&m_uc->protocol_whitelist);
    //av_freep(&m_uc);
    ffurl_closep(&m_uc);
    if (m_hctx) {
        av_freep(&m_hctx);
        m_hctx = NULL;
    }
}

//void HJHLink::setIOInterruptCB(AVIOInterruptCB* cb)
//{
//    if(!cb || !cb->callback || !cb->opaque) {
//        return;
//    }
//    m_uc->interrupt_callback = *cb;
//    return;
//}

void HJHLink::procParams(AVDictionary* params)
{
    if (!params) {
        return;
    }
    JPHTTPShared* s = m_hctx;
    AVDictionaryEntry* e = NULL;
    e = av_dict_get(params, "offset", NULL, 0);
    if (e && e->value) {
        s->off = atoll(e->value);
    }
    e = av_dict_get(params, "end_off", NULL, 0);
    if (e && e->value) {
        s->end_off = atoll(e->value);
    }
    e = av_dict_get(params, "uri", NULL, 0);
    if (e && e->value) {
        s->uri = av_strdup(e->value);
    }
    e = av_dict_get(params, "location", NULL, 0);
    if (e && e->value) {
        s->location = av_strdup(e->value);
    }
    e = av_dict_get(params, "auth_type", NULL, 0);
    if (e && e->value) {
        s->auth_state.auth_type = atoi(e->value);
    }
    e = av_dict_get(params, "http_proxy", NULL, 0);
    if (e && e->value) {
        s->http_proxy = av_strdup(e->value);
    }
    e = av_dict_get(params, "headers", NULL, 0);
    if (e && e->value) {
        s->headers = av_strdup(e->value);
    }
    e = av_dict_get(params, "mime_type", NULL, 0);
    if (e && e->value) {
        s->mime_type = av_strdup(e->value);
    }
    e = av_dict_get(params, "http_version", NULL, 0);
    if (e && e->value) {
        s->http_version = av_strdup(e->value);
    }
    e = av_dict_get(params, "user_agent", NULL, 0);
    if (e && e->value) {
        s->user_agent = av_strdup(e->value);
    }
    e = av_dict_get(params, "referer", NULL, 0);
    if (e && e->value) {
        s->referer = av_strdup(e->value);
    }
    e = av_dict_get(params, "content_type", NULL, 0);
    if (e && e->value) {
        s->content_type = av_strdup(e->value);
    }
    e = av_dict_get(params, "seekable", NULL, 0);
    if (e && e->value) {
        s->seekable = atoi(e->value);
    }
    e = av_dict_get(params, "chunked_post", NULL, 0);
    if (e && e->value) {
        s->chunked_post = atoi(e->value);
    }
    e = av_dict_get(params, "multiple_requests", NULL, 0);
    if (e && e->value) {
        s->multiple_requests = atoi(e->value);
    }
    e = av_dict_get(params, "listen", NULL, 0);
    if (e && e->value) {
        s->listen = atoi(e->value);
    }
    e = av_dict_get(params, "cookies", NULL, 0);
    if (e && e->value) {
        s->cookies = av_strdup(e->value);
    }
    e = av_dict_get(params, "icy", NULL, 0);
    if (e && e->value) {
        s->icy = atoi(e->value);
    }
    e = av_dict_get(params, "icy_metadata_headers", NULL, 0);
    if (e && e->value) {
        s->icy_metadata_headers = av_strdup(e->value);
    }
    e = av_dict_get(params, "icy_metadata_packet", NULL, 0);
    if (e && e->value) {
        s->icy_metadata_packet = av_strdup(e->value);
    }
    e = av_dict_get(params, "send_expect_100", NULL, 0);
    if (e && e->value) {
        s->send_expect_100 = atoi(e->value);
    }
    e = av_dict_get(params, "method", NULL, 0);
    if (e && e->value) {
        s->method = av_strdup(e->value);
    }
    e = av_dict_get(params, "reconnect", NULL, 0);
    if (e && e->value) {
        s->reconnect = atoi(e->value);
    }
    e = av_dict_get(params, "reconnect_at_eof", NULL, 0);
    if (e && e->value) {
        s->reconnect_at_eof = atoi(e->value);
    }
    e = av_dict_get(params, "reconnect_on_network_error", NULL, 0);
    if (e && e->value) {
        s->reconnect_on_network_error = atoi(e->value);
    }
    e = av_dict_get(params, "reconnect_streamed", NULL, 0);
    if (e && e->value) {
        s->reconnect_streamed = atoi(e->value);
    }
    e = av_dict_get(params, "reconnect_delay_max", NULL, 0);
    if (e && e->value) {
        s->reconnect_delay_max = atoi(e->value);
    }
    e = av_dict_get(params, "reconnect_on_http_error", NULL, 0);
    if (e && e->value) {
        s->reconnect_on_http_error = av_strdup(e->value);
    }
    e = av_dict_get(params, "listen", NULL, 0);
    if (e && e->value) {
        s->listen = atoi(e->value);
    }
    e = av_dict_get(params, "resource", NULL, 0);
    if (e && e->value) {
        s->resource = av_strdup(e->value);
    }
    e = av_dict_get(params, "reply_code", NULL, 0);
    if (e && e->value) {
        s->reply_code = atoi(e->value);
    }

    return;
}

void HJHLink::freeParams(JPHTTPShared* s)
{
    av_freep(&s->uri);
    av_freep(&s->location);
    av_freep(&s->http_proxy);
    av_freep(&s->headers);
    av_freep(&s->mime_type);
    av_freep(&s->http_version);
    av_freep(&s->user_agent);
    av_freep(&s->referer);
#if FF_API_HTTP_USER_AGENT
    av_freep(&s->user_agent_deprecated);
#endif
    av_freep(&s->content_type);
    av_freep(&s->post_data);
    av_freep(&s->cookies);
    av_freep(&s->icy_metadata_headers);
    av_freep(&s->icy_metadata_packet);
    av_freep(&s->method);
    av_freep(&s->reconnect_on_http_error);
    av_freep(&s->resource);
    av_freep(&s->new_location);

    av_dict_free(&s->chained_options);
    av_dict_free(&s->cookie_dict);
    av_dict_free(&s->redirect_cache);
}

int HJHLink::onInterruptCB(void* ctx)
{
    //HJFLogi("entry");
    HJHLink* xio = (HJHLink*)ctx;
    if (xio) {
        return xio->m_isQuit;
    }
    return false;
}

void HJHLink::ff_http_init_auth_state(URLContext* dest, const URLContext* src)
{
    memcpy(&((JPHTTPShared*)dest->priv_data)->auth_state,
        &((JPHTTPShared*)src->priv_data)->auth_state,
        sizeof(HTTPAuthState));
    memcpy(&((JPHTTPShared*)dest->priv_data)->proxy_auth_state,
        &((JPHTTPShared*)src->priv_data)->proxy_auth_state,
        sizeof(HTTPAuthState));
}

int HJHLink::http_open_cnx_internal(URLContext* h, AVDictionary** options)
{
    const char* path, * proxy_path, * lower_proto = "tcp", * local_path;
    char *env_http_proxy, *env_no_proxy;
    char *hashmark;
    char hostname[1024], hoststr[1024], proto[10];
    char auth[1024], proxyauth[1024] = "";
    char path1[MAX_URL_SIZE], sanitized_path[MAX_URL_SIZE + 1];
    char buf[1024], urlbuf[MAX_URL_SIZE];
    int port, use_proxy, err = 0;
    JPHTTPShared* s = m_hctx;

    av_url_split(proto, sizeof(proto), auth, sizeof(auth),
                 hostname, sizeof(hostname), &port,
                 path1, sizeof(path1), s->location);
    ff_url_join(hoststr, sizeof(hoststr), NULL, NULL, hostname, port, NULL);

    env_http_proxy = getenv_utf8("http_proxy");
    proxy_path = s->http_proxy ? s->http_proxy : env_http_proxy;

    env_no_proxy = getenv_utf8("no_proxy");
    use_proxy  = !ff_http_match_no_proxy(env_no_proxy, hostname) &&
                 proxy_path && av_strstart(proxy_path, "http://", NULL);
    freeenv_utf8(env_no_proxy);

    if (!strcmp(proto, "https")) {
        lower_proto = "tls";
        use_proxy   = 0;
        if (port < 0)
            port = 443;
        /* pass http_proxy to underlying protocol */
        if (s->http_proxy) {
            err = av_dict_set(options, "http_proxy", s->http_proxy, 0);
            if (err < 0)
                goto end;
        }
    }
    if (port < 0)
        port = 80;

    hashmark = strchr(path1, '#');
    if (hashmark)
        *hashmark = '\0';

    if (path1[0] == '\0') {
        path = "/";
    } else if (path1[0] == '?') {
        snprintf(sanitized_path, sizeof(sanitized_path), "/%s", path1);
        path = sanitized_path;
    } else {
        path = path1;
    }
    local_path = path;
    if (use_proxy) {
        /* Reassemble the request URL without auth string - we don't
         * want to leak the auth to the proxy. */
        ff_url_join(urlbuf, sizeof(urlbuf), proto, NULL, hostname, port, "%s",
                    path1);
        path = urlbuf;
        av_url_split(NULL, 0, proxyauth, sizeof(proxyauth),
                     hostname, sizeof(hostname), &port, NULL, 0, proxy_path);
    }

    ff_url_join(buf, sizeof(buf), lower_proto, NULL, hostname, port, NULL);

    if (!s->hd) {
        err = ffurl_open_whitelist(&s->hd, buf, AVIO_FLAG_READ_WRITE,
                                   &h->interrupt_callback, options,
                                   h->protocol_whitelist, h->protocol_blacklist, h);
    }

end:
    freeenv_utf8(env_http_proxy);
    return err < 0 ? err : http_connect(
        h, path, local_path, hoststr, auth, proxyauth);
}

int HJHLink::http_should_reconnect(JPHTTPShared*s, int err)
{
    const char *status_group;
    char http_code[4];

    switch (err) {
    case AVERROR_HTTP_BAD_REQUEST:
    case AVERROR_HTTP_UNAUTHORIZED:
    case AVERROR_HTTP_FORBIDDEN:
    case AVERROR_HTTP_NOT_FOUND:
    case AVERROR_HTTP_OTHER_4XX:
        status_group = "4xx";
        break;

    case AVERROR_HTTP_SERVER_ERROR:
        status_group = "5xx";
        break;

    default:
        return s->reconnect_on_network_error;
    }

    if (!s->reconnect_on_http_error)
        return 0;

    if (av_match_list(status_group, s->reconnect_on_http_error, ',') > 0)
        return 1;

    snprintf(http_code, sizeof(http_code), "%d", s->http_code);

    return av_match_list(http_code, s->reconnect_on_http_error, ',') > 0;
}

char* HJHLink::redirect_cache_get(JPHTTPShared*s)
{
    AVDictionaryEntry *re;
    int64_t expiry;
    char *delim;

    re = av_dict_get(s->redirect_cache, s->location, NULL, AV_DICT_MATCH_CASE);
    if (!re) {
        return NULL;
    }

    delim = strchr(re->value, ';');
    if (!delim) {
        return NULL;
    }

    expiry = strtoll(re->value, NULL, 10);
    if (time(NULL) > expiry) {
        return NULL;
    }

    return delim + 1;
}

int HJHLink::redirect_cache_set(JPHTTPShared*s, const char *source, const char *dest, int64_t expiry)
{
    const char *value;
    int ret;

    value = av_asprintf("%" PRIi64";%s", expiry, dest);
    if (!value) {
        return AVERROR(ENOMEM);
    }

    ret = av_dict_set(&s->redirect_cache, source, value, AV_DICT_MATCH_CASE | AV_DICT_DONT_STRDUP_VAL);
    if (ret < 0)
        return ret;

    return 0;
}

int HJHLink::http_open_cnx(URLContext* h, AVDictionary** options)
{
    JPHTTPShared* s = m_hctx;
    HTTPAuthType cur_auth_type, cur_proxy_auth_type;
    int ret, attempts = 0, redirects = 0;
    int reconnect_delay = 0;
    uint64_t off;
    char *cached;

redo:

    cached = redirect_cache_get(s);
    if (cached) {
        av_free(s->location);
        s->location = av_strdup(cached);
        if (!s->location) {
            ret = AVERROR(ENOMEM);
            goto fail;
        }
        goto redo;
    }

    av_dict_copy(options, s->chained_options, 0);

    cur_auth_type       = (HTTPAuthType)s->auth_state.auth_type;
    cur_proxy_auth_type = (HTTPAuthType)s->auth_state.auth_type;

    off = s->off;
    ret = http_open_cnx_internal(h, options);
    if (ret < 0) {
        if (!http_should_reconnect(s, ret) ||
            reconnect_delay > s->reconnect_delay_max)
            goto fail;

        HJFLogw("Will reconnect at {} in {} second(s).", off, reconnect_delay);
        ret = ff_network_sleep_interruptible(1000U * 1000 * reconnect_delay, &h->interrupt_callback);
        if (ret != AVERROR(ETIMEDOUT))
            goto fail;
        reconnect_delay = 1 + 2 * reconnect_delay;

        /* restore the offset (http_connect resets it) */
        s->off = off;

        ffurl_closep(&s->hd);
        goto redo;
    }

    attempts++;
    if (s->http_code == 401) {
        if ((cur_auth_type == HTTP_AUTH_NONE || s->auth_state.stale) &&
            s->auth_state.auth_type != HTTP_AUTH_NONE && attempts < 4) {
            ffurl_closep(&s->hd);
            goto redo;
        } else
            goto fail;
    }
    if (s->http_code == 407) {
        if ((cur_proxy_auth_type == HTTP_AUTH_NONE || s->proxy_auth_state.stale) &&
            s->proxy_auth_state.auth_type != HTTP_AUTH_NONE && attempts < 4) {
            ffurl_closep(&s->hd);
            goto redo;
        } else
            goto fail;
    }
    if ((s->http_code == 301 || s->http_code == 302 ||
         s->http_code == 303 || s->http_code == 307 || s->http_code == 308) &&
        s->new_location) {
        /* url moved, get next */
        ffurl_closep(&s->hd);
        if (redirects++ >= MAX_REDIRECTS)
            return AVERROR(EIO);

        if (!s->expires) {
            s->expires = (s->http_code == 301 || s->http_code == 308) ? INT64_MAX : -1;
        }

        if (s->expires > time(NULL) && av_dict_count(s->redirect_cache) < MAX_CACHED_REDIRECTS) {
            redirect_cache_set(s, s->location, s->new_location, s->expires);
        }

        av_free(s->location);
        s->location = s->new_location;
        s->new_location = NULL;

        /* Restart the authentication process with the new target, which
         * might use a different auth mechanism. */
        memset(&s->auth_state, 0, sizeof(s->auth_state));
        attempts = 0;
        HJFLogw("warning, http code:{} redirects:{}, location uri:{}", s->http_code, redirects, s->location ? s->location : "");
        goto redo;
    }
    return 0;

fail:
    if (s->hd)
        ffurl_closep(&s->hd);
    if (ret < 0)
        return ret;
    return ff_http_averror(s->http_code, AVERROR(EIO));
}

int HJHLink::ff_http_do_new_request(URLContext *h, const char *uri) {
    return ff_http_do_new_request2(h, uri, NULL);
}

int HJHLink::ff_http_do_new_request2(URLContext *h, const char *uri, AVDictionary **opts)
{
    //HTTPContext *s = h->priv_data;
    JPHTTPShared* s = m_hctx;
    AVDictionary *options = NULL;
    int ret;
    char hostname1[1024], hostname2[1024], proto1[10], proto2[10];
    int port1, port2;

    if (!h->prot ||
        !(!strcmp(h->prot->name, "http") ||
          !strcmp(h->prot->name, "https")))
        return AVERROR(EINVAL);

    av_url_split(proto1, sizeof(proto1), NULL, 0,
                 hostname1, sizeof(hostname1), &port1,
                 NULL, 0, s->location);
    av_url_split(proto2, sizeof(proto2), NULL, 0,
                 hostname2, sizeof(hostname2), &port2,
                 NULL, 0, uri);
    if (port1 != port2 || strncmp(hostname1, hostname2, sizeof(hostname2)) != 0) {
        HJFLoge("Cannot reuse HTTP connection for different host: {}:{} != {}:{}",
            hostname1, port1,
            hostname2, port2
        );
        return AVERROR(EINVAL);
    }

    if (!s->end_chunked_post) {
        ret = http_shutdown(h->flags);
        if (ret < 0)
            return ret;
    }

    if (s->willclose)
        return AVERROR_EOF;

    s->end_chunked_post = 0;
    s->chunkend      = 0;
    s->off           = 0;
    s->icy_data_read = 0;

    av_free(s->location);
    s->location = av_strdup(uri);
    if (!s->location)
        return AVERROR(ENOMEM);

    av_free(s->uri);
    s->uri = av_strdup(uri);
    if (!s->uri)
        return AVERROR(ENOMEM);

    if ((ret = av_opt_set_dict(s, opts)) < 0)
        return ret;

    HJFLogi("Opening \'{}\' for {}", uri, h->flags & AVIO_FLAG_WRITE ? "writing" : "reading");
    ret = http_open_cnx(h, &options);
    av_dict_free(&options);
    return ret;
}

int HJHLink::ff_http_averror(int status_code, int default_averror)
{
    switch (status_code) {
        case 400: return AVERROR_HTTP_BAD_REQUEST;
        case 401: return AVERROR_HTTP_UNAUTHORIZED;
        case 403: return AVERROR_HTTP_FORBIDDEN;
        case 404: return AVERROR_HTTP_NOT_FOUND;
        default: break;
    }
    if (status_code >= 400 && status_code <= 499)
        return AVERROR_HTTP_OTHER_4XX;
    else if (status_code >= 500)
        return AVERROR_HTTP_SERVER_ERROR;
    else
        return default_averror;
}

int HJHLink::http_write_reply(URLContext* h, int status_code)
{
    JPHTTPShared* s = m_hctx;
    int ret, body = 0, reply_code, message_len;
    const char* reply_text, * content_type;
    char message[BUFFER_SIZE];
    content_type = "text/plain";

    if (status_code < 0)
        body = 1;
    switch (status_code) {
    case AVERROR_HTTP_BAD_REQUEST:
    case 400:
        reply_code = 400;
        reply_text = "Bad Request";
        break;
    case AVERROR_HTTP_FORBIDDEN:
    case 403:
        reply_code = 403;
        reply_text = "Forbidden";
        break;
    case AVERROR_HTTP_NOT_FOUND:
    case 404:
        reply_code = 404;
        reply_text = "Not Found";
        break;
    case 200:
        reply_code = 200;
        reply_text = "OK";
        content_type = s->content_type ? s->content_type : "application/octet-stream";
        break;
    case AVERROR_HTTP_SERVER_ERROR:
    case 500:
        reply_code = 500;
        reply_text = "Internal server error";
        break;
    default:
        return AVERROR(EINVAL);
    }
    if (body) {
        s->chunked_post = 0;
        message_len = snprintf(message, sizeof(message),
                 "HTTP/1.1 %03d %s\r\n"
                 "Content-Type: %s\r\n"
                 "Content-Length: %" SIZE_SPECIFIER"\r\n"
                 "%s"
                 "\r\n"
                 "%03d %s\r\n",
                 reply_code,
                 reply_text,
                 content_type,
                 strlen(reply_text) + 6, // 3 digit status code + space + \r\n
                 s->headers ? s->headers : "",
                 reply_code,
                 reply_text);
    } else {
        s->chunked_post = 1;
        message_len = snprintf(message, sizeof(message),
                 "HTTP/1.1 %03d %s\r\n"
                 "Content-Type: %s\r\n"
                 "Transfer-Encoding: chunked\r\n"
                 "%s"
                 "\r\n",
                 reply_code,
                 reply_text,
                 content_type,
                 s->headers ? s->headers : "");
    }
    HJFLogt("HTTP reply header: {}", message);
    if ((ret = ffurl_write(s->hd, (const unsigned char*)message, message_len)) < 0)
        return ret;
    return 0;
}

void HJHLink::handle_http_errors(URLContext* h, int error)
{
    av_assert0(error < 0);
    http_write_reply(h, error);
}

int HJHLink::http_handshake()
{
    URLContext* c = m_uc;
    int ret, err;
    JPHTTPShared* ch = m_hctx;
    URLContext* cl = ch->hd;

    HJFLogi("entry");
    if (!m_uc) {
        return AVERROR(EINVAL);
    }
    switch (ch->handshake_step) {
    case LOWER_PROTO:
        HJFLogi("Lower protocol");
        if ((ret = ffurl_handshake(cl)) > 0)
            return 2 + ret;
        if (ret < 0)
            return ret;
        ch->handshake_step = READ_HEADERS;
        ch->is_connected_server = 1;
        return 2;
    case READ_HEADERS:
        HJFLogi("Read headers");
        if ((err = http_read_header(c)) < 0) {
            handle_http_errors(c, err);
            return err;
        }
        ch->handshake_step = WRITE_REPLY_HEADERS;
        return 1;
    case WRITE_REPLY_HEADERS:
        av_log(c, AV_LOG_TRACE, "Reply code: %d\n", ch->reply_code);
        if ((err = http_write_reply(c, ch->reply_code)) < 0)
            return err;
        ch->handshake_step = FINISH;
        return 1;
    case FINISH:
        return 0;
    }
    // this should never be reached.
    return AVERROR(EINVAL);
}

int HJHLink::http_listen(URLContext* h, const char* uri, int flags, AVDictionary** options)
{
    JPHTTPShared* s = m_hctx;
    int ret;
    char hostname[1024], proto[10];
    char lower_url[100];
    const char* lower_proto = "tcp";
    int port;
    av_url_split(proto, sizeof(proto), NULL, 0, hostname, sizeof(hostname), &port,
        NULL, 0, uri);
    if (!strcmp(proto, "https"))
        lower_proto = "tls";
    ff_url_join(lower_url, sizeof(lower_url), lower_proto, NULL, hostname, port,
        NULL);
    if ((ret = av_dict_set_int(options, "listen", s->listen, 0)) < 0)
        goto fail;
    if ((ret = ffurl_open_whitelist(&s->hd, lower_url, AVIO_FLAG_READ_WRITE,
        &h->interrupt_callback, options,
        h->protocol_whitelist, h->protocol_blacklist, h
    )) < 0)
        goto fail;
    s->handshake_step = LOWER_PROTO;
    if (s->listen == HTTP_SINGLE) { /* single client */
        s->reply_code = 200;
        while ((ret = http_handshake()) > 0);
    }
fail:
    av_dict_free(&s->chained_options);
    av_dict_free(&s->cookie_dict);
    return ret;
}

int HJHLink::http_open(const char* uri, int flags, AVDictionary** options, AVDictionary* params)
{
    int ret = 0;
    JPHTTPShared* s = m_hctx;
    //AVIOInterruptCB interrupt_callback;
    //interrupt_callback.callback = onInterruptCB;
    //interrupt_callback.opaque = (void*)this;
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
    URLContext* h = m_uc;
    m_isQuit = false;
    this->m_uri = uri;
    h->flags = this->flags = flags;
    m_objname = HJFMT("HTTPLink_{}", getID());
    if (options)
    {
        AVDictionaryEntry* e = av_dict_get(*options, "obj_name", NULL, 0);
        if (e && e->value) {
            m_objname = std::string((const char*)e->value);
        }
    }
    HJFLogi("id:{}, obj name:{}, m_uri:{}, flags:{}", getID(), m_objname, m_uri, flags);

    procParams(params);
    //
    if (s->seekable == 1)
        h->is_streamed = 0;
    else
        h->is_streamed = 1;

    s->filesize = UINT64_MAX;

    s->location = av_strdup(uri);
    if (!s->location)
        return AVERROR(ENOMEM);

    s->uri = av_strdup(uri);
    if (!s->uri)
        return AVERROR(ENOMEM);

    if (options)
        av_dict_copy(&s->chained_options, *options, 0);

    if (s->headers) {
        int len = (int)strlen(s->headers);
        if (len < 2 || strcmp("\r\n", s->headers + len - 2)) {
            HJFLogw("warning, No trailing CRLF found in HTTP header.");
            ret = av_reallocp(&s->headers, len + 3);
            if (ret < 0)
                goto bail_out;
            s->headers[len]     = '\r';
            s->headers[len + 1] = '\n';
            s->headers[len + 2] = '\0';
        }
    }

    if (s->listen) {
        return http_listen(h, uri, flags, options);
    }
    ret = http_open_cnx(h, options);
bail_out:
    if (ret < 0) {
        av_dict_free(&s->chained_options);
        av_dict_free(&s->cookie_dict);
        av_dict_free(&s->redirect_cache);
        av_freep(&s->new_location);
        av_freep(&s->uri);
    }

    return ret;
}

// todo ??
int HJHLink::http_accept(URLContext** c)
{
    int ret;
    JPHTTPShared* sc = m_hctx;
    JPHTTPShared* cc;
    URLContext* sl = sc->hd;
    URLContext* cl = NULL;

    HJFLogw("warning, http_accept entry");
    av_assert0(sc->listen);
    if ((ret = ffurl_alloc(c, this->m_uri.c_str(), this->flags, &sl->interrupt_callback)) < 0)
        goto fail;
    cc = (JPHTTPShared*)(*c)->priv_data;
    if ((ret = ffurl_accept(sl, &cl)) < 0)
        goto fail;
    cc->hd = cl;
    cc->is_multi_client = 1;
    return 0;
fail:
    if (c) {
        ffurl_closep(c);
    }
    return ret;
}

int HJHLink::http_getc(JPHTTPShared* s)
{
    int len;
    if (s->buf_ptr >= s->buf_end) {
        len = ffurl_read(s->hd, s->buffer, BUFFER_SIZE);
        if (len < 0) {
            return len;
        }
        else if (len == 0) {
            return AVERROR_EOF;
        }
        else {
            s->buf_ptr = s->buffer;
            s->buf_end = s->buffer + len;
        }
    }
    return *s->buf_ptr++;
}

int HJHLink::http_get_line(JPHTTPShared* s, char* line, int line_size)
{
    int ch;
    char* q;

    q = line;
    for (;;) {
        ch = http_getc(s);
        if (ch < 0)
            return ch;
        if (ch == '\n') {
            /* process line */
            if (q > line && q[-1] == '\r')
                q--;
            *q = '\0';

            return 0;
        }
        else {
            if ((q - line) < line_size - 1)
                *q++ = ch;
        }
    }
}

int HJHLink::check_http_code(URLContext* h, int http_code, const char* end)
{
    JPHTTPShared* s = m_hctx;
    /* error codes are 4xx and 5xx, but regard 401 as a success, so we
     * don't abort until all headers have been parsed. */
    if (http_code >= 400 && http_code < 600 &&
        (http_code != 401 || s->auth_state.auth_type != HTTP_AUTH_NONE) &&
        (http_code != 407 || s->proxy_auth_state.auth_type != HTTP_AUTH_NONE)) {
        end += strspn(end, SPACE_CHARS);
        HJFLogw("HTTP error {} {}", http_code, end);
        return ff_http_averror(http_code, AVERROR(EIO));
    }
    return 0;
}

int HJHLink::parse_location(JPHTTPShared* s, const char* p)
{
    char redirected_location[MAX_URL_SIZE];
    ff_make_absolute_url(redirected_location, sizeof(redirected_location),
                         s->location, p);
    av_freep(&s->new_location);
    s->new_location = av_strdup(redirected_location);
    if (!s->new_location)
        return AVERROR(ENOMEM);
    return 0;
}

/* "bytes $from-$to/$document_size" */
void HJHLink::parse_content_range(URLContext* h, const char* p)
{
    JPHTTPShared* s = m_hctx;
    const char* slash;

    if (!strncmp(p, "bytes ", 6)) {
        p     += 6;
        s->off = strtoull(p, NULL, 10);
        if ((slash = strchr(p, '/')) && strlen(slash) > 0)
            s->filesize_from_content_range = strtoull(slash + 1, NULL, 10);
    }
    if (s->seekable == -1 && (!s->is_akamai || s->filesize != 2147483647))
        h->is_streamed = 0; /* we _can_ in fact seek */
}
int HJHLink::parse_content_encoding(URLContext* h, const char* p)
{
    if (!av_strncasecmp(p, "gzip", 4) ||
        !av_strncasecmp(p, "deflate", 7)) {
#if CONFIG_ZLIB
        JPHTTPShared* s = m_hctx;

        s->compressed = 1;
        inflateEnd(&s->inflate_stream);
        if (inflateInit2(&s->inflate_stream, 32 + 15) != Z_OK) {
            HJFLogw("Error during zlib initialisation: {}",
                   s->inflate_stream.msg);
            return AVERROR(ENOSYS);
        }
        if (zlibCompileFlags() & (1 << 17)) {
            HJFLogw("Your zlib was compiled without gzip support.");
            return AVERROR(ENOSYS);
        }
#else
        av_log(h, AV_LOG_WARNING,
               "Compressed (%s) content, need zlib with gzip support\n", p);
        return AVERROR(ENOSYS);
#endif /* CONFIG_ZLIB */
    } else if (!av_strncasecmp(p, "identity", 8)) {
        // The normal, no-encoding case (although servers shouldn't include
        // the header at all if this is the case).
    } else {
        HJFLogw("Unknown content coding: {}", p);
    }
    return 0;
}
// Concat all Icy- header lines
int HJHLink::parse_icy(JPHTTPShared* s, const char* tag, const char* p)
{
    int len = 4 + strlen(p) + strlen(tag);
    int is_first = !s->icy_metadata_headers;
    int ret;

    av_dict_set(&s->metadata, tag, p, 0);

    if (s->icy_metadata_headers)
        len += strlen(s->icy_metadata_headers);

    if ((ret = av_reallocp(&s->icy_metadata_headers, len)) < 0)
        return ret;

    if (is_first)
        *s->icy_metadata_headers = '\0';

    av_strlcatf(s->icy_metadata_headers, len, "%s: %s\n", tag, p);

    return 0;
}

int HJHLink::parse_set_cookie_expiry_time(const char* exp_str, struct tm* buf)
{
    char exp_buf[MAX_EXPIRY];
    int i, j, exp_buf_len = MAX_EXPIRY - 1;
    char* expiry;

    // strip off any punctuation or whitespace
    for (i = 0, j = 0; exp_str[i] != '\0' && j < exp_buf_len; i++) {
        if ((exp_str[i] >= '0' && exp_str[i] <= '9') ||
            (exp_str[i] >= 'A' && exp_str[i] <= 'Z') ||
            (exp_str[i] >= 'a' && exp_str[i] <= 'z')) {
            exp_buf[j] = exp_str[i];
            j++;
        }
    }
    exp_buf[j] = '\0';
    expiry = exp_buf;

    // move the string beyond the day of week
    while ((*expiry < '0' || *expiry > '9') && *expiry != '\0')
        expiry++;

    return av_small_strptime(expiry, "%d%b%Y%H%M%S", buf) ? 0 : AVERROR(EINVAL);
}

int HJHLink::parse_set_cookie(const char* set_cookie, AVDictionary** dict)
{
    char *param, *next_param, *cstr, *back;
    char *saveptr = NULL;

    if (!set_cookie[0])
        return 0;

    if (!(cstr = av_strdup(set_cookie)))
        return AVERROR(EINVAL);

    // strip any trailing whitespace
    back = &cstr[strlen(cstr)-1];
    while (strchr(WHITESPACES, *back)) {
        *back='\0';
        if (back == cstr)
            break;
        back--;
    }

    next_param = cstr;
    while ((param = av_strtok(next_param, ";", &saveptr))) {
        char *name, *value;
        next_param = NULL;
        param += strspn(param, WHITESPACES);
        if ((name = av_strtok(param, "=", &value))) {
            if (av_dict_set(dict, name, value, 0) < 0) {
                av_free(cstr);
                return -1;
            }
        }
    }

    av_free(cstr);
    return 0;
}

int HJHLink::parse_cookie(const char* p, AVDictionary** cookies)
{
    JPHTTPShared* s = m_hctx;
    AVDictionary *new_params = NULL;
    AVDictionaryEntry *e, *cookie_entry;
    char *eql, *name;

    // ensure the cookie is parsable
    if (parse_set_cookie(p, &new_params))
        return -1;

    // if there is no cookie value there is nothing to parse
    cookie_entry = av_dict_get(new_params, "", NULL, AV_DICT_IGNORE_SUFFIX);
    if (!cookie_entry || !cookie_entry->value) {
        av_dict_free(&new_params);
        return -1;
    }

    // ensure the cookie is not expired or older than an existing value
    if ((e = av_dict_get(new_params, "expires", NULL, 0)) && e->value) {
        struct tm new_tm = {0};
        if (!parse_set_cookie_expiry_time(e->value, &new_tm)) {
            AVDictionaryEntry *e2;

            // if the cookie has already expired ignore it
            if (av_timegm(&new_tm) < av_gettime() / 1000000) {
                av_dict_free(&new_params);
                return 0;
            }

            // only replace an older cookie with the same name
            e2 = av_dict_get(*cookies, cookie_entry->key, NULL, 0);
            if (e2 && e2->value) {
                AVDictionary *old_params = NULL;
                if (!parse_set_cookie(p, &old_params)) {
                    e2 = av_dict_get(old_params, "expires", NULL, 0);
                    if (e2 && e2->value) {
                        struct tm old_tm = {0};
                        if (!parse_set_cookie_expiry_time(e->value, &old_tm)) {
                            if (av_timegm(&new_tm) < av_timegm(&old_tm)) {
                                av_dict_free(&new_params);
                                av_dict_free(&old_params);
                                return -1;
                            }
                        }
                    }
                }
                av_dict_free(&old_params);
            }
        }
    }
    av_dict_free(&new_params);

    // duplicate the cookie name (dict will dupe the value)
    if (!(eql = (char*)strchr(p, '='))) return AVERROR(EINVAL);
    if (!(name = av_strndup(p, eql - p))) return AVERROR(ENOMEM);

    // add the cookie to the dictionary
    av_dict_set(cookies, name, eql, AV_DICT_DONT_STRDUP_KEY);

    return 0;
}

int HJHLink::cookie_string(AVDictionary* dict, char** cookies)
{
    const AVDictionaryEntry *e = NULL;
    int len = 1;

    // determine how much memory is needed for the cookies string
    while ((e = av_dict_iterate(dict, e)))
        len += strlen(e->key) + strlen(e->value) + 1;

    // reallocate the cookies
    e = NULL;
    if (*cookies) av_free(*cookies);
    *cookies = (char*)av_malloc(len);
    if (!*cookies) return AVERROR(ENOMEM);
    *cookies[0] = '\0';

    // write out the cookies
    while ((e = av_dict_iterate(dict, e)))
        av_strlcatf(*cookies, len, "%s%s\n", e->key, e->value);

    return 0;
}

void HJHLink::parse_expires(JPHTTPShared*s, const char *p)
{
    struct tm tm;

    if (!parse_set_cookie_expiry_time(p, &tm)) {
        s->expires = av_timegm(&tm);
    }
}

void HJHLink::parse_cache_control(JPHTTPShared*s, const char *p)
{
    char *age;
    int offset;

    /* give 'Expires' higher priority over 'Cache-Control' */
    if (s->expires) {
        return;
    }

    if (av_stristr(p, "no-cache") || av_stristr(p, "no-store")) {
        s->expires = -1;
        return;
    }

    age = av_stristr(p, "s-maxage=");
    offset = 9;
    if (!age) {
        age = av_stristr(p, "max-age=");
        offset = 8;
    }

    if (age) {
        s->expires = time(NULL) + atoi(p + offset);
    }
}

int HJHLink::process_line(URLContext* h, char* line, int line_count)
{
    JPHTTPShared* s = m_hctx;
    const char* auto_method = h->flags & AVIO_FLAG_READ ? "POST" : "GET";
    char* tag, * p, * end, * method, * resource, * version;
    int ret;

    /* end of header */
    if (line[0] == '\0') {
        s->end_header = 1;
        return 0;
    }

    p = line;
    if (line_count == 0) {
        if (s->is_connected_server) {
            // HTTP method
            method = p;
            while (*p && !av_isspace(*p))
                p++;
            *(p++) = '\0';
            HJFLogt("Received method: {}", method);
            if (s->method) {
                if (av_strcasecmp(s->method, method)) {
                    HJFLoge("error, Received and expected HTTP method do not match. ({} expected, {} received)", s->method, method);
                    return ff_http_averror(400, AVERROR(EIO));
                }
            }
            else {
                // use autodetected HTTP method to expect
                av_log(h, AV_LOG_TRACE, "Autodetected %s HTTP method\n", auto_method);
                if (av_strcasecmp(auto_method, method)) {
                    HJFLoge("error£¬ Received and autodetected HTTP method did not match "
                        "({} autodetected {} received)", auto_method, method);
                    return ff_http_averror(400, AVERROR(EIO));
                }
                if (!(s->method = av_strdup(method)))
                    return AVERROR(ENOMEM);
            }

            // HTTP resource
            while (av_isspace(*p))
                p++;
            resource = p;
            while (*p && !av_isspace(*p))
                p++;
            *(p++) = '\0';
            HJFLogt("Requested resource: {}", resource);
            if (!(s->resource = av_strdup(resource)))
                return AVERROR(ENOMEM);

            // HTTP version
            while (av_isspace(*p))
                p++;
            version = p;
            while (*p && !av_isspace(*p))
                p++;
            *p = '\0';
            if (av_strncasecmp(version, "HTTP/", 5)) {
                HJFLoge("error, Malformed HTTP version string.");
                return ff_http_averror(400, AVERROR(EIO));
            }
            HJFLogt("HTTP version string: {}", version);
        } else {
            if (av_strncasecmp(p, "HTTP/1.0", 8) == 0)
                s->willclose = 1;
            while (*p != '/' && *p != '\0')
                p++;
            while (*p == '/')
                p++;
            av_freep(&s->http_version);
            s->http_version = av_strndup(p, 3);
            while (!av_isspace(*p) && *p != '\0')
                p++;
            while (av_isspace(*p))
                p++;
            s->http_code = (int)strtol(p, &end, 10);

            HJFLogt("http_code={}", s->http_code);

            if ((ret = check_http_code(h, s->http_code, end)) < 0)
                return ret;
        }
    } else {
        while (*p != '\0' && *p != ':')
            p++;
        if (*p != ':')
            return 1;

        *p  = '\0';
        tag = line;
        p++;
        while (av_isspace(*p))
            p++;
        if (!av_strcasecmp(tag, "Location")) {
            if ((ret = parse_location(s, p)) < 0)
                return ret;
        } else if (!av_strcasecmp(tag, "Content-Length") &&
                   s->filesize == UINT64_MAX) {
            s->filesize = strtoull(p, NULL, 10);
        } else if (!av_strcasecmp(tag, "Content-Range")) {
            parse_content_range(h, p);
        } else if (!av_strcasecmp(tag, "Accept-Ranges") &&
                   !strncmp(p, "bytes", 5) &&
                   s->seekable == -1) {
            h->is_streamed = 0;
        } else if (!av_strcasecmp(tag, "Transfer-Encoding") &&
                   !av_strncasecmp(p, "chunked", 7)) {
            s->filesize  = UINT64_MAX;
            s->chunksize = 0;
        } else if (!av_strcasecmp(tag, "WWW-Authenticate")) {
            ff_http_auth_handle_header(&s->auth_state, tag, p);
        } else if (!av_strcasecmp(tag, "Authentication-Info")) {
            ff_http_auth_handle_header(&s->auth_state, tag, p);
        } else if (!av_strcasecmp(tag, "Proxy-Authenticate")) {
            ff_http_auth_handle_header(&s->proxy_auth_state, tag, p);
        } else if (!av_strcasecmp(tag, "Connection")) {
            if (!strcmp(p, "close"))
                s->willclose = 1;
        } else if (!av_strcasecmp(tag, "Server")) {
            if (!av_strcasecmp(p, "AkamaiGHost")) {
                s->is_akamai = 1;
            } else if (!av_strncasecmp(p, "MediaGateway", 12)) {
                s->is_mediagateway = 1;
            }
        } else if (!av_strcasecmp(tag, "Content-Type")) {
            av_free(s->mime_type);
            s->mime_type = av_get_token((const char **)&p, ";");
        } else if (!av_strcasecmp(tag, "Set-Cookie")) {
            if (parse_cookie(p, &s->cookie_dict))
                HJFLogw("warning, Unable to parse {}", p);
        } else if (!av_strcasecmp(tag, "Icy-MetaInt")) {
            s->icy_metaint = strtoull(p, NULL, 10);
        } else if (!av_strncasecmp(tag, "Icy-", 4)) {
            if ((ret = parse_icy(s, tag, p)) < 0)
                return ret;
        } else if (!av_strcasecmp(tag, "Content-Encoding")) {
            if ((ret = parse_content_encoding(h, p)) < 0)
                return ret;
        } else if (!av_strcasecmp(tag, "Expires")) {
            parse_expires(s, p);
        } else if (!av_strcasecmp(tag, "Cache-Control")) {
            parse_cache_control(s, p);
        }
    }
    return 1;
}

/**
 * Create a string containing cookie values for use as a HTTP cookie header
 * field value for a particular path and domain from the cookie values stored in
 * the HTTP protocol context. The cookie string is stored in *cookies, and may
 * be NULL if there are no valid cookies.
 *
 * @return a negative value if an error condition occurred, 0 otherwise
 */
int HJHLink::get_cookies(char** cookies, const char* path, const char* domain)
{
    JPHTTPShared* s = m_hctx;
    // cookie strings will look like Set-Cookie header field values.  Multiple
    // Set-Cookie fields will result in multiple values delimited by a newline
    int ret = 0;
    char *cookie, *set_cookies, *next;
    char *saveptr = NULL;

    // destroy any cookies in the dictionary.
    av_dict_free(&s->cookie_dict);

    if (!s->cookies)
        return 0;

    next = set_cookies = av_strdup(s->cookies);
    if (!next)
        return AVERROR(ENOMEM);

    *cookies = NULL;
    while ((cookie = av_strtok(next, "\n", &saveptr)) && !ret) {
        AVDictionary *cookie_params = NULL;
        AVDictionaryEntry *cookie_entry, *e;

        next = NULL;
        // store the cookie in a dict in case it is updated in the response
        if (parse_cookie(cookie, &s->cookie_dict))
            HJFLogw("warning, Unable to parse {}", cookie);

        // continue on to the next cookie if this one cannot be parsed
        if (parse_set_cookie(cookie, &cookie_params))
            goto skip_cookie;

        // if the cookie has no value, skip it
        cookie_entry = av_dict_get(cookie_params, "", NULL, AV_DICT_IGNORE_SUFFIX);
        if (!cookie_entry || !cookie_entry->value)
            goto skip_cookie;

        // if the cookie has expired, don't add it
        if ((e = av_dict_get(cookie_params, "expires", NULL, 0)) && e->value) {
            struct tm tm_buf = {0};
            if (!parse_set_cookie_expiry_time(e->value, &tm_buf)) {
                if (av_timegm(&tm_buf) < av_gettime() / 1000000)
                    goto skip_cookie;
            }
        }

        // if no domain in the cookie assume it appied to this request
        if ((e = av_dict_get(cookie_params, "domain", NULL, 0)) && e->value) {
            // find the offset comparison is on the min domain (b.com, not a.b.com)
            int domain_offset = (int)(strlen(domain) - strlen(e->value));
            if (domain_offset < 0)
                goto skip_cookie;

            // match the cookie domain
            if (av_strcasecmp(&domain[domain_offset], e->value))
                goto skip_cookie;
        }

        // if a cookie path is provided, ensure the request path is within that path
        e = av_dict_get(cookie_params, "path", NULL, 0);
        if (e && av_strncasecmp(path, e->value, strlen(e->value)))
            goto skip_cookie;

        // cookie parameters match, so copy the value
        if (!*cookies) {
            *cookies = av_asprintf("%s=%s", cookie_entry->key, cookie_entry->value);
        } else {
            char *tmp = *cookies;
            *cookies = av_asprintf("%s; %s=%s", tmp, cookie_entry->key, cookie_entry->value);
            av_free(tmp);
        }
        if (!*cookies)
            ret = AVERROR(ENOMEM);

    skip_cookie:
        av_dict_free(&cookie_params);
    }

    av_free(set_cookies);

    return ret;
}

int HJHLink::has_header(const char* str, const char* header)
{
    /* header + 2 to skip over CRLF prefix. (make sure you have one!) */
    if (!str)
        return 0;
    return av_stristart(str, header + 2, NULL) || av_stristr(str, header);
}

int HJHLink::http_read_header(URLContext* h)
{
    JPHTTPShared* s = m_hctx;
    char line[MAX_URL_SIZE];
    int err = 0;

    av_freep(&s->new_location);
    s->expires = 0;
    s->chunksize = UINT64_MAX;
    s->filesize_from_content_range = UINT64_MAX;

    for (;;) {
        if ((err = http_get_line(s, line, sizeof(line))) < 0)
            return err;

        HJFLogi("header={}", line);

        err = process_line(h, line, s->line_count);
        if (err < 0)
            return err;
        if (err == 0)
            break;
        s->line_count++;
    }

    // filesize from Content-Range can always be used, even if using chunked Transfer-Encoding
    if (s->filesize_from_content_range != UINT64_MAX)
        s->filesize = s->filesize_from_content_range;

    if (s->seekable == -1 && s->is_mediagateway && s->filesize == 2000000000)
        h->is_streamed = 1; /* we can in fact _not_ seek */

    // add any new cookies into the existing cookie string
    cookie_string(s->cookie_dict, &s->cookies);
    av_dict_free(&s->cookie_dict);

    return err;
}

/**
 * Escape unsafe characters in path in order to pass them safely to the HTTP
 * request. Insipred by the algorithm in GNU wget:
 * - escape "%" characters not followed by two hex digits
 * - escape all "unsafe" characters except which are also "reserved"
 * - pass through everything else
 */
void HJHLink::bprint_escaped_path(AVBPrint *bp, const char *path)
{
#define NEEDS_ESCAPE(ch) \
    ((ch) <= ' ' || (ch) >= '\x7f' || \
     (ch) == '"' || (ch) == '%' || (ch) == '<' || (ch) == '>' || (ch) == '\\' || \
     (ch) == '^' || (ch) == '`' || (ch) == '{' || (ch) == '}' || (ch) == '|')
    while (*path) {
        char buf[1024];
        char *q = buf;
        while (*path && q - buf < sizeof(buf) - 4) {
            if (path[0] == '%' && av_isxdigit(path[1]) && av_isxdigit(path[2])) {
                *q++ = *path++;
                *q++ = *path++;
                *q++ = *path++;
            } else if (NEEDS_ESCAPE(*path)) {
                q += snprintf(q, 4, "%%%02X", (uint8_t)*path++);
            } else {
                *q++ = *path++;
            }
        }
        av_bprint_append_data(bp, buf, q - buf);
    }
}

int HJHLink::http_connect(URLContext* h, const char* path, const char* local_path,
                        const char *hoststr, const char *auth,
                        const char *proxyauth)
{
    JPHTTPShared* s = m_hctx;
    int post, err;
    AVBPrint request;
    char *authstr = NULL, *proxyauthstr = NULL;
    uint64_t off = s->off;
    const char *method;
    int send_expect_100 = 0;

    av_bprint_init_for_buffer(&request, (char*)s->buffer, sizeof(s->buffer));

    /* send http header */
    post = h->flags & AVIO_FLAG_WRITE;

    if (s->post_data) {
        /* force POST method and disable chunked encoding when
         * custom HTTP post data is set */
        post            = 1;
        s->chunked_post = 0;
    }

    if (s->method)
        method = s->method;
    else
        method = post ? "POST" : "GET";

    authstr      = ff_http_auth_create_response(&s->auth_state, auth,
                                                local_path, method);
    proxyauthstr = ff_http_auth_create_response(&s->proxy_auth_state, proxyauth,
                                                local_path, method);

     if (post && !s->post_data) {
        if (s->send_expect_100 != -1) {
            send_expect_100 = s->send_expect_100;
        } else {
            send_expect_100 = 0;
            /* The user has supplied authentication but we don't know the auth type,
             * send Expect: 100-continue to get the 401 response including the
             * WWW-Authenticate header, or an 100 continue if no auth actually
             * is needed. */
            if (auth && *auth &&
                s->auth_state.auth_type == HTTP_AUTH_NONE &&
                s->http_code != 401)
                send_expect_100 = 1;
        }
    }

    av_bprintf(&request, "%s ", method);
    bprint_escaped_path(&request, path);
    av_bprintf(&request, " HTTP/1.1\r\n");

    if (post && s->chunked_post)
        av_bprintf(&request, "Transfer-Encoding: chunked\r\n");
    /* set default headers if needed */
    if (!has_header(s->headers, "\r\nUser-Agent: "))
        av_bprintf(&request, "User-Agent: %s\r\n", s->user_agent);
    if (s->referer) {
        /* set default headers if needed */
        if (!has_header(s->headers, "\r\nReferer: "))
            av_bprintf(&request, "Referer: %s\r\n", s->referer);
    }
    if (!has_header(s->headers, "\r\nAccept: "))
        av_bprintf(&request, "Accept: */*\r\n");
    // Note: we send the Range header on purpose, even when we're probing,
    // since it allows us to detect more reliably if a (non-conforming)
    // server supports seeking by analysing the reply headers.
    if (!has_header(s->headers, "\r\nRange: ") && !post && (s->off > 0 || s->end_off || s->seekable != 0)) {
        av_bprintf(&request, "Range: bytes=%" PRIu64"-", s->off);
        if (s->end_off)
            av_bprintf(&request, "%" PRId64, s->end_off - 1);
        av_bprintf(&request, "\r\n");
    }
    if (send_expect_100 && !has_header(s->headers, "\r\nExpect: "))
        av_bprintf(&request, "Expect: 100-continue\r\n");

    if (!has_header(s->headers, "\r\nConnection: "))
        av_bprintf(&request, "Connection: %s\r\n", s->multiple_requests ? "keep-alive" : "close");

    if (!has_header(s->headers, "\r\nHost: "))
        av_bprintf(&request, "Host: %s\r\n", hoststr);
    if (!has_header(s->headers, "\r\nContent-Length: ") && s->post_data)
        av_bprintf(&request, "Content-Length: %d\r\n", s->post_datalen);

    if (!has_header(s->headers, "\r\nContent-Type: ") && s->content_type)
        av_bprintf(&request, "Content-Type: %s\r\n", s->content_type);
    if (!has_header(s->headers, "\r\nCookie: ") && s->cookies) {
        char *cookies = NULL;
        if (!get_cookies(&cookies, path, hoststr) && cookies) {
            av_bprintf(&request, "Cookie: %s\r\n", cookies);
            av_free(cookies);
        }
    }
    if (!has_header(s->headers, "\r\nIcy-MetaData: ") && s->icy)
        av_bprintf(&request, "Icy-MetaData: 1\r\n");

    /* now add in custom headers */
    if (s->headers)
        av_bprintf(&request, "%s", s->headers);

    if (authstr)
        av_bprintf(&request, "%s", authstr);
    if (proxyauthstr)
        av_bprintf(&request, "Proxy-%s", proxyauthstr);
    av_bprintf(&request, "\r\n");

    av_log(h, AV_LOG_DEBUG, "request: %s\n", request.str);

    if (!av_bprint_is_complete(&request)) {
        av_log(h, AV_LOG_ERROR, "overlong headers\n");
        err = AVERROR(EINVAL);
        goto done;
    }
    HJFLogi("request: {}", request.str);
    if ((err = ffurl_write(s->hd, (const uint8_t*)request.str, request.len)) < 0)
        goto done;

    if (s->post_data)
        if ((err = ffurl_write(s->hd, s->post_data, s->post_datalen)) < 0)
            goto done;

    /* init input buffer */
    s->buf_ptr          = s->buffer;
    s->buf_end          = s->buffer;
    s->line_count       = 0;
    s->off              = 0;
    s->icy_data_read    = 0;
    s->filesize         = UINT64_MAX;
    s->willclose        = 0;
    s->end_chunked_post = 0;
    s->end_header       = 0;
#if CONFIG_ZLIB
    s->compressed       = 0;
#endif
    if (post && !s->post_data && !send_expect_100) {
        /* Pretend that it did work. We didn't read any header yet, since
         * we've still to send the POST data, but the code calling this
         * function will check http_code after we return. */
        s->http_code = 200;
        err = 0;
        goto done;
    }

    /* wait for header */
    err = http_read_header(h);
    if (err < 0)
        goto done;

    if (s->new_location)
        s->off = off;

    err = (off == s->off) ? 0 : -1;
done:
    av_freep(&authstr);
    av_freep(&proxyauthstr);
    return err;
}

int HJHLink::http_buf_read(URLContext* h, uint8_t* buf, int size)
{
    JPHTTPShared* s = m_hctx;
    int len;

    if (s->chunksize != UINT64_MAX) {
        if (s->chunkend) {
            return AVERROR_EOF;
        }
        if (!s->chunksize) {
            char line[32];
            int err;

            do {
                if ((err = http_get_line(s, line, sizeof(line))) < 0)
                    return err;
            } while (!*line);    /* skip CR LF from last chunk */

            s->chunksize = strtoull(line, NULL, 16);

            HJFLogt("Chunked encoding data size: {}", s->chunksize);

            if (!s->chunksize && s->multiple_requests) {
                http_get_line(s, line, sizeof(line)); // read empty chunk
                s->chunkend = 1;
                return 0;
            }
            else if (!s->chunksize) {
                HJFLogw("warning, Last chunk received, closing conn");
                ffurl_closep(&s->hd);
                return 0;
            }
            else if (s->chunksize == UINT64_MAX) {
                HJFLoge("error, Invalid chunk size:{}", s->chunksize);
                return AVERROR(EINVAL);
            }
        }
        size = FFMIN(size, s->chunksize);
    }

    /* read bytes from input buffer first */
    len = s->buf_end - s->buf_ptr;
    if (len > 0) {
        if (len > size)
            len = size;
        memcpy(buf, s->buf_ptr, len);
        s->buf_ptr += len;
    } else {
        uint64_t target_end = s->end_off ? s->end_off : s->filesize;
        if ((!s->willclose || s->chunksize == UINT64_MAX) && s->off >= target_end)
            return AVERROR_EOF;
        len = ffurl_read(s->hd, buf, size);
        if ((!len || len == AVERROR_EOF) &&
            (!s->willclose || s->chunksize == UINT64_MAX) && s->off < target_end) {
            HJFLoge("error, Stream ends prematurely at {}, should be{}", s->off, target_end);
            return AVERROR(EIO);
        }
    }
    if (len > 0) {
        s->off += len;
        if (s->chunksize > 0 && s->chunksize != UINT64_MAX) {
            av_assert0(s->chunksize >= len);
            s->chunksize -= len;
        }
    }
    return len;
}

#if CONFIG_ZLIB
#define DECOMPRESS_BUF_SIZE (256 * 1024)
int HJHLink::http_buf_read_compressed(URLContext* h, uint8_t* buf, int size)
{
    JPHTTPShared* s = m_hctx;
    int ret;

    if (!s->inflate_buffer) {
        s->inflate_buffer = (uint8_t*)av_malloc(DECOMPRESS_BUF_SIZE);
        if (!s->inflate_buffer)
            return AVERROR(ENOMEM);
    }

    if (s->inflate_stream.avail_in == 0) {
        int read = http_buf_read(h, s->inflate_buffer, DECOMPRESS_BUF_SIZE);
        if (read <= 0)
            return read;
        s->inflate_stream.next_in = s->inflate_buffer;
        s->inflate_stream.avail_in = read;
    }

    s->inflate_stream.avail_out = size;
    s->inflate_stream.next_out = buf;

    ret = inflate(&s->inflate_stream, Z_SYNC_FLUSH);
    if (ret != Z_OK && ret != Z_STREAM_END)
        av_log(h, AV_LOG_WARNING, "inflate return value: %d, %s\n",
            ret, s->inflate_stream.msg);

    return size - s->inflate_stream.avail_out;
}
#else
int HJHLink::http_buf_read_compressed(URLContext* h, uint8_t* buf, int size)
{
    return 0;
}
#endif /* CONFIG_ZLIB */


int HJHLink::http_read_stream(URLContext* h, uint8_t* buf, int size)
{
    JPHTTPShared* s = m_hctx;
    int err, new_location, read_ret;
    int64_t seek_ret;
    int reconnect_delay = 0;

    if (!s->hd)
        return AVERROR_EOF;

    if (s->end_chunked_post && !s->end_header) {
        err = http_read_header(h);
        if (err < 0)
            return err;
    }

#if CONFIG_ZLIB
    if (s->compressed)
        return http_buf_read_compressed(h, buf, size);
#endif /* CONFIG_ZLIB */
    read_ret = http_buf_read(h, buf, size);
    while (read_ret < 0) {
        uint64_t target = h->is_streamed ? 0 : s->off;

        if (read_ret == AVERROR_EXIT)
            break;

        if (h->is_streamed && !s->reconnect_streamed)
            break;

        if (!(s->reconnect && s->filesize > 0 && s->off < s->filesize) &&
            !(s->reconnect_at_eof && read_ret == AVERROR_EOF))
            break;

        if (reconnect_delay > s->reconnect_delay_max)
            return AVERROR(EIO);

        HJFLogw("warning, Will reconnect at {} in {} second(s), error={}.", s->off, reconnect_delay, read_ret);
        err = ff_network_sleep_interruptible(1000U * 1000 * reconnect_delay, &h->interrupt_callback);
        if (err != AVERROR(ETIMEDOUT))
            return err;
        reconnect_delay = 1 + 2*reconnect_delay;
        seek_ret = http_seek_internal(h, target, SEEK_SET, 1);
        if (seek_ret >= 0 && seek_ret != target) {
            HJFLoge("error, Failed to reconnect at {}.", target);
            return read_ret;
        }

        read_ret = http_buf_read(h, buf, size);
    }

    return read_ret;
}

// Like http_read_stream(), but no short reads.
// Assumes partial reads are an error.
int HJHLink::http_read_stream_all(URLContext* h, uint8_t* buf, int size)
{
    int pos = 0;
    while (pos < size) {
        int len = http_read_stream(h, buf + pos, size - pos);
        if (len < 0)
            return len;
        pos += len;
    }
    return pos;
}

void HJHLink::update_metadata(char* data)
{
    JPHTTPShared* s = m_hctx;
    char* key;
    char* val;
    char* end;
    char* next = data;

    while (*next) {
        key = next;
        val = strstr(key, "='");
        if (!val)
            break;
        end = strstr(val, "';");
        if (!end)
            break;

        *val = '\0';
        *end = '\0';
        val += 2;

        av_dict_set(&s->metadata, key, val, 0);
        HJFLogi("Metadata update for {}: {}", key, val);

        next = end + 2;
    }
}

int HJHLink::store_icy(URLContext* h, int size)
{
    JPHTTPShared* s = m_hctx;
    /* until next metadata packet */
    uint64_t remaining;

    if (s->icy_metaint < s->icy_data_read)
        return AVERROR_INVALIDDATA;
    remaining = s->icy_metaint - s->icy_data_read;

    if (!remaining) {
        /* The metadata packet is variable sized. It has a 1 byte header
         * which sets the length of the packet (divided by 16). If it's 0,
         * the metadata doesn't change. After the packet, icy_metaint bytes
         * of normal data follows. */
        uint8_t ch;
        int len = http_read_stream_all(h, &ch, 1);
        if (len < 0)
            return len;
        if (ch > 0) {
            char data[255 * 16 + 1];
            int ret;
            len = ch * 16;
            ret = http_read_stream_all(h, (uint8_t*)data, len);
            if (ret < 0)
                return ret;
            data[len + 1] = 0;
            if ((ret = av_opt_set(s, "icy_metadata_packet", data, 0)) < 0)
                return ret;
            update_metadata((char*)data);
        }
        s->icy_data_read = 0;
        remaining = s->icy_metaint;
    }

    return FFMIN(size, remaining);
}

int HJHLink::http_read(uint8_t* buf, int size)
{
    URLContext* h = m_uc;
    JPHTTPShared* s = m_hctx;

    //HJFLogi("entry id:{}, size:{}", getID(), size);
    if (!m_uc) {
        return -1;
    }
    do
    {
        if (m_preBuffer && m_preBuffer->size() > 0) {
            size = m_preBuffer->read(buf, size);
            HJFLogi("read date size:{} from cache", size);
            break;
        }
        if (s->icy_metaint > 0) {
            size = store_icy(h, size);
            if (size < 0)
                break;
        }

        size = http_read_stream(h, buf, size);
        if (size > 0)
            s->icy_data_read += size;
    } while (false);
    //HJFLogi("end, size:{}", size);

    return size;
}

int HJHLink::http_pre_read(int size)
{
    HJFLogi("entry id:{}, obj name:{}, size:{}", getID(), m_objname, size);
    if (!m_preBuffer) {
        m_preBuffer = std::make_shared<HJBuffer>(size);
    }
    do
    {
        int ret = http_read(m_preBuffer->data(), m_preBuffer->capacity());
        if (ret < 0) {
            HJFLoge("error, http read failed:{}", ret);
            return ret;
        }
        if (ret > 0) {
            m_preBuffer->addSize(ret);
        }
    } while (m_preBuffer->size() <= 0);
    HJFLogi("end id:{} obj name:{}, read link buffer size:{}", getID(), m_objname, m_preBuffer->size());

    return m_preBuffer->size();
}

int HJHLink::http_write(const uint8_t* buf, int size)
{
    char temp[11] = "";  /* 32-bit hex + CRLF + nul */
    int ret;
    char crlf[] = "\r\n";
    JPHTTPShared* s = m_hctx;

    //HJFLogi("entry id:{}, size:{}", getID(), size);
    if (!s->chunked_post) {
        /* non-chunked data is sent without any special encoding */
        return ffurl_write(s->hd, buf, size);
    }

    /* silently ignore zero-size data since chunk encoding that would
     * signal EOF */
    if (size > 0) {
        /* upload data using chunked encoding */
        snprintf(temp, sizeof(temp), "%x\r\n", size);

        if ((ret = ffurl_write(s->hd, (const unsigned char*)temp, (int)strlen(temp))) < 0 ||
            (ret = ffurl_write(s->hd, buf, size)) < 0 ||
            (ret = ffurl_write(s->hd, (const unsigned char*)crlf, sizeof(crlf) - 1)) < 0)
            return ret;
    }
    //HJFLogi("end id:{}, size:{}", getID(), size);

    return size;
}

int HJHLink::http_shutdown(int flags)
{
    int ret = 0;
    char footer[] = "0\r\n\r\n";
    JPHTTPShared* s = m_hctx;

    HJFLogi("entry, flags:{}", flags);
    /* signal end of chunked encoding if used */
    if (((flags & AVIO_FLAG_WRITE) && s->chunked_post) ||
        ((flags & AVIO_FLAG_READ) && s->chunked_post && s->listen)) {
        ret = ffurl_write(s->hd, (const unsigned char*)footer, sizeof(footer) - 1);
        ret = ret > 0 ? 0 : ret;
        /* flush the receive buffer when it is write only mode */
        if (!(flags & AVIO_FLAG_READ)) {
            char buf[1024];
            int read_ret;
            s->hd->flags |= AVIO_FLAG_NONBLOCK;
            read_ret = ffurl_read(s->hd, (unsigned char*)buf, sizeof(buf));
            s->hd->flags &= ~AVIO_FLAG_NONBLOCK;
            if (read_ret < 0 && read_ret != AVERROR(EAGAIN)) {
                HJFLoge("error, URL read error: {}", read_ret);
                ret = read_ret;
            }
        }
        s->end_chunked_post = 1;
    }

    return ret;
}


int HJHLink::http_close()
{
    int ret = 0;
    URLContext* h = m_uc;
    JPHTTPShared* s = m_hctx;

    //    HJFLogi("entry id:{}, obj name:{}", getID(), m_objname);
    m_isQuit = true;
#if CONFIG_ZLIB
    inflateEnd(&s->inflate_stream);
    av_freep(&s->inflate_buffer);
#endif /* CONFIG_ZLIB */

    if (s->hd && !s->end_chunked_post)
        /* Close the write direction by sending the end of chunked encoding. */
        ret = http_shutdown(h ? h->flags : 0);

    if (s->hd)
        ffurl_closep(&s->hd);
    av_dict_free(&s->chained_options);

    //
    freeParams(s);

    return ret;
}

int64_t HJHLink::http_seek_internal(URLContext* h, int64_t off, int whence, int force_reconnect)
{
    JPHTTPShared* s = m_hctx;
    URLContext* old_hd = s->hd;
    uint64_t old_off = s->off;
    uint8_t old_buf[BUFFER_SIZE];
    int old_buf_size, ret;
    AVDictionary* options = NULL;

    if (whence == AVSEEK_SIZE)
        return s->filesize;
    else if (!force_reconnect &&
        ((whence == SEEK_CUR && off == 0) ||
            (whence == SEEK_SET && off == s->off)))
        return s->off;
    else if ((s->filesize == UINT64_MAX && whence == SEEK_END))
        return AVERROR(ENOSYS);

    if (whence == SEEK_CUR)
        off += s->off;
    else if (whence == SEEK_END)
        off += s->filesize;
    else if (whence != SEEK_SET)
        return AVERROR(EINVAL);
    if (off < 0)
        return AVERROR(EINVAL);
    s->off = off;

    if (s->off && h->is_streamed)
        return AVERROR(ENOSYS);

    /* do not try to make a new connection if seeking past the end of the file */
    if (s->end_off || s->filesize != UINT64_MAX) {
        uint64_t end_pos = s->end_off ? s->end_off : s->filesize;
        if (s->off >= end_pos)
            return s->off;
    }

    /* if the location changed (redirect), revert to the original uri */
    if (strcmp(s->uri, s->location)) {
        char *new_uri;
        new_uri = av_strdup(s->uri);
        if (!new_uri)
            return AVERROR(ENOMEM);
        av_free(s->location);
        s->location = new_uri;
    }

    /* we save the old context in case the seek fails */
    old_buf_size = s->buf_end - s->buf_ptr;
    memcpy(old_buf, s->buf_ptr, old_buf_size);
    s->hd = NULL;

    /* if it fails, continue on old connection */
    if ((ret = http_open_cnx(h, &options)) < 0) {
        av_dict_free(&options);
        memcpy(s->buffer, old_buf, old_buf_size);
        s->buf_ptr = s->buffer;
        s->buf_end = s->buffer + old_buf_size;
        s->hd = old_hd;
        s->off = old_off;
        return ret;
    }
    av_dict_free(&options);
    ffurl_close(old_hd);
    return off;
}

int64_t HJHLink::http_seek(int64_t off, int whence)
{
    URLContext* h = m_uc;
    //    HJFLogi("entry, id:{}, obj name:{}, off:{}, whence:{}", getID(), m_objname, off, whence);
    return http_seek_internal(h, off, whence, 0);
}

int HJHLink::http_get_file_handle()
{
    JPHTTPShared* s = m_hctx;
    HJFLogi("entry");
    return ffurl_get_file_handle(s->hd);
}
int HJHLink::http_get_short_seek()
{
    JPHTTPShared* s = m_hctx;
    //HJFLogi("entry");
    if (s->short_seek_size >= 1)
        return s->short_seek_size;
    return ffurl_get_short_seek(s->hd);
}

/////////////////////////////////////////////////////////////////////////////
HJHLinkQueue::HJHLinkQueue()
{
    
}
HJHLinkQueue::~HJHLinkQueue()
{
    HJ_AUTO_LOCK(m_mutex);
    for (auto& it : m_links) {
        auto& link = it.second;
        link->http_close();
    }
    m_links.clear();
    //
    for (auto& it : m_usedLinks) {
        auto& link = it.second;
        link->http_close();
    }
    m_usedLinks.clear();
}

void HJHLinkQueue::addLink(const HJHLink::Ptr& link)
{
    HJ_AUTO_LOCK(m_mutex);
    m_links.emplace(link->getName(), link);
    HJFLogi("http link count:{}", m_links.size());
    return;
}
HJHLink::Ptr HJHLinkQueue::getLink(const std::string& name)
{
    HJ_AUTO_LOCK(m_mutex);
    HJHLink::Ptr link = nullptr;
    auto it = m_links.find(name);
    if (it != m_links.end()) {
        link = it->second;
    }
    return link;
}
HJHLink::Ptr HJHLinkQueue::removeLink(const std::string& name)
{
    HJ_AUTO_LOCK(m_mutex);
    HJHLink::Ptr link = nullptr;
    auto it = m_links.find(name);
    if (it != m_links.end()) {
        link = it->second;
        m_links.erase(it);
    }
//    if(link) {
//        m_usedLinks.emplace(link->getID(), link);
//    }
    return link;
}
void HJHLinkQueue::freeLink(size_t handle)
{
    HJ_AUTO_LOCK(m_mutex);
    for (auto it = m_links.begin(); it != m_links.end(); it++)
    {
        auto& link = it->second;
        if (handle == link->getID()) {
            HJFLogi("free name link url:{}, id:{}", link->getName(), link->getID());
            link->http_close();
            //
            m_links.erase(it);
            return;
        }
    }
}

size_t HJHLinkQueue::getLinkSize()
{
    HJ_AUTO_LOCK(m_mutex);
    return m_links.size();
}

//
void HJHLinkQueue::addUsedLink(const HJHLink::Ptr& link)
{
    HJ_AUTO_LOCK(m_mutex);
    m_usedLinks.emplace(link->getID(), link);
//    HJFLogi("http used link count:{}", m_usedLinks.size());
    return;
}

HJHLink::Ptr HJHLinkQueue::getUsedLink(size_t handle)
{
    HJ_AUTO_LOCK(m_mutex);
    HJHLink::Ptr link = nullptr;
    auto it = m_usedLinks.find(handle);
    if (it != m_usedLinks.end()) {
        link = it->second;
    }
    return link;
}

void HJHLinkQueue::freeUsedLink(size_t handle)
{
    HJ_AUTO_LOCK(m_mutex);
    HJHLink::Ptr link = nullptr;
    auto it = m_usedLinks.find(handle);
    if (it != m_usedLinks.end()) {
        link = it->second;
        HJFLogi("free name link url:{}, id:{}", link->getName(), link->getID());
        link->http_close();
        //
        m_usedLinks.erase(it);
    }
}

/////////////////////////////////////////////////////////////////////////////
HJ_INSTANCE(HJHLinkManager)
HJHLinkManager::HJHLinkManager()
{
}

HJHLinkManager::~HJHLinkManager()
{
    done();
}

int HJHLinkManager::init()
{
//    HJFLogi("entry");
    if (!HJGlobalSettingsManager::getUseHTTPPool()) {
//        HJFLoge("globle setting http pool : false");
        return HJErrNotAlready;
    }
    int res = HJ_OK;
    do
    {
        m_executor = HJExecutorManager::createExecutor(HJUtilitys::generateUUID());
        if (!m_executor) {
            res = HJErrFatal;
            break;
        }
        m_links = std::make_shared<HJHLinkQueue>();
        m_security = true;
        HJFLogi("hlink manager init end");
    } while (false);
    
    if (res != HJ_OK) {
        HJFLogw("error, init failed.");
        HJ_EXCEPT(HJException::ERR_INVALID_CALL, "init failed");
    }

    return res;
}

AVDictionary* HJHLinkManager::getDefaultOptions()
{
    AVDictionary* defaultOptions{ NULL };
    av_dict_set_int(&defaultOptions, "use_private_jitter", 1, 0);
    av_dict_set(&defaultOptions, "protocol_whitelist", "http,https,tls,rtp,tcp,udp,crypto,httpproxy", 0);
    //av_dict_set(&defaultOptions, "obj_name", "TLSLink", 0);
    int max_rtmp_reconnection_waittime = HJGlobalSettingsManager::getUrlConnectTimeout();
    av_dict_set_int(&defaultOptions, "timeout", max_rtmp_reconnection_waittime, 0);

    return defaultOptions;
}

AVDictionary* HJHLinkManager::getDefaultParams()
{
    AVDictionary* defaultParams{ NULL };

    av_dict_set(&defaultParams, "user_agent", DEFAULT_USER_AGENT, 0);
    av_dict_set(&defaultParams, "user_agent_deprecated", DEFAULT_USER_AGENT, 0);
    av_dict_set_int(&defaultParams, "seekable", -1, 0);
    av_dict_set_int(&defaultParams, "chunked_post", 1, 0);
    av_dict_set_int(&defaultParams, "icy", 1, 0);
    av_dict_set_int(&defaultParams, "reconnect_delay_max", 120, 0);
    av_dict_set_int(&defaultParams, "reply_code", 200, 0);

    return defaultParams;
}

std::string HJHLinkManager::replaceProtocol(const std::string& url) {
    if (url.compare(0, 12, "fasthttps://") == 0) {
        return "https://" + url.substr(12);
    }
    else if (url.compare(0, 11, "fasthttp://") == 0) {
        return "http://" + url.substr(11);
    }
    return url;
}

void HJHLinkManager::done()
{
//    HJFLogi("entry");
    terminate();
    m_links = nullptr;
}

void HJHLinkManager::terminate()
{
//    HJFLogi("entry");
    m_security = false;
    if(m_executor) {
        m_executor->stop();
//        m_executor = nullptr;
    }
//    HJFLogi("end");
}

float HJHLinkManager::getHitRate(HJHLink::Ptr& link, std::string objname, bool isHit)
{
    float hitRate = 0.0;
    {
        HJ_AUTO_LOCK(m_mutex);
        m_linkTotalCount++;
        if (isHit) {
            m_linkHitCount++;
        }
        hitRate = m_linkHitCount / (float)m_linkTotalCount;
        objname = objname.empty() ? link->getName() : objname;
    }
    HJTMSetProcInfo(objname, "http hit rate", hitRate);

    return hitRate;
}

size_t HJHLinkManager::getLink(URLContext* h, const std::string inUrl, int flags, AVDictionary** options, AVDictionary* params)
{
    if(!m_security) {
        return 0;
    }
    HJHLink::Ptr link = nullptr;
    int streamtype = 0;
    std::string objname = "";
    std::string url = replaceProtocol(inUrl);
    
    HJFLogi("entry, url:{}, flags:{}", url, flags);
    if (options)
    {
        AVDictionaryEntry* e = av_dict_get(*options, "streamtype", NULL, 0);
        if (e && e->value) {
            streamtype = atoi((const char*)e->value);
        }
        //HJFMT("HTTPLink_{}", getID());
        if (options)
        {
            AVDictionaryEntry* e = av_dict_get(*options, "obj_name", NULL, 0);
            if (e && e->value) {
                objname = std::string((const char*)e->value);
            }
        }
    }
    if (2 == streamtype)
    {
        link = m_links->removeLink(url);
        if (link) {
            remDefaultUrl(url);
            float hitRate = getHitRate(link, objname, true);
            HJFLogi("get http link pool success, id:{}, hit rate:{}, url:{}", link->getID(), hitRate, url);
            return (size_t)new HJObjectHolder<HJHLink::Ptr>(link);
        }
    }
    link = createLink(url, flags, options, params);
    if (!link) {
        return 0;
    }
//    m_links->addUsedLink(link);
//    addDefaultUrl(url);
    //
    if (2 == streamtype) {
        float hitRate = getHitRate(link, objname, false);
        HJFLogi("get http link pool null, create success, id:{}, hit rate:{}, streamtype:{}, url:{}", link->getID(), hitRate, streamtype, url);
    } else {
        HJFLogi("get http link create success, id:{}, streamtype:{}, url:{}", link->getID(), streamtype, url);
    }
    
    return (size_t)new HJObjectHolder<HJHLink::Ptr>(link);
}

//int HJHLinkManager::freeLink(size_t handle)
//{
//    m_links->freeUsedLink(handle);
//    m_links->freeLink(handle);
//
//    return HJ_OK;
//}

//HJHLink::Ptr HJHLinkManager::getUsedLinkByID(size_t handle)
//{
//    return m_links->getUsedLink(handle);
//}

HJHLink::Ptr HJHLinkManager::createLink(const std::string url, int flags, AVDictionary** options, AVDictionary* params)
{
    auto link = std::make_shared<HJHLink>(url);
    size_t gid = HJHLinkManager::getGlobalID();
    link->setID(gid);
    //
    int res = link->http_open(url.c_str(), flags, options, params);
    if (HJ_OK != res) {
        link->http_close();
        HJFLoge("error, tls open failed, url:{}", url);
        return nullptr;
    }
    //HJFLogi("create http link name:{}, id:{}", name, gid);

    return link;
}

void HJHLinkManager::asyncCreateLink(const std::string& url)
{
    if (!m_security || !m_executor) {
        return;
    }
    HJFLogi("async create link entry, url:{}", url);
    HJHLinkManager::Wtr wtr = sharedFrom(this);
    m_executor->async([wtr, url]() {
        HJHLinkManager::Ptr mgr = wtr.lock();
        if (!mgr) {
            return;
        }
        AVDictionary* defaultOptions = mgr->getDefaultOptions();
        AVDictionary* defaultParams = mgr->getDefaultParams();
        auto link = mgr->createLink(url, mgr->m_defaultFlags, &defaultOptions, defaultParams);
        av_freep(&defaultOptions);
        av_freep(&defaultParams);
        if (!link) {
            HJFLoge("error, async create link failed, url:{}", url);
            return;
        }
        int res = link->http_pre_read(mgr->m_defaultReadSize);
        if (res < 0) {
            HJFLoge("error, async create http pre read failed");
            return;
        }

        if (mgr->m_links->getLinkSize() < mgr->m_urlLinkMax) {
            mgr->m_links->addLink(link);
            HJFLogi("async create link ok, links size:{}, url:{}", mgr->m_links->getLinkSize(), url);
            //
            mgr->asyncCheckLink(link->getID(), HJ_SEC_TO_MS(mgr->m_urlLinkAliveTime), url);
        } else {
            link->http_close();
            HJFLogi("async create link ok, close url:()", url);
        }
    });
}

//void HJHLinkManager::checkLink(size_t handle)
//{
//    freeLink(handle);
//}

void HJHLinkManager::asyncCheckLink(size_t handle, int64_t delayTime, const std::string url)
{
    if (!m_security || !m_executor) {
        return;
    }
    HJHLinkManager::Wtr wtr = sharedFrom(this);
    m_executor->async([wtr, url, handle]() {
        HJHLinkManager::Ptr mgr = wtr.lock();
        if (!mgr) {
            return;
        }
        mgr->m_links->freeLink(handle);
        mgr->remDefaultUrl(url);
    }, delayTime, false);
}


bool HJHLinkManager::addDefaultUrl(const std::string& url)
{
    HJ_AUTO_LOCK(m_tryMutex);
    auto it = m_tryUrls.find(url);
    if (it != m_tryUrls.end()) {
        HJFLogi("aready exist url:{}", url);
        return false;
    }
    m_tryUrls.emplace(url, url);
    
    HJFLogi("set try url:{}", url);
    return true;
}

void HJHLinkManager::remDefaultUrl(const std::string& url)
{
    HJ_AUTO_LOCK(m_tryMutex);
    auto it = m_tryUrls.find(url);
    if (it != m_tryUrls.end()) {
        m_tryUrls.erase(it);
        HJFLogi("remove try url:{}", url);
    }
    return;
}

void HJHLinkManager::setDefaultUrl(const std::string& url)
{
    if (!m_security) {
        return;
    }
    bool res = addDefaultUrl(url);
    if(res) {
        asyncCreateLink(url);
    }
    return;
}

const size_t HJHLinkManager::getGlobalID()
{
    static std::atomic<size_t> s_globalHLinkIDCounter(0);
    return s_globalHLinkIDCounter.fetch_add(1, std::memory_order_relaxed);
}

NS_HJ_END

/////////////////////////////////////////////////////////////////////////////
size_t hjhlink_create(URLContext* h, const char* url, int flags, AVDictionary** options, AVDictionary* params)
{
    auto& manager = HJHLinkManagerInstance();
    if (!manager) {
        return HJ_INVALID_HAND;
    }
    std::string murl{ url };
    return manager->getLink(h, murl, flags, options, params);
}

int hjhlink_destroy(size_t handle)
{
    if(!handle) {
        return 0;
    }
    HJ::HJObjectHolder<HJ::HJHLink::Ptr>* linkHolder = (HJ::HJObjectHolder<HJ::HJHLink::Ptr> *)handle;
    HJ::HJHLink::Ptr link = linkHolder->get();
    if(link) {
        link->http_close();
    }
    delete linkHolder;
    
//    auto& manager = HJHLinkManagerInstance();
//    if (!manager) {
//        return HJ_INVALID_HAND;
//    }
//    return manager->freeLink(handle);
    return 0;
}
//
void hjhlink_done()
{
    auto& manager = HJHLinkManagerInstance();
    if (!manager) {
        return;
    }
    manager->done();
    manager->destoryInstance();
    return;
}

//HJ::HJHLink::Ptr hjhlink_get_link(size_t handle)
//{
//    auto& manager = HJHLinkManagerInstance();
//    if (!manager) {
//        return nullptr;
//    }
//    auto link = manager->getUsedLinkByID(handle);
//    if (!link) {
//        HJFLogi("error, hlink get link is null, id:{}", handle);
//        return nullptr;
//    }
//    return link;
//}

HJ::HJHLink::Ptr hjhlink_get_link(size_t handle)
{
    if(!handle) {
        return nullptr;
    }
    HJ::HJObjectHolder<HJ::HJHLink::Ptr>* linkHolder = (HJ::HJObjectHolder<HJ::HJHLink::Ptr> *)handle;
    return linkHolder->get();
}

int hjhlink_accept(size_t handle, URLContext** c)
{
    auto link = hjhlink_get_link(handle);
    if (!link) {
        return HJErrNotAlready;
    }
    return link->http_accept(c);
}
int hjhlink_handshake(size_t handle)
{
    auto link = hjhlink_get_link(handle);
    if (!link) {
        return HJErrNotAlready;
    }
    return link->http_handshake();
}
int hjhlink_read(size_t handle, uint8_t* buf, int size)
{
    auto link = hjhlink_get_link(handle);
    if (!link) {
        return HJErrNotAlready;
    }
    return link->http_read(buf, size);
}
int hjhlink_write(size_t handle, const uint8_t* buf, int size)
{
    auto link = hjhlink_get_link(handle);
    if (!link) {
        return HJErrNotAlready;
    }
    return link->http_write(buf, size);
}
int64_t hjhlink_seek(size_t handle, int64_t off, int whence)
{
    auto link = hjhlink_get_link(handle);
    if (!link) {
        return HJErrNotAlready;
    }
    return link->http_seek(off, whence);
}
int hjhlink_get_file_handle(size_t handle)
{
    auto link = hjhlink_get_link(handle);
    if (!link) {
        return HJErrNotAlready;
    }
    return link->http_get_file_handle();
}
int hjhlink_get_short_seek(size_t handle)
{
    auto link = hjhlink_get_link(handle);
    if (!link) {
        return HJErrNotAlready;
    }
    return link->http_get_short_seek();
}
int hjhlink_shutdown(size_t handle, int flags)
{
    auto link = hjhlink_get_link(handle);
    if (!link) {
        return HJErrNotAlready;
    }
    return link->http_shutdown(flags);
}
