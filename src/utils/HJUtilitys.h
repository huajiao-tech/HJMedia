//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJVersion.h"
#include "HJBuffer.h"
//#include "HJByteBuffer.h"
#include "HJCommons.h"
#include "HJLog.h"
#include "HJTime.h"
#include "HJAny.h"

NS_HJ_BEGIN
#define HJ_UUID_LEN_DEFAULT    16
//***********************************************************************************//
class HJUtilitys
{
public:
    template<typename T>
    static const std::string toString(const T& data) {
        return std::move(std::to_string(data));
    }
    template<typename T>
    static const std::wstring toWString(const T& data) {
        return std::move(std::to_wstring(data));
    }
    template<typename T>
    static std::string toSString(T arg) {
        std::stringstream ss;
        ss << arg;
        return ss.str();
    }
    template <class T>
    static T stringToNum(const std::string& str) {
        std::istringstream iss(str);
        T num;
        iss >> num;
        return num;
    }
    
    static std::string makeRandStr(int sz = 16, bool printable = true);
    static std::string hexDump(const void *buf, size_t len);
    static std::string hexMem(const void* buf, size_t len);
    static std::string exePath(bool isExe = true);
    static std::string exeDir(bool isExe = true);
    static std::string exeName(bool isExe = true);
    static std::string logDir();
    static std::string dumpsDir();
    static std::string checkDir(const std::string& dir);
    static std::string concatenateDir(const std::string& pre, const std::string& suf);
    static std::string concatenatePath(const std::string& dir, const std::string& filename);
    static std::string concateString(const std::string& pre, const std::string& suf);
    static std::string convertBackslashesToForward(const std::string& dir);
    static std::string getDirectory(const std::string& dir);
    //
    static std::string m_globalWorkDir;
    static void setWorkDir(const std::string& dir);
    static std::string& getWorkDir();

    static std::vector<std::string> split(const std::string& s, const char *delim);
    //去除前后的空格、回车符、制表符...
    static std::string& trim(std::string &s,const std::string &chars=" \r\n\t");
    static std::string trim(std::string &&s,const std::string &chars=" \r\n\t");
    // string转小写
    static std::string &strToLower(std::string &str);
    static std::string strToLower(std::string &&str);
    // string转大写
    static std::string &strToUpper(std::string &str);
    static std::string strToUpper(std::string &&str);
    //替换子字符串
    static void replace(std::string &str, const std::string &old_str, const std::string &new_str);
    //判断是否为ip
//    static bool isIP(const char *str);
    static bool isLocalhost(const std::string& url);
    //字符串是否以xx开头
    static bool startWith(const std::string &str, const std::string &substr);
    //字符串是否以xx结尾
    static bool endWith(const std::string &str, const std::string &substr);
    static bool containWith(const std::string& str, const std::string& substr);
    //ANSI & UTF8
    static std::string AnsiToUtf8(const std::string& ansiString);
    static std::string Utf8ToAnsi(const std::string& utf8String);
    static std::string WideStringToString(const std::wstring& wstr);
    static std::wstring StringToWideString(const std::string& str);
    static std::wstring s2ws(const std::string& str);
    static std::string ws2s(const std::wstring& wstr);
    
    static void removeTrailingNulls(std::string& str);
    //
    static bool isInteger(double number);
    static bool memoryCompare(const uint8_t* p0, const size_t size0, const uint8_t* p1, const size_t size1);
    //
    template<typename T>
    static size_t hash(T ctx) {
        return std::hash<T>()(ctx);
    }
    
    template< typename... Args >
    static std::string formatString(const char* format, Args... args)
    {
        size_t length = std::snprintf(nullptr, 0, format, args...);
        if (length <= 0) {
            return "";
        }
        std::string str(length + 1, '\0');
        std::snprintf(str.data(), str.size(), format, args...);
        return str;
    }

    static const std::string getFunctionName(const char* func) {
        if (!func) {
            return "";
        }
#ifndef _WIN32
        return std::string(func);
#else
        auto pos = strrchr(func, ':');
        return std::string(pos ? pos + 1 : func);
#endif
    }

    static const std::string removeSuffix(const std::string& str);
    static const std::string getSuffix(const std::string& str);
    static const std::string getPrefix(const std::string& str);
    
