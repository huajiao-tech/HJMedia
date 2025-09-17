/*
 * HTTP protocol for ffmpeg client
 * Copyright (c) 2000, 2001 Fabrice Bellard
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
#include "HJLogPrintf.h"
#include "config.h"
#include "config_components.h"

#if CONFIG_ZLIB
#include <zlib.h>
#endif /* CONFIG_ZLIB */

#include "libavutil/avassert.h"
#include "libavutil/avstring.h"
#include "libavutil/opt.h"
#include "libavutil/time.h"
#include "libavutil/parseutils.h"
#include "libavutil/internal.h"

#include "libavformat/avformat.h"
#include "libavformat/http.h"
#include "libavformat/httpauth.h"
#include "libavformat/internal.h"
#include "libavformat/network.h"
#include "libavformat/os_support.h"
#include "libavformat/url.h"

//#include "JPTLSLinkManagerInterface.h"
#include "HJHLinksManagerInterface.h"

typedef struct JPHTTPContext {
    const AVClass *class;
//    URLContext *hd;
//    //unsigned char buffer[BUFFER_SIZE], *buf_ptr, *buf_end;
//    int line_count;
//    int http_code;
//    /* Used if "Transfer-Encoding: chunked" otherwise -1. */
//    uint64_t chunksize;
//    int chunkend;
    uint64_t off, end_off;  // , filesize;
    char* uri;
    char *location;
    HTTPAuthState auth_state;
    //HTTPAuthState proxy_auth_state;
    char *http_proxy;
    char *headers;
    char *mime_type;
    char *http_version;
    char *user_agent;
    char *referer;
#if FF_API_HTTP_USER_AGENT
    char *user_agent_deprecated;
#endif
    char *content_type;
//    /* Set if the server correctly handles Connection: close and will close
//     * the connection after feeding us the content. */
//    int willclose;
    int seekable;           /**< Control seekability, 0 = disable, 1 = enable, -1 = probe. */
    int chunked_post;
//    /* A flag which indicates if the end of chunked encoding has been sent. */
//    int end_chunked_post;
//    /* A flag which indicates we have finished to read POST reply. */
//    int end_header;
//    /* A flag which indicates if we use persistent connections. */
    int multiple_requests;
    uint8_t *post_data;
    //int post_datalen;
//    int is_akamai;
//    int is_mediagateway;
    char *cookies;          ///< holds newline (\n) delimited Set-Cookie header field values (without the "Set-Cookie: " field name)
//    /* A dictionary containing cookies keyed by cookie name */
//    AVDictionary *cookie_dict;
    int icy;
//    /* how much data was read since the last ICY metadata packet */
//    uint64_t icy_data_read;
//    /* after how many bytes of read data a new metadata packet will be found */
//    uint64_t icy_metaint;
    char *icy_metadata_headers;
    char *icy_metadata_packet;
    AVDictionary *metadata;
//#if CONFIG_ZLIB
//    int compressed;
//    z_stream inflate_stream;
//    uint8_t *inflate_buffer;
//#endif /* CONFIG_ZLIB */
//    AVDictionary *chained_options;
    int send_expect_100;
    char *method;
    int reconnect;
    int reconnect_at_eof;
    int reconnect_on_network_error;
    int reconnect_streamed;
    int reconnect_delay_max;
    char* reconnect_on_http_error;
    int listen;
    char *resource;
    int reply_code;
//    int is_multi_client;
//    JPHandshakeState handshake_step;
//    int is_connected_server;
    int short_seek_size;
    //int64_t expires;
    //char* new_location;
    //AVDictionary* redirect_cache;
    //uint64_t filesize_from_content_range;
    //
    size_t handle;
} JPHTTPContext;

#define OFFSET(x) offsetof(JPHTTPContext, x)
#define D AV_OPT_FLAG_DECODING_PARAM
#define E AV_OPT_FLAG_ENCODING_PARAM
#define DEFAULT_USER_AGENT "Lavf/" AV_STRINGIFY(LIBAVFORMAT_VERSION)

