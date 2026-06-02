//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJUtilitys.h"
#include "HJMediaDBUtils.h"
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
inline auto CreateCategoryTable(const std::string& db_path) 
{
    return make_storage(db_path,
        make_table(HJ_DB_TABLE_MAIN_NAME, // 主表名
            make_column("category", &HJDBCategoryInfo::name, primary_key()),
            make_column("type", &HJDBCategoryInfo::type),
            make_column("max_size", &HJDBCategoryInfo::max_size),
            make_column("file_count", &HJDBCategoryInfo::file_count),
            make_column("total_size", &HJDBCategoryInfo::total_size),
            make_column("local_dir", &HJDBCategoryInfo::local_dir)
        )
    );
}

inline auto CreateFileTable(const std::string& db_path, const std::string& category)
{
    using namespace sqlite_orm;
    return make_storage(db_path,
        make_table(category,
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
            make_column("block_status", &HJDBFileInfo::block_status),
            make_column("spans", &HJDBFileInfo::spans)
        )
    );
}
using HJCategoryTable = decltype(CreateCategoryTable(""));
using HJFileTable = decltype(CreateFileTable("", ""));

//***********************************************************************************//
class HJMediaDBManager {

public:
    HJ_DECLARE_PUWTR(HJMediaDBManager);

    HJMediaDBManager(std::string db_file);
    virtual ~HJMediaDBManager();

    /**
    * 初始化并校验数据库与文件系统的一致性, 如果校验失败则抛出运行时异常
    */
    int init();

    bool addOrUpdateCategory(const HJDBCategoryInfo& categoryInfo);
    bool deleteCategory(const std::string& name);

    int addCategoryTable(const std::string& name);
    int deleteCategoryTable(const std::string& name);

    std::vector<HJDBFileInfo> addOrUpdateFile(const std::string& name, const HJDBFileInfo& file_info);

    /**
     * 更新文件信息，支持修改 rid (Primary Key)
     * 注意：如果 new_rid 已存在（且不等于 old_rid），则更新失败返回 false
     */
    bool updateFileInfo(const std::string& category, const std::string& old_rid, const std::string& new_rid, const std::string& new_local_url);
private:
    // Internal no-lock versions (caller must hold m_mutex)
    int addCategoryTableInternal(const std::string& name);
    int deleteCategoryTableInternal(const std::string& name);
    std::vector<HJDBFileInfo> getFilesInCategoryInternal(const std::string& name);

public:
    bool deleteFile(const std::string& category, const std::string& rid);
    bool deleteFiles(const std::string& category, const std::vector<std::string>& rids);
    std::optional<HJDBFileInfo> getFile(const std::string& category, const std::string& rid);
    int incFileUseCount(const std::string& category, const std::string& rid);

    std::vector<HJDBFileInfo> getFilesInCategory(const std::string& category);
    //category
    bool addOrUpdateCategoryInternal(const HJDBCategoryInfo& categoryInfo);
    std::shared_ptr<HJDBCategoryInfo> getCategoryInfo(const std::string& category);
    std::vector<HJDBCategoryInfo> getAllCategoryInfos();

    int updateCategoryMaxSize(const std::string& cache_dir, int64_t max_size);
private:
    enum class SynchronousMode {
        OFF = 0,
        NORMAL = 1,
        FULL = 2,
    };
    static const size_t kMaxCategoryNameLength = 64;
    bool isValidCategoryName(const std::string& name) const;
    void syncCategoryStatsInternal(const std::string& name);
    std::vector<HJDBFileInfo> enforceCategorySizeLimit(const std::string& name);
    // 更新分类category的统计信息
    void updateCategoryStats(const std::string& name, int file_count, int64_t total_size);

    std::shared_ptr<HJFileTable> getFileTable(const std::string& name) {
        auto it = m_fileTables.find(name);
        if (it != m_fileTables.end()) {
            return it->second;
        }
        return nullptr;
    }
private:
    mutable std::shared_mutex                           m_mutex;
    bool                                                m_initialized{false};
    std::string                                         m_db_file{""};
    //
    std::unique_ptr<HJCategoryTable>                    m_categoryTable{};
    std::map<std::string, HJDBCategoryInfo>             m_categoryInfos;
    //
    std::map<std::string, std::shared_ptr<HJFileTable>> m_fileTables;
    SynchronousMode                                     m_sync_mode{SynchronousMode::OFF};
};

NS_HJ_END
