//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJMediaDBManager.h"
#include "HJException.h"
#include "HJFLog.h"
#include "HJFileUtil.h"
#include <filesystem>
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <string_view>

namespace fs = std::filesystem;
using namespace sqlite_orm;

NS_HJ_BEGIN
namespace {
const char* kPragmaCacheSize = "PRAGMA cache_size = -2560;";
const char* kPragmaTempStore = "PRAGMA temp_store = MEMORY;";

template <typename Storage>
void configureStorage(Storage& storage) {
    storage.on_open = [](sqlite3* db) {
        sqlite3_exec(db, kPragmaCacheSize, nullptr, nullptr, nullptr);
        sqlite3_exec(db, kPragmaTempStore, nullptr, nullptr, nullptr);
    };
}

template <typename Storage, typename Func>
void runStorageTransaction(Storage& storage, Func&& func) {
    storage.begin_transaction();
    try {
        func();
        storage.commit();
    } catch (...) {
        try {
            storage.rollback();
        } catch (...) {
        }
        throw;
    }
}
}  // namespace
//***********************************************************************************//
HJMediaDBManager::HJMediaDBManager(std::string db_file)
    : m_db_file(std::move(db_file))
{
    auto t0 = HJCurrentSteadyMS();
    try {
        m_categoryTable = std::make_unique<HJCategoryTable>(CreateCategoryTable(m_db_file));
        m_categoryTable->sync_schema();

        // PRAGMA 优化：提升写入性能
        // WAL 模式：读写并发更好，写入性能更高
        m_categoryTable->pragma.journal_mode(journal_mode::WAL);
        // NORMAL 模式：应用崩溃安全，掉电可能丢最近事务（权衡安全性和性能）
        m_categoryTable->pragma.synchronous(static_cast<int>(m_sync_mode));  // 0=OFF(最快), 1=NORMAL, 2=FULL(默认)

        int res = init();
        if (HJ_OK != res) {
            HJ_EXCEPT(HJException::ERR_INVALID_STATE, HJFMT("Error init db: {}, file:{}", res, m_db_file));
        }
    } catch (const HJException& e) {
        HJ_EXCEPT(HJException::ERR_INVALID_STATE, HJFMT("Error create db: {}, file:{}", e.what(), m_db_file));
    }
    HJFLogi("init storage manager cost: {} ms", HJCurrentSteadyMS() - t0);
}

HJMediaDBManager::~HJMediaDBManager() = default;

int HJMediaDBManager::init() 
{
    auto t0 = HJCurrentSteadyMS();
    HJ_AUTOU_LOCK(m_mutex);
    if (m_initialized) {
        return HJ_OK;
    }
    auto categoryInfos = m_categoryTable->get_all<HJDBCategoryInfo>();
    for (const auto& info : categoryInfos) {
        if (!isValidCategoryName(info.name)) {
            HJ_EXCEPT(HJException::ERR_INVALIDPARAMS, HJFMT("Invalid category name: {}", info.name));
        }
        m_categoryInfos[info.name] = info;
        // Create file table if not exists
        if (!getFileTable(info.name)) {
            int res = addCategoryTableInternal(info.name);
            if (HJ_OK != res) {
                HJ_EXCEPT(HJException::ERR_INVALID_STATE, HJFMT("Error create folder table: {}", info.name));
            }
        }
        //
        auto fileInfos = getFilesInCategoryInternal(info.name);
        for (const auto& file : fileInfos) {
            std::string file_path = file.local_url; //HJUtilitys::concatenatePath(m_media_dir, file.local_url);
            if (!HJFileUtil::isFileExist(file_path)) {
                //HJ_EXCEPT(HJException::ERR_FILE_NOT_FOUND, HJFMT("Validation Error - File: {} in folder:{} exists in DB but not on disk.", file.local_url, info.name));
                HJFLogw("warning - File: {} in folder:{} exists in DB but not on disk.", file.local_url, info.name);
            }
        }
        syncCategoryStatsInternal(info.name);
    }
    HJFLogi("init storage manager success. cost: {}", HJCurrentSteadyMS() - t0);

    m_initialized = true;
    return HJ_OK;
}

