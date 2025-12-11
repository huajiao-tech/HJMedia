//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR: 
//CREATE TIME: 
//***********************************************************************************//
#include "HJServerUtils.h"
#include "HJFLog.h"
#include "HJMediaStorageManager.h"
#include "HJMediaUtils.h"

NS_HJ_BEGIN
//***********************************************************************************//
std::string HJServerUtils::getCategory(int priority)
{
    switch (priority) {
        case 0: {
            return HJ_DB_TABLE_GIFTS_NAME;
        }
        case 1: {
            return HJ_DB_TABLE_MEDIAS_NAME;
        }
    }
    return HJFMT("media_{}", priority);
}

std::pair<std::string, std::string> HJServerUtils::makeLocalUrl(const std::string& url, const std::string& rid, const std::string& cache_dir)
{
    auto suffix = HJMediaUtils::checkMediaSuffix(HJUtilitys::getSuffix(url));
    std::string name = rid.empty() ? HJ2STR(HJUtilitys::hash(url)) : rid;
	auto localUrl = HJUtilitys::concatenatePath(cache_dir, HJFMT("{}{}", name, suffix));
    auto blockUrl = HJUtilitys::concatenatePath(cache_dir, HJFMT("{}{}", name, ".blocks"));
    return std::make_pair(localUrl, blockUrl);
}

NS_HJ_END