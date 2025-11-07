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
        m_folderStorage = std::make_unique<HJFolderStorage>(CreateFolderStorage(m_db_file));
        m_folderStorage->sync_schema();

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
    m_folder_cache.clear();
    // m_file_cache.clear();
    //
    auto subfolders = m_folderStorage->get_all<HJXFolderInfo>();
    for (const auto& folder : subfolders) {
        std::string folder_path = HJUtilitys::concatenatePath(m_media_dir, folder.name);
        if (HJFileUtil::isDirExist(folder_path))
        {
            m_folder_cache[folder.name] = folder;
            //
            auto files_in_db = GetFilesInFolder(folder.name);
            for (const auto& file_info : files_in_db) {
                std::string file_path = HJUtilitys::concatenatePath(folder_path, file_info.local_url);
                if (!HJFileUtil::fileExist(file_path.c_str())) {
                    HJ_EXCEPT(HJException::ERR_FILE_NOT_FOUND, HJFMT("Validation Error - File: {} in folder:{} exists in DB but not on disk.", file_info.local_url, folder.name));
                }
                //fs::path file_path = folder_path / file_info.local_url;
                //if (!fs::exists(file_path)) {
                //    HJ_EXCEPT(HJException::ERR_FILE_NOT_FOUND, HJFMT("Validation Error - File: {} in folder:{} exists in DB but not on disk.", file_info.local_url, folder.name));
                //}
            }
            // m_file_cache[folder.name] = files_in_db;
        } else {
            //HJ_EXCEPT(HJException::ERR_FILE_NOT_FOUND, HJFMT("Validation Error - Folder: {} exists in DB but not on disk.", folder.name));
            HJFileUtil::create_path(folder_path.c_str());
            HJFLogw("Validation Error - Folder: {} exists in DB but not on disk.", folder.name);
        }
    }
    HJFLogi("init storage manager success.");

    return HJ_OK;
}

bool HJMediaStorageManager::addFolder(const HJXFolderInfo& folder_info) {
    try
    {
        if (m_folder_cache.count(folder_info.name)) {
            return false;
        }
        m_folderStorage->replace(folder_info);
        m_folderStorage->sync_schema(true);
        //
        m_folder_cache[folder_info.name] = folder_info;
        //
        if(!getFileStorage(folder_info.name)) 
        {
            int res = addFolderTable(folder_info.name);
            if(HJ_OK != res) {
                return false;
            }
        }
        HJFLogi("add folder: {}, type: {}, max_size: {}, file_count: {}, total_size: {} success.", folder_info.name, folder_info.type, folder_info.max_size, folder_info.file_count, folder_info.total_size);
        return true;
    } catch(const HJException& e) {
        HJFLogw("addFolder exception: {}", e.what()); 
    }
    return false;
}

bool HJMediaStorageManager::deleteFolder(const std::string& folder_name) {
    try
    {
        if (!m_folder_cache.count(folder_name)) {
            return false;
        }
        m_folderStorage->transaction([&]() {
            m_folderStorage->remove<HJXFolderInfo>(folder_name);
            
            // m_folderStorage->drop_table(folder_name);
            return true;
        });

        m_folder_cache.erase(folder_name);
        //
        int res = deleteFolderTable(folder_name);
        if(HJ_OK != res) {
            return false;
        }
        HJFLogi("delete folder: {} success.", folder_name);
        return true;
    } catch(const HJException& e) {
        HJFLogw("deleteFolder exception: {}", e.what());
    }
    return false;
}

int HJMediaStorageManager::addFolderTable(const std::string& folder_name)
{
    try
    {
        auto storage = std::make_unique<HJFileStorage>(CreateFileStorage(m_db_file, folder_name));
        storage->sync_schema();
        m_storages[folder_name] = std::move(storage);
        return HJ_OK;
    } catch(const HJException& e) {
        HJ_EXCEPT(HJException::ERR_INVALID_STATE, HJFMT("Error create folder table: {}, {}", e.what(), folder_name));
    }
    return HJErrNewObj;
}

int HJMediaStorageManager::deleteFolderTable(const std::string& folder_name)
{
    try
    {
        auto storage = getFileStorage(folder_name);
        if(storage){
            storage->drop_table(folder_name);
            //
            m_storages.erase(folder_name);
        }
        return HJ_OK;
    } catch(const HJException& e) {
        HJ_EXCEPT(HJException::ERR_INVALID_STATE, HJFMT("Error delete folder table: {}, {}", e.what(), folder_name));
    }
    return HJErrFatal;
}

