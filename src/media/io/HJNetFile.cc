//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR: 
//CREATE TIME: 
//***********************************************************************************//
#include "HJNetFile.h"
#include "HJFLog.h"
#include "HJFFUtils.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJNetFile::HJNetFile()
{
}

HJNetFile::~HJNetFile()
{
    close();
}

int HJNetFile::open(HJUrl::Ptr url)
{
    if (!url || url->getUrl().empty()) {
        return HJErrInvalidParams;
    }

    int res = HJXIOBase::open(url);
    if (HJ_OK != res) {
        return res;
    }

    auto& url_str = url->getUrl();
    int flags = 0;
    if (url->getMode() & HJ_XIO_READ) {
        flags |= AVIO_FLAG_READ;
    }
    if (url->getMode() & HJ_XIO_WRITE) {
        flags |= AVIO_FLAG_WRITE;
    }

    AVDictionary* options = nullptr;
    // Set some default options if needed, e.g., timeout
    // av_dict_set(&options, "timeout", "5000000", 0); 

    res = ffurl_open_whitelist(&m_inner, url_str.c_str(), flags, m_interruptCB, &options, nullptr, nullptr, nullptr);
    if (options) {
        av_dict_free(&options);
    }

    if (res < 0) {
        HJFLoge("error, ffurl_open_whitelist failed: {}, url: {}", res, url_str);
        return res;
    }

    m_size = ffurl_size(m_inner);
    m_pos = 0;
    m_eof = false;
    return HJ_OK;
}

void HJNetFile::close()
{
    if (m_inner) {
        ffurl_closep(&m_inner);
        m_inner = nullptr;
    }
    m_pos = 0;
    m_size = 0;
}

int HJNetFile::read(void* buffer, size_t cnt)
{
    if (!m_inner || !buffer || cnt == 0) {
        return HJErrInvalidParams;
    }

    if (m_eof) {
        return 0;
    }

    setIORunTime(HJCurrentSteadyUS());
    int ret = ffurl_read(m_inner, (uint8_t*)buffer, (int)cnt);
    if (ret == AVERROR_EOF) {
        m_eof = true;
        return 0;
    } else if (ret < 0) {
        HJFLoge("error, ffurl_read failed: {}, url: {}", ret, getUrl());
        return ret;
    }

    m_pos += ret;
    return ret;
}

int HJNetFile::write(const void* buffer, size_t cnt)
{
    if (!m_inner || !buffer || cnt == 0) {
        return HJErrInvalidParams;
    }

    setIORunTime(HJCurrentSteadyUS());
    int ret = ffurl_write(m_inner, (const uint8_t*)buffer, (int)cnt);
    if (ret < 0) {
        HJFLoge("error, ffurl_write failed: {}, url: {}", ret, getUrl());
        return ret;
    }

    m_pos += ret;
    return ret;
}

int HJNetFile::seek(int64_t offset, int whence)
{
    if (!m_inner) {
        return HJErrInvalidParams;
    }

    if (whence == HJ_SEEK_SIZE) {
        return (int)m_size;
    }

    int64_t new_pos = -1;
    if (whence == SEEK_SET) {
        new_pos = offset;
    } else if (whence == SEEK_CUR) {
        new_pos = m_pos + offset;
    } else if (whence == SEEK_END) {
        new_pos = m_size + offset;
    }

    if (new_pos < 0) {
        HJFLoge("error, invalid seek position: {}, offset: {}, whence: {}, url: {}", new_pos, offset, whence, getUrl());
        return HJErrIOSeek;
    }

    setIORunTime(HJCurrentSteadyUS());
    int64_t ret = ffurl_seek(m_inner, offset, whence);
    if (ret < 0) {
        HJFLoge("error, ffurl_seek failed: {}, offset: {}, whence: {}, url: {}", (int)ret, offset, whence, getUrl());
        return HJErrIOSeek;
    }

    m_pos = ret;
    m_eof = false;
    return HJ_OK;
}

int HJNetFile::flush()
{
    return HJ_OK;
}

int64_t HJNetFile::tell()
{
    return m_pos;
}

int64_t HJNetFile::size()
{
    return m_size;
}

bool HJNetFile::eof()
{
    return m_eof;
}

NS_HJ_END