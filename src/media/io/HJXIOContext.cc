//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJXIOContext.h"
#include "HJFFUtils.h"

NS_HJ_BEGIN
//***********************************************************************************//
const std::map<int, int> HJXIOContext::XIO_AVIO_MODE_MAPS = {
    {HJ_XIO_READ, AVIO_FLAG_READ},
    {HJ_XIO_WRITE, AVIO_FLAG_WRITE},
    {HJ_XIO_RW, AVIO_FLAG_READ_WRITE}
};

HJXIOContext::HJXIOContext()
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
    int res = HJXIOBase::open(url);
    if (HJ_OK != res) {
        return res;
    }
    HJLogi("entry url:" + m_url->getUrl());
    do {
        //
        int flags = HJXIOContext::xioToAvioMode(m_url->getMode());
        AVDictionary* opts = NULL;
        if (url->haveValue("multiple_requests")) {
            av_dict_set(&opts, "multiple_requests", "1", 0);
        }
        int ret = avio_open2(&m_avio, m_url->getUrl().c_str(), flags, m_interruptCB, &opts);
        if (opts) {
            av_dict_free(&opts);
        }
        if (ret < 0) {
            res = HJErrIOOpen;
            break;
        }
        m_size = size();
    } while (false);
    
    return res;
}

int HJXIOContext::open(AVIOContext* avio)
{
    if (!avio) {
        return HJErrInvalidParams;
    }
    HJLogi("entry avio");
    m_avio = avio;
    m_size = size();
    
    return HJ_OK;
}

void HJXIOContext::close()
{
    m_isQuit = true;
    avio_closep(&m_avio);
}

int HJXIOContext::read(void* buffer, size_t cnt)
{
    if(!m_avio) {
        return HJErrIORead;
    }
    return avio_read(m_avio, (unsigned char *)buffer, (int)cnt);
}

int HJXIOContext::write(const void* buffer, size_t cnt)
{
    if(!m_avio) {
        return HJErrIOWrite;
    }
    avio_write(m_avio, (const unsigned char *)buffer, (int)cnt);
    
    return (int)cnt;
}

int HJXIOContext::seek(int64_t offset, int whence)
{
    if (m_avio) {
        int64_t ret = avio_seek(m_avio, offset, whence);
        if(ret != offset) {
            return HJErrIOSeek;
        }
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
        return (avio_feof(m_avio) || (tell() >= m_size)) ? true : false;
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
    if (m_avio) {
        return ff_get_line(m_avio, (char*)buffer, (int)maxlen);
    }
    return 0;
}

int HJXIOContext::getChompLine(void* buffer, size_t maxlen)
{
    if (m_avio) {
        return ff_get_chomp_line(m_avio, (char *)buffer, (int)maxlen);
    }
    return 0;
}


NS_HJ_END
