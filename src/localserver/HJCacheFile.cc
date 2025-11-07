//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR: 
//CREATE TIME: 
//***********************************************************************************//
#include "HJCacheFile.h"
#include "HJFLog.h"
#include "HJException.h"
#include "HJXIOFile.h"
#include "HJFileUtil.h"
#include "HJMediaUtils.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJCacheFile::HJCacheFile()
{
	
}

HJCacheFile::~HJCacheFile()
{
	close();
}

/**
 * @brief 
 * 
 * @param url 
 * @param rid
 * @param cacheDir 
 * @param preCacheSize
 * @return int 
 */
int HJCacheFile::open(HJUrl::Ptr url)
{
	if (!url || url->getUrl().empty()) {
		return HJErrInvalidParams;
	}
    int res = HJXIOBase::open(url);
	if (HJ_OK != res) {
		return res;
	}
    getOptions();
    //
    auto suffix = HJMediaUtils::checkMediaSuffix(HJUtilitys::getSuffix(url->getUrl()));
    auto rid = m_rid.empty() ? HJ2STR(HJUtilitys::hash(url)) : m_rid;
	auto localUrl = HJUtilitys::concatenatePath(m_cacheDir, HJFMT("{}{}", rid, suffix));
    auto blockUrl = HJUtilitys::concatenatePath(m_cacheDir, HJFMT("{}{}", rid, ".blocks"));

    return HJ_OK;
}
void HJCacheFile::close()
{

}
int HJCacheFile::read(void* buffer, size_t cnt)
{
    return HJ_OK;
}
int HJCacheFile::write(const void* buffer, size_t cnt)
{
    return HJ_OK;
}
int HJCacheFile::seek(int64_t offset, int whence)
{
    return HJ_OK;
}
int HJCacheFile::flush()
{
    return HJ_OK;
}
int64_t HJCacheFile::tell()
{
    return 0;
}
int64_t HJCacheFile::size()
{
    return 0;
}
bool HJCacheFile::eof()
{
    return false;
}

void HJCacheFile::getOptions()
{
    m_rid = m_url->getString("rid");
    m_cacheDir = m_url->getString("cacheDir");
    m_preCacheSize = m_url->getInt("preCacheSize");
}


NS_HJ_END