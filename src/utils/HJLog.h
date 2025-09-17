//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJLogPrintf.h"
#include "HJObject.h"

NS_HJ_BEGIN
//***********************************************************************************//
typedef enum HJLMode {
    HJLMode_NONE = (1 << 0),
    HJLMode_CONSOLE = (1 << 1),
    HJLMode_FILE = (1 << 2),
    HJLMode_CALLBACK = (1 << 3),
} HJLMode;

class HJLogEntry : public HJObject
{
public:
    HJ_DECLARE_PUWTR(HJLogEntry);
    HJLogEntry(const size_t& key) {
        setID(key);
    }
    virtual ~HJLogEntry() {};

    void setRuntime(int64_t nowtime)
    {
        int64_t n = (nowtime - m_runtime) / m_interval;
        m_runtime = m_runtime + n * m_interval;
    }
public:
    uint64_t m_runtime = 0;          //us
    uint64_t m_interval = 1000;      //ms
};

class HJLog final {
public:
    static HJLog& Instance();
    explicit HJLog() {};

    int init(const bool valid = true, const std::string& logDir = "", const int logLevel = 0, const int logMode = HJLMode_FILE, const bool rotatingFile = true, const int max_size = 1024 * 1024 * 10, const int max_files = 5);
    void done();

    void log(HJLLevel level, const char* file, int line, const char* function, const std::string& tag, const std::string& msg);

    void perLog(HJLLevel level, const char* file, int line, const char* function, const std::string& tag, const std::string& msg, int64_t interval);

    void vaprintf(HJLLevel level, const char* file, int line, const char* function, const std::string& tag, const char* fmt, va_list args);
    void mprintf(HJLLevel level, const char* file, int line, const char* function, const std::string& tag, const char* fmt, ...);
    //
    void setValid(const bool valid) {
        m_valid = valid;
    }
    const bool getValid() const {
        return m_valid;
    }
    std::string getLogDir() const {
        return m_logDir;
    }
    void reset() {
        HJ_AUTOU_LOCK(m_mutex);
        m_entrys.clear();
    }
private:
    static const std::string getFunctionName(const char* func);
    
    static size_t getHashKey(const char* file, int line, const char* function) {
        const char* safeFile = (file != nullptr) ? file : "";
        const char* safeFunction = (function != nullptr) ? function : "";

        std::string combined = std::string(safeFile) + ":" +
            std::to_string(line) + ":" +
            std::string(safeFunction);

        std::hash<std::string> hasher;
        return hasher(combined);
    }
private:
    std::atomic<bool>   m_valid = { true };
	std::string         m_logDir = { "" };
    //
    std::mutex          m_mutex;
    std::map<size_t, HJLogEntry::Ptr> m_entrys;
};
//***********************************************************************************//
#define HJLogL(level, msg, ...) ::HJ::HJLog::Instance().log(level, __FILENAME__, __LINE__, __FUNCTION__, HJ_LTAG, msg)
#define HJLogt(msg, ...) HJLogL(HJLOG_TRACE, msg, ##__VA_ARGS__)
#define HJLogd(msg, ...) HJLogL(HJLOG_DEBUG, msg, ##__VA_ARGS__)
#define HJLogi(msg, ...) HJLogL(HJLOG_INFO, msg, ##__VA_ARGS__)
#define HJLogw(msg, ...) HJLogL(HJLOG_WARN, msg, ##__VA_ARGS__)
#define HJLoge(msg, ...) HJLogL(HJLOG_ERROR, msg, ##__VA_ARGS__)
#define HJLoga(msg, ...) HJLogL(HJLOG_ALARM, msg, ##__VA_ARGS__)
#define HJLogf(msg, ...) HJLogL(HJLOG_FATAL, msg, ##__VA_ARGS__)

#define HJPERLogL(level, interval, msg) ::HJ::HJLog::Instance().perLog(level, __FILENAME__, __LINE__, __FUNCTION__, HJ_LTAG, msg, interval)
#define HJPERLogt(interval, msg) HJPERLogL(HJLOG_TRACE, interval, msg)
#define HJPERLogd(interval, msg) HJPERLogL(HJLOG_DEBUG, interval, msg)
#define HJPERLogi(interval, msg) HJPERLogL(HJLOG_INFO,  interval, msg)
#define HJPERLogw(interval, msg) HJPERLogL(HJLOG_WARN,  interval, msg)
#define HJPERLoge(interval, msg) HJPERLogL(HJLOG_ERROR, interval, msg)
#define HJPERLoga(interval, msg) HJPERLogL(HJLOG_ALARM, interval, msg)
#define HJPERLogf(interval, msg) HJPERLogL(HJLOG_FATAL, interval, msg)

#define HJPER1Logi(msg) HJPERLogi(1000, msg)
#define HJPER2Logi(msg) HJPERLogi(2000, msg)
#define HJPER5Logi(msg) HJPERLogi(5000, msg)

//***********************************************************************************//
class HJLogStream : public std::ostringstream, public HJObject {
public:
    using Ptr = std::shared_ptr<HJLogStream>;
    HJLogStream(HJLLevel level, const char* file, int line, const char* function, const std::string& tag = HJ_LTAG);
    HJLogStream(const HJLogStream& other);
    ~HJLogStream();
    
private:
    HJLLevel       m_level;
    std::string     m_file;
    std::string     m_func;
    int             m_line;
    std::string     m_tag;
};

NS_HJ_END