static const AVOption options[] = {
    { "seekable", "control seekability of connection", OFFSET(seekable), AV_OPT_TYPE_BOOL, { .i64 = -1 }, -1, 1, D },
    { "chunked_post", "use chunked transfer-encoding for posts", OFFSET(chunked_post), AV_OPT_TYPE_BOOL, { .i64 = 1 }, 0, 1, E },
    { "http_proxy", "set HTTP proxy to tunnel through", OFFSET(http_proxy), AV_OPT_TYPE_STRING, { .str = NULL }, 0, 0, D | E },
    { "headers", "set custom HTTP headers, can override built in default headers", OFFSET(headers), AV_OPT_TYPE_STRING, { .str = NULL }, 0, 0, D | E },
    { "content_type", "set a specific content type for the POST messages", OFFSET(content_type), AV_OPT_TYPE_STRING, { .str = NULL }, 0, 0, D | E },
    { "user_agent", "override User-Agent header", OFFSET(user_agent), AV_OPT_TYPE_STRING, { .str = DEFAULT_USER_AGENT }, 0, 0, D },
    { "referer", "override referer header", OFFSET(referer), AV_OPT_TYPE_STRING, { .str = NULL }, 0, 0, D },
    { "multiple_requests", "use persistent connections", OFFSET(multiple_requests), AV_OPT_TYPE_BOOL, { .i64 = 0 }, 0, 1, D | E },
    { "post_data", "set custom HTTP post data", OFFSET(post_data), AV_OPT_TYPE_BINARY, .flags = D | E },
    { "mime_type", "export the MIME type", OFFSET(mime_type), AV_OPT_TYPE_STRING, { .str = NULL }, 0, 0, AV_OPT_FLAG_EXPORT | AV_OPT_FLAG_READONLY },
    { "http_version", "export the http response version", OFFSET(http_version), AV_OPT_TYPE_STRING, { .str = NULL }, 0, 0, AV_OPT_FLAG_EXPORT | AV_OPT_FLAG_READONLY },
    { "cookies", "set cookies to be sent in applicable future requests, use newline delimited Set-Cookie HTTP field value syntax", OFFSET(cookies), AV_OPT_TYPE_STRING, { .str = NULL }, 0, 0, D },
    { "icy", "request ICY metadata", OFFSET(icy), AV_OPT_TYPE_BOOL, { .i64 = 1 }, 0, 1, D },
    { "icy_metadata_headers", "return ICY metadata headers", OFFSET(icy_metadata_headers), AV_OPT_TYPE_STRING, { .str = NULL }, 0, 0, AV_OPT_FLAG_EXPORT },
    { "icy_metadata_packet", "return current ICY metadata packet", OFFSET(icy_metadata_packet), AV_OPT_TYPE_STRING, { .str = NULL }, 0, 0, AV_OPT_FLAG_EXPORT },
    { "metadata", "metadata read from the bitstream", OFFSET(metadata), AV_OPT_TYPE_DICT, {0}, 0, 0, AV_OPT_FLAG_EXPORT },
    { "auth_type", "HTTP authentication type", OFFSET(auth_state.auth_type), AV_OPT_TYPE_INT, { .i64 = HTTP_AUTH_NONE }, HTTP_AUTH_NONE, HTTP_AUTH_BASIC, D | E, .unit = "auth_type"},
    { "none", "No auth method set, autodetect", 0, AV_OPT_TYPE_CONST, { .i64 = HTTP_AUTH_NONE }, 0, 0, D | E, .unit = "auth_type"},
    { "basic", "HTTP basic authentication", 0, AV_OPT_TYPE_CONST, { .i64 = HTTP_AUTH_BASIC }, 0, 0, D | E, .unit = "auth_type"},
    { "send_expect_100", "Force sending an Expect: 100-continue header for POST", OFFSET(send_expect_100), AV_OPT_TYPE_BOOL, { .i64 = -1 }, -1, 1, E },
    { "location", "The actual location of the data received", OFFSET(location), AV_OPT_TYPE_STRING, { .str = NULL }, 0, 0, D | E },
    { "offset", "initial byte offset", OFFSET(off), AV_OPT_TYPE_INT64, { .i64 = 0 }, 0, INT64_MAX, D },
    { "end_offset", "try to limit the request to bytes preceding this offset", OFFSET(end_off), AV_OPT_TYPE_INT64, { .i64 = 0 }, 0, INT64_MAX, D },
    { "method", "Override the HTTP method or set the expected HTTP method from a client", OFFSET(method), AV_OPT_TYPE_STRING, { .str = NULL }, 0, 0, D | E },
    { "reconnect", "auto reconnect after disconnect before EOF", OFFSET(reconnect), AV_OPT_TYPE_BOOL, { .i64 = 0 }, 0, 1, D },
    { "reconnect_at_eof", "auto reconnect at EOF", OFFSET(reconnect_at_eof), AV_OPT_TYPE_BOOL, { .i64 = 0 }, 0, 1, D },
    { "reconnect_on_network_error", "auto reconnect in case of tcp/tls error during connect", OFFSET(reconnect_on_network_error), AV_OPT_TYPE_BOOL, { .i64 = 0 }, 0, 1, D },
    { "reconnect_on_http_error", "list of http status codes to reconnect on", OFFSET(reconnect_on_http_error), AV_OPT_TYPE_STRING, { .str = NULL }, 0, 0, D },
    { "reconnect_streamed", "auto reconnect streamed / non seekable streams", OFFSET(reconnect_streamed), AV_OPT_TYPE_BOOL, { .i64 = 0 }, 0, 1, D },
    { "reconnect_delay_max", "max reconnect delay in seconds after which to give up", OFFSET(reconnect_delay_max), AV_OPT_TYPE_INT, { .i64 = 120 }, 0, UINT_MAX/1000/1000, D },
    { "listen", "listen on HTTP", OFFSET(listen), AV_OPT_TYPE_INT, { .i64 = 0 }, 0, 2, D | E },
    { "resource", "The resource requested by a client", OFFSET(resource), AV_OPT_TYPE_STRING, { .str = NULL }, 0, 0, E },
    { "reply_code", "The http status code to return to a client", OFFSET(reply_code), AV_OPT_TYPE_INT, { .i64 = 200}, INT_MIN, 599, E},
    { "short_seek_size", "Threshold to favor readahead over seek.", OFFSET(short_seek_size), AV_OPT_TYPE_INT, { .i64 = 0 }, 0, INT_MAX, D },
    { NULL }
};