bool HJMediaDBManager::addOrUpdateCategoryInternal(const HJDBCategoryInfo& categoryInfo) 
{
    if (!isValidCategoryName(categoryInfo.name)) {
        HJFLogw("addOrUpdateCategory invalid name: {}", categoryInfo.name);
        return false;
    }
    auto t0 = HJCurrentSteadyMS();
    try
    {
        auto it = m_categoryInfos.find(categoryInfo.name);
        bool is_new = (it == m_categoryInfos.end());
        bool need_update = is_new || it->second != categoryInfo;
        HJDBCategoryInfo old_info;
        if (!is_new) {
            old_info = it->second;
        }
        
        // Update or insert category info if changed or new
        if (need_update) {
            m_categoryTable->replace(categoryInfo);
            m_categoryInfos[categoryInfo.name] = categoryInfo;
            HJFLogi("update folder: {}, type: {}, max_size: {}, file_count: {}, total_size: {} success.", 
                    categoryInfo.name, categoryInfo.type, categoryInfo.max_size, 
                    categoryInfo.file_count, categoryInfo.total_size);
        }
        //
        if(!getFileTable(categoryInfo.name)) 
        {
            int res = addCategoryTableInternal(categoryInfo.name);
            if(HJ_OK != res) {
                if (is_new) {
                    m_categoryTable->remove<HJDBCategoryInfo>(categoryInfo.name);
                    m_categoryInfos.erase(categoryInfo.name);
                } else if (need_update) {
                    m_categoryTable->replace(old_info);
                    m_categoryInfos[categoryInfo.name] = old_info;
                }
                return false;
            }
            HJFLogi("create category file table: {}, {} success.", categoryInfo.name, categoryInfo.local_dir);
        }
        HJFLogi("end, cost: {}", HJCurrentSteadyMS() - t0);
        return true;
    } catch(const HJException& e) {
        HJFLogw("addCategory exception: {}", e.what()); 
    }
    return false;
}

bool HJMediaDBManager::addOrUpdateCategory(const HJDBCategoryInfo& categoryInfo) 
{
    HJ_AUTOU_LOCK(m_mutex);
    return addOrUpdateCategoryInternal(categoryInfo);
}

bool HJMediaDBManager::deleteCategory(const std::string& name) {
    HJ_AUTOU_LOCK(m_mutex);
    if (!isValidCategoryName(name)) {
        HJFLogw("deleteCategory invalid name: {}", name);
        return false;
    }
    auto t0 = HJCurrentSteadyMS();
    try
    {
        if (!m_categoryInfos.count(name)) {
            return false;
        }
        m_categoryTable->transaction([&]() {
            m_categoryTable->remove<HJDBCategoryInfo>(name);
            return true;
        });

        m_categoryInfos.erase(name);
        //
        int res = deleteCategoryTableInternal(name);
        if(HJ_OK != res) {
            return false;
        }
        HJFLogi("delete category: {} success, cost:{}.", name, HJCurrentSteadyMS() - t0);
        return true;
    } catch(const HJException& e) {
        HJFLogw("deleteCategory exception: {}", e.what());
    }
    return false;
}

int HJMediaDBManager::addCategoryTableInternal(const std::string& name)
{
    // NOTE: Caller must hold m_mutex
    if (!isValidCategoryName(name)) {
        HJFLogw("addCategoryTable invalid name: {}", name);
        return HJErrInvalidParams;
    }
    auto t0 = HJCurrentSteadyMS();
    try
    {
        auto storage = std::make_unique<HJFileTable>(CreateFileTable(m_db_file, name));
        storage->sync_schema();
        storage->pragma.journal_mode(journal_mode::WAL);
        storage->pragma.synchronous(static_cast<int>(m_sync_mode));
        m_fileTables[name] = std::move(storage);
        HJFLogi("end, cost:{} ms", HJCurrentSteadyMS() - t0);
        return HJ_OK;
    } catch(const HJException& e) {
        HJFLogw("Error create folder table: {}, {}", e.what(), name);
    }
    return HJErrNewObj;
}

int HJMediaDBManager::addCategoryTable(const std::string& name)
{
    HJ_AUTOU_LOCK(m_mutex);
    return addCategoryTableInternal(name);
}

