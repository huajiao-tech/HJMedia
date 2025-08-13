#ifndef _HJLOGS_PRINTF_H_H_20240919
#define _HJLOGS_PRINTF_H_H_20240919

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#   define __FILENAME__ (strrchr(__FILE__, '\\') ? (strrchr(__FILE__, '\\') + 1) : __FILE__)
#else
#   define __FILENAME__ (strrchr(__FILE__, '/') ? (strrchr(__FILE__, '/') + 1) : __FILE__)
#endif

#define HJ_LTAG  "HJ"

typedef enum HJLLevel {
	HJLOG_TRACE = 0,
	HJLOG_DEBUG,
	HJLOG_INFO,
	HJLOG_WARN,
	HJLOG_ERROR,
	HJLOG_ALARM,
	HJLOG_FATAL
} HJLLevel;

#ifdef WINDOWS
	int vasprintf(char** strp, const char* fmt, va_list ap);
#endif

void HJLogPrintf(HJLLevel level, const char* file, int line, const char* function, const char* tag, const char* fmt, ...);
//***********************************************************************************//
#define JFMTLogP(level, msg, ...) HJLogPrintf(level, __FILENAME__, __LINE__, __FUNCTION__, HJ_LTAG, msg, ##__VA_ARGS__)
#define JPLogt(msg, ...) JFMTLogP(HJLOG_TRACE, msg, ##__VA_ARGS__)
#define JPLogd(msg, ...) JFMTLogP(HJLOG_DEBUG, msg, ##__VA_ARGS__)
#define JPLogi(msg, ...) JFMTLogP(HJLOG_INFO, msg, ##__VA_ARGS__)
#define JPLogw(msg, ...) JFMTLogP(HJLOG_WARN, msg, ##__VA_ARGS__)
#define JPLoge(msg, ...) JFMTLogP(HJLOG_ERROR, msg, ##__VA_ARGS__)
#define JPLoga(msg, ...) JFMTLogP(HJLOG_ALARM, msg, ##__VA_ARGS__)
#define JPLogf(msg, ...) JFMTLogP(HJLOG_FATAL, msg, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif  //_HJLOGS_PRINTF_H_H_20240919
