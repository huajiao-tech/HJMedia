//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJLocalFileManager.h"
#include "HJMediaDBManager.h"
#include "HJBlockData.h"
#include "HJFileUtil.h"
#include "HJFLog.h"
#include <filesystem>

namespace fs = std::filesystem;

NS_HJ_BEGIN
//***********************************************************************************//
HJLocalFileManager::HJLocalFileManager(std::shared_ptr<HJMediaDBManager> db_manager, 
                                       const std::string& media_dir)
    : m_dbManager(std::move(db_manager))
{
    if (!m_dbManager) {
        HJFLogw("HJLocalFileManager: db_manager is null");
    }
}

HJLocalFileManager::~HJLocalFileManager() = default;

bool HJLocalFileManager::isFileCompleted(const HJDBFileInfo& file_info)
{
    // 判断条件：status == COMPLETED (2) 或 isComplete() == true
    if (file_info.status == static_cast<int>(HJFileStatus::COMPLETED)) {
        return true;
    }
    return file_info.isComplete();
}

std::optional<HJDBFileInfo> HJLocalFileManager::getDBFileInternal(const std::string& rid, const std::string& category)
{
    if (!m_dbManager || rid.empty()) {
        HJFLogw("db_manager is null");
        return std::nullopt;
    }
    std::optional<HJDBFileInfo> db_info = std::nullopt;
    if(!category.empty()) 
    {
        db_info = m_dbManager->getFile(category, rid);
    } else {
        auto categorys = m_dbManager->getAllCategoryInfos();
        for (const auto& category : categorys) {
            db_info = m_dbManager->getFile(category.name, rid);
            if (db_info.has_value()) {
                break;
            }
        }
    }
    if (db_info.has_value()) {
        std::map<std::string, std::string> local_urls;
        for (const auto& span : db_info->spans) {
            if (!span.local_url.empty()) {
                local_urls.emplace(span.local_url, span.local_url);
            }
        }

        for (const auto& xurl : local_urls) {
            if (!HJFileUtil::isFileExist(xurl.second)) {
                return std::nullopt;
            }
        }
    }

    return db_info;
}

HJShareBlob::Ptr HJLocalFileManager::getAreadyShareBlob(const std::string& url, const std::string& dir, const std::string& rid)
{
    HJ_AUTOU_LOCK(m_mutex);
    if (!m_dbManager || url.empty()) {
        HJFLogw("checkDBFile: db_manager is null");
        return nullptr;
    }
    HJShareBlob::Ptr blob = nullptr;
    auto xinfo = HJXIOFileInfo(url, dir, rid);
    if (!HJFileUtil::isFileExist(xinfo.local_url)) {
        return nullptr;
    }
    auto it = m_shareBlobs.find(xinfo.rid);
    if(it != m_shareBlobs.end()) {
        blob = it->second.lock();  // 尝试从弱引用获取强引用
        if(!blob) {
            m_shareBlobs.erase(it);  // 清理已过期的弱引用
        }
    }
    if (!blob) {
        auto db_info_opt = getDBFileInternal(xinfo.rid, xinfo.category);
        if(db_info_opt.has_value() && db_info_opt->total_length > 0) 
        {
            HJFLogi("got db info, rid: {}, size:{}/{}", db_info_opt->rid, db_info_opt->size, db_info_opt->total_length);
            HJLocalFileManager::Wtr wself = HJSharedFromThis();
            blob = HJCreates<HJShareBlob>(db_info_opt.value(), [wself](const HJNotification::Ptr ntfy) -> int {
                HJLocalFileManager::Ptr tself = wself.lock();
                if (!tself) {
                    return HJ_OK;
                }
                return tself->procShareBlobNotify(ntfy);
            });
            int res = blob->init();
            if (HJ_OK != res) {
                HJFLogw("error, init share blob failed, url: {}", url);
                return nullptr;
            }
            m_shareBlobs[db_info_opt->rid] = blob;  // 存储弱引用
        }
    }
    if(blob) {
        // blob->setShare(true);
        HJFLogi("got share blob, rid: {}, shared blok size:{}", xinfo.rid, m_shareBlobs.size());
    }
    return blob;
}