int HJMediaDBManager::deleteCategoryTableInternal(const std::string& name)
{
    // NOTE: Caller must hold m_mutex
    if (!isValidCategoryName(name)) {
        HJFLogw("deleteCategoryTable invalid name: {}", name);
        return HJErrInvalidParams;
    }
    auto t0 = HJCurrentSteadyMS();
    try
    {
        auto storage = getFileTable(name);
        if(storage){
            storage->remove_all<HJDBFileInfo>();
            //
            m_fileTables.erase(name);
        }
        HJFLogi("end, cost:{} ms", HJCurrentSteadyMS() - t0);
        return HJ_OK;
    } catch(const HJException& e) {
        HJFLogw("Error delete folder table: {}, {}", e.what(), name);
    }
    return HJErrFatal;
}

int HJMediaDBManager::deleteCategoryTable(const std::string& name)
{
    HJ_AUTOU_LOCK(m_mutex);
    return deleteCategoryTableInternal(name);
}

std::vector<HJDBFileInfo> HJMediaDBManager::addOrUpdateFile(const std::string& name, const HJDBFileInfo& file_info) 
{
    HJ_AUTOU_LOCK(m_mutex);
    if (!isValidCategoryName(name)) {
        HJFLogw("addOrUpdateFile invalid name: {}", name);
        return {};
    }
    if (!m_categoryInfos.count(name)) {
        // HJ_EXCEPT(HJException::ERR_INVALID_STATE, HJFMT("Error, folder:{} does not exist in DB", name));
        return {};
    }
    if (file_info.category.empty() || file_info.category != name || !file_info.block_status.size()) {
        HJFLogw("addOrUpdateFile category mismatch, name: {}, file_category: {}", name, file_info.category);
        return {};
    }
    auto t0 = HJCurrentSteadyMS();
    try
    {
        auto storage = getFileTable(name);
        if (!storage) {
            HJ_EXCEPT(HJException::ERR_INVALID_STATE, HJFMT("Error, folder:{} storage is null", name));
        }
        HJDBFileInfo file_record = file_info;
        if (file_record.category.empty()) {
            file_record.category = name;
        }
        storage->replace(file_record);
        auto t1 = HJCurrentSteadyMS();

        // 检测并执行清理
        auto info = enforceCategorySizeLimit(name);

        HJFLogi("end, cost: {} / {} ms", t1 - t0,  HJCurrentSteadyMS() - t0);
        // HJFLogi("time:{}/{}, url: {}, local_url: {}, size: {}, create_time: {}, modify_time: {}, use_count: {}", t1 - t0, HJCurrentSteadyMS() - t0, file_record.url, file_record.local_url, file_record.size, file_record.create_time, file_record.modify_time, file_record.use_count);
        return info;
    } catch(const HJException& e) {
        HJ_EXCEPT(HJException::ERR_INVALID_STATE, HJFMT("Error, folder:{} does not exist in DB", name));
    }
    return {};
}

bool HJMediaDBManager::deleteFile(const std::string& category, const std::string& rid) {
    HJ_AUTOU_LOCK(m_mutex);
    if (!isValidCategoryName(category)) {
        HJFLogw("deleteFile invalid category: {}", category);
        return false;
    }
    if (!m_categoryInfos.count(category)) {
        return false;
    }
    auto t0 = HJCurrentSteadyMS();
    try
    {
        auto storage = getFileTable(category);
        if (!storage) {
            HJ_EXCEPT(HJException::ERR_INVALID_STATE, HJFMT("Error, folder:{} storage is null", category));
        }
        auto file_to_delete = storage->get_pointer<HJDBFileInfo>(rid);
        if (!file_to_delete) {
            return false;
        }
        storage->remove<HJDBFileInfo>(rid);
        HJFLogi("Delete file: {}, size: {} success.", file_to_delete->url, file_to_delete->size);
        
        syncCategoryStatsInternal(category);
        HJFLogi("end, cost:{} ms", HJCurrentSteadyMS() - t0);
        return true;
    } catch(const HJException& e) {
        HJ_EXCEPT(HJException::ERR_INVALID_STATE, HJFMT("Error, folder:{} does not exist in DB", category));
    }   
    return false;
}