std::vector<HJXFileInfo> HJMediaStorageManager::AddOrUpdateFile(const std::string& folder_name, const HJXFileInfo& file_info) 
{
    if (!m_folder_cache.count(folder_name)) {
        // HJ_EXCEPT(HJException::ERR_INVALID_STATE, HJFMT("Error, folder:{} does not exist in DB", folder_name));
        return {};
    }

    try
    {
        auto storage = getFileStorage(folder_name);
        if (!storage) {
            HJ_EXCEPT(HJException::ERR_INVALID_STATE, HJFMT("Error, folder:{} storage is null", folder_name));
        }
        int64_t old_size = 0;
        int file_count_delta = 0;
        auto existing_file = storage->get_pointer<HJXFileInfo>(file_info.rid);
        if (existing_file) {
            old_size = existing_file->size;
        } else {
            file_count_delta = 1;
        }
        storage->replace(file_info);
        HJFLogi("url: {}, local_url: {}, size: {}, create_time: {}, modify_time: {}, use_count: {}", file_info.url, file_info.local_url, file_info.size, file_info.create_time, file_info.modify_time, file_info.use_count);

        UpdateFolderStats(folder_name, file_count_delta, file_info.size - old_size);

        // 检测并执行清理
        return EnforceFolderSizeLimit(folder_name);
    } catch(const HJException& e) {
        HJ_EXCEPT(HJException::ERR_INVALID_STATE, HJFMT("Error, folder:{} does not exist in DB", folder_name));
    }
    return {};
}

bool HJMediaStorageManager::DeleteFile(const std::string& folder_name, int64_t file_rid) {
    if (!m_folder_cache.count(folder_name)) {
        return false;
    }
    try
    {
        auto storage = getFileStorage(folder_name);
        if (!storage) {
            HJ_EXCEPT(HJException::ERR_INVALID_STATE, HJFMT("Error, folder:{} storage is null", folder_name));
        }
        auto file_to_delete = storage->get_pointer<HJXFileInfo>(file_rid);
        if (!file_to_delete) {
            return false;
        }
        storage->remove<HJXFileInfo>(file_rid);
        HJFLogi("Delete file: {}, size: {} success.", file_to_delete->url, file_to_delete->size);
        
        UpdateFolderStats(folder_name, -1, -file_to_delete->size);
        return true;
    } catch(const HJException& e) {
        HJ_EXCEPT(HJException::ERR_INVALID_STATE, HJFMT("Error, folder:{} does not exist in DB", folder_name));
    }   
    return false;
}

std::vector<HJXFileInfo> HJMediaStorageManager::GetFilesInFolder(const std::string& folder_name) {
    try {
        auto storage = getFileStorage(folder_name);
        if (!m_folder_cache.count(folder_name) || storage) {
            HJFLogw("Table '{}' does not exist.", folder_name);
            return {};
        }
        return storage->get_all<HJXFileInfo>();
    } catch (const HJException& e) {
        HJFLogw("Table '{}' does not exist or error occurred: {}", folder_name, e.what());
    }
    return {};
}

std::shared_ptr<HJXFolderInfo> HJMediaStorageManager::GetHJXFolderInfo(const std::string& folder_name) {
    if (m_folder_cache.count(folder_name)) {
        return std::make_shared<HJXFolderInfo>(m_folder_cache.at(folder_name));
    }
    return nullptr;
}

std::vector<HJXFolderInfo> HJMediaStorageManager::GetAllHJXFolderInfos() {
    std::vector<HJXFolderInfo> result;
    for(const auto& pair : m_folder_cache) {
        result.push_back(pair.second);
    }
    return result;
}

std::vector<HJXFileInfo> HJMediaStorageManager::EnforceFolderSizeLimit(const std::string& folder_name) 
{
    if (!m_folder_cache.count(folder_name)) {
        return {};
    }
    auto& folder_info = m_folder_cache.at(folder_name);
    if (folder_info.total_size <= folder_info.max_size) {
        return {};
    }
    std::vector<HJXFileInfo> deleted_files;
    auto all_files = GetFilesInFolder(folder_name); //getFileCache(folder_name);
    std::sort(all_files.begin(), all_files.end(), [](const HJXFileInfo& a, const HJXFileInfo& b) {
        if (a.create_time != b.create_time) {
            return a.create_time < b.create_time; // 创建时间早的优先
        }
        return a.use_count < b.use_count; // 使用次数少的优先
    });

    int64_t current_total_size = folder_info.total_size;
    int current_file_count = folder_info.file_count;

    auto storage = getFileStorage(folder_name);
    if (!storage) {
        HJ_EXCEPT(HJException::ERR_INVALID_STATE, HJFMT("Error, folder:{} storage is null", folder_name));
    }
    for (const auto& file_to_delete : all_files) {
        if (current_total_size <= folder_info.max_size) {
            break;
        }
        
        storage->remove<HJXFileInfo>(file_to_delete.rid, folder_name);
        deleted_files.push_back(file_to_delete);

        current_total_size -= file_to_delete.size;
        current_file_count -= 1;
    }

    UpdateFolderStats(folder_name, current_file_count - folder_info.file_count, current_total_size - folder_info.total_size);

    return deleted_files;
}

void HJMediaStorageManager::UpdateFolderStats(const std::string& folder_name, int file_count_delta, int64_t total_size_delta) {
    if (!m_folder_cache.count(folder_name)) return;

    auto& folder_info = m_folder_cache.at(folder_name);
    folder_info.file_count += file_count_delta;
    folder_info.total_size += total_size_delta;

    m_folderStorage->update(folder_info);

    HJFLogi("folder: {}, file_count: {}, total_size: {}", folder_name, folder_info.file_count, folder_info.total_size);

    return;
}

NS_HJ_END