    static std::string limitString(const char *name, size_t max_size);

    static std::vector<uint8_t> makeBytes(const char* s) {
        return std::vector<uint8_t>(s, s + std::strlen(s));
    }
    static std::vector<uint8_t> makeBytes(const std::string& str) {
        return std::vector<uint8_t>(str.begin(), str.end());
    }
    /**
     * 设置线程名
     */
    static void setThreadName(const char *name);

    /**
     * 获取线程名
     */
    static std::string getThreadName();
    
    /**
     *
     */
    static unsigned char randomChar() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        return static_cast<unsigned char>(dis(gen));
    }
    /**
     * [0, 100)
     */
    static int random() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 100);
        return static_cast<int>(dis(gen));
    }
    /**
     * [min, max)
     */
    static int random(int min, int max) {
#if defined(HJ_OS_DARWIN)
        return (int)(min + arc4random() % (max - min + 1));
#else
        return (int)(min + rand() % (max - min + 1));
#endif
    }
    /**
     * [0.0, 1.0)
     */
    static double randomf() {
        std::random_device rd;
        std::mt19937 gen(rd());
        return std::generate_canonical<double, 10>(gen);
    }
    static double randomf(double min, double max) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(min, max);
        return static_cast<double>(dis(gen));
    }

    static std::string generateUUID(const unsigned int len = HJ_UUID_LEN_DEFAULT) {
        std::stringstream ss;
        for(unsigned int i = 0; i < len; i++) {
            auto rc = randomChar();
            std::stringstream hexstream;
            hexstream << std::hex << int(rc);
            auto hex = hexstream.str();
            ss << (hex.length() < 2 ? '0' + hex : hex);
        }
        return ss.str();
    }

    static std::vector<uint8_t> parseUuidTo16Bytes(const std::string& uuid);
    static std::string uuidBytesToHex(const uint8_t* p16);

    static std::string getTimeStr(const char* fmt, time_t time = 0);
    static std::string getTimeToString();
    static std::string formatMSToString(int64_t t);
    static std::tuple<std::string, std::string, std::string> parseRtmpUrl(std::string url);
    static void splitParam(const std::string& param, std::vector<std::pair<std::string, std::string>>& result);
    static std::vector<std::pair<std::string, std::string>> parseUrl(const std::string& url);
    static std::string getCoreUrl(const std::string& url);
private:
    static bool hexIsSafe(uint8_t b) {
        return b >= ' ' && b < 128;;
    }
#if defined(__MACH__) || defined(__APPLE__)
    static int uvExePath(char *buffer, int *size);
#endif
};

#define HJ2String(v)   HJ::HJUtilitys::toString(v)
#define HJ2STR(v)      HJ2String(v)
#define HJ2SSTR(v)     HJ::HJUtilitys::toSString(v)
#define HJSTR2PTR(s)   (void *)HJ::HJUtilitys::stringToNum<size_t>(s)
#define HJSTR2i(s)     HJ::HJUtilitys::stringToNum<int>(s)
#define HJSTR(v)       std::string(v)      //const char * -> std::string

#define HJAnsiToUtf8(s) HJ::HJUtilitys::AnsiToUtf8(s)
#define HJUtf8ToAnsi(s) HJ::HJUtilitys::Utf8ToAnsi(s)

#define HJCurrentDirectory() HJ::HJUtilitys::getDirectory(__FILE__)
#define HJConcateDirectory(pre, suf) HJ::HJUtilitys::concatenateDir(pre, suf)

//***********************************************************************************//
class HJNonCopyable {
protected:
    HJNonCopyable() {}
    ~HJNonCopyable() {}
private:
    HJNonCopyable(const HJNonCopyable &that) = delete;
    HJNonCopyable(HJNonCopyable &&that) = delete;
    HJNonCopyable &operator=(const HJNonCopyable &that) = delete;
    HJNonCopyable &operator=(HJNonCopyable &&that) = delete;
};

