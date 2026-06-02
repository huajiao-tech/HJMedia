//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR: 
//CREATE TIME: 
//***********************************************************************************//
#pragma once
#include "HJUtilitys.h"
#include "HJNotify.h"

NS_HJ_BEGIN
//***********************************************************************************//
#define HJ_BLOCK_SIZE_DEFAULT  32*1024

using HJCacheOptions = std::vector<std::pair<std::string, int>>;
typedef struct HJLocalIOParams
{
    HJCacheOptions    cache_opts;                              // <dir, size(MB)>
    std::string       medias_dir{""};                          // medias directory
    int               medias_cache_max{200};                 // medias cache limited in MB
    int               block_size{HJ_BLOCK_SIZE_DEFAULT};       // block size for read/write`
    int               retry_max{10};                            // retry max times
} HJLocalIOParams;

class HJLocalIODelegate : public virtual HJObject
{
public:
    HJ_DECLARE_PUWTR(HJLocalIODelegate);

    virtual int onLocalIONotify(HJNotification::Ptr ntfy) = 0;
};

typedef enum HJLocalIONotifyType
{
    HJ_LOCALIO_NOTIFY_NONE = 0,
    HJ_LOCALIO_NOTIFY_START,
    HJ_LOCALIO_NOTIFY_STOP,
    HJ_LOCALIO_NOTIFY_ERROR,
    HJ_LOCALIO_NOTIFY_CACHE_START,
    HJ_LOCALIO_NOTIFY_CACHE_PROGRESS,
    HJ_LOCALIO_NOTIFY_CACHE_COMPLETED,
    HJ_LOCALIO_NOTIFY_CACHE_FAILED,
} HJLocalIONotifyType;

typedef struct HJXIOFileInfo
{
    std::string url{""};
	std::string rid{""};
    std::string local_url{""};
    std::string category{""};
	int         preCacheSize{-1};   //-1: all file, 0: no cache, >0: cache size
	int         priority{0};        //HJPriority

    std::string tmp_url{""};        //*.hjio, *.hjtd
    std::string tmp_rid{""};

    HJListener listener{};

    HJXIOFileInfo() = default;
    HJXIOFileInfo(const std::string& i_url, const std::string& i_dir, const std::string& i_rid = "");
    HJXIOFileInfo(const std::string& i_url, const std::string& i_dir, const std::string& i_rid, const std::string& i_category, int i_preCacheSize, int i_priority);
} HJXIOFileInfo;

// class HJLocalIOUtils
// {
// public:
//     static HJXIOFileInfo makeXIOFileInfo(const std::string& url, const std::string& dir, const std::string& rid = "");
// };



NS_HJ_END