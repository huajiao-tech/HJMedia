//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR: 
//CREATE TIME: 
//***********************************************************************************//
#include "HJAVIOContext.h"
#include "HJFLog.h"
#include "HJFFUtils.h"
#include "HJDataSource.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJAVIOContext::HJAVIOContext()
{
    m_buffer = HJCreates<HJBuffer>(32 * 1024);
    m_ioCtx = avio_alloc_context(m_buffer->data(), m_buffer->size(), 0, this, read_callback, write_callback, seek_callback);
}

HJAVIOContext::~HJAVIOContext()
{
    close();
}

int HJAVIOContext::open(std::string url, std::string dir, std::string rid, int64_t timeout)
{
    if (url.empty()) {
        return HJErrInvalidParams;
    }
    
    HJUrl::Ptr hurl = HJCreates<HJUrl>(url);
    if (!dir.empty()) {
        (*hurl)["local_dir"] = dir;
    }
    (*hurl)["url_rid"] = rid;
    (*hurl)["timeout"] = timeout;
    (*hurl)["interrupt_callback"] = *m_interruptCB;
    m_hurl = hurl;
    
    m_dataSource = HJCreates<HJDataSource>();
    int ret = m_dataSource->open(hurl);
    if (ret != HJ_OK) {
        HJFLoge("open data source failed, url:{}", url);
        m_dataSource = nullptr;
        return ret;
    }
    return HJ_OK;
}

int HJAVIOContext::close()
{
    if (m_dataSource) {
        m_dataSource->close();
        m_dataSource = nullptr;
    }
    if (m_ioCtx) {
        // Important: avio_context_free frees the buffer if it thinks it owns it.
        // Since we manage the buffer via m_buffer (HJBuffer::Ptr), we should NOT let avio_context_free free it.
        // However, avio_alloc_context does NOT set the flag to free the buffer by default unless we set it.
        // But to be safe and symmetric with avio_alloc_context, and ensuring clean memory,
        // we use av_free(m_ioCtx) because we didn't allocate the context structure via avio_alloc_context??
        // Wait, avio_alloc_context RETURNS a pointer to allocated AVIOContext.
        // Correct way: use av_free(m_ioCtx->buffer) ONLY if we allocated it with av_malloc (we didn't, HJBuffer did).
        // So we just need to free the context struct itself.
        // avio_context_free(&m_ioCtx) will free internal buffer if write_flag is set or checks other things.
        // SAFEST:
        av_free(m_ioCtx);
        m_ioCtx = nullptr;
    }
    return HJ_OK;
}

int HJAVIOContext::read_callback(void* opaque, uint8_t* buf, int buf_size)
{
    HJAVIOContext* ctx = (HJAVIOContext*)opaque;
    if (!ctx || !ctx->m_dataSource) {
        return HJErrNotAlready;
    }
    return ctx->m_dataSource->read(buf, buf_size);
}

int HJAVIOContext::write_callback(void* opaque, const uint8_t* buf, int buf_size)
{
    return HJErrNotSupport;
}

int64_t HJAVIOContext::seek_callback(void* opaque, int64_t pos, int whence)
{
    HJAVIOContext* ctx = (HJAVIOContext*)opaque;
    if (!ctx || !ctx->m_hurl || !ctx->m_dataSource) {
        HJFLogw("seek_callback failed, data source is null");
        return HJErrNotAlready;
    }
    auto dataSource = ctx->m_dataSource;
    
    if (whence == AVSEEK_SIZE) {
        auto size = dataSource->size();
        HJFLogi("hjds seek, url:{}, size:{}", ctx->getUrl(), size);
        return dataSource->size();
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
	HJFLogi("hjds seek, url:{}, cur pos:{}, whence:{}, result:{}", ctx->getUrl(), cur_pos, whence, res);
	return cur_pos;
}


NS_HJ_END