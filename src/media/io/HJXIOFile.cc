//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJXIOFile.h"

NS_HJ_BEGIN
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
	const char* path = m_url->getUrl().c_str();
	std::ios::openmode mode = m_url->getMode();
    if (HJXFMode_CREATE == mode || HJXFMode_WONLY == mode) {
//#if !defined(HJ_OS_DARWIN)
//        mode |= std::ios::_Noreplace;
//#endif
    }
	m_file->open(path, mode);
	if (!m_file->is_open()) {
		HJLoge("error, file:" + m_url->getUrl() + " maybe not exist");
		return HJErrNotFind;
	}
    checkState();
	seek(0, std::ios::end);
    checkState();
	m_size = tell();
    checkState();
	seek(0, std::ios::beg);

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
	int rdcnt = 0;
	m_file->read((char *)buffer, cnt);
	rdcnt = (int)m_file->gcount();
	if (!m_file->good()) {
		rdcnt = 0;
	}
	return rdcnt;
}

int HJXIOFile::write(const void* buffer, size_t cnt)
{
	if (!m_file || !buffer) {
		return HJErrIOWrite;
	}
	m_file->write((const char*)buffer, cnt);
	if (!m_file->good()){
		cnt = 0;
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
	if (mode == HJXFMode_WONLY) {
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
	if (mode == HJXFMode_WONLY) {
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
    int res = HJ_OK;
    if (m_file->fail()) {
        //std::ios::iostate stat = m_file->rdstate();
        if(m_file->fail() || m_file->bad()) {
            HJLoge("checkState error, stat:" + HJ2STR(m_file->rdstate()));
            res = HJErrIOFail;
        }
        m_file->clear();
    }
    return HJ_OK;
}

bool HJXIOFile::eof()
{
    return (m_file->eof() || (tell() >= m_size) ) ? true : false;
}

NS_HJ_END