HJShareBlob::Ptr HJLocalFileManager::getShareBlob(const std::string& url, const std::string& dir, const std::string& rid, int64_t fileLength)
{
    HJ_AUTOU_LOCK(m_mutex);
    if (!m_dbManager || url.empty()) {
        HJFLogw("checkDBFile: db_manager is null");
        return nullptr;
    }
    int res = HJ_OK;
    HJShareBlob::Ptr blob = nullptr;
    do
    {
        auto xinfo = HJXIOFileInfo(url, dir, rid);
        auto it = m_shareBlobs.find(xinfo.rid);
        if(it != m_shareBlobs.end()) {
            blob = it->second.lock();  // 尝试从弱引用获取强引用
            if(!blob) {
                m_shareBlobs.erase(it);  // 清理已过期的弱引用
            }
        }
        if(!blob) {
            auto db_info_opt = getDBFileInternal(xinfo.rid, xinfo.category);
            if(!db_info_opt.has_value() || db_info_opt->total_length != fileLength) {
                auto newDBInfo = HJDBFileInfo(xinfo.rid, xinfo.url, xinfo.local_url, static_cast<int64_t>(fileLength), static_cast<int>(HJBlockData::kBlockSize));
                res = updateFileInfoInternal(newDBInfo);
                if (res != HJ_OK) {
                    HJFLoge("error, update file info failed, url: {}", url);
                    return nullptr;
                }
                db_info_opt = newDBInfo;
            }

            HJLocalFileManager::Wtr wself = HJSharedFromThis();
            blob = HJCreates<HJShareBlob>(db_info_opt.value(), [wself](const HJNotification::Ptr ntfy) -> int {
                HJLocalFileManager::Ptr tself = wself.lock();
                if(!tself) {
                    return HJ_OK;
                }
                return tself->procShareBlobNotify(ntfy);
            });
            res = blob->init();
            if(HJ_OK != res) {
                HJFLogw("error, init share blob failed, url: {}", url);
                break;
            }
            m_shareBlobs[db_info_opt->rid] = blob;  // 存储弱引用
        }
        if(blob) {
            // blob->setShare(true);
            HJFLogi("got share blob, rid: {}, shared blok size:{}", xinfo.rid, m_shareBlobs.size());
        }
    } while (false);

    return blob;
}

int HJLocalFileManager::procShareBlobNotify(const HJNotification::Ptr& ntfy)
{
    HJFileStatus status = static_cast<HJFileStatus>(ntfy->getID());
    switch (status)
    {
    case HJFileStatus::NEW: {
        break;
    }
    case HJFileStatus::PENDING:
    case HJFileStatus::COMPLETED: {
        if (ntfy->haveValue("db_info")) {
            auto db_info = ntfy->getValue<HJDBFileInfo>("db_info");
            updateFileInfo(db_info);
            break;
        }
    }
    case HJFileStatus::FAILED:
    default:
        break;
    }

    return HJ_OK;
}

/**
 * @brief 1、文件完整  -- rid 为 url的hash值或者外部传入
 * @brief 2、文件不完整 -- url被rename, local_url -> tmp_url; 
 * @brief play checkDBFile - 1、play - 必须返回值: a、不存在(创建), b、dcache下载一部分不完整, 下载过程已经结束，c、 dcache下载正在进行中, d、下载完整 -> ok; 
 * @brief download/cache - dcache checkDBFile - 2、dcache：（player没有占用) -- 不存在、不完整 -> ok, 
 * @param url 文件 URL
 * @param rid 文件 rid
 * @param dir 文件目录
 * @return 文件信息
 * file & block info
 */
std::optional<HJXIOFileInfo> HJLocalFileManager::checkXIOFile(const std::string& url, const std::string& dir, const std::string& rid)
{
    HJ_AUTOU_LOCK(m_mutex);
    if (!m_dbManager || url.empty()) {
        HJFLogw("checkDBFile: db_manager is null");
        return std::nullopt;
    }
    int res = HJ_OK;
    auto xinfo = HJXIOFileInfo(url, dir, rid);
    auto file_length = HJFileUtil::fileSize(xinfo.local_url);
    //auto isExist = HJFileUtil::isFileExist(xinfo.local_url);
    //
    auto db_info_opt = getDBFileInternal(xinfo.rid, xinfo.category);
    //rid
    if(db_info_opt.has_value() && xinfo.local_url == db_info_opt->local_url && db_info_opt->total_length == file_length)
    {
        auto& db_info = db_info_opt.value();
        if (isFileCompleted(db_info)) {
            HJFLogi("file completed, rid: {}, url: {}", db_info.rid, db_info.url);
            return xinfo;
        }
        xinfo.tmp_url = HJMediaUtils::makeHJTDFile(xinfo.local_url, HJLocalFileManager::getGlobalID());
        xinfo.tmp_rid = HJMediaUtils::makeUrlRid(xinfo.tmp_url);
        //rename
        res = moveFileInternal(db_info, xinfo.tmp_rid, xinfo.tmp_url);
        if (res != HJ_OK) {
            HJFLoge("error, move file failed, rid: {}, url: {}", db_info.rid, db_info.url);
            return std::nullopt;
        }
        return xinfo;
    } else 
    {
        xinfo.tmp_url = HJMediaUtils::makeHJTDFile(xinfo.local_url, HJLocalFileManager::getGlobalID());
        xinfo.tmp_rid = HJMediaUtils::makeUrlRid(xinfo.tmp_url);
        //
        auto newDBInfo = HJDBFileInfo(xinfo.tmp_rid, xinfo.url, xinfo.tmp_url, static_cast<int64_t>(0), static_cast<int>(HJBlockData::kBlockSize));
                //
        res = updateFileInfoInternal(newDBInfo);
        if (res != HJ_OK) {
            HJFLoge("error, update file info failed, url: {}", url);
            return std::nullopt;
        }
        return xinfo;
    }
    return std::nullopt;
}

