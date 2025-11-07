//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJXIOFile.h"

NS_HJ_BEGIN
const std::map<int, int> HJXIOFile::XIO_FILE_MODE_MAPS = {
	{HJ_XIO_READ, HJXF_MODE_RONLY},
	{HJ_XIO_WRITE, HJXF_MODE_WONLY},
	{HJ_XIO_RW, HJXF_MODE_RW}
};
//***********************************************************************************//
HJXIOFile::HJXIOFile()
{
}

HJXIOFile::~HJXIOFile()
{
    close();
}

int HJXIOFile::open(HJUrl::Ptr url)
{
	if (!url || url->getUrl().empty()) {
		return HJErrInvalidParams;
	}
	int res = HJXIOBase::open(url);
	if (HJ_OK != res) {
		return res;
	}
	m_file = std::make_unique<HJFStream>();
	m_size = 0;
	const char* path = m_url->getUrl().c_str();
	std::ios::openmode fmode = xioToFMode(m_url->getMode());
	m_file->open(path, fmode);
	if (!m_file->is_open()) {
		HJLoge("error, file:" + m_url->getUrl() + " maybe not exist");
		return HJErrNotFind;
	}

	int mode = m_url->getMode();
	if (mode == HJ_XIO_READ || mode == HJ_XIO_RW) {
		if (HJ_OK != seek(0, std::ios::end)) {
			HJLoge("seek to end failed.");
			return HJErrIOSeek;
		}
		m_size = tell();
		if (HJ_OK != seek(0, std::ios::beg)) {
			HJLoge("seek to beg failed.");
			return HJErrIOSeek;
		}
	}

	return res;
}

void HJXIOFile::close()
{
	if(m_file) {
		m_file->close();
		m_file = nullptr;
	}
}

int HJXIOFile::read(void* buffer, size_t cnt)
{
	if (!m_file || !buffer) {
		return HJErrIORead;
	}
	m_file->read((char *)buffer, cnt);
	return (int)m_file->gcount();
}

int HJXIOFile::write(const void* buffer, size_t cnt)
{
	if (!m_file || !buffer) {
		return HJErrIOWrite;
	}
	m_file->write((const char*)buffer, cnt);
	if (!m_file->good()){
		return HJErrIOWrite;
	}
    int64_t pos = tell();
    if (pos > m_size) {
        m_size = pos;
    }
	return (int)cnt;
}

int HJXIOFile::seek(int64_t offset, int whence)
{
	if (!m_file) {
		return HJErrIOSeek;
	}
    std::ios::openmode mode = m_url->getMode();
    std::ios::seekdir dir = (std::ios::seekdir)whence;
    checkState();
	if (mode == HJXF_MODE_WONLY) {
		m_file->seekp(offset, dir);
	} else {
		m_file->seekg(offset, dir);
	}
    int ret = checkState();
    if (HJ_OK != ret) {
        return HJErrIOSeek;
    }
	return HJ_OK;
}

int HJXIOFile::flush()
{
	if (!m_file) {
		return HJErrNotAlready;
	}
	m_file->flush();

	return HJ_OK;
}

int64_t HJXIOFile::tell()
{
	if (!m_file) {
		return 0;
	}
	size_t lar = 0;
	std::ios::openmode mode = m_url->getMode();
    checkState();
	if (mode == HJXF_MODE_WONLY) {
		lar = m_file->tellp();
	}else {
		lar = m_file->tellg();
	}
	return lar;
}

int64_t HJXIOFile::size()
{
	return m_size;
}

int HJXIOFile::checkState()
{
    if (m_file->fail() || m_file->bad()) {
        HJLoge("checkState error, stat:" + HJ2STR(m_file->rdstate()));
        m_file->clear();
        return HJErrIOFail;
    }
    return HJ_OK;
}

bool HJXIOFile::eof()
{
    return (m_file->eof() || (tell() >= m_size) ) ? true : false;
}

int HJXIOFile::xioToFMode(int mode)
{
	int flags = HJXF_MODE_RW;
	auto it = XIO_FILE_MODE_MAPS.find(mode);
	if (it != XIO_FILE_MODE_MAPS.end()) {
		flags = it->second;
	}
	return flags;
}

NS_HJ_END
