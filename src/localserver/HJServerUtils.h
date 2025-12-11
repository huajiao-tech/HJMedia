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
#define HJ_HTTP_PORT_DEFAULT        8125
#define HJ_FILE_BLOCK_SIZE_DEFAULT  32*1024
//
typedef struct HJServerParams
{
    std::string cache_dir = "";
    int cache_size = 500;  //MB
    std::string device_id = "";
    bool mobile_enable = true;
    //http
    std::string ip = "0.0.0.0";
	int port = HJ_HTTP_PORT_DEFAULT;
    std::string media_dir = "";
    //
    std::string log_dir = "";
    int log_level = 0;
    int log_mode = 0;
    int log_max_size = 0;
} HJServerParams;

class HJServerDelegate : public virtual HJObject
{
public:
    HJ_DECLARE_PUWTR(HJServerDelegate);

    virtual int onLocalServerNotify(HJNotification::Ptr ntfy) = 0;
};

typedef enum HJServerNotifyType
{
    HJ_SERVER_NOTIFY_NONE = 0,
    HJ_SERVER_NOTIFY_START,
    HJ_SERVER_NOTIFY_STOP,
    HJ_SERVER_NOTIFY_ERROR,
    HJ_SERVER_NOTIFY_CACHE_START,
    HJ_SERVER_NOTIFY_CACHE_PROGRESS,
    HJ_SERVER_NOTIFY_CACHE_COMPLETE,
    HJ_SERVER_NOTIFY_CACHE_FAILED,
} HJServerNotifyType;

class HJServerUtils
{
public:
    static std::string getCategory(int priority);
    static std::pair<std::string, std::string> makeLocalUrl(const std::string& url, const std::string& rid, const std::string& cache_dir);
};

NS_HJ_END