//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include <exception>
#include "HJFLog.h"
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
#elif defined(HarmonyOS)
#   include <spdlog/sinks/harmony_sink.h>
#endif
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "HJTime.h"

#define HJ_LOG_BUFFER_CACHE        1024 * 1024
//#define HJ_HAVE_ASYNC_LOG          1

NS_HJ_BEGIN
//***********************************************************************************//
#if defined(__ANDROID__)
#include <android/log.h>
#   define LOG_I(...) ((void)__android_log_print(ANDROID_LOG_ERROR, HJ_LTAG, __VA_ARGS__))
#else
#   define LOG_I(...) printf(__VA_ARGS__)
#endif

HJLog& HJLog::Instance() {
    static std::shared_ptr<HJLog> s_instance(new HJLog());
    static HJLog& s_instance_ref = *s_instance;
    return s_instance_ref;
}
//***********************************************************************************//
int HJLog::init(const bool valid/* = true*/, const std::string& logDir, const int logLevel/* = 0*/, const int logMode/* = HJLMODE_FILE*/, const bool rotatingFile, const int max_size, const int max_files)
{
    m_valid = valid;
    m_logDir = logDir;
    if (!m_valid) {
        return -1;
    }
//     LOG_I("init entry, logDir:%s \n", logDir.c_str());
    try {
#if HJ_HAVE_ASYNC_LOG
        spdlog::init_thread_pool(HJ_LOG_BUFFER_CACHE, 1);//std::thread::hardware_concurrency());
#endif
        //
        std::vector<spdlog::sink_ptr> sinks;
        //CONSOLE
        if (logMode & HJLMode_CONSOLE)
        {
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            sinks.push_back(console_sink);
#if defined(_WIN32)
            auto ms_sink = std::make_shared<spdlog::sinks::msvc_sink_mt>(false); //DebugView display; if (check_debugger_present_ && !IsDebuggerPresent()) not display log
            sinks.push_back(ms_sink);
#elif defined(__ANDROID__)
            auto as_sink = std::make_shared<spdlog::sinks::android_sink_mt>();
            sinks.push_back(as_sink);
#elif defined(HarmonyOS)
            auto as_sink = std::make_shared<spdlog::sinks::harmony_sink_mt>();
            sinks.push_back(as_sink);
#endif
//            LOG_I("create console log \n");
        }
        if ((logMode & HJLMode_FILE) && !logDir.empty())
        {
            std::string logName = "_HJLog.txt";
            spdlog::sink_ptr file_sink = nullptr;
            if (rotatingFile) {
                file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(logDir + logName, max_size, max_files);
            } else {
                file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logDir + logName, true);
            }
//            file_sink->set_level(spdlog::level::trace);
            sinks.push_back(file_sink);
//            LOG_I("create file log:%s \n", (logDir + logName).c_str());
        }
        std::shared_ptr<spdlog::logger> default_logger = nullptr;
#if HJ_HAVE_ASYNC_LOG
        default_logger = std::make_shared<spdlog::async_logger>("aysnc", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
#else
        default_logger = std::make_shared<spdlog::logger>("", sinks.begin(), sinks.end());
#endif
        
//        auto& inst = spdlog::details::registry::instance();
//        LOG_I("spdlog instance handle:%zu, this:%zu \n", (size_t)&inst, (size_t)this);
        spdlog::set_default_logger(default_logger);
        spdlog::set_pattern("%s(%#): [%L %D %T.%e %P %t %!] %v");
        spdlog::flush_on(spdlog::level::warn);
        spdlog::flush_every(std::chrono::seconds(1));
        spdlog::set_level((spdlog::level::level_enum)logLevel);
    }
    catch (std::exception& ex) {
        return -1;
    }
    HJFLogi("init log:{}, logDir:{}, logLevel:{}, logMode:{}, rotatingFile:{}", m_valid, logDir, logLevel, logMode, rotatingFile);
    return 0;
}

void HJLog::done()
{
    spdlog::drop_all();
    spdlog::shutdown();
    m_valid = false;
}

void HJLog::log(HJLLevel level, const char* file, int line, const char* function, const std::string& tag, const std::string& msg)
{
    if (!m_valid) {
        return;
    }
    //std::string massage = tag + ": " + msg;
    spdlog::log(spdlog::source_loc{ file, line, function }, (spdlog::level::level_enum)level, msg.c_str());
    return;
}

void HJLog::perLog(HJLLevel level, const char* file, int line, const char* function, const std::string& tag, const std::string& msg, int64_t interval)
{
    if (!m_valid) {
        return;
    }
    HJ_AUTOU_LOCK(m_mutex);
    size_t key = getHashKey(file, line, function);
    auto it = m_entrys.find(key);
    if (it != m_entrys.end()) 
    {
        auto& entry = it->second;
        auto now = HJCurrentSteadyUS();
        if (now >= entry->m_runtime + entry->m_interval) {
            entry->setRuntime(now);
            //
            log(level, file, line, function, tag, msg);
        }
    } else {
        auto entry = HJCreates<HJLogEntry>(key);
        entry->m_runtime = HJCurrentSteadyUS();
        entry->m_interval = HJ_MS_TO_US(interval);
        m_entrys[key] = entry;
        //
        log(level, file, line, function, tag, msg);
    }
    return;
}

void HJLog::vaprintf(HJLLevel level, const char* file, int line, const char* function, const std::string& tag, const char* fmt, va_list args)
{
    if (!m_valid) {
        return;
    }
    int size = std::vsnprintf(nullptr, 0, fmt, args);
    if (size > 0) {
        std::string msg(size + 1, '\0');
        std::vsnprintf(msg.data(), msg.size(), fmt, args);
        //
        std::string massage = "<" + tag + ">: " + msg;
        spdlog::log(spdlog::source_loc{ file, line, function }, (spdlog::level::level_enum)level, massage.c_str());
    }
    return;
}

void HJLog::mprintf(HJLLevel level, const char* file, int line, const char* function, const std::string& tag, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    HJLog::vaprintf(level, file, line, function, tag, fmt, args);
    va_end(args);
    return;
}

//***********************************************************************************//
HJLogStream::HJLogStream(HJLLevel level, const char *file, int line, const char *function, const std::string& tag)
    : m_level(level)
    , m_file(file)
    , m_func(function)
    , m_line(line)
    , m_tag(tag)
{
}
HJLogStream::HJLogStream(const HJLogStream& other)
    : m_level(other.m_level)
    , m_file(other.m_file)
    , m_func(other.m_func)
    , m_line(other.m_line)
    , m_tag(other.m_tag)
{
}

HJLogStream::~HJLogStream() {
    if (!str().empty()) {
        spdlog::log(spdlog::source_loc{ m_file.c_str(), m_line, m_func.c_str() }, (spdlog::level::level_enum)m_level, ("<" + m_tag + ">: " + str()).c_str());
    }
}

//***********************************************************************************//
NS_HJ_END
