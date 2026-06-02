//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJXIOContext.h"
#include "HJFFUtils.h"
#include "HJFlog.h"
#include <climits>

NS_HJ_BEGIN
//***********************************************************************************//
const std::map<int, int> HJXIOContext::XIO_AVIO_MODE_MAPS = {
    {HJ_XIO_READ, AVIO_FLAG_READ},
    {HJ_XIO_WRITE, AVIO_FLAG_WRITE},
    {HJ_XIO_RW, AVIO_FLAG_READ_WRITE}
};

HJXIOContext::HJXIOContext()
    : m_avio(nullptr)
    , m_close_with_avio_closep(false)
{
}

HJXIOContext::~HJXIOContext()
{
    close();
}

int HJXIOContext::open(HJUrl::Ptr url)
{
    if (!url || url->getUrl().empty()) {
        return HJErrInvalidParams;
    }
    if (m_avio) {
        close();
    }
    m_isQuit = false;
    m_size = 0;
    m_close_with_avio_closep = true;
    int res = HJXIOBase::open(url);
    if (HJ_OK != res) {
        return res;
    }
    HJLogi("entry url:" + m_url->getUrl());
    do {
        //
        int flags = HJXIOContext::xioToAvioMode(m_url->getMode());
        AVDictionary* opts = NULL;
        if (url->haveValue("http_persistent")) {
            auto flag = url->getInt("http_persistent");
            av_dict_set_int(&opts, "http_persistent", flag, 0);
            av_dict_set_int(&opts, "multiple_requests", flag, 0);
        }
        if (url->haveValue("timeout")) {
            av_dict_set_int(&opts, "timeout", url->getInt64("timeout"), 0);
        }
        int ret = avio_open2(&m_avio, m_url->getUrl().c_str(), flags, m_interruptCB, &opts);
        if (opts) {
            av_dict_free(&opts);
        }
        if (ret < 0) {
            res = HJErrIOOpen;
            avio_closep(&m_avio);
            HJFLoge("error, avio_open2 failed, url:{}, res:{},{}", m_url->getUrl(), res, HJ_AVErr2Str(ret));
            break;
        }
        int64_t avio_size = size();
        if (avio_size >= 0) {
            m_size = static_cast<size_t>(avio_size);
        }
    } while (false);
    
    return res;
}

int HJXIOContext::open(AVIOContext* avio)
{
    if (!avio) {
        return HJErrInvalidParams;
    }
    if (m_avio) {
        close();
    }
    HJLogi("entry avio");
    m_isQuit = false;
    m_avio = avio;
    m_close_with_avio_closep = false;
    m_size = 0;
    int64_t avio_size = size();
    if (avio_size >= 0) {
        m_size = static_cast<size_t>(avio_size);
    }
    return HJ_OK;
}

void HJXIOContext::close()
{
    m_isQuit = true;
    if (m_avio) {
        if (m_close_with_avio_closep) {
            avio_closep(&m_avio);
        } else {
            avio_context_free(&m_avio);
        }
    }
    m_size = 0;
    m_close_with_avio_closep = false;
}

int HJXIOContext::read(void* buffer, size_t cnt)
{
    if(!m_avio) {
        return HJErrIORead;
    }
    if (!buffer || cnt == 0 || cnt > static_cast<size_t>(INT_MAX)) {
        return HJErrInvalidParams;
    }
    return avio_read(m_avio, (unsigned char *)buffer, (int)cnt);
}

int HJXIOContext::write(const void* buffer, size_t cnt)
{
    if(!m_avio) {
        return HJErrIOWrite;
    }
    if (!buffer || cnt == 0 || cnt > static_cast<size_t>(INT_MAX)) {
        return HJErrInvalidParams;
    }
    avio_write(m_avio, (const unsigned char *)buffer, (int)cnt);
    if (m_avio->error < 0) {
        return HJErrIOWrite;
    }
    return (int)cnt;
}

int HJXIOContext::seek(int64_t offset, int whence)
{
    if (!m_avio) {
        return HJErrIOSeek;
    }
    int64_t ret = avio_seek(m_avio, offset, whence);
    if (ret < 0) {
        return HJErrIOSeek;
    }
    return HJ_OK;
}

int HJXIOContext::flush()
{
    if (m_avio) {
        avio_flush(m_avio);
    }
    return HJ_OK;
}

int64_t HJXIOContext::tell()
{
    if (m_avio) {
        return avio_tell(m_avio);
    }
    return 0;
}

int64_t HJXIOContext::size()
{
    if(m_avio) {
        return avio_size(m_avio);
    }
    return 0;
}

bool HJXIOContext::eof()
{
    if(m_avio) {
        if (avio_feof(m_avio)) {
            return true;
        }
        if (m_size > 0) {
            int64_t pos = tell();
            return (pos >= 0 && static_cast<size_t>(pos) >= m_size);
        }
    }
    return false;
}

int HJXIOContext::xioToAvioMode(int mode)
{
    int flags = 0;
    auto it = XIO_AVIO_MODE_MAPS.find(mode);
    if (it != XIO_AVIO_MODE_MAPS.end()) {
        flags = it->second;
    }
    return flags;
}

int HJXIOContext::getLine(void* buffer, size_t maxlen)
{
    if (!m_avio || !buffer || maxlen == 0 || maxlen > static_cast<size_t>(INT_MAX)) {
        return 0;
    }
    return ff_get_line(m_avio, (char*)buffer, (int)maxlen);
}

int HJXIOContext::getChompLine(void* buffer, size_t maxlen)
{
    if (!m_avio || !buffer || maxlen == 0 || maxlen > static_cast<size_t>(INT_MAX)) {
        return 0;
    }
    return ff_get_chomp_line(m_avio, (char *)buffer, (int)maxlen);
}


NS_HJ_END