AVDictionary* getParmasOptions(JPHTTPContext* s)
{
    AVDictionary* opts = NULL;
    {
        char buffer[128];
        snprintf(buffer, sizeof(buffer), "%"PRIu64, s->off);
        av_dict_set(&opts, "offset", buffer, AV_DICT_MATCH_CASE);
    }
    {
        char buffer[128];
        snprintf(buffer, sizeof(buffer), "%"PRIu64, s->end_off);
        av_dict_set(&opts, "end_off", buffer, AV_DICT_MATCH_CASE);
    }
    if (s->uri) {
        av_dict_set(&opts, "uri", s->uri, AV_DICT_MATCH_CASE);
    }
    if (s->location) {
        av_dict_set(&opts, "location", s->location, AV_DICT_MATCH_CASE);
    }
    av_dict_set_int(&opts, "auth_type", s->auth_state.auth_type, AV_DICT_MATCH_CASE);
    if (s->http_proxy) {
        av_dict_set(&opts, "http_proxy", s->http_proxy, AV_DICT_MATCH_CASE);
    }
    if (s->headers) {
        av_dict_set(&opts, "headers", s->headers, AV_DICT_MATCH_CASE);
    }
    if (s->mime_type) {
        av_dict_set(&opts, "mime_type", s->mime_type, AV_DICT_MATCH_CASE);
    }
    if (s->http_version) {
        av_dict_set(&opts, "http_version", s->http_version, AV_DICT_MATCH_CASE);
    }
    if (s->user_agent) {
        av_dict_set(&opts, "user_agent", s->user_agent, AV_DICT_MATCH_CASE);
    }
    if (s->referer) {
        av_dict_set(&opts, "referer", s->referer, AV_DICT_MATCH_CASE);
    }
#if FF_API_HTTP_USER_AGENT
    if (s->user_agent_deprecated) {
        av_dict_set(&opts, "user_agent_deprecated", s->user_agent_deprecated, AV_DICT_MATCH_CASE);
    }
#endif
    if (s->content_type) {
        av_dict_set(&opts, "content_type", s->content_type, AV_DICT_MATCH_CASE);
    }

    av_dict_set_int(&opts, "seekable", s->seekable, AV_DICT_MATCH_CASE);
    av_dict_set_int(&opts, "chunked_post", s->chunked_post, AV_DICT_MATCH_CASE);
    av_dict_set_int(&opts, "multiple_requests", s->multiple_requests, AV_DICT_MATCH_CASE);
    
    if (s->post_data) {
        av_dict_set(&opts, "post_data", s->post_data, AV_DICT_MATCH_CASE);
    }
    if (s->cookies) {
        av_dict_set(&opts, "cookies", s->cookies, AV_DICT_MATCH_CASE);
    }
    av_dict_set_int(&opts, "icy", s->icy, AV_DICT_MATCH_CASE);
    if (s->icy_metadata_headers) {
        av_dict_set(&opts, "icy_metadata_headers", s->icy_metadata_headers, AV_DICT_MATCH_CASE);
    }
    if (s->icy_metadata_packet) {
        av_dict_set(&opts, "icy_metadata_packet", s->icy_metadata_packet, AV_DICT_MATCH_CASE);
    }
    //metadata
    av_dict_set_int(&opts, "send_expect_100", s->send_expect_100, AV_DICT_MATCH_CASE);
    if (s->method) {
        av_dict_set(&opts, "method", s->method, AV_DICT_MATCH_CASE);
    }
    av_dict_set_int(&opts, "reconnect", s->reconnect, AV_DICT_MATCH_CASE);
    av_dict_set_int(&opts, "reconnect_at_eof", s->reconnect_at_eof, AV_DICT_MATCH_CASE);
    av_dict_set_int(&opts, "reconnect_on_network_error", s->reconnect_on_network_error, AV_DICT_MATCH_CASE);
    av_dict_set_int(&opts, "reconnect_streamed", s->reconnect_streamed, AV_DICT_MATCH_CASE);
    av_dict_set_int(&opts, "reconnect_delay_max", s->reconnect_delay_max, AV_DICT_MATCH_CASE);
    if (s->reconnect_on_http_error) {
        av_dict_set(&opts, "reconnect_on_http_error", s->reconnect_on_http_error, AV_DICT_MATCH_CASE);
    }
    av_dict_set_int(&opts, "listen", s->listen, AV_DICT_MATCH_CASE);
    if (s->resource) {
        av_dict_set(&opts, "resource", s->resource, AV_DICT_MATCH_CASE);
    }
    av_dict_set_int(&opts, "reply_code", s->reply_code, AV_DICT_MATCH_CASE);
    av_dict_set_int(&opts, "short_seek_size", s->short_seek_size, AV_DICT_MATCH_CASE);

    char* new_location;
    AVDictionary* redirect_cache;
    uint64_t filesize_from_content_range;

    return opts;
}

static int http_open(URLContext* h, const char* uri, int flags,
    AVDictionary** options)
{
    int ret = 0;
    JPHTTPContext* s = h->priv_data;
    AVDictionary* params = getParmasOptions(s);
    s->handle = hjhlink_create(h, uri, flags, options, params);
    if (s->handle <= 0) {
        JPLoge("error, tls open url:%s failed, flags:%d", uri, flags);
        ret = AVERROR(EINVAL);
    }
    av_dict_free(&params);
    //
    if (uri) {
        s->location = av_strdup(uri);
    }

    return ret;
}

static int http_accept(URLContext *s, URLContext **c)
{
    JPHTTPContext* sc = s->priv_data;

    return hjhlink_accept(sc->handle, c);
}

static int http_handshake(URLContext* c)
{
    JPHTTPContext* ch = c->priv_data;

    return hjhlink_handshake(ch->handle);
}

static int http_read(URLContext *h, uint8_t *buf, int size)
{
    JPHTTPContext *s = h->priv_data;

    return hjhlink_read(s->handle, buf, size);
}

/* used only when posting data */
static int http_write(URLContext *h, const uint8_t *buf, int size)
{
    JPHTTPContext *s = h->priv_data;

    return hjhlink_write(s->handle, buf, size);
}

static int http_shutdown(URLContext *h, int flags)
{
    JPHTTPContext* s = h->priv_data;

    return hjhlink_shutdown(s->handle, flags);
}

