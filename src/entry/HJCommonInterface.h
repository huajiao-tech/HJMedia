#pragma once

#include <string>
#include <functional>

using HJNAPIEntryNotify = std::function<void(int i_type, const std::string& i_msgInfo)>;
using HJStatEntryNotify = std::function<void(const std::string& i_name, int i_nType, const std::string& i_info)>;

typedef enum HJEntryType {
    HJEntryType_Pusher,
    HJEntryType_Player,
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

typedef struct HJEntryStatInfo
{
    bool bUseStat = false;
    int64_t uid = 0;
    std::string device = "";
    std::string sn = "";
    int interval = 10;
    HJStatEntryNotify statNotify = nullptr;
} HJEntryStatInfo;




