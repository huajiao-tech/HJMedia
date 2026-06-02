//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJLocalIOUtils.h"
#include "HJAVIOContext.h"
#include "HJDownloads.h"
#include "HJMediaDBUtils.h"

NS_HJ_BEGIN
class HJMediaDBManager;
class HJLocalFileManager;
//***********************************************************************************//
class HJDataSourceKit : public HJLocalIODelegate
{
public:
    HJ_INSTANCE_DECL(HJDataSourceKit);
    HJDataSourceKit();
    virtual ~HJDataSourceKit();

    int init(HJParams::Ptr opts, HJListener listener = nullptr);
    void done();
    int setCacheSize(const std::string& dir, const int size);

    //
    HJAVIOContext::Ptr getAVIOContext(const std::string& url, const std::string& dir, const std::string& rid = "", int64_t timeout = 3000000);
    // std::pair<std::string, bool> getLocalUrl(const std::string& url, const std::string& dir, const std::string& rid = "");
    // std::string makeLocalUrl(const std::string& url, const std::string& rid = "", const std::string& dir = "");
    //
    std::string download(const std::string& url, const std::string& dir, const std::string& rid = "", int preCacheSize = -1, int priority = 0, HJListener listener = nullptr);
    int cancel(const std::string& rid, bool deleteFile = false);

    static std::string getVersion();
public:
    std::shared_ptr<HJLocalFileManager> getFileManager() const {
        return m_fileManager;
    }
    std::vector<HJDBCategoryInfo> getAllCategoryInfos();
private:
    int parseParams(const HJParams::Ptr& params);
    int setupMediaDB();
    int destroyMediaDB();
    int checkCategory(const std::string& cache_dir, const int cache_size);

    // HJXIOFileInfo makeXIOFileInfo(const std::string& url, const std::string& rid, const std::string& dir);
    virtual int onLocalIONotify(HJNotification::Ptr ntfy) {
        return notify(ntfy);
    }

    virtual int notify(HJNotification::Ptr ntf) {
        if (m_listener) {
            return m_listener(std::move(ntf));
        }
        return HJ_OK;
    }
private:
    HJLocalIOParams     m_params;
    HJListener          m_listener{};
    std::shared_ptr<HJMediaDBManager>   m_dbManager{nullptr};
    std::shared_ptr<HJLocalFileManager> m_fileManager{nullptr};
    std::shared_ptr<HJDownloads>        m_downloads{nullptr};

};

NS_HJ_END