//***********************************************************************************//
template <typename Mutex, typename CondVar>
class HJBaseSemaphore {
public:
    using Ptr = std::shared_ptr<HJBaseSemaphore<Mutex, CondVar>>;
    //
    explicit HJBaseSemaphore(size_t count = 0, bool cog = true) 
        : mCount(count)
        , m_cog(cog) { }
    HJBaseSemaphore(const HJBaseSemaphore&) = delete;
    HJBaseSemaphore(HJBaseSemaphore&&) = delete;
    HJBaseSemaphore& operator=(const HJBaseSemaphore&) = delete;
    HJBaseSemaphore& operator=(HJBaseSemaphore&&) = delete;
    //
    void notify(uint32_t n = 1) {
        HJ_AUTO_LOCK(mMutex);
        mCount += n;
        if(1 == n) {
            mCv.notify_one();
        } else {
            mCv.notify_all();
        }
    }
    void wait() {
        HJ_AUTOU_LOCK(mMutex);
        mCv.wait(lock, [&]{ return mCount > 0; });
        --mCount;
    }
    bool tryWait() {
        HJ_AUTO_LOCK(mMutex);
        if (mCount > 0) {
            --mCount;
            return true;
        }
        return false;
    }
    template<class Rep, class Period>
    bool waitFor(const std::chrono::duration<Rep, Period>& d) {
        HJ_AUTOU_LOCK(mMutex);
        auto finished = mCv.wait_for(lock, d);
        if (finished == std::cv_status::timeout) {
            if (m_cog) {
                --mCount;
            }
            return true;
        }
        return false;
    }
    /**
    * notity before wait
    */
    template<class Rep, class Period>
    bool waitBFor(const std::chrono::duration<Rep, Period>& d) {
        HJ_AUTOU_LOCK(mMutex);
        auto finished = mCv.wait_for(lock, d, [&]{ return mCount > 0; });
        if(finished) {
            if (m_cog) {
                --mCount;
            }
            return true;
        }
        return false;
    }
    template<class Clock, class Duration>
    bool waitUntil(const std::chrono::time_point<Clock, Duration>& t) {
        HJ_AUTOU_LOCK(mMutex);
        auto finished = mCv.wait_until(lock, t);
        if (finished == std::cv_status::timeout) {
            if (m_cog) {
                --mCount;
            }
            return true;
        }
        return false; //finished;
    }
    /**
    * notity before wait
    */
    template<class Clock, class Duration>
    bool waitBUntil(const std::chrono::time_point<Clock, Duration>& t) {
        HJ_AUTOU_LOCK(mMutex);
        auto finished = mCv.wait_until(lock, t, [&]{ return mCount > 0; });
        if (finished) {
            if (m_cog) {
                --mCount;
            }
            return true;
        }
        return false; //finished;
    }
    const size_t getCount() const {
        return mCount;
    }
    void reset() {
        HJ_AUTO_LOCK(mMutex);
        mCount = 0;
    }
private:
    Mutex       mMutex;
    CondVar     mCv;
    size_t      mCount = 0;
    bool        m_cog = true;
};
using HJSemaphore = HJBaseSemaphore<std::mutex, std::condition_variable>;

//***********************************************************************************//
class HJCondition {
public:
    using Ptr = std::shared_ptr<HJCondition>;
    ~HJCondition() = default;

    void wait() {
        HJ_AUTOU_LOCK(m_mutex);
        m_cv.wait(lock, [this] { return m_ready; });
        m_ready = false;
    }

    template <class Rep, class Period>
    bool wait_for(const std::chrono::duration<Rep, Period>& rel_time) {
        HJ_AUTOU_LOCK(m_mutex);
        bool is_ready = m_cv.wait_for(lock, rel_time, [this] { return m_ready; });
        if (is_ready) {
            m_ready = false;
        }
        return is_ready;
    }

    void notify(uint32_t n = 1) {
        HJ_AUTOU_LOCK(m_mutex);
        m_ready = true;
        if (n == 1) {
            m_cv.notify_one();
        } else {
            m_cv.notify_all();
        }
    }

    void reset() {
        HJ_AUTOU_LOCK(m_mutex);
        m_ready = false;
    }
private:
    bool m_ready = false;
    std::mutex m_mutex;
    std::condition_variable m_cv;
};
//***********************************************************************************//
template<typename T>
class HJList : public std::list<T> {
public:
    template<typename ... ARGS>
    HJList(ARGS &&...args) : std::list<T>(std::forward<ARGS>(args)...) {};
    ~HJList() = default;

