//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR: 
//CREATE TIME: 
//***********************************************************************************//
#include "HJLocalIOUtils.h"
#include "HJFLog.h"
#include "HJMediaUtils.h"
#include "HJMediaDBUtils.h"
#include "HJExecutor.h"

NS_HJ_BEGIN
//***********************************************************************************//
HJXIOFileInfo::HJXIOFileInfo(const std::string& i_url, const std::string& i_dir, const std::string& i_rid)
    : url(i_url)
{
    rid = i_rid.empty() ? HJMediaUtils::makeUrlRid(url) : i_rid;
    local_url = HJMediaUtils::makeLocalUrl(i_dir, url, rid);
    if(!i_dir.empty()) {
        category = HJDBCategoryInfo::makeCategoryName(i_dir);
    }
    priority = HJ_PRIORITY_NORMAL;
}

HJXIOFileInfo::HJXIOFileInfo(const std::string& i_url, const std::string& i_dir, const std::string& i_rid, const std::string& i_category, int i_preCacheSize, int i_priority)
    : url(i_url)
    , category(i_category)
    , preCacheSize(i_preCacheSize)
    , priority(i_priority)

{
    rid = i_rid.empty() ? HJMediaUtils::makeUrlRid(url) : i_rid;
    local_url = HJMediaUtils::makeLocalUrl(i_dir, url, rid);
}

// HJXIOFileInfo HJLocalIOUtils::makeXIOFileInfo(const std::string& url, const std::string& dir, const std::string& rid)
// {
//     std::string local_dir = dir;
//     HJXIOFileInfo info;
//     info.url = url;
//     info.rid = rid.empty() ? HJMediaUtils::makeUrlRid(url) : rid;
//     info.local_url = HJMediaUtils::makeLocalUrl(local_dir, url, info.rid);
//     if(!local_dir.empty()) {
//         info.category = HJDBCategoryInfo::makeCategoryName(local_dir);
//     }
//     info.priority = HJ_PRIORITY_NORMAL;
//     return info;
// }

NS_HJ_END