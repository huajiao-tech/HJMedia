//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJUtilitys.h"
#include "HJMediaStorageUtils.h"
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <shared_mutex>
//#if defined(__clang__)
//#pragma clang diagnostic push
//#pragma clang diagnostic ignored "-Wc++20-extensions"
//#pragma clang diagnostic ignored "-Wc++26-extensions"
//#endif

#include "HJSqliteBlobAdapter.h"
#include "sqlite_orm/sqlite_orm.h"

//#if defined(__clang__)
//#pragma clang diagnostic pop
//#endif

NS_HJ_BEGIN
//***********************************************************************************//
// 使用 inline 自由函数来定义数据库结构，以便 decltype 可以推导其类型
using namespace sqlite_orm;
inline auto CreateCategoryStorage(const std::string& db_path) 
{
    return make_storage(db_path,
        make_table(HJ_DB_TABLE_MAIN_NAME, // 主表名
            make_column("category", &HJDBCategoryInfo::name, primary_key()),
            make_column("type", &HJDBCategoryInfo::type),
            make_column("max_size", &HJDBCategoryInfo::max_size),
            make_column("file_count", &HJDBCategoryInfo::file_count),
            make_column("total_size", &HJDBCategoryInfo::total_size)
        )
    );
}

inline auto CreateFileStorage(const std::string& db_path, const std::string& folderName)
{
    using namespace sqlite_orm;
    return make_storage(db_path,
        make_table(folderName,
            make_column("rid", &HJDBFileInfo::rid, primary_key()),
            make_column("url", &HJDBFileInfo::url),
            make_column("local_url", &HJDBFileInfo::local_url),
            make_column("status", &HJDBFileInfo::status),
            make_column("size", &HJDBFileInfo::size),
            make_column("total_length", &HJDBFileInfo::total_length),
            make_column("category", &HJDBFileInfo::category),
            make_column("create_time", &HJDBFileInfo::create_time),
            make_column("modify_time", &HJDBFileInfo::modify_time),
            make_column("use_count", &HJDBFileInfo::use_count),
            make_column("level", &HJDBFileInfo::level),
            make_column("block_size", &HJDBFileInfo::block_size),
            make_column("block_status", &HJDBFileInfo::block_status)
        )
    );
}
using HJCategoryStorage = decltype(CreateCategoryStorage(""));
using HJFileStorage = decltype(CreateFileStorage("", ""));

//***********************************************************************************//
class HJMediaStorageManager {

public:
    HJ_DECLARE_PUWTR(HJMediaStorageManager);

    HJMediaStorageManager(std::string db_file, std::string media_dir);
    virtual ~HJMediaStorageManager();

    /**
    * 初始化并校验数据库与文件系统的一致性, 如果校验失败则抛出运行时异常
    */
    int init();

    bool addCategory(const HJDBCategoryInfo& categoryInfo);
    bool deleteCategory(const std::string& name);
    int addCategoryTable(const std::string& name);
    int deleteCategoryTable(const std::string& name);

    std::vector<HJDBFileInfo> AddOrUpdateFile(const std::string& name, const HJDBFileInfo& file_info);
    bool DeleteFile(const std::string& name, const std::string& rid);
    std::optional<HJDBFileInfo> GetFile(const std::string& category, const std::string& rid);

    std::vector<HJDBFileInfo> GetFilesInCategory(const std::string& name);

    std::shared_ptr<HJDBCategoryInfo> GetCategoryInfo(const std::string& name);
    std::vector<HJDBCategoryInfo> GetAllCategoryInfos();
private:
    std::vector<HJDBFileInfo> EnforceCategorySizeLimit(const std::string& name);
    // 更新文件夹的统计信息
    void UpdateCategoryStats(const std::string& name, int file_count_delta, int64_t total_size_delta);

    std::shared_ptr<HJFileStorage> getFileStorage(const std::string& name) {
        auto it = m_storages.find(name);
        if (it != m_storages.end()) {
            return it->second;
        }
        return nullptr;
    }
    // std::vector<HJDBFileInfo>& getFileCache(const std::string& name) {
    //     auto it = m_file_cache.find(name);
    //     if (it != m_file_cache.end()) {
    //         return it->second;
    //     }
    //     return {};
    // }
private:
    std::string m_db_file{""};
    std::string m_media_dir{""};
    std::map<std::string, std::shared_ptr<HJFileStorage>> m_storages;
    std::unique_ptr<HJCategoryStorage>                    m_categoryStorage{};
    std::map<std::string, HJDBCategoryInfo>               m_categoryInfos;
    mutable std::shared_mutex                             m_mutex;
    // std::map<std::string, std::vector<HJDBFileInfo>> m_file_cache;
};

NS_HJ_END