bool HJMediaDBManager::deleteFiles(const std::string& category, const std::vector<std::string>& rids) {
    HJ_AUTOU_LOCK(m_mutex);
    if (!isValidCategoryName(category)) {
        HJFLogw("deleteFiles invalid category: {}", category);
        return false;
    }
    if (!m_categoryInfos.count(category)) {
        return false;
    }
    if (rids.empty()) {
        return false;
    }
    auto t0 = HJCurrentSteadyMS();
    try
    {
        auto storage = getFileTable(category);
        if (!storage) {
            HJ_EXCEPT(HJException::ERR_INVALID_STATE, HJFMT("Error, folder:{} storage is null", category));
        }
        std::vector<std::string_view> rid_views;
        rid_views.reserve(rids.size());
        for (const auto& rid : rids) {
            if (!rid.empty()) {
                rid_views.emplace_back(rid);
            }
        }
        if (rid_views.empty()) {
            return false;
        }
        runStorageTransaction(*storage, [&]() {
            storage->remove_all<HJDBFileInfo>(where(in(&HJDBFileInfo::rid, rid_views)));
        });
        syncCategoryStatsInternal(category);
        HJFLogi("deleteFiles end, count: {}, cost:{} ms", rid_views.size(), HJCurrentSteadyMS() - t0);
        return true;
    } catch(const HJException& e) {
        HJFLogw("deleteFiles exception: {}", e.what());
    }   
    return false;
}

std::optional<HJDBFileInfo> HJMediaDBManager::getFile(const std::string& category, const std::string& rid)
{
    HJ_AUTOS_LOCK(m_mutex);
    if (!isValidCategoryName(category)) {
        HJFLogw("getFile invalid name: {}", category);
        return {};
    }
    if (!m_categoryInfos.count(category)) {
        return {};
    }
    auto t0 = HJCurrentSteadyMS();
    try
    {
        auto storage = getFileTable(category);
        if (!storage) {
            HJ_EXCEPT(HJException::ERR_INVALID_STATE, HJFMT("Error, folder:{} storage is null", category));
        }
        auto file = storage->get_pointer<HJDBFileInfo>(rid);
        if (!file) {
            return {};
        }
        HJFLogi("end, cost:{} ms", HJCurrentSteadyMS() - t0);
        return *file;
    } catch(const HJException& e) {
        HJ_EXCEPT(HJException::ERR_INVALID_STATE, HJFMT("Error, folder:{} does not exist in DB", category));
    }
    return {};
}

int HJMediaDBManager::incFileUseCount(const std::string& category, const std::string& rid)
{
    HJ_AUTOU_LOCK(m_mutex);
    if (!isValidCategoryName(category)) {
        HJFLogw("incFileUseCount invalid name: {}", category);
        return HJErrInvalidParams;
    }
    auto t0 = HJCurrentSteadyMS();
    try
    { 
        auto storage = getFileTable(category);
        if (!storage) {
            HJ_EXCEPT(HJException::ERR_INVALID_STATE, HJFMT("Error, folder:{} storage is null", category));
        }
        auto file = storage->get_pointer<HJDBFileInfo>(rid);
        if (!file) {
            return HJErrInvalidParams;
        }
        file->use_count += 1;
        storage->replace(*file);
        HJFLogi("incFileUseCount cost: {}ms, file: {}, use_count: {} success.", HJCurrentSteadyMS() - t0, file->url, file->use_count);
        return HJ_OK;
    } catch(const HJException& e) {
        HJ_EXCEPT(HJException::ERR_INVALID_STATE, HJFMT("Error, folder:{} does not exist in DB", category));
    }
    return HJErrFatal;
}

std::vector<HJDBFileInfo> HJMediaDBManager::getFilesInCategoryInternal(const std::string& name) {
    // NOTE: Caller must hold m_mutex
    auto t0 = HJCurrentSteadyMS();
    try {
        auto storage = getFileTable(name);
        if (!m_categoryInfos.count(name) || !storage) {
            HJFLogw("Table '{}' does not exist.", name);
            return {};
        }
        auto files = storage->get_all<HJDBFileInfo>();
        HJFLogi("end, cost:{} ms", HJCurrentSteadyMS() - t0);
        return files;
    } catch (const HJException& e) {
        HJFLogw("Table '{}' does not exist or error occurred: {}", name, e.what());
    }
    return {};
}

std::vector<HJDBFileInfo> HJMediaDBManager::getFilesInCategory(const std::string& category) {
    HJ_AUTOS_LOCK(m_mutex)
    if (!isValidCategoryName(category)) {
        HJFLogw("getFilesInCategory invalid name: {}", category);
        return {};
    }
    return getFilesInCategoryInternal(category);
}