std::optional<HJDBFileInfo> HJLocalFileManager::getDBFile(const std::string& rid, const std::string& category)
{
    HJ_AUTOU_LOCK(m_mutex);
    return getDBFileInternal(rid, category);
}

std::optional<HJDBFileInfo> HJLocalFileManager::getCompletedFile(const std::string& rid, const std::string& category)
{
    HJ_AUTOU_LOCK(m_mutex);
    auto dbFile = getDBFileInternal(rid, category);
    if (!dbFile.has_value()) {
        return std::nullopt;
    }
    if (isFileCompleted(dbFile.value())) {
        return dbFile;
    }
    return std::nullopt;
}

int HJLocalFileManager::updateFileInfoInternal(const HJDBFileInfo& file_info)
{
    if (!m_dbManager) {
        HJFLogw("update file Info: db_manager is null");
        return HJErrNotAlready;
    }
    auto deleted_files = m_dbManager->addOrUpdateFile(file_info.category, file_info);
    if (!deleted_files.empty()) {
        cleanupDeletedFiles(deleted_files);
    }
    HJFLogi("end, category: {}, rid: {}, deleted_count: {}", file_info.category, file_info.rid, deleted_files.size());
    
    return HJ_OK;
}

int HJLocalFileManager::updateFileInfo(const HJDBFileInfo& file_info)
{
    HJ_AUTOU_LOCK(m_mutex);
    return updateFileInfoInternal(file_info);
}

int HJLocalFileManager::completedFileInternal(const std::string& hjtd_url, HJDBFileInfo& file_info)
{
    auto t0 = HJCurrentSteadyMS();
    auto local_url = file_info.local_url;
    if (!m_dbManager || local_url.empty()) {
        HJFLoge("error, db_manager or local_url is null");
        return HJErrNotAlready;
    }
    if (!HJFileUtil::rename(hjtd_url, local_url)) {
        HJFLoge("rename file failed, old url: {}, new url: {}", hjtd_url, local_url);
        return HJErrInvalidFile;
    }
    file_info.status = static_cast<int>(HJFileStatus::COMPLETED);
    file_info.size = file_info.total_length;
    file_info.modify_time = HJCurrentSteadyMS();

    auto deleted_files = m_dbManager->addOrUpdateFile(file_info.category, file_info);
    if (!deleted_files.empty()) {
        cleanupDeletedFiles(deleted_files);
    }
    HJFLogi("update File Info: category: {}, rid: {}, deleted_count: {}, time: {}", file_info.category, file_info.rid, deleted_files.size(), HJCurrentSteadyMS() - t0);
    
    return HJ_OK;
}

int HJLocalFileManager::completedFile(const std::string& hjtd_url, HJDBFileInfo& file_info)
{
    HJ_AUTOU_LOCK(m_mutex);
    return completedFileInternal(hjtd_url, file_info);
}

int HJLocalFileManager::completedFile(const std::string& hjtd_url, const std::string& rid)
{
    HJ_AUTOU_LOCK(m_mutex);

    if (!m_dbManager || hjtd_url.empty()) {
        HJFLoge("error, db_manager or local_url is null");
        return HJErrNotAlready;
    }
    auto parent_dir = HJFileUtil::parentDir(hjtd_url);
    auto category = HJDBCategoryInfo::makeCategoryName(parent_dir);

    auto file_info = m_dbManager->getFile(category, rid);
    if (!file_info.has_value()) {
        HJFLoge("error, file not found, category: {}, rid: {}", category, rid);
        return HJErrNotAlready;
    }
    return completedFileInternal(hjtd_url, file_info.value());
}

int HJLocalFileManager::incFileUseCount(const std::string& rid, const std::string& local_url)
{
    HJ_AUTOU_LOCK(m_mutex);
    
    if (!m_dbManager || rid.empty() || local_url.empty()) {
        HJFLogw("incFileUseCount: db_manager or rid is null, rid: {}, local_url: {}", rid, local_url);
        return HJErrNotAlready;
    }
    
    auto parent_dir = HJFileUtil::parentDir(local_url);
    auto category = HJDBCategoryInfo::makeCategoryName(parent_dir);

    return m_dbManager->incFileUseCount(category, rid);
}

