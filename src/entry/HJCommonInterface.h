#pragma once
#ifndef HJ_COMMON_INTERFACE_H_20260114
#define HJ_COMMON_INTERFACE_H_20260114

#include <string>
#include <functional>
#include <vector>
#include <utility>
#include <cstdint>

using HJNAPIEntryNotify = std::function<void(int i_type, const std::string& i_msgInfo)>;
using HJStatEntryNotify = std::function<void(const std::string& i_name, int i_nType, const std::string& i_info)>;

typedef enum HJEntryBaseRenderNotifyType
{
	HJ_RENDER_NOTIFY_ERROR_DEFAULT = 100,
	HJ_RENDER_NOTIFY_ERROR_UPDATE = 101,
	HJ_RENDER_NOTIFY_ERROR_DRAW = 102,
	HJ_RENDER_NOTIFY_ERROR_VIDEOCAPTURE_INIT = 103,
	HJ_RENDER_NOTIFY_ERROR_PNGSEQ_INIT = 104,
	HJ_RENDER_NOTIFY_PNGSEQ_COMPLETE = 105,
	HJ_RENDER_NOTIFY_NEED_SURFACE = 106,

	HJ_RENDER_NOTIFY_FACEU_ERROR = 107,
	HJ_RENDER_NOTIFY_FACEU_COMPLETE = 108,

	HJ_RENDER_NOTIFY_NodeCreateSuccess = 109,
	HJ_RENDER_NOTIFY_NodeCreateFailed = 110,

	HJ_RENDER_NOTIFY_NodeConnectSuccess = 111,
	HJ_RENDER_NOTIFY_NodeConnectFailed = 112,

	HJ_RENDER_NOTIFY_NodeDisconnectSuccess = 113,
	HJ_RENDER_NOTIFY_NodeDisconnectFailed = 114,

	HJ_RENDER_NOTIFY_NodeDeleteSuccess = 115,
	HJ_RENDER_NOTIFY_NodeDeleteFailed = 116,

	HJ_RENDER_NOTIFY_NodeEnableSuccess = 117,
	HJ_RENDER_NOTIFY_NodeEnableFailed = 118,

	HJ_RENDER_NOTIFY_NodeLinkInfoChangeSuccess = 119,
	HJ_RENDER_NOTIFY_NodeLinkInfoChangeFailed = 120,

    HJ_RENDER_NOTIFY_INIT_ERROR               = 130,
    HJ_RENDER_NOTIFY_INIT_SUCCESS             = 131,

    HJ_RENDER_NOTIFY_CACHE_START = 200,
    HJ_RENDER_NOTIFY_CACHE_PROGRESS = 201,
    HJ_RENDER_NOTIFY_CACHE_COMPLETED = 202,
    HJ_RENDER_NOTIFY_CACHE_FAILED = 203,
} HJEntryBaseRenderNotifyType;

typedef enum HJEntryType {
    HJEntryType_Pusher,
    HJEntryType_Player,
    HJEntryType_Render,
    HJEntryType_Inference,
} HJEntryType;

typedef enum HJEntryLogMode {
    HJLogMode_NONE = (1 << 0),
    HJLogLMode_CONSOLE = (1 << 1),
    HJLLogMode_FILE = (1 << 2),
    HJLLogMode_CALLBACK = (1 << 3),
} HJEntryLogMode;

typedef enum HJEntryLogLevel {
	HJLOGLevel_TRACE = 0,
	HJLOGLevel_DEBUG = 1,
	HJLOGLevel_INFO  = 2,
	HJLOGLevel_WARN  = 3,
	HJLOGLevel_ERROR = 4,
	HJLOGLevel_ALARM = 5,
	HHJLOGLevel_FATAL = 6,
} HJPlayerLogLevel;

typedef struct HJEntryContextInfo
{
    bool logIsValid = true;
    std::string logDir = "";
    int  logLevel = HJLOGLevel_INFO;
    int  logMode = HJLogLMode_CONSOLE | HJLLogMode_FILE;
    int  logMaxFileSize = 0;
    int  logMaxFileNum = 0;
} HJEntryContextInfo;

typedef struct HJEntryContextPlayerInfo : public HJEntryContextInfo
{
    std::string medias_dir = "";
    int         medias_cache_max = 200;
    std::vector<std::pair<std::string, int>> other_dirs_options;
    int         download_retry_max = 10;
} HJEntryContextPlayerInfo;

typedef struct HJEntryStatInfo
{
    bool bUseStat = false;
    int64_t uid = 0;
    std::string device = "";
    std::string sn = "";
    int interval = 10;
    HJStatEntryNotify statNotify = nullptr;
} HJEntryStatInfo;



typedef enum HJUnifyWrapperDataType
{
    HJUnifyWrapperDataType_Unknown = -1,
    HJUnifyWrapperDataType_NV12 = 0,
    HJUnifyWrapperDataType_RGBA = 1,
    HJUnifyWrapperDataType_RGB = 2,

} HJUnifyWrapperDataType;

typedef struct HJUnifyWrapperData
{
    HJUnifyWrapperDataType dataType = HJUnifyWrapperDataType_Unknown;
    unsigned char* data[4] = {nullptr, nullptr, nullptr, nullptr};
    int nPitch[4] = {0, 0, 0, 0};
    int width = 0;
    int height = 0;
    int64_t timeStamp = 0;
    int nElapseCount = 1;
    int nLatencyCnt = 0;
} HJUnifyWrapperData;

typedef struct HJUnifyWrapperRect
{
    int x = 0;
    int y = 0;
    int w = 0;
	int h = 0;
} HJUnifyWrapperRect;


#endif
