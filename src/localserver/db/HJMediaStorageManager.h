//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJUtilitys.h"
#include <string>
#include <vector>
#include <memory>
#include <map>
//#if defined(__clang__)
//#pragma clang diagnostic push
//#pragma clang diagnostic ignored "-Wc++20-extensions"
//#pragma clang diagnostic ignored "-Wc++26-extensions"
//#endif

#include "sqlite_orm/sqlite_orm.h"

//#if defined(__clang__)
//#pragma clang diagnostic pop
//#endif

NS_HJ_BEGIN
#define HJ_DB_TABLE_MAIN_NAME "folders"
#define HJ_DB_TABLE_GIFTS_NAME "gifts"
//***********************************************************************************//
// 需求 2: 子文件夹信息结构体
struct HJXFolderInfo
{
    std::string name;      // 主键, 子文件夹名称
    int type;              // 子文件夹类型
    int64_t max_size;      // 子文件夹限制大小 
    int file_count;        // 子文件夹文件数量
    int64_t total_size;    // 子文件夹总存储占用空间大小

    bool operator==(const HJXFolderInfo& other) const {
        return name == other.name &&
               type == other.type &&
               max_size == other.max_size &&
               file_count == other.file_count &&
               total_size == other.total_size;
    }
};

// 需求 3: 文件信息结构体
struct HJXFileInfo
{
    int64_t rid;           // 主键, 文件唯一标识
    std::string url;       // 文件网络URL
    std::string local_url; // 文件本地URL
    int status;            // 文件状态
    int64_t size;          // 文件大小
    int type;              // 文件所属子文件夹类型
    int64_t create_time;   // 文件创建时间
    int64_t modify_time;   // 文件修改时间
    int use_count;         // 文件使用次数
	int level;			   // 文件缓存级别

    bool operator==(const HJXFileInfo& other) const {
        return rid == other.rid &&
               url == other.url &&
               local_url == other.local_url &&
               status == other.status &&
               size == other.size &&
               type == other.type &&
               create_time == other.create_time &&
               modify_time == other.modify_time &&
               use_count == other.use_count &&
               level == other.level;
    }
};

// 使用 inline 自由函数来定义数据库结构，以便 decltype 可以推导其类型
using namespace sqlite_orm;
inline auto CreateFolderStorage(const std::string& db_path) 
{
    return make_storage(db_path,
        make_table(HJ_DB_TABLE_MAIN_NAME, // 主表名
            make_column("name", &HJXFolderInfo::name, primary_key()),
            make_column("type", &HJXFolderInfo::type),
            make_column("max_size", &HJXFolderInfo::max_size),
            make_column("file_count", &HJXFolderInfo::file_count),
            make_column("total_size", &HJXFolderInfo::total_size)
        )
    );
}

inline auto CreateFileStorage(const std::string& db_path, const std::string& folderName)
{
    using namespace sqlite_orm;
    return make_storage(db_path,
        make_table(folderName,
            make_column("rid", &HJXFileInfo::rid, primary_key()),
            make_column("url", &HJXFileInfo::url),
            make_column("local_url", &HJXFileInfo::local_url),
            make_column("status", &HJXFileInfo::status),
            make_column("size", &HJXFileInfo::size),
            make_column("type", &HJXFileInfo::type),
            make_column("create_time", &HJXFileInfo::create_time),
            make_column("modify_time", &HJXFileInfo::modify_time),
            make_column("use_count", &HJXFileInfo::use_count),
            make_column("level", &HJXFileInfo::level)
        )
    );
}
using HJFolderStorage = decltype(CreateFolderStorage(""));
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

    bool addFolder(const HJXFolderInfo& folder_info);
    bool deleteFolder(const std::string& folder_name);
    int addFolderTable(const std::string& folder_name);
    int deleteFolderTable(const std::string& folder_name);

    std::vector<HJXFileInfo> AddOrUpdateFile(const std::string& folder_name, const HJXFileInfo& file_info);
    bool DeleteFile(const std::string& folder_name, int64_t file_rid);
    std::vector<HJXFileInfo> GetFilesInFolder(const std::string& folder_name);

    std::shared_ptr<HJXFolderInfo> GetHJXFolderInfo(const std::string& folder_name);
    std::vector<HJXFolderInfo> GetAllHJXFolderInfos();
private:
    std::vector<HJXFileInfo> EnforceFolderSizeLimit(const std::string& folder_name);
    // 更新文件夹的统计信息
    void UpdateFolderStats(const std::string& folder_name, int file_count_delta, int64_t total_size_delta);

    std::shared_ptr<HJFileStorage> getFileStorage(const std::string& name) {
        auto it = m_storages.find(name);
        if (it != m_storages.end()) {
            return it->second;
        }
        return nullptr;
    }
    // std::vector<HJXFileInfo>& getFileCache(const std::string& name) {
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
    std::unique_ptr<HJFolderStorage> m_folderStorage{};
    std::map<std::string, HJXFolderInfo> m_folder_cache;
    // std::map<std::string, std::vector<HJXFileInfo>> m_file_cache;
};

NS_HJ_END
