/*
 * TCP protocol
 * Copyright (c) 2002 Fabrice Bellard
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
#include "HJTCPLinkManagerInterface.h"

#include "libavutil/avassert.h"
#include "libavutil/parseutils.h"
#include "libavutil/opt.h"
#include "libavutil/time.h"
#include "libavutil/internal.h"

#include "libavformat/avformat.h"
#include "libavformat/network.h"
#include "libavformat/os_support.h"
#include "libavformat/url.h"
#if HAVE_POLL_H
#include <poll.h>
#endif

typedef struct JPTCPContext {
    const AVClass *class;
    int fd;
    int listen;
    char* local_port;
    char* local_addr;
    int open_timeout;
    int rw_timeout;
    int listen_timeout;
    int recv_buffer_size;
    int send_buffer_size;
    int tcp_nodelay;
#if !HAVE_WINSOCK2_H
    int tcp_mss;
#endif /* !HAVE_WINSOCK2_H */
    char *str_sid;
    char *str_url;
    int   use_ipv4;
    char str_ip[400];
    unsigned short port;
    int ecode;
    char* obj_name;
    //
    size_t handle;
} JPTCPContext;

#define OFFSET(x) offsetof(JPTCPContext, x)
#define D AV_OPT_FLAG_DECODING_PARAM
#define E AV_OPT_FLAG_ENCODING_PARAM
static const AVOption options[] = {
    { "listen",          "Listen for incoming connections",  OFFSET(listen),         AV_OPT_TYPE_INT, { .i64 = 0 },     0,       2,       .flags = D|E },
    { "local_port",      "Local port",                                         OFFSET(local_port),     AV_OPT_TYPE_STRING, {.str = NULL },     0,       0, .flags = D | E },
    { "local_addr",      "Local address",                                      OFFSET(local_addr),     AV_OPT_TYPE_STRING, {.str = NULL },     0,       0, .flags = D | E },
    { "timeout",     "set timeout (in microseconds) of socket I/O operations", OFFSET(rw_timeout),     AV_OPT_TYPE_INT, { .i64 = -1 },         -1, INT_MAX, .flags = D|E },
    { "listen_timeout",  "Connection awaiting timeout (in milliseconds)",      OFFSET(listen_timeout), AV_OPT_TYPE_INT, { .i64 = -1 },         -1, INT_MAX, .flags = D|E },
    { "send_buffer_size", "Socket send buffer size (in bytes)",                OFFSET(send_buffer_size), AV_OPT_TYPE_INT, { .i64 = -1 },         -1, INT_MAX, .flags = D|E },
    { "recv_buffer_size", "Socket receive buffer size (in bytes)",             OFFSET(recv_buffer_size), AV_OPT_TYPE_INT, { .i64 = -1 },         -1, INT_MAX, .flags = D|E },
    { "tcp_nodelay", "Use TCP_NODELAY to disable nagle's algorithm",           OFFSET(tcp_nodelay), AV_OPT_TYPE_BOOL, { .i64 = 0 },             0, 1, .flags = D|E },
#if !HAVE_WINSOCK2_H
    { "tcp_mss",     "Maximum segment size for outgoing TCP packets",          OFFSET(tcp_mss),     AV_OPT_TYPE_INT, { .i64 = -1 },         -1, INT_MAX, .flags = D|E },
#endif /* !HAVE_WINSOCK2_H */
    { "report_sid", "sid for report",OFFSET(str_sid), AV_OPT_TYPE_STRING, {.str = "daxi"}, CHAR_MIN,CHAR_MAX,.flags = D|E },
    { "report_url", "url for report",OFFSET(str_url), AV_OPT_TYPE_STRING, {.str = 0}, CHAR_MIN,CHAR_MAX,.flags = D|E },
    { "use_ipv4", "use ipv4", OFFSET(use_ipv4), AV_OPT_TYPE_INT, { .i64 = 0 },-1, INT_MAX, .flags = D|E },
    { "obj_name", "obj name",OFFSET(obj_name), AV_OPT_TYPE_STRING, {.str = 0}, CHAR_MIN,CHAR_MAX,.flags = D | E },
    { NULL }
};

static const AVClass tcp_class = {
    .class_name = "tcp",
    .item_name  = av_default_item_name,
    .option     = options,
    .version    = LIBAVUTIL_VERSION_INT,
};

/* return non zero if error */
static int tcp_open(URLContext *h, const char *uri, int flags)
{
    int ret = 0;
    JPTCPContext* s = h->priv_data;
    if (s->rw_timeout >= 0) {
        s->open_timeout = h->rw_timeout = s->rw_timeout;
    }
    s->handle = hjtcp_link_create(uri, flags);
    if (s->handle <= 0) {
        JPLoge("error, tcp open url:%s failed, flags:%d", uri, flags);
        return AVERROR(EINVAL);
    }
    //
    if (!hjtcp_link_is_alive(s->handle)) 
    {
        AVDictionary* opts = NULL;
        av_dict_set(&opts, "obj_name", s->obj_name, AV_DICT_MATCH_CASE);
        av_dict_set_int(&opts, "timeout", s->rw_timeout, AV_DICT_MATCH_CASE);
        av_dict_set_int(&opts, "send_buffer_size", s->send_buffer_size, AV_DICT_MATCH_CASE);
        av_dict_set_int(&opts, "recv_buffer_size", s->recv_buffer_size, AV_DICT_MATCH_CASE);
        av_dict_set_int(&opts, "tcp_nodelay", s->tcp_nodelay, AV_DICT_MATCH_CASE);
        ret = hjtcp_link_open(s->handle, uri, flags, &opts);
        av_dict_free(&opts);
    } else {
        JPLogi("find aready tcp link, uri:%s", uri);
    }
    return ret;
}

static int tcp_accept(URLContext *s, URLContext **c)
{
    JPTCPContext *sc = s->priv_data;
    JPLogi("tcp accept entry");

    return hjtcp_link_accept(sc->handle);
}

static int tcp_read(URLContext *h, uint8_t *buf, int size)
{
    JPTCPContext *s = h->priv_data;

    return hjtcp_link_read(s->handle, buf, size);
}

static int tcp_write(URLContext *h, const uint8_t *buf, int size)
{
    JPTCPContext *s = h->priv_data;

    return hjtcp_link_write(s->handle, buf, size);
}

static int tcp_shutdown(URLContext *h, int flags)
{
    JPTCPContext *s = h->priv_data;

    return hjtcp_link_shutdown(s->handle, flags);
}

static int tcp_close(URLContext *h)
{
    JPTCPContext* s = h->priv_data;

    //return hjtcp_link_unused(s->handle);
    return hjtcp_link_destroy(s->handle);
}

static int tcp_get_file_handle(URLContext *h)
{
    JPTCPContext *s = h->priv_data;

    return hjtcp_link_get_file_handle(s->handle);
}

static int tcp_get_window_size(URLContext *h)
{
    JPTCPContext *s = h->priv_data;

    return hjtcp_link_get_window_size(s->handle);
}

const URLProtocol ff_hjtcp_protocol = {
    .name                = "tcp",
    .url_open            = tcp_open,
    .url_accept          = tcp_accept,
    .url_read            = tcp_read,
    .url_write           = tcp_write,
    .url_close           = tcp_close,
    .url_get_file_handle = tcp_get_file_handle,
    .url_get_short_seek  = tcp_get_window_size,
    .url_shutdown        = tcp_shutdown,
    .priv_data_size      = sizeof(JPTCPContext),
    .flags               = URL_PROTOCOL_FLAG_NETWORK,
    .priv_data_class     = &tcp_class,
};