int HJLocalFileManager::moveFileInternal(const HJDBFileInfo& db_info, const std::string& new_rid, const std::string& new_local_url)
{
    if (!m_dbManager) {
        HJFLogw("warning, db_manager is null");
        return HJErrNotAlready;
    }

    auto isExist = HJFileUtil::isFileExist(db_info.local_url);
    if(db_info.size > 0 && !isExist) {
        HJFLogw("warning, source file not exist or size is zero: {}, size: {}", db_info.local_url, db_info.size);
        return HJErrInvalidFile;
    }

    if(isExist) {
        HJFileUtil::rename(db_info.local_url, new_local_url);
    }

    if(!m_dbManager->updateFileInfo(db_info.category, db_info.rid, new_rid, new_local_url)) {
        HJFLoge("error, failed to update db info, category: {}, old_rid: {}, new_rid: {}", 
                db_info.category,  db_info.rid, new_rid);
        return HJErrInvalidFile;
    }
    return HJ_OK;
}

int HJLocalFileManager::moveFile(const std::string& category, const std::string& old_rid, const std::string& old_local_url, const std::string& new_rid, const std::string& new_local_url)
{
    HJ_AUTOU_LOCK(m_mutex);
    if (!m_dbManager) {
        HJFLogw("warning, db_manager is null");
        return HJErrNotAlready;
    }
    auto isExist = HJFileUtil::isFileExist(old_local_url);
    if(!isExist) {
        HJFLogw("warning, source file not exist or size is zero: {}", old_local_url);
        return HJErrInvalidFile;
    }
    HJFileUtil::rename(old_local_url, new_local_url);

    if(!m_dbManager->updateFileInfo(category, old_rid, new_rid, new_local_url)) {
        HJFLoge("error, failed to update db info, category: {}, old_rid: {}, new_rid: {}", 
                category,  old_rid, new_rid);
        return HJErrInvalidFile;
    }
    return HJ_OK;
}

bool HJLocalFileManager::renameFile(const std::string& category, 
                                     const std::string& rid, 
                                     const std::string& new_local_url)
{
    HJ_AUTOU_LOCK(m_mutex);
    
    if (!m_dbManager) {
        HJFLogw("warning, db_manager is null");
        return false;
    }
    
    // 获取当前文件信息
    auto file_info = m_dbManager->getFile(category, rid);
    if (!file_info.has_value()) {
        HJFLogw("warning, file not found, category: {}, rid: {}", category, rid);
        return false;
    }
    
    const std::string old_path = file_info->local_url;
    const std::string new_path = new_local_url;
    
    // 检查源文件是否存在
    if (!HJFileUtil::isFileExist(old_path)) {
        HJFLogw("warning, source file not exist: {}", old_path);
        return false;
    }
    
    // 检查目标文件是否已存在
    if (HJFileUtil::isFileExist(new_path)) {
        HJFLogw("warning, target file already exists: {}", new_path);
        return false;
    }
    
    // 确保目标目录存在
    const std::string new_parent = HJFileUtil::parentDir(new_path);
    if (!new_parent.empty() && !HJFileUtil::isDirExist(new_parent)) {
        if (!HJFileUtil::makeDir(new_parent)) {
            HJFLoge("error, failed to create target directory: {}", new_parent);
            return false;
        }
    }
    
    // 重命名物理文件
    try {
        fs::rename(old_path, new_path);
    } catch (const fs::filesystem_error& e) {
        HJFLoge("error, failed to rename file: {} -> {}, error: {}", 
                old_path, new_path, e.what());
        return false;
    }
    
    // 更新数据库中的 local_url
    HJDBFileInfo updated_info = file_info.value();
    updated_info.local_url = new_local_url;
    updated_info.modify_time = HJCurrentSteadyMS();
    
    auto deleted_files = m_dbManager->addOrUpdateFile(category, updated_info);
    if (!deleted_files.empty()) {
        cleanupDeletedFiles(deleted_files);
    }
    
    HJFLogi("success, {} -> {}", old_path, new_path);
    return true;
}

void HJLocalFileManager::cleanupDeletedFiles(const std::vector<HJDBFileInfo>& files_to_delete)
{
    for (const auto& file : files_to_delete) {
        const std::string local_url = file.local_url; //HJUtilitys::concatenatePath(m_media_dir, file.local_url);
        
        if (HJFileUtil::isFileExist(local_url)) {
            int result = HJFileUtil::removeFile(local_url);
            if (result >= 0) {
                HJFLogi("deleted file: {}", local_url);
            } else {
                HJFLogw("failed to delete file: {}", local_url);
            }
        } else {
            HJFLogd("file not exist, skip: {}", local_url);
        }
    }
}

const size_t HJLocalFileManager::getGlobalID()
{
    static std::atomic<size_t> g_fileIDCounter(0);
    return g_fileIDCounter.fetch_add(1, std::memory_order_relaxed);
}

NS_HJ_END