std::shared_ptr<HJDBCategoryInfo> HJMediaDBManager::getCategoryInfo(const std::string& category) {
    HJ_AUTOS_LOCK(m_mutex)
    if (!isValidCategoryName(category)) {
        HJFLogw("getCategoryInfo invalid name: {}", category);
        return nullptr;
    }
    if (m_categoryInfos.count(category)) {
        return std::make_shared<HJDBCategoryInfo>(m_categoryInfos.at(category));
    }
    return nullptr;
}

std::vector<HJDBCategoryInfo> HJMediaDBManager::getAllCategoryInfos() {
    HJ_AUTOS_LOCK(m_mutex)
    std::vector<HJDBCategoryInfo> result;
    for(const auto& pair : m_categoryInfos) {
        result.push_back(pair.second);
    }
    return result;
}

int HJMediaDBManager::updateCategoryMaxSize(const std::string& cache_dir, int64_t max_size)
{
    HJ_AUTOU_LOCK(m_mutex);
    auto category = HJDBCategoryInfo::makeCategoryName(cache_dir);
    if (!isValidCategoryName(category)) {
        HJFLogw("updateCategoryMaxSize invalid name: {}", category);
        return HJErrInvalidParams;
    }
    auto it = m_categoryInfos.find(category);
    if (it == m_categoryInfos.end()) {
        auto new_info = HJDBCategoryInfo::makeCategoryInfo(cache_dir, max_size, category);
        if(!addOrUpdateCategoryInternal(new_info)){
            return HJErrFatal;
        }
        return HJ_OK;
    }
    auto t0 = HJCurrentSteadyMS();
    try
    {
        if (it->second.max_size != max_size) {
            HJDBCategoryInfo new_info = it->second;
            new_info.max_size = max_size;
            
            runStorageTransaction(*m_categoryTable, [&]() {
                m_categoryTable->replace(new_info);
            });
            
            it->second = new_info;
            HJFLogi("updateCategoryMaxSize cost: {}ms, folder: {}, max_size: {} success.", HJCurrentSteadyMS() - t0, category, new_info.max_size);
        }
    } catch(const HJException& e) {
        HJ_EXCEPT(HJException::ERR_INVALID_STATE, HJFMT("Error update category max size: {}, {}", category, e.what()));
    }
    return HJ_OK;
}

bool HJMediaDBManager::isValidCategoryName(const std::string& name) const
{
    if (name.empty() || name.size() > kMaxCategoryNameLength) {
        return false;
    }
    for (char c : name) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (!(std::isalnum(uc) || c == '_')) {
            return false;
        }
    }
    return true;
}

void HJMediaDBManager::syncCategoryStatsInternal(const std::string& name)
{
    if (!m_categoryInfos.count(name)) {
        return;
    }
    auto storage = getFileTable(name);
    if (!storage) {
        HJ_EXCEPT(HJException::ERR_INVALID_STATE, HJFMT("Error, folder:{} storage is null", name));
    }
    try {
        int file_count = static_cast<int>(storage->count<HJDBFileInfo>());
        int64_t total_size = 0;
        auto total_size_ptr = storage->sum(&HJDBFileInfo::size);
        if (total_size_ptr) {
            total_size = static_cast<int64_t>(*total_size_ptr);
        }
        updateCategoryStats(name, file_count, total_size);
    } catch (const std::exception& e) {
        HJ_EXCEPT(HJException::ERR_INVALID_STATE, HJFMT("Error sync category stats: {}, {}", name, e.what()));
    }
}