static int http_close(URLContext *h)
{
    JPHTTPContext* s = h->priv_data;

    return hjhlink_destroy(s->handle);
}

static int64_t http_seek(URLContext *h, int64_t off, int whence)
{
    JPHTTPContext* s = h->priv_data;

    return hjhlink_seek(s->handle, off, whence);
}

static int http_get_file_handle(URLContext *h)
{
    JPHTTPContext* s = h->priv_data;

    return hjhlink_get_file_handle(s->handle);
}

static int http_get_short_seek(URLContext *h)
{
    JPHTTPContext* s = h->priv_data;

    return hjhlink_get_short_seek(s->handle);
}

#define HTTP_CLASS(flavor)                          \
static const AVClass flavor ## _context_class = {   \
    .class_name = # flavor,                         \
    .item_name  = av_default_item_name,             \
    .option     = options,                          \
    .version    = LIBAVUTIL_VERSION_INT,            \
}

#if CONFIG_HTTP_PROTOCOL
HTTP_CLASS(http);

const URLProtocol ff_hjhttp_protocol = {
    .name                = "fasthttp",
    .url_open2           = http_open,
    .url_accept          = http_accept,
    .url_handshake       = http_handshake,
    .url_read            = http_read,
    .url_write           = http_write,
    .url_seek            = http_seek,
    .url_close           = http_close,
    .url_get_file_handle = http_get_file_handle,
    .url_get_short_seek  = http_get_short_seek,
    .url_shutdown        = http_shutdown,
    .priv_data_size      = sizeof(JPHTTPContext),
    .priv_data_class     = &http_context_class,
    .flags               = URL_PROTOCOL_FLAG_NETWORK,
    .default_whitelist   = "http,https,tls,rtp,tcp,udp,crypto,httpproxy"
};
#endif /* CONFIG_HTTP_PROTOCOL */

#if CONFIG_HTTPS_PROTOCOL
HTTP_CLASS(https);

const URLProtocol ff_hjhttps_protocol = {
    .name                = "fasthttps",
    .url_open2           = http_open,
    .url_read            = http_read,
    .url_write           = http_write,
    .url_seek            = http_seek,
    .url_close           = http_close,
    .url_get_file_handle = http_get_file_handle,
    .url_get_short_seek  = http_get_short_seek,
    .url_shutdown        = http_shutdown,
    .priv_data_size      = sizeof(JPHTTPContext),
    .priv_data_class     = &https_context_class,
    .flags               = URL_PROTOCOL_FLAG_NETWORK,
    .default_whitelist   = "http,https,tls,rtp,tcp,udp,crypto,httpproxy"
};
#endif /* CONFIG_HTTPS_PROTOCOL */
