//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJRingBuffer.h"
#include "HJAsyncIO.h"
#include "HJFLog.h"
#include "HJXIOBlob.h"

#define HJ_HAVE_EXASYNC      1
/**
* AVFifo
* RingBuffer
* async
*/
NS_HJ_BEGIN
//***********************************************************************************//
class HJAsyncIO
{
public:
	using Ptr = std::shared_ptr<HJAsyncIO>;
	HJAsyncIO();
	~HJAsyncIO();

	int open(URLContext* h, const char* arg, int flags, AVDictionary** options);
	int close();
	int read(unsigned char* buf, int size);
	int64_t seek(int64_t pos, int whence);
private:
	static int exasync_check_interrupt(void* arg);
	static void* async_buffer_task(void* arg);
public:
	URLContext*		inner = NULL;
	int             seek_request = 0;
	int64_t         seek_pos = 0;
	int             seek_whence = 0;
	int             seek_completed = 0;
	int64_t         seek_ret = 0;

	int             inner_io_error = 0;
	int             io_error = 0;
	int             io_eof_reached = 0;

	int64_t					logical_pos = 0;
	int64_t					logical_size = 0;
	HJRingBuffer::Ptr		ring = nullptr;

	pthread_cond_t  cond_wakeup_main;
	pthread_cond_t  cond_wakeup_background;
	pthread_mutex_t mutex;
	pthread_t       async_buffer_thread;

	int             abort_request = 0;
	AVIOInterruptCB interrupt_callback = {NULL};
};

HJAsyncIO::HJAsyncIO()
{
}

HJAsyncIO::~HJAsyncIO()
{
}

int HJAsyncIO::open(URLContext* h, const char* arg, int flags, AVDictionary** options)
{
	if (!h) {
		return HJErrNotAlready;
	}
	ring = std::make_shared<HJRingBuffer>(HJ_BUFFER_CAPACITY, HJ_READ_BACK_CAPACITY);

	AVIOInterruptCB  interrupt_callback = { exasync_check_interrupt, this };
	int ret = ffurl_open_whitelist(&inner, arg, flags, &interrupt_callback, options, h->protocol_whitelist, h->protocol_blacklist, h);
	if (ret != 0) {
		HJLoge("error, ffurl_open failed : " + HJ2STR(ret) + ", " + HJ2SSTR(arg));
		goto url_fail;
	}

	logical_size = ffurl_size(inner);
	h->is_streamed = inner->is_streamed;

	ret = pthread_mutex_init(&mutex, NULL);
	if (ret != 0) {
		ret = AVERROR(ret);
		HJLoge("error, pthread_mutex_init failed : " + HJ2STR(ret));
		goto mutex_fail;
	}

	ret = pthread_cond_init(&cond_wakeup_main, NULL);
	if (ret != 0) {
		ret = AVERROR(ret);
		HJLoge("error, pthread_cond_init failed : " + HJ2STR(ret));
		goto cond_wakeup_main_fail;
	}

	ret = pthread_cond_init(&cond_wakeup_background, NULL);
	if (ret != 0) {
		ret = AVERROR(ret);
		HJLoge("error, pthread_cond_init failed : " + HJ2STR(ret));
		goto cond_wakeup_background_fail;
	}

	ret = pthread_create(&async_buffer_thread, NULL, async_buffer_task, this);
	if (ret) {
		ret = AVERROR(ret);
		HJLoge("error, pthread_create failed : " + HJ2STR(ret));
		goto thread_fail;
	}
	HJLogi("open end, logical_size:" + HJ2STR(logical_size));

	return 0;

thread_fail:
	pthread_cond_destroy(&cond_wakeup_background);
cond_wakeup_background_fail:
	pthread_cond_destroy(&cond_wakeup_main);
cond_wakeup_main_fail:
	pthread_mutex_destroy(&mutex);
mutex_fail:
	ffurl_closep(&inner);
url_fail:
	ring = nullptr;

	return ret;
}

int HJAsyncIO::close()
{
	pthread_mutex_lock(&mutex);
	abort_request = 1;
	pthread_cond_signal(&cond_wakeup_background);
	pthread_mutex_unlock(&mutex);

	int ret = pthread_join(async_buffer_thread, NULL);
	if (ret != 0) {
		HJLoge("error, pthread_join(): " + HJ2STR(ret));
	}

	pthread_cond_destroy(&cond_wakeup_background);
	pthread_cond_destroy(&cond_wakeup_main);
	pthread_mutex_destroy(&mutex);
	ffurl_closep(&inner);
	ring = nullptr;

	return HJ_OK;
}

