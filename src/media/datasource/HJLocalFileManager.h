//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJUtilitys.h"
#include "HJMediaDBUtils.h"
#include "HJLocalIOUtils.h"
#include "HJShareBlob.h"
#include <optional>

NS_HJ_BEGIN
//***********************************************************************************//
class HJMediaDBManager;

/**
 * HJLocalFileManager - 本地文件信息管理
 * 
 * 职责：
 * 1. 获取已下载完整的文件信息
 * 2. 更新文件信息到数据库，并处理超限文件清理
 * 3. 文件重命名功能
 * 
 * 线程安全：使用 std::shared_mutex 实现读写分离
 */
class HJLocalFileManager : public HJObject {
public:
    HJ_DECLARE_PUWTR(HJLocalFileManager);

    /**
     * 构造函数
     * @param db_manager 数据库管理器（线程安全，可共享）
     * @param media_dir 媒体文件根目录
     */
    HJLocalFileManager(std::shared_ptr<HJMediaDBManager> db_manager, 
                       const std::string& media_dir);
    virtual ~HJLocalFileManager();

    HJShareBlob::Ptr getAreadyShareBlob(const std::string& url, const std::string& dir = "", const std::string& rid = "");
    HJShareBlob::Ptr getShareBlob(const std::string& url, const std::string& dir = "", const std::string& rid = "", int64_t fileLength = 0);

    std::optional<HJXIOFileInfo> checkXIOFile(const std::string& url, const std::string& dir = "", const std::string& rid = "");

    /**
     * 获取文件信息
     * 
     * @param rid 文件唯一标识
     * @param category 分类名称
     * @return 文件信息
     * 
     * 获取逻辑：
     * 1. 获取文件信息列表
     * 2. 遍历列表，判断文件是否完整
     * 3. 获取第一个完整文件
     */

    std::optional<HJDBFileInfo> getDBFile(const std::string& rid, const std::string& category = "");
    /**
     * 获取已下载完整的文件信息
     * 
     * @param category 分类名称
     * @param rid 文件唯一标识
     * @return 完整的文件信息，如果文件不完整或不存在返回空
     * 
     * 判断条件：status == COMPLETED 或 isComplete() == true
     */
    std::optional<HJDBFileInfo> getCompletedFile(const std::string& rid, const std::string& category = "");                              

    /**
     * 判断文件是否完整
     */
    static bool isFileCompleted(const HJDBFileInfo& file_info);

    /**
     * 更新文件信息到数据库
     * 
     * @param file_info 文件信息
     * @return 是否更新成功
     * 
     * 注意：调用 HJMediaDBManager::addOrUpdateFile 后，
     *      如果返回需要清理的文件列表，会自动清理这些物理文件
     */
    int updateFileInfo(const HJDBFileInfo& file_info);

    /**
     * 获取已下载完成的文件信息
     * 
     * @param hjtd_url 文件的HJTD地址
     * @param file_info 文件信息
     * @return 是否获取成功
     * 
     * 获取逻辑：
     * 1. 获取文件信息列表
     * 2. 遍历列表，判断文件是否完整
     * 3. 获取第一个完整文件

     */
    int completedFile(const std::string& hjtd_url, HJDBFileInfo& file_info);
    int completedFile(const std::string& hjtd_url, const std::string& rid);

    int incFileUseCount(const std::string& rid, const std::string& local_url);

    int moveFile(const std::string& category, const std::string& old_rid, const std::string& old_local_url, const std::string& new_rid, const std::string& new_local_url);
    /**
     * 重命名文件
     * 
     * @param category 分类名称
     * @param rid 文件唯一标识
     * @param new_local_url 新的本地URL路径（相对于 media_dir）
     * @return 是否重命名成功
     * 
     * 操作：
     * 1. 重命名物理文件
     * 2. 更新数据库中的 local_url
     */
    bool renameFile(const std::string& category, 
                    const std::string& rid, 
                    const std::string& new_local_url);


    static const size_t getGlobalID();
private:
    std::optional<HJDBFileInfo> getDBFileInternal(const std::string& rid, const std::string& category = "");

    int moveFileInternal(const HJDBFileInfo& db_info, const std::string& new_rid, const std::string& new_local_url);

    int updateFileInfoInternal(const HJDBFileInfo& file_info);

    int completedFileInternal(const std::string& hjtd_url, HJDBFileInfo& file_info);
    /**
     * 清理被淘汰的文件（内部方法）
     * 
     * @param files_to_delete 需要删除的文件列表
     * 
     * 由 updateFileInfo 调用，删除超限被淘汰的物理文件
     */
    void cleanupDeletedFiles(const std::vector<HJDBFileInfo>& files_to_delete);

    int procShareBlobNotify(const HJNotification::Ptr& ntfy);
private:
    mutable std::mutex                                  m_mutex;
    std::shared_ptr<HJMediaDBManager>                   m_dbManager;
    std::unordered_map<std::string, HJShareBlob::Wtr>   m_shareBlobs;  // 弱引用缓存，由使用方持有强引用控制生命周期
};

NS_HJ_END
