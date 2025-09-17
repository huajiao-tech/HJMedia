/*
 * TLS/SSL Protocol
 * Copyright (c) 2011 Martin Storsjo
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
#include "HJTLSLinkManagerInterface.h"

#include "libavformat/avformat.h"
#include "libavformat/internal.h"
#include "libavformat/network.h"
#include "libavformat/os_support.h"
#include "libavformat/url.h"
#include "libavformat/tls.h"
#include "libavcodec/internal.h"
#include "libavutil/avstring.h"
#include "libavutil/avutil.h"
#include "libavutil/opt.h"
#include "libavutil/parseutils.h"
#include "libavutil/thread.h"
#include "libavutil/time.h"

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

typedef struct JPTLSContext {
    const AVClass *class;
    TLSShared tls_shared;
    //
    size_t handle;
} JPTLSContext;

static int tls_close(URLContext *h)
{
    JPTLSContext *c = h->priv_data;

    //return hjtls_link_unused(c->handle);
    return hjtls_link_destroy(c->handle);
}

static int tls_open(URLContext *h, const char *uri, int flags, AVDictionary **options)
{
    int ret = 0;
    JPTLSContext *p = h->priv_data;
    //TLSShared *c = &p->tls_shared;

    char* obj_name = NULL;
    if (options)
    {
        AVDictionaryEntry* e = av_dict_get(*options, "obj_name", NULL, 0);
        if (e && e->value) {
            obj_name = av_strdup((const char*)e->value);
        }
    }
    int64_t t0 = av_gettime_relative();
    p->handle = hjtls_link_create(uri, flags, options);
    if (p->handle <= 0) {
        JPLoge("error, tls open url:%s failed, flags:%d", uri, flags);
        av_freep(&obj_name);
        return AVERROR(EINVAL);
    }
    //ret = hjtls_link_open(p->handle, uri, flags, options);
    int64_t dtt_time = (av_gettime_relative() - t0) / 1000;
    if (obj_name) {
        hjtls_link_set_proc_info(obj_name, "dns_tcp_tls", dtt_time);
    }
    JPLogi("end, tls open handle:%d, dns tcp tls time:%lld", p->handle, dtt_time);
    av_freep(&obj_name);

    return ret;
}

static int tls_read(URLContext *h, uint8_t *buf, int size)
{
    JPTLSContext *c = h->priv_data;

    return hjtls_link_read(c->handle, buf, size);
}

static int tls_write(URLContext *h, const uint8_t *buf, int size)
{
    JPTLSContext *c = h->priv_data;

    return hjtls_link_write(c->handle, buf, size);
}

static int tls_get_file_handle(URLContext *h)
{
    JPTLSContext *c = h->priv_data;
    return hjtls_link_get_file_handle(c->handle);
}

static int tls_get_short_seek(URLContext* h)
{
    JPTLSContext* c = h->priv_data;
    return hjtls_link_tls_get_short_seek(c->handle);
}

static const AVOption options[] = {
    TLS_COMMON_OPTIONS(JPTLSContext, tls_shared),
    { NULL }
};

static const AVClass tls_class = {
    .class_name = "tls",
    .item_name  = av_default_item_name,
    .option     = options,
    .version    = LIBAVUTIL_VERSION_INT,
};

const URLProtocol ff_hjtls_protocol = {
    .name           = "tls",
    .url_open2      = tls_open,
    .url_read       = tls_read,
    .url_write      = tls_write,
    .url_close      = tls_close,
    .url_get_file_handle = tls_get_file_handle,
    .url_get_short_seek = tls_get_short_seek,
    .priv_data_size = sizeof(JPTLSContext),
    .flags          = URL_PROTOCOL_FLAG_NETWORK,
    .priv_data_class = &tls_class,
};