int HJAsyncIO::read(unsigned char* buf, int size)
{
	int     read_complete = !buf;
	int           to_read = size;
	int           ret = 0;

	//HJLogi("read entry, size:" + HJ2STR(size) + ", logical_pos:" + HJ2STR(logical_pos) + ", logical_size:" + HJ2STR(logical_size));
	pthread_mutex_lock(&mutex);

	while (to_read > 0) {
		int fifo_size, to_copy;
		if (exasync_check_interrupt(this)) {
			ret = AVERROR_EXIT;
			break;
		}
		fifo_size = ring->size();
		to_copy = FFMIN(to_read, fifo_size);
		if (to_copy > 0) {
			ring->read(buf, to_copy);
			if (buf)
				buf = (uint8_t*)buf + to_copy;
			logical_pos += to_copy;
			to_read -= to_copy;
			ret = size - to_read;

			if (to_read <= 0 || !read_complete)
				break;
		}
		else if (io_eof_reached) {
			if (ret <= 0) {
				if (io_error)
					ret = io_error;
				else
					ret = AVERROR_EOF;
			}
			break;
		}
		pthread_cond_signal(&cond_wakeup_background);
		pthread_cond_wait(&cond_wakeup_main, &mutex);
	}

	pthread_cond_signal(&cond_wakeup_background);
	pthread_mutex_unlock(&mutex);

	//HJLogi("read end, logical_pos:" + HJ2STR(logical_pos) + ", read size:" + HJ2STR(ret));

	return ret;
}

int64_t HJAsyncIO::seek(int64_t pos, int whence)
{
	int64_t       ret;
	int64_t       new_logical_pos;
	int fifo_size;
	int fifo_size_of_read_back;

	//HJLogi("seek entry, pos:" + HJ2STR(pos) + ", whence:" + HJ2STR(whence));
	if (whence == AVSEEK_SIZE) {
		HJLogi("async_seek: AVSEEK_SIZE: " + HJ2STR((int64_t)logical_size));
		return logical_size;
	}
	else if (whence == SEEK_CUR) {
		HJLogi("async_seek: " + HJ2STR(pos));
		new_logical_pos = pos + logical_pos;
	}
	else if (whence == SEEK_SET) {
		HJLogi("async_seek: " + HJ2STR(pos));
		new_logical_pos = pos;
	}
	else {
		return AVERROR(EINVAL);
	}
	if (new_logical_pos < 0)
		return AVERROR(EINVAL);

	fifo_size = ring->size();
	fifo_size_of_read_back = ring->sizeOfReadBack();
	if (new_logical_pos == logical_pos) {
		/* current position */
		return logical_pos;
	}
	else if ((new_logical_pos >= (logical_pos - fifo_size_of_read_back)) &&
		(new_logical_pos < (logical_pos + fifo_size + HJ_SHORT_SEEK_THRESHOLD))) {
		int pos_delta = (int)(new_logical_pos - logical_pos);
		/* fast seek */
		HJLogi("async_seek: fask_seek " + HJ2STR(new_logical_pos) + 
			" from " +  HJ2STR((int)logical_pos) + " dist " + HJ2STR((int)(new_logical_pos - logical_pos)) + " : " + HJ2STR(fifo_size));

		if (pos_delta > 0) {
			// fast seek forwards
			read(NULL, pos_delta);
		}
		else {
			// fast seek backwards
			ring->drain(pos_delta);
			logical_pos = new_logical_pos;
		}

		return logical_pos;
	}
	else if (logical_size <= 0) {
		/* can not seek */
		return AVERROR(EINVAL);
	}
	else if (new_logical_pos > logical_size) {
		/* beyond end */
		return AVERROR(EINVAL);
	}

	pthread_mutex_lock(&mutex);
	seek_request = 1;
	seek_pos = new_logical_pos;
	seek_whence = SEEK_SET;
	seek_completed = 0;
	seek_ret = 0;

	while (1) {
		if (exasync_check_interrupt(this)) {
			ret = AVERROR_EXIT;
			break;
		}
		if (seek_completed) {
			if (seek_ret >= 0)
				logical_pos = seek_ret;
			ret = seek_ret;
			break;
		}
		pthread_cond_signal(&cond_wakeup_background);
		pthread_cond_wait(&cond_wakeup_main, &mutex);
	}

	pthread_mutex_unlock(&mutex);
	//HJLogi("seek end, logical_pos:" + HJ2STR(logical_pos));

	return ret;
}

