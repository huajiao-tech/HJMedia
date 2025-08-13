#include "HJLogPrintf.h"

#include "spdlog/spdlog.h"
#include "spdlog/async.h"
#include "spdlog/logger.h"
#include "spdlog/fmt/fmt.h"
#include "spdlog/fmt/bundled/printf.h"

#include "spdlog/sinks/stdout_sinks.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#if defined(_WIN32)
#   include "spdlog/sinks/msvc_sink.h"
#elif defined(__ANDROID__)
#   include "spdlog/sinks/android_sink.h"
#endif
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/daily_file_sink.h"

#ifdef WINDOWS
int vasprintf(char** strp, const char* fmt, va_list ap)
{
    // _vscprintf tells you how big the buffer needs to be
    int len = _vscprintf(fmt, ap);
    if (len == -1) {
        return -1;
    }
    size_t size = (size_t)len + 1;
    char* str = (char*)malloc(size);
    if (!str) {
        return -1;
    }
    // _vsprintf_s is the "secure" version of vsprintf
    int r = vsprintf_s(str, len + 1, fmt, ap);
    if (r == -1) {
        free(str);
        return -1;
    }
    *strp = str;
    return r;
}
#endif

void HJLogPrintf(HJLLevel level, const char* file, int line, const char* function, const char* tag, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    va_list args_copy;
    va_copy(args_copy, args);
    int size = std::vsnprintf(nullptr, 0, fmt, args_copy);
    va_end(args_copy);

    if (size > 0)
    {
        std::string result(size, '\0');
        std::vsnprintf(result.data(), result.size() + 1, fmt, args);
        spdlog::log(spdlog::source_loc{ file, line, function }, (spdlog::level::level_enum)level, result.c_str());
    }
    va_end(args);
}