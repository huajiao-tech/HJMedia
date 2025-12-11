//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "HJMediaStorageManager.h"
#include "HJException.h"
#include "HJFLog.h"
#include "HJFileUtil.h"
#include <filesystem>
#include <stdexcept>
#include <algorithm>

namespace fs = std::filesystem;
using namespace sqlite_orm;

NS_HJ_BEGIN
//***********************************************************************************//
HJMediaStorageManager::HJMediaStorageManager(std::string db_file, std::string media_dir)
    : m_db_file(std::move(db_file)),
      m_media_dir(std::move(media_dir))
{
    auto t0 = HJCurrentSteadyUS();
    try {
        m_categoryStorage = std::make_unique<HJCategoryStorage>(CreateCategoryStorage(m_db_file));
        m_categoryStorage->sync_schema();

        auto storage = std::make_unique<HJFileStorage>(CreateFileStorage(m_db_file, HJ_DB_TABLE_GIFTS_NAME));
        storage->sync_schema();
        m_storages[HJ_DB_TABLE_GIFTS_NAME] = std::move(storage);
    } catch (const HJException& e) {
        HJ_EXCEPT(HJException::ERR_INVALID_STATE, HJFMT("Error create db: {}, file:{}", e.what(), m_db_file));
    }
    HJFLogi("init storage manager cost: {}", HJCurrentSteadyUS() - t0);
}

HJMediaStorageManager::~HJMediaStorageManager() = default;

int HJMediaStorageManager::init() 
{
    HJ_AUTOU_LOCK(m_mutex);
    auto categoryInfos = m_categoryStorage->get_all<HJDBCategoryInfo>();
    for (const auto& info : categoryInfos) {
        m_categoryInfos[info.name] = info;
        auto fileInfos = GetFilesInCategory(info.name);
        for (const auto& file : fileInfos) {
            std::string file_path = HJUtilitys::concatenatePath(m_media_dir, file.local_url);
            if (!HJFileUtil::fileExist(file_path)) {
                HJ_EXCEPT(HJException::ERR_FILE_NOT_FOUND, HJFMT("Validation Error - File: {} in folder:{} exists in DB but not on disk.", file.local_url, info.name));
            }
        }
    }
    HJFLogi("init storage manager success.");

    return HJ_OK;
}

bool HJMediaStorageManager::addCategory(const HJDBCategoryInfo& categoryInfo) {
    HJ_AUTOU_LOCK(m_mutex);
    try
    {
        if (m_categoryInfos.count(categoryInfo.name)) {
            return false;
        }
        m_categoryStorage->replace(categoryInfo);
        m_categoryStorage->sync_schema(true);
        //
        m_categoryInfos[categoryInfo.name] = categoryInfo;
        //
        if(!getFileStorage(categoryInfo.name)) 
        {
            int res = addCategoryTable(categoryInfo.name);
            if(HJ_OK != res) {
                return false;
            }
        }
        HJFLogi("add folder: {}, type: {}, max_size: {}, file_count: {}, total_size: {} success.", categoryInfo.name, categoryInfo.type, categoryInfo.max_size, categoryInfo.file_count, categoryInfo.total_size);
        return true;
    } catch(const HJException& e) {
        HJFLogw("addCategory exception: {}", e.what()); 
    }
    return false;
}

bool HJMediaStorageManager::deleteCategory(const std::string& name) {
    HJ_AUTOU_LOCK(m_mutex);
    try
    {
        if (!m_categoryInfos.count(name)) {
            return false;
        }
        m_categoryStorage->transaction([&]() {
            m_categoryStorage->remove<HJDBCategoryInfo>(name);
            
            // m_categoryStorage->drop_table(name);
            return true;
        });

        m_categoryInfos.erase(name);
        //
        int res = deleteCategoryTable(name);
        if(HJ_OK != res) {
            return false;
        }
        HJFLogi("delete folder: {} success.", name);
        return true;
    } catch(const HJException& e) {
        HJFLogw("deleteCategory exception: {}", e.what());
    }
    return false;
}

int HJMediaStorageManager::addCategoryTable(const std::string& name)
{
    HJ_AUTOU_LOCK(m_mutex);
    try
    {
        auto storage = std::make_unique<HJFileStorage>(CreateFileStorage(m_db_file, name));
        storage->sync_schema();
        m_storages[name] = std::move(storage);
        return HJ_OK;
    } catch(const HJException& e) {
        HJ_EXCEPT(HJException::ERR_INVALID_STATE, HJFMT("Error create folder table: {}, {}", e.what(), name));
    }
    return HJErrNewObj;
}

int HJMediaStorageManager::deleteCategoryTable(const std::string& name)
{
    HJ_AUTOU_LOCK(m_mutex);
    try
    {
        auto storage = getFileStorage(name);
        if(storage){
            storage->drop_table(name);
            //
            m_storages.erase(name);
        }
        return HJ_OK;
    } catch(const HJException& e) {
        HJ_EXCEPT(HJException::ERR_INVALID_STATE, HJFMT("Error delete folder table: {}, {}", e.what(), name));
    }
    return HJErrFatal;
}

std::vector<HJDBFileInfo> HJMediaStorageManager::AddOrUpdateFile(const std::string& name, const HJDBFileInfo& file_info) 
{
    HJ_AUTOU_LOCK(m_mutex);
    if (!m_categoryInfos.count(name)) {
        // HJ_EXCEPT(HJException::ERR_INVALID_STATE, HJFMT("Error, folder:{} does not exist in DB", name));
        return {};
    }

    try
    {
        auto storage = getFileStorage(name);
        if (!storage) {
            HJ_EXCEPT(HJException::ERR_INVALID_STATE, HJFMT("Error, folder:{} storage is null", name));
        }
        int64_t old_size = 0;
        int file_count_delta = 0;
        auto existing_file = storage->get_pointer<HJDBFileInfo>(file_info.rid);
        if (existing_file) {
            old_size = existing_file->size;
        } else {
            file_count_delta = 1;
        }
        storage->replace(file_info);
        HJFLogi("url: {}, local_url: {}, size: {}, create_time: {}, modify_time: {}, use_count: {}", file_info.url, file_info.local_url, file_info.size, file_info.create_time, file_info.modify_time, file_info.use_count);

        UpdateCategoryStats(name, file_count_delta, file_info.size - old_size);

        // 检测并执行清理
        return EnforceCategorySizeLimit(name);
    } catch(const HJException& e) {
        HJ_EXCEPT(HJException::ERR_INVALID_STATE, HJFMT("Error, folder:{} does not exist in DB", name));
    }
    return {};
}

bool HJMediaStorageManager::DeleteFile(const std::string& name, const std::string& rid) {
    HJ_AUTOU_LOCK(m_mutex);
    if (!m_categoryInfos.count(name)) {
        return false;
    }
    try
    {
        auto storage = getFileStorage(name);
        if (!storage) {
            HJ_EXCEPT(HJException::ERR_INVALID_STATE, HJFMT("Error, folder:{} storage is null", name));
        }
        auto file_to_delete = storage->get_pointer<HJDBFileInfo>(rid);
        if (!file_to_delete) {
            return false;
        }
        storage->remove<HJDBFileInfo>(rid);
        HJFLogi("Delete file: {}, size: {} success.", file_to_delete->url, file_to_delete->size);
        
        UpdateCategoryStats(name, -1, -file_to_delete->size);
        return true;
    } catch(const HJException& e) {
        HJ_EXCEPT(HJException::ERR_INVALID_STATE, HJFMT("Error, folder:{} does not exist in DB", name));
    }   
    return false;
}

