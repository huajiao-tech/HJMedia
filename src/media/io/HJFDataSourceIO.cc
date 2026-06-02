//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJFDataSourceIO.h"
#include "HJDataSource.h"
#include "HJFLog.h"

#define HJ_HAVE_DATASOURCE_IO      0

NS_HJ_BEGIN
//***********************************************************************************//


NS_HJ_END

NS_HJ_USING
//***********************************************************************************//
int hjds_open(URLContext* h, const char* arg, int flags, AVDictionary** options)
{
    if (!h || !arg || !*arg) {
		HJLoge("error, params failed");
		return HJErrInvalidParams;
	}
    HJFDataSourceIOContext* c = (HJFDataSourceIOContext *)h->priv_data;
	if (!c) {
		HJLoge("error, params failed");
		return HJErrInvalidParams;
	}
	av_strstart(arg, "hjds:", &arg);

    int res = HJ_OK;
    std::string local_dir = "";
	std::string url_rid = "";
	int64_t timeout = 0;
    if (options && *options) {
		const AVDictionaryEntry* e = NULL;
		e = av_dict_get(*options, "opaque", NULL, 0);
		if (e) {
			c->m_opaque = HJSTR2PTR(std::string(e->value));
		}
		e = av_dict_get(*options, "local_dir", NULL, 0);
		if (e) {
			local_dir = std::string(e->value);
		}
		e = av_dict_get(*options, "url_rid", NULL, 0);
        if (e) {
			url_rid = std::string(e->value);
		}
		e = av_dict_get(*options, "timeout", NULL, 0);
		if (e) {
			timeout = atoll(e->value);
		}
	}
	c->m_url = (char *)malloc(strlen(arg) + 1);
	if (!c->m_url) {
		HJFLoge("error, alloc url failed");
		return HJErrMemAlloc;
	}
	strcpy(c->m_url, arg);
	c->m_flags = flags;
	HJFLogi("hjds begin, open url:{}, flags:{}",  c->m_url, c->m_flags);

    std::string remote_url = std::string(c->m_url);
    HJUrl::Ptr murl = HJCreates<HJUrl>(remote_url);
    (*murl)["local_dir"] = local_dir;
    (*murl)["url_rid"] = url_rid;
    (*murl)["timeout"] = timeout;
	if (NULL != h->interrupt_callback.callback) {
		(*murl)["interrupt_callback"] = h->interrupt_callback;
	}
    HJDataSource* dataSource = new HJDataSource();
    res = dataSource->open(murl);
    if(HJ_OK != res) {
        delete dataSource;
		if (c->m_url) {
			free(c->m_url);
			c->m_url = NULL;
		}
        HJFLoge("error, open data source failed");
        return res;
    }
    c->m_io = (void *)dataSource;
	h->is_streamed = 0;

    return res;
}

int hjds_read(URLContext* h, unsigned char* buf, int size)
{
    HJFDataSourceIOContext* c = (HJFDataSourceIOContext*)h->priv_data;
	if (!c || !c->m_io) {
		HJLoge("error, invalid async io");
		return HJErrNotAlready;
	}
	if (!buf || size == 0) {
		return 0;
	}
	if (size < 0) {
		return HJErrInvalidParams;
	}
    HJDataSource* dataSource = (HJDataSource*)c->m_io;
    return dataSource->read(buf, static_cast<size_t>(size));
}

int64_t hjds_seek(URLContext* h, int64_t pos, int whence)
{ 
    HJFDataSourceIOContext* c = (HJFDataSourceIOContext*)h->priv_data;
	if (!c || !c->m_io) {
		HJLoge("error, invalid async io");
		return HJErrNotAlready;
	}
    
    HJDataSource* dataSource = (HJDataSource*)c->m_io;
    if (whence & AVSEEK_SIZE) {
        auto size = dataSource->size();
		HJFLogi("hjds seek, url:{}, size:{}", c->m_url, size);
		return size;
    }
	if (whence & AVSEEK_FORCE) {
		whence &= ~AVSEEK_FORCE;
	}
    
	int mapped_whence = whence;
	switch (whence) {
		case SEEK_SET:
			mapped_whence = std::ios::beg;
			break;
		case SEEK_CUR:
			mapped_whence = std::ios::cur;
			break;
		case SEEK_END:
			mapped_whence = std::ios::end;
			break;
		default:
			return HJErrInvalidParams;
	}

    int res = dataSource->seek(pos, mapped_whence);
    if (HJ_OK != res) {
		HJFLoge("error, seek data source failed, result:{}", res);
        return res;
    }
    auto cur_pos = dataSource->tell();
	HJFLogi("hjds seek, url:{}, cur pos:{}, whence:{}, result:{}", c->m_url, cur_pos, whence, res);
	return cur_pos;
}

int hjds_close(URLContext* h)
{
    if (!h) {
		return HJErrInvalidParams;
	}
    HJFDataSourceIOContext* c = (HJFDataSourceIOContext*)h->priv_data;
	if (!c) {
		return HJErrInvalidParams;
	}
	if (c->m_io) {
		HJDataSource* dataSource = (HJDataSource*)c->m_io;
		delete dataSource;
		c->m_io = NULL;
	}
	//
	if (c->m_url) {
		free(c->m_url);
		c->m_url = NULL;
	}

    return HJ_OK;
}
//***********************************************************************************//
#if HJ_HAVE_DATASOURCE_IO
static const AVOption hjds_options[] = {
	{NULL},
};

static const AVClass hjds_context_class = {
	"HJDS",				//class_name
	av_default_item_name,	//item_name
	hjds_options,		//option
	LIBAVUTIL_VERSION_INT,	//version
	0,						//log_level_offset_offset
	0,						//parent_log_context_offset
	AV_CLASS_CATEGORY_NA,	//category
	NULL,					//AVClassCategory(*get_category)(void* ctx)
	NULL,					//query_ranges
	NULL,					//child_next
	NULL,					//child_class_iterate
};

const URLProtocol ff_hjds_protocol = {
	"hjds",				//name
	NULL,					//url_open
	hjds_open,			//url_open2
	NULL,					//url_accept
	NULL,					//url_handshake
	hjds_read,			//url_read
	NULL,					//url_write
	hjds_seek,			//url_seek
	hjds_close,			//url_close
	NULL,					//url_read_pause
	NULL,					//url_read_seek
	NULL,					//url_get_file_handle
	NULL,					//url_get_multi_file_handle
	NULL,					//url_get_short_seek
	NULL,					//url_shutdown
	&hjds_context_class, //priv_data_class
	sizeof(HJFDataSourceIOContext), //priv_data_size
	0,						//flags
	NULL,					//url_check
	NULL,					//url_open_dir
	NULL,					//url_read_dir
	NULL,					//url_close_dir
	NULL,					//url_delete
	NULL,					//url_move
	NULL,					//default_whitelist
};
#endif