    void append(HJList<T> &other) {
        if (other.empty()) {
            return;
        }
        this->insert(this->end(), other.begin(), other.end());
        other.clear();
    }

    template<typename FUNC>
    void for_each(FUNC &&func) {
        for (auto &t : *this) {
            func(t);
        }
    }

    template<typename FUNC>
    void for_each(FUNC &&func) const {
        for (auto &t : *this) {
            func(t);
        }
    }
};

//***********************************************************************************//
template<typename T>
struct HJFuncTraits;
//普通函数
template<typename Ret, typename... Args>
struct HJFuncTraits<Ret(Args...)>
{
public:
    enum { arity = sizeof...(Args) };
    typedef Ret FunctionType(Args...);
    typedef Ret RetType;
    using STLFuncType = std::function<FunctionType>;
    typedef Ret(*pointer)(Args...);

    template<size_t I>
    struct args
    {
        static_assert(I < arity, "index is out of range, index must less than sizeof Args");
        using type = typename std::tuple_element<I, std::tuple<Args...> >::type;
    };
};
//函数指针
template<typename Ret, typename... Args>
struct HJFuncTraits<Ret(*)(Args...)> : HJFuncTraits<Ret(Args...)>{};

//std::function
template <typename Ret, typename... Args>
struct HJFuncTraits<std::function<Ret(Args...)>> : HJFuncTraits<Ret(Args...)>{};

//member function
#define HJ_FUNC_TRAITS(...) \
    template <typename ReturnType, typename ClassType, typename... Args>\
    struct HJFuncTraits<ReturnType(ClassType::*)(Args...) __VA_ARGS__> : HJFuncTraits<ReturnType(Args...)>{}; \

HJ_FUNC_TRAITS()
HJ_FUNC_TRAITS(const)
HJ_FUNC_TRAITS(volatile)
HJ_FUNC_TRAITS(const volatile)

//函数对象
template<typename Callable>
struct HJFuncTraits : HJFuncTraits<decltype(&Callable::operator())>{};

//***********************************************************************************//
class HJCreator {
public:
    template<typename C, typename ...ArgsType>
    static std::shared_ptr<C> create(ArgsType &&...args) {
        std::shared_ptr<C> ret(new C(std::forward<ArgsType>(args)...), [](C* ptr) {
            ptr->onDestory();
            delete ptr;
            });
        ret->onCreate();
        return ret;
    }
    template<typename C, typename ...ArgsType>
    static C* createp(ArgsType &&...args) {
        return new C(std::forward<ArgsType>(args)...);
    }
    template<typename C>
    static void freep(HJHND& the) {
        if (!the) {
            return;
        }
        C* obj = (C*)the;
        delete obj;
        the = NULL;
    }
private:
    HJCreator() = default;
    ~HJCreator() = default;
};

//***********************************************************************************//
class HJOnceToken {
public:
    using task = std::function<void(void)>;

    template<typename FUNC>
    HJOnceToken(const FUNC &onConstructed, task onDestructed = nullptr) {
        onConstructed();
        m_onDestructed = std::move(onDestructed);
    }
    HJOnceToken(std::nullptr_t, task onDestructed = nullptr) {
        m_onDestructed = std::move(onDestructed);
    }
    ~HJOnceToken() {
        if (m_onDestructed) {
            m_onDestructed();
        }
    }
private:
    HJOnceToken() = delete;
    HJOnceToken(const HJOnceToken &) = delete;
    HJOnceToken(HJOnceToken &&) = delete;
    HJOnceToken &operator=(const HJOnceToken &) = delete;
    HJOnceToken &operator=(HJOnceToken &&) = delete;
private:
    task m_onDestructed;
};

//***********************************************************************************//
template <typename T> 
class HJSingleton
{
private:
    using Ptr = std::shared_ptr<T>;
    HJSingleton(const HJSingleton<T>& other);
    HJSingleton& operator=(const HJSingleton<T>& other);
public:
    HJSingleton(){
        mSingleton.reset((T *)this);
    }
    ~HJSingleton() {
        mSingleton = nullptr;
    }
    static Ptr& getSingleton() {
        return mSingleton;
    }
    static T* getInstance() {
        return mSingleton ? mSingleton.get() : NULL;
    }
protected:
    static Ptr mSingleton;
};

NS_HJ_END
