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

class HJLog final {
public:
    static HJLog& Instance();
    explicit HJLog() {};

    int init(const bool valid = true, const std::string& logDir = "", const int logLevel = 0, const int logMode = HJLMode_FILE, const bool rotatingFile = true, const int max_size = 1024 * 1024 * 10, const int max_files = 5);
    void done();

    void log(HJLLevel level, const char* file, int line, const char* function, const std::string& tag, const std::string& msg);

    //template <typename... Args>
    //void fmtLog(HJLLevel level, const char* file, int line, const char* function, const std::string& tag, const char* msg, const Args &... args);

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

    //template <typename... Args>
    //static std::string formatString(const char* msg, const Args &... args);
private:
    static const std::string getFunctionName(const char* func);
    
private:
    std::atomic<bool>   m_valid = { true };
	std::string         m_logDir = { "" };
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

//#define HJFMTLogL(level, msg, ...) ::HJ::HJLog::Instance().fmtLog(level, __FILENAME__, __LINE__, __FUNCTION__, HJ_LTAG, msg, ##__VA_ARGS__)
//#define HJFLogt(msg, ...) HJFMTLogL(HJLOG_TRACE, msg, ##__VA_ARGS__)
//#define HJFLogd(msg, ...) HJFMTLogL(HJLOG_DEBUG, msg, ##__VA_ARGS__)
//#define HJFLogi(msg, ...) HJFMTLogL(HJLOG_INFO, msg, ##__VA_ARGS__)
//#define HJFLogw(msg, ...) HJFMTLogL(HJLOG_WARN, msg, ##__VA_ARGS__)
//#define HJFLoge(msg, ...) HJFMTLogL(HJLOG_ERROR, msg, ##__VA_ARGS__)
//#define HJFLoga(msg, ...) HJFMTLogL(HJLOG_ALARM, msg, ##__VA_ARGS__)
//#define HJFLogf(msg, ...) HJFMTLogL(HJLOG_FATAL, msg, ##__VA_ARGS__)

//#define HJFMT(msg, ...) ::HJ::HJLog::formatString(msg, ##__VA_ARGS__)

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