int HJAsyncIO::exasync_check_interrupt(void* arg)
{
	//URLContext* h = arg;
	//Context* c = h->priv_data;

	HJAsyncIO* c = (HJAsyncIO*)arg;
	if (c->abort_request)
		return 1;

	if (ff_check_interrupt(&c->interrupt_callback))
		c->abort_request = 1;

	return c->abort_request;
}

void* HJAsyncIO::async_buffer_task(void* arg)
{
	HJAsyncIO* c = (HJAsyncIO*)arg;
	HJRingBuffer::Ptr ring = c->ring;
	int           ret = 0;
	int64_t       seek_ret;

    ff_thread_setname("async");
    
	while (true)
	{
		int fifo_space, to_copy;

		//HJLogi("async_buffer_task entry");
		pthread_mutex_lock(&c->mutex);
		if (exasync_check_interrupt(c)) {
			c->io_eof_reached = 1;
			c->io_error = AVERROR_EXIT;
			pthread_cond_signal(&c->cond_wakeup_main);
			pthread_mutex_unlock(&c->mutex);
			break;
		}

		if (c->seek_request) {
			seek_ret = ffurl_seek(c->inner, c->seek_pos, c->seek_whence);
			if (seek_ret >= 0) {
				c->io_eof_reached = 0;
				c->io_error = 0;
				ring->reset();
			}

			c->seek_completed = 1;
			c->seek_ret = seek_ret;
			c->seek_request = 0;

			//HJLogi("async_buffer_task seek pos:" + HJ2STR(c->seek_pos));
			pthread_cond_signal(&c->cond_wakeup_main);
			pthread_mutex_unlock(&c->mutex);
			continue;
		}

		fifo_space = ring->space();
		if (c->io_eof_reached || fifo_space <= 0) {
			pthread_cond_signal(&c->cond_wakeup_main);
			pthread_cond_wait(&c->cond_wakeup_background, &c->mutex);
			pthread_mutex_unlock(&c->mutex);
			//HJLogi("async_buffer_task io_eof_reached:" + HJ2STR(c->io_eof_reached));
			continue;
		}
		pthread_mutex_unlock(&c->mutex);

		to_copy = FFMIN(4096, fifo_space);
		ret = ring->writeFromCB([=](uint8_t* buf, size_t* size) -> int {
			int ret = ffurl_read(c->inner, buf, *size);
			*size = ret > 0 ? ret : 0;
			c->inner_io_error = ret < 0 ? ret : 0;

			return ret;//c->inner_io_error;
		}, to_copy);

		pthread_mutex_lock(&c->mutex);
		if (ret <= 0) {
			c->io_eof_reached = 1;
			if (c->inner_io_error < 0)
				c->io_error = c->inner_io_error;
		}
		pthread_cond_signal(&c->cond_wakeup_main);
		pthread_mutex_unlock(&c->mutex);

		//HJLogi("async_buffer_task write end, write size:" + HJ2STR(ret));
	}

	return NULL;
}
NS_HJ_END


NS_HJ_USING
//***********************************************************************************//
int exasync_open(URLContext* h, const char* arg, int flags, AVDictionary** options)
{
	HJExasyncContext* c = (HJExasyncContext *)h->priv_data;
	if (!c || !arg) {
		HJLoge("error, params failed");
		return HJErrNotAlready;
	}
	av_strstart(arg, "exasync:", &arg);

	int res = HJ_OK;
	if (*options) {
		const AVDictionaryEntry* e = NULL;
		e = av_dict_get(*options, "opaque", NULL, 0);
		if (e) {
			c->m_opaque = HJSTR2PTR(std::string(e->value));
		}
		e = av_dict_get(*options, "bloburl", NULL, 0);
		if (e) {
			std::string blobUrl = std::string(e->value);
			HJXIOBlob* blob = HJCreator::createp<HJXIOBlob>();
			res = blob->open(std::make_shared<HJUrl>(blobUrl, HJ_XIO_WRITE));
			if (HJ_OK != res) {
				HJFLoge("error, blob open failed:{}", res);
				return res;
			}
			c->m_blob = (void*)blob;
		}
	}
	c->m_url = (char *)malloc(strlen(arg) + 1);
	if (c->m_url) {
		strcpy(c->m_url, arg);
	}
	c->m_flags = flags;
	HJLogi("exasync begin, open url:" + HJ2SSTR(c->m_url) + ", flags:" + HJ2STR(c->m_flags));

	HJAsyncIO* asyncIO = new HJAsyncIO();
	res = asyncIO->open(h, c->m_url, c->m_flags, options);
	if (res < HJ_OK){
		HJFLoge("error, async iof failed:{}", res);
		return res;
	}
	c->m_asyncIO = (void *)asyncIO;

	return res;
}

int exasync_read(URLContext* h, unsigned char* buf, int size)
{
	HJExasyncContext* c = (HJExasyncContext*)h->priv_data;
	if (!c || !c->m_asyncIO) {
		HJLoge("error, invalid async io");
		return HJErrNotAlready;
	}
	HJAsyncIO* asyncIO = (HJAsyncIO *)c->m_asyncIO;
	int ret = asyncIO->read(buf, size);
	//
	HJXIOBlob* blob = (HJXIOBlob*)c->m_blob;
	if (blob) {
		blob->write(buf, ret);
	}
	HJFLogi("exasync read, size:{}, ret:{}", size, ret);

	return ret;
}

int64_t exasync_seek(URLContext* h, int64_t pos, int whence)
{
	HJExasyncContext* c = (HJExasyncContext*)h->priv_data;
	if (!c || !c->m_asyncIO) {
		HJLoge("error, invalid async io");
		return HJErrNotAlready;
	}
	HJAsyncIO* asyncIO = (HJAsyncIO*)c->m_asyncIO;
	int64_t ret = asyncIO->seek(pos, whence);
	//
	HJXIOBlob* blob = (HJXIOBlob*)c->m_blob;
	if (blob) { 
		switch (whence)
		{
			case SEEK_SET:
			case SEEK_CUR:
				blob->seek(pos, whence);
				break;
			case SEEK_END:
				HJFLogi("seek end, pos:{}, ret:{}", pos, ret);
				break;
			case AVSEEK_SIZE: {
				if (ret > 0) {
					blob->setSize(ret);
				}
				break;
			}
		}
	}
	HJFLogi("exasync seek, pos:{}, whence:{}, ret:{}", pos, whence, ret);

	return ret;
}

int exasync_close(URLContext* h)
{
	HJExasyncContext* c = (HJExasyncContext*)h->priv_data;
	if (!c || !c->m_asyncIO) {
		HJLoge("error, invalid async io");
		return HJErrNotAlready;
	}
	HJAsyncIO* asyncIO = (HJAsyncIO*)c->m_asyncIO;
	int ret = asyncIO->close();
	delete asyncIO;
	//
	c->m_asyncIO = NULL;
	if (c->m_url) {
		free(c->m_url);
		c->m_url = NULL;
	}
	HJCreator::freep<HJXIOBlob>(c->m_blob);

	return ret;
}

//***********************************************************************************//
#if HJ_HAVE_EXASYNC
static const AVOption exasync_options[] = {
	{NULL},
};

static const AVClass exasync_context_class = {
	"Exasync",				//class_name
	av_default_item_name,	//item_name
	exasync_options,		//option
	LIBAVUTIL_VERSION_INT,	//version
	0,						//log_level_offset_offset
	0,						//parent_log_context_offset
	AV_CLASS_CATEGORY_NA,	//category
	NULL,					//AVClassCategory(*get_category)(void* ctx)
	NULL,					//query_ranges
	NULL,					//child_next
	NULL,					//child_class_iterate
};

const URLProtocol ff_exasync_protocol = {
	"exasync",				//name
	NULL,					//url_open
	exasync_open,			//url_open2
	NULL,					//url_accept
	NULL,					//url_handshake
	exasync_read,			//url_read
	NULL,					//url_write
	exasync_seek,			//url_seek
	exasync_close,			//url_close
	NULL,					//url_read_pause
	NULL,					//url_read_seek
	NULL,					//url_get_file_handle
	NULL,					//url_get_multi_file_handle
	NULL,					//url_get_short_seek
	NULL,					//url_shutdown
	&exasync_context_class, //priv_data_class
	sizeof(HJExasyncContext), //priv_data_size
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