std::optional<HJDBFileInfo> HJMediaStorageManager::GetFile(const std::string& category, const std::string& rid)
{
    HJ_AUTOS_LOCK(m_mutex);
    if (!m_categoryInfos.count(category)) {
        return {};
    }
    try
    {
        auto storage = getFileStorage(category);
        if (!storage) {
            HJ_EXCEPT(HJException::ERR_INVALID_STATE, HJFMT("Error, folder:{} storage is null", category));
        }
        auto file = storage->get_pointer<HJDBFileInfo>(rid);
        if (!file) {
            return {};
        }
        return *file;
    } catch(const HJException& e) {
        HJ_EXCEPT(HJException::ERR_INVALID_STATE, HJFMT("Error, folder:{} does not exist in DB", category));
    }
    return {};
}

std::vector<HJDBFileInfo> HJMediaStorageManager::GetFilesInCategory(const std::string& name) {
    HJ_AUTOS_LOCK(m_mutex)
    try {
        auto storage = getFileStorage(name);
        if (!m_categoryInfos.count(name) || !storage) {
            HJFLogw("Table '{}' does not exist.", name);
            return {};
        }
        return storage->get_all<HJDBFileInfo>();
    } catch (const HJException& e) {
        HJFLogw("Table '{}' does not exist or error occurred: {}", name, e.what());
    }
    return {};
}

std::shared_ptr<HJDBCategoryInfo> HJMediaStorageManager::GetCategoryInfo(const std::string& name) {
    HJ_AUTOS_LOCK(m_mutex)
    if (m_categoryInfos.count(name)) {
        return std::make_shared<HJDBCategoryInfo>(m_categoryInfos.at(name));
    }
    return nullptr;
}

std::vector<HJDBCategoryInfo> HJMediaStorageManager::GetAllCategoryInfos() {
    HJ_AUTOS_LOCK(m_mutex)
    std::vector<HJDBCategoryInfo> result;
    for(const auto& pair : m_categoryInfos) {
        result.push_back(pair.second);
    }
    return result;
}

std::vector<HJDBFileInfo> HJMediaStorageManager::EnforceCategorySizeLimit(const std::string& name) 
{
    if (!m_categoryInfos.count(name)) {
        return {};
    }
    auto& categoryInfo = m_categoryInfos.at(name);
    if (categoryInfo.total_size <= categoryInfo.max_size) {
        return {};
    }
    std::vector<HJDBFileInfo> deleted_files;
    auto all_files = GetFilesInCategory(name); //getFileCache(name);
    std::sort(all_files.begin(), all_files.end(), [](const HJDBFileInfo& a, const HJDBFileInfo& b) {
        if (a.create_time != b.create_time) {
            return a.create_time < b.create_time; // 创建时间早的优先
        }
        return a.use_count < b.use_count; // 使用次数少的优先
    });

    int64_t current_total_size = categoryInfo.total_size;
    int current_file_count = categoryInfo.file_count;

    auto storage = getFileStorage(name);
    if (!storage) {
        HJ_EXCEPT(HJException::ERR_INVALID_STATE, HJFMT("Error, folder:{} storage is null", name));
    }
    for (const auto& file_to_delete : all_files) {
        if (current_total_size <= categoryInfo.max_size) {
            break;
        }
        
        storage->remove<HJDBFileInfo>(file_to_delete.rid);
        deleted_files.push_back(file_to_delete);

        current_total_size -= file_to_delete.size;
        current_file_count -= 1;
    }

    UpdateCategoryStats(name, current_file_count - categoryInfo.file_count, current_total_size - categoryInfo.total_size);

    return deleted_files;
}

void HJMediaStorageManager::UpdateCategoryStats(const std::string& name, int file_count_delta, int64_t total_size_delta) {
    if (!m_categoryInfos.count(name)) return;

    auto& categoryInfo = m_categoryInfos.at(name);
    categoryInfo.file_count += file_count_delta;
    categoryInfo.total_size += total_size_delta;

    m_categoryStorage->update(categoryInfo);

    HJFLogi("folder: {}, file_count: {}, total_size: {}", name, categoryInfo.file_count, categoryInfo.total_size);

    return;
}

NS_HJ_END
