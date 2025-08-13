#pragma once

#include <stdbool.h>
#define OHOS_LOG_TAG    "HJMedia"
enum HJMediaHOLogLevel {
    IL_INFO,
    IL_DEBUG,
    IL_WARN,
    IL_ERROR,
    IL_FATAL
};

#define LOGI(...) __ohos_log_print(IL_INFO, OHOS_LOG_TAG, __VA_ARGS__)
#define LOGW(...) __ohos_log_print(IL_WARN, OHOS_LOG_TAG, __VA_ARGS__)
#define LOGE(...) __ohos_log_print(IL_ERROR, OHOS_LOG_TAG, __VA_ARGS__)
#define LOGF(...) __ohos_log_print(IL_FATAL, OHOS_LOG_TAG, __VA_ARGS__)
#define LOGD(...) __ohos_log_print(IL_DEBUG, OHOS_LOG_TAG, __VA_ARGS__)

#define OHOS_LOG_BUF_SIZE (4096)
#ifdef __cplusplus
extern "C" {
#endif
extern bool OHOS_LOG_ON;//log开关、默认关
void __ohos_log_print(enum HJMediaHOLogLevel level, const char* tag, const char* fmt, ...);
void __ohos_log_print_debug(enum HJMediaHOLogLevel level, const char* tag,const char* file,int line, const char* fmt, ...);
#ifdef __cplusplus
}
#endif