std::vector<HJDBFileInfo> HJMediaDBManager::enforceCategorySizeLimit(const std::string& name) 
{
    // NOTE: Caller must hold m_mutex
    if (!m_categoryInfos.count(name)) {
        return {};
    }
    
    auto storage = getFileTable(name);
    if (!storage) {
        HJ_EXCEPT(HJException::ERR_INVALID_STATE, HJFMT("Error, folder:{} storage is null", name));
    }
    
    // 优化1: 使用 DB ORDER BY 替代内存排序，按 create_time 升序、use_count 升序排序
    auto all_files = storage->get_all<HJDBFileInfo>(
        multi_order_by(
            order_by(&HJDBFileInfo::create_time).asc(),
            order_by(&HJDBFileInfo::use_count).asc()
        )
    );
    
    // 优化2: 从获取的数据计算统计值，避免额外的 COUNT + SUM 查询
    int64_t total_size = 0;
    for (const auto& f : all_files) {
        total_size += f.size;
    }
    
    auto& categoryInfo = m_categoryInfos.at(name);
    
    // 检查是否需要清理
    if (total_size <= categoryInfo.max_size) {
        // 顺带更新缓存（如果有偏差）
        if (categoryInfo.file_count != static_cast<int>(all_files.size()) || 
            categoryInfo.total_size != total_size) {
            updateCategoryStats(name, static_cast<int>(all_files.size()), total_size);
        }
        return {};
    }
    
    // 优化3: 收集待删除的 rid，使用批量 DELETE 替代循环逐条删除
    std::vector<std::string> rids_to_delete;
    std::vector<HJDBFileInfo> deleted_files;
    int64_t current_total_size = total_size;
    
    for (const auto& file : all_files) {
        if (current_total_size <= categoryInfo.max_size) {
            break;
        }
        rids_to_delete.push_back(file.rid);
        deleted_files.push_back(file);
        current_total_size -= file.size;
    }
    
    // 批量删除
    if (!rids_to_delete.empty()) {
        auto t_batch = HJCurrentSteadyMS();
        runStorageTransaction(*storage, [&]() {
            storage->remove_all<HJDBFileInfo>(where(in(&HJDBFileInfo::rid, rids_to_delete)));
        });
        HJFLogi("batch remove {} files, cost:{}ms", deleted_files.size(), HJCurrentSteadyMS() - t_batch);
    }
    
    // 更新统计
    int new_file_count = static_cast<int>(all_files.size() - deleted_files.size());
    updateCategoryStats(name, new_file_count, current_total_size);
    
    return deleted_files;
}

void HJMediaDBManager::updateCategoryStats(const std::string& name, int file_count, int64_t total_size) {
    if (!m_categoryInfos.count(name)) {
        return;
    }
    if (file_count < 0) {
        file_count = 0;
    }
    if (total_size < 0) {
        total_size = 0;
    }
    auto t0 = HJCurrentSteadyMS();
    auto& categoryInfo = m_categoryInfos.at(name);
    if (categoryInfo.file_count != file_count || categoryInfo.total_size != total_size)
    {
        categoryInfo.file_count = file_count;
        categoryInfo.total_size = total_size;

        runStorageTransaction(*m_categoryTable, [&]() {
            m_categoryTable->replace(categoryInfo);
        });

        HJFLogi("update cost: {}ms, folder: {}, file_count: {}, total_size: {}", HJCurrentSteadyMS() - t0, name, categoryInfo.file_count, categoryInfo.total_size);
    }

    return;
}

bool HJMediaDBManager::updateFileInfo(const std::string& category, const std::string& old_rid, const std::string& new_rid, const std::string& new_local_url)
{
    HJ_AUTOU_LOCK(m_mutex);
    if (!isValidCategoryName(category)) {
        HJFLogw("updateFileInfo invalid category: {}", category);
        return false;
    }
    if (!m_categoryInfos.count(category)) {
        return false;
    }

    auto t0 = HJCurrentSteadyMS();
    try
    {
        auto storage = getFileTable(category);
        if (!storage) {
            HJ_EXCEPT(HJException::ERR_INVALID_STATE, HJFMT("Error, folder:{} storage is null", category));
        }

        // checking if new_rid exists if it is different from old_rid
        if (new_rid != old_rid) {
            auto count = storage->count<HJDBFileInfo>(where(c(&HJDBFileInfo::rid) == new_rid));
            if (count > 0) {
                HJFLogw("updateFileInfo failed: new_rid {} already exists in category {}", new_rid, category);
                return false;
            }
        }

        runStorageTransaction(*storage, [&]() {
            storage->update_all(
                set(
                    c(&HJDBFileInfo::rid) = new_rid,
                    c(&HJDBFileInfo::local_url) = new_local_url,
                    c(&HJDBFileInfo::modify_time) = HJCurrentSteadyMS()
                ),
                where(c(&HJDBFileInfo::rid) == old_rid)
            );
        });

        HJFLogi("updateFileInfo success. category:{}, old_rid:{}, new_rid:{}, cost:{} ms", 
            category, old_rid, new_rid, HJCurrentSteadyMS() - t0);
        return true;
    } catch(const HJException& e) {
        HJFLogw("updateFileInfo exception: {}", e.what());
    }
    return false;
}

NS_HJ_END
