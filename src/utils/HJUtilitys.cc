//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <algorithm>
#include <random>
#include <filesystem>
#include "HJUtilitys.h"
#if defined(HJ_OS_HARMONY)
#include <unistd.h> //Harmonyos readlink need this head file
#endif
#include <locale> 
#include <codecvt>
#include <chrono>
#if defined(HJ_HAVE_REGEX)
#include <regex>
#endif
#if defined(_WIN32)
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
extern "C" const IMAGE_DOS_HEADER __ImageBase;
#endif // defined(_WIN32)

#if defined(__MACH__) || defined(__APPLE__)
#include <limits.h>
#include <mach-o/dyld.h> /* _NSGetExecutablePath */
#include "osys/HJISysUtils.h"
#endif


NS_HJ_BEGIN
//***********************************************************************************//
static constexpr char CCH[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
std::string HJUtilitys::m_globalWorkDir = "";
//***********************************************************************************//
std::string HJUtilitys::makeRandStr(int sz, bool printable) {
    std::string ret;
    ret.resize(sz);
    std::mt19937 rng(std::random_device{}());
    for (int i = 0; i < sz; ++i) {
        if (printable) {
            uint32_t x = rng() % (sizeof(CCH) - 1);
            ret[i] = CCH[x];
        } else {
            ret[i] = rng() % 0xFF;
        }
    }
    return ret;
}

std::string HJUtilitys::hexDump(const void *buf, size_t len) {
    std::string ret("\r\n");
    char tmp[8];
    const uint8_t *data = (const uint8_t *) buf;
    for (size_t i = 0; i < len; i += 16) {
        for (int j = 0; j < 16; ++j) {
            if (i + j < len) {
                int sz = snprintf(tmp, sizeof(tmp), "%.2x ", data[i + j]);
                ret.append(tmp, sz);
            } else {
                int sz = snprintf(tmp, sizeof(tmp), "   ");
                ret.append(tmp, sz);
            }
        }
        for (int j = 0; j < 16; ++j) {
            if (i + j < len) {
                ret += (hexIsSafe(data[i + j]) ? data[i + j] : '.');
            } else {
                ret += (' ');
            }
        }
        ret += ('\n');
    }
    return ret;
}

std::string HJUtilitys::hexMem(const void *buf, size_t len) {
    std::string ret;
    char tmp[8];
    const uint8_t *data = (const uint8_t *) buf;
    for (size_t i = 0; i < len; ++i) {
        int sz = sprintf(tmp, "%.2x ", data[i]);
        ret.append(tmp, sz);
    }
    return ret;
}

#if defined(__MACH__) || defined(__APPLE__)
int HJUtilitys::uvExePath(char *buffer, int *size) {
    /* realpath(exepath) may be > PATH_MAX so double it to be on the safe side. */
    char abspath[PATH_MAX * 2 + 1];
    char exepath[PATH_MAX + 1];
    uint32_t exepath_size;
    size_t abspath_size;

    if (buffer == nullptr || size == nullptr || *size == 0)
        return -EINVAL;

    exepath_size = sizeof(exepath);
    if (_NSGetExecutablePath(exepath, &exepath_size))
        return -EIO;

    if (realpath(exepath, abspath) != abspath)
        return -errno;

    abspath_size = strlen(abspath);
    if (abspath_size == 0)
        return -EIO;

    *size -= 1;
    if ((size_t) *size > abspath_size)
        *size = (int)abspath_size;

    memcpy(buffer, abspath, *size);
    buffer[*size] = '\0';

    return 0;
}
#endif //defined(__MACH__) || defined(__APPLE__)

std::string HJUtilitys::exePath(bool isExe /*= true*/) {
    char buffer[1024 * 2 + 1] = {0};
    int n = -1;
#if defined(_WIN32)
    n = GetModuleFileNameA(isExe?nullptr:(HINSTANCE)&__ImageBase, buffer, sizeof(buffer));
#elif defined(__MACH__) || defined(__APPLE__)
    n = sizeof(buffer);
    if (uvExePath(buffer, &n) != 0) {
        n = -1;
    }
#elif defined(ANDROID)
    // “/sdcard”， “/mnt/sdcard”， “/storage/emulated/0”
    n = readlink("/sdcard", buffer, sizeof(buffer));
#elif defined(__linux__)
    n = readlink("/proc/self/exe", buffer, sizeof(buffer));
#endif

    std::string filePath;
    if (n <= 0) {
        filePath = "./";
    } else {
        filePath = buffer;
    }

#if defined(_WIN32)
    //windows下把路径统一转换层unix风格，因为后续都是按照unix风格处理的
    for (auto &ch : filePath) {
        if (ch == '\\') {
            ch = '/';
        }
    }
#endif //defined(_WIN32)

    return filePath;
}

std::string HJUtilitys::exeDir(bool isExe /*= true*/) {
    auto path = exePath(isExe);
    return path.substr(0, path.rfind('/') + 1);
}

std::string HJUtilitys::exeName(bool isExe /*= true*/) {
    auto path = exePath(isExe);
    return path.substr(path.rfind('/') + 1);
}

std::string HJUtilitys::logDir() {
    std::string logDir = "";
#if defined(HJ_OS_DARWIN)
    logDir = HJISysUtils::getDocumentsPath() + "/";
#elif defined(HJ_OS_ANDROID)
    logDir = "/storage/emulated/0/";
#else
    logDir = exeDir();
#endif
    return logDir;// + "mlog/";
}

std::string HJUtilitys::dumpsDir() {
    std::string dir = "";
#if defined(HJ_OS_DARWIN)
    dir = HJISysUtils::getDocumentsPath() + "/";
#else
    dir = exeDir();
#endif
    return dir + "dumps/";
}

std::string HJUtilitys::checkDir(const std::string& dir)
{
    if (dir.empty()) {
        return dir;
    }
    if (dir.back() == '\0') {
        std::string str = dir.substr(0, dir.size() - 1);
        return HJUtilitys::checkDir(str);
    } else {
        if (dir.back() == '/' || dir.back() == '\\') {
            return dir;
        }
        if (std::string::npos != dir.rfind('\\')) {
            return dir + "\\";
        }
        return dir + "/";
    }
}

const std::string HJUtilitys::removeSuffix(const std::string& str)
{
    return str.substr(0, str.rfind('.'));
}

const std::string HJUtilitys::getSuffix(const std::string& str)
{
    return str.substr(str.rfind('.'));
}

const std::string HJUtilitys::getPrefix(const std::string& str)
{
    size_t colonPos = str.find(':');
    if (colonPos != std::string::npos) {
        return str.substr(0, colonPos);
    }
    return "";
}

std::string HJUtilitys::concatenateDir(const std::string& pre, const std::string& suf)
{
    std::string res = HJUtilitys::checkDir(pre) + suf;
    return HJUtilitys::checkDir(res);
}

std::string HJUtilitys::concatenatePath(const std::string& dir, const std::string& filename)
{
    return HJUtilitys::checkDir(dir) + filename;
}

std::string HJUtilitys::concateString(const std::string& pre, const std::string& suf)
{
    if (pre.empty()) {
        return pre + suf;
    }
    if (pre.back() == '\0') {
        return pre.substr(0, pre.size() - 1) + suf;
    }
    return pre + suf;
}

std::string HJUtilitys::convertBackslashesToForward(const std::string& dir)
{
    std::string result = dir;
    std::replace(result.begin(), result.end(), '\\', '/');
    return result;
}

std::string HJUtilitys::getDirectory(const std::string& dir)
{
    std::filesystem::path pathName = HJUtilitys::convertBackslashesToForward(dir);
    return pathName.parent_path().string() + "/";
}

void HJUtilitys::setWorkDir(const std::string& dir)
{
    m_globalWorkDir = dir;
}
std::string& HJUtilitys::getWorkDir()
{
    return m_globalWorkDir;
}

std::vector<std::string> HJUtilitys::split(const std::string &s, const char *delim) {
    std::vector<std::string> ret;
    size_t last = 0;
    auto index = s.find(delim, last);
    while (index != std::string::npos) {
        if (index - last > 0) {
            ret.push_back(s.substr(last, index - last));
        }
        last = index + strlen(delim);
        index = s.find(delim, last);
    }
    if (!s.size() || s.size() - last > 0) {
        ret.push_back(s.substr(last));
    }
    return ret;
}

#define TRIM(s, chars) \
do{ \
    std::string map(0xFF, '\0'); \
    for (auto &ch : chars) { \
        map[(unsigned char &)ch] = '\1'; \
    } \
    while( s.size() && map.at((unsigned char &)s.back())) s.pop_back(); \
    while( s.size() && map.at((unsigned char &)s.front())) s.erase(0,1); \
}while(0);

//去除前后的空格、回车符、制表符
std::string& HJUtilitys::trim(std::string &s, const std::string &chars) {
    TRIM(s, chars);
    return s;
}

std::string HJUtilitys::trim(std::string &&s, const std::string &chars) {
    TRIM(s, chars);
    return std::move(s);
}

// string转小写
std::string& HJUtilitys::strToLower(std::string &str) {
    transform(str.begin(), str.end(), str.begin(), towlower);
    return str;
}

// string转大写
std::string& HJUtilitys::strToUpper(std::string &str) {
    transform(str.begin(), str.end(), str.begin(), towupper);
    return str;
}

// string转小写
std::string HJUtilitys::strToLower(std::string &&str) {
    transform(str.begin(), str.end(), str.begin(), towlower);
    return std::move(str);
}

// string转大写
std::string HJUtilitys::strToUpper(std::string &&str) {
    transform(str.begin(), str.end(), str.begin(), towupper);
    return std::move(str);
}

void HJUtilitys::replace(std::string &str, const std::string &old_str, const std::string &new_str) {
    if (old_str.empty() || old_str == new_str) {
        return;
    }
    auto pos = str.find(old_str);
    if (pos == std::string::npos) {
        return;
    }
    str.replace(pos, old_str.size(), new_str);
    replace(str, old_str, new_str);
}

bool HJUtilitys::startWith(const std::string &str, const std::string &substr) {
    return str.find(substr) == 0;
}

bool HJUtilitys::endWith(const std::string &str, const std::string &substr) {
    auto pos = str.rfind(substr);
    return pos != std::string::npos && pos == str.size() - substr.size();
}
bool HJUtilitys::containWith(const std::string& str, const std::string& substr) {
    return (std::string::npos != str.find(substr));
}

std::string HJUtilitys::AnsiToUtf8(const std::string& ansiString) {
#if defined(HJ_OS_WINDOWS)
    int requiredSize = MultiByteToWideChar(CP_ACP, 0, ansiString.c_str(), -1, nullptr, 0);
    if (requiredSize == 0) {
        return "";
    }
    std::wstring utf16String(requiredSize, L'\0');
    if (MultiByteToWideChar(CP_ACP, 0, ansiString.c_str(), -1, &utf16String[0], requiredSize) == 0) {
        return "";
    }
    requiredSize = WideCharToMultiByte(CP_UTF8, 0, utf16String.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (requiredSize == 0) {
        return "";
    }
    std::string utf8String(requiredSize, '\0');
    if (WideCharToMultiByte(CP_UTF8, 0, utf16String.c_str(), -1, &utf8String[0], requiredSize, nullptr, nullptr) == 0) {
        return "";
    }
    return utf8String.erase(utf8String.find_last_not_of("\0"));
#else
    return ansiString;
#endif
}

std::string HJUtilitys::Utf8ToAnsi(const std::string& utf8String) {
#if defined(HJ_OS_WINDOWS)
    int requiredSize = MultiByteToWideChar(CP_UTF8, 0, utf8String.c_str(), -1, nullptr, 0);
    if (requiredSize == 0) {
        return "";
    }
    std::wstring utf16String(requiredSize, L'\0');
    if (MultiByteToWideChar(CP_UTF8, 0, utf8String.c_str(), -1, &utf16String[0], requiredSize) == 0) {
        return "";
    }
    requiredSize = WideCharToMultiByte(CP_ACP, 0, utf16String.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (requiredSize == 0) {
        return "";
    }
    std::string ansiString(requiredSize, '\0');
    if (WideCharToMultiByte(CP_ACP, 0, utf16String.c_str(), -1, &ansiString[0], requiredSize, nullptr, nullptr) == 0) {
        return "";
    }
    return ansiString.erase(ansiString.find_last_not_of("\0"));
#else
    return utf8String;
#endif
}

std::string HJUtilitys::WideStringToString(const std::wstring& wstr)
{
    if (wstr.empty()) {
        return std::string();
    }
    size_t pos;
    size_t begin = 0;
    std::string ret;
#if defined(HJ_OS_WINDOWS)
    int size;
    pos = wstr.find(static_cast<wchar_t>(0), begin);
    while (pos != std::wstring::npos && begin < wstr.length())
    {
        std::wstring segment = std::wstring(&wstr[begin], pos - begin);
        size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, &segment[0], (int)segment.size(), NULL, 0, NULL, NULL);
        std::string converted = std::string(size, 0);
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, &segment[0], (int)segment.size(), &converted[0], (int)converted.size(), NULL, NULL);
        ret.append(converted);
        ret.append({ 0 });
        begin = pos + 1;
        pos = wstr.find(static_cast<wchar_t>(0), begin);
    }
    if (begin <= wstr.length())
    {
        std::wstring segment = std::wstring(&wstr[begin], wstr.length() - begin);
        size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, &segment[0], (int)segment.size(), NULL, 0, NULL, NULL);
        std::string converted = std::string(size, 0);
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, &segment[0], (int)segment.size(), &converted[0], (int)converted.size(), NULL, NULL);
        ret.append(converted);
    }
#elif defined(HJ_OS_LINUX) || defined(HJ_OS_MACOS) || defined(HJ_OS_EMSCRIPTEN)
    size_t size;
    pos = wstr.find(static_cast<wchar_t>(0), begin);
    while (pos != std::wstring::npos && begin < wstr.length())
    {
        std::wstring segment = std::wstring(&wstr[begin], pos - begin);
        size = wcstombs(nullptr, segment.c_str(), 0);
        std::string converted = std::string(size, 0);
        wcstombs(&converted[0], segment.c_str(), converted.size());
        ret.append(converted);
        ret.append({ 0 });
        begin = pos + 1;
        pos = wstr.find(static_cast<wchar_t>(0), begin);
    }
    if (begin <= wstr.length())
    {
        std::wstring segment = std::wstring(&wstr[begin], wstr.length() - begin);
        size = wcstombs(nullptr, segment.c_str(), 0);
        std::string converted = std::string(size, 0);
        wcstombs(&converted[0], segment.c_str(), converted.size());
        ret.append(converted);
    }
#else
    ret = ws2s(wstr);
#endif
    return ret;
}

std::wstring HJUtilitys::StringToWideString(const std::string& str)
{
    if (str.empty()){
        return std::wstring();
    }

    size_t pos;
    size_t begin = 0;
    std::wstring ret;
#if defined(HJ_OS_WINDOWS)
    int size = 0;
    pos = str.find(static_cast<char>(0), begin);
    while (pos != std::string::npos) {
        std::string segment = std::string(&str[begin], pos - begin);
        std::wstring converted = std::wstring(segment.size() + 1, 0);
        size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, &segment[0], (int)segment.size(), &converted[0], (int)converted.length());
        converted.resize(size);
        ret.append(converted);
        ret.append({ 0 });
        begin = pos + 1;
        pos = str.find(static_cast<char>(0), begin);
    }
    if (begin < str.length()) {
        std::string segment = std::string(&str[begin], str.length() - begin);
        std::wstring converted = std::wstring(segment.size() + 1, 0);
        size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, segment.c_str(), (int)segment.size(), &converted[0], (int)converted.length());
        converted.resize(size);
        ret.append(converted);
    }

#elif defined(HJ_OS_LINUX) || defined(HJ_OS_MACOS) || defined(HJ_OS_EMSCRIPTEN)
    size_t size;
    pos = str.find(static_cast<char>(0), begin);
    while (pos != std::string::npos)
    {
        std::string segment = std::string(&str[begin], pos - begin);
        std::wstring converted = std::wstring(segment.size(), 0);
        size = mbstowcs(&converted[0], &segment[0], converted.size());
        converted.resize(size);
        ret.append(converted);
        ret.append({ 0 });
        begin = pos + 1;
        pos = str.find(static_cast<char>(0), begin);
    }
    if (begin < str.length())
    {
        std::string segment = std::string(&str[begin], str.length() - begin);
        std::wstring converted = std::wstring(segment.size(), 0);
        size = mbstowcs(&converted[0], &segment[0], converted.size());
        converted.resize(size);
        ret.append(converted);
    }
#else
    ret = s2ws(str);
#endif
    return ret;
}

std::wstring HJUtilitys::s2ws(const std::string& str)
{
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;

    return converterX.from_bytes(str);
}

std::string HJUtilitys::ws2s(const std::wstring& wstr)
{
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;

    return converterX.to_bytes(wstr);
}
/**
 * \0\0\0
 */
void HJUtilitys::removeTrailingNulls(std::string& str) {
    auto it = std::find_if_not(str.rbegin(), str.rend(),
                               [](char c) { return c == '\0'; });
    size_t count = std::distance(str.rbegin(), it);
    str.erase(str.size() - count);
    return;
}

bool HJUtilitys::isInteger(double number)
{
    double fraction, integer;
    fraction = modf(number, &integer);
    return (fabs(fraction) < HJ_FLT_EPSILON);
}

bool HJUtilitys::memoryCompare(const uint8_t* p0, const size_t size0, const uint8_t* p1, const size_t size1)
{
    if (size0 != size1 || !p0 || !p1) {
        return false;
    }
    return !memcmp(p0, p1, size0);
}

//bool HJUtilitys::isIP(const char *str) {
//    return INADDR_NONE != inet_addr(str);
//}

#if defined(HJ_HAVE_REGEX)
bool HJUtilitys::isLocalhost(const std::string& url) {
    std::regex host_regex(R"((https?|rtsp|rtmp|ftp|sftp)://([^:/?#]+)(:[0-9]+)?)");
    std::smatch match;
    if (std::regex_search(url, match, host_regex)) {
        std::string host = match[2];
        return host == "localhost" || host == "127.0.0.1" || host == "::1";
    }
    return false;
}
#else
bool HJUtilitys::isLocalhost(const std::string& url) {
    size_t pos = url.find("://");
    if (pos != std::string::npos) {
        std::string hostPart = url.substr(pos + 3);
        pos = hostPart.find("/");
        if (pos != std::string::npos) {
            hostPart = hostPart.substr(0, pos);
        }
        pos = hostPart.find(":");
        if (pos != std::string::npos) {
            hostPart = hostPart.substr(0, pos);
        }
        return (hostPart == "localhost" || hostPart == "127.0.0.1" || hostPart == "::1");
    }
    return false;
}
#endif //

//static thread_local std::string thread_name;
std::string HJUtilitys::limitString(const char *name, size_t max_size) {
    std::string str = name;
    if (str.size() + 1 > max_size) {
        auto erased = str.size() + 1 - max_size + 3;
        str.replace(5, erased, "...");
    }
    return str;
}
void HJUtilitys::setThreadName(const char *name) {
    assert(name);
#if defined(__linux) || defined(__linux__) || defined(__MINGW32__)
    pthread_setname_np(pthread_self(), limitString(name, 16).data());
#elif defined(__MACH__) || defined(__APPLE__)
    pthread_setname_np(limitString(name, 32).data());
#elif defined(_MSC_VER)
    // SetThreadDescription was added in 1607 (aka RS1). Since we can't guarantee the user is running 1607 or later, we need to ask for the function from the kernel.
    using SetThreadDescriptionFunc = HRESULT(WINAPI * )(_In_ HANDLE hThread, _In_ PCWSTR lpThreadDescription);
    static auto setThreadDescription = reinterpret_cast<SetThreadDescriptionFunc>(::GetProcAddress(::GetModuleHandle("Kernel32.dll"), "SetThreadDescription"));
    if (setThreadDescription) {
        // Convert the thread name to Unicode
        wchar_t threadNameW[MAX_PATH];
        size_t numCharsConverted;
        errno_t wcharResult = mbstowcs_s(&numCharsConverted, threadNameW, name, MAX_PATH - 1);
        if (wcharResult == 0) {
            HRESULT hr = setThreadDescription(::GetCurrentThread(), threadNameW);
            if (!SUCCEEDED(hr)) {
                int i = 0;
                i++;
            }
        }
    } else {
        // For understanding the types and values used here, please see:
        // https://docs.microsoft.com/en-us/visualstudio/debugger/how-to-set-a-thread-name-in-native-code

        const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack(push, 8)
        struct THREADNAME_INFO {
            DWORD dwType = 0x1000; // Must be 0x1000
            LPCSTR szName;         // Pointer to name (in user address space)
            DWORD dwThreadID;      // Thread ID (-1 for caller thread)
            DWORD dwFlags = 0;     // Reserved for future use; must be zero
        };
#pragma pack(pop)

        THREADNAME_INFO info;
        info.szName = name;
        info.dwThreadID = (DWORD) - 1;

        __try{
                RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), reinterpret_cast<const ULONG_PTR *>(&info));
        } __except(GetExceptionCode() == MS_VC_EXCEPTION ? EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_EXECUTE_HANDLER) {
        }
    }
#else
    //thread_name = name ? name : "";
#endif
}

std::string HJUtilitys::getThreadName() {
#if ((defined(__linux) || defined(__linux__)) && !defined(ANDROID)) || (defined(__MACH__) || defined(__APPLE__)) || (defined(ANDROID) && __ANDROID_API__ >= 26) || defined(__MINGW32__)
    std::string ret;
    ret.resize(32);
    auto tid = pthread_self();
    pthread_getname_np(tid, (char *) ret.data(), ret.size());
    if (ret[0]) {
        ret.resize(strlen(ret.data()));
        return ret;
    }
    return std::to_string((uint64_t) tid);
#elif defined(_MSC_VER)
    using GetThreadDescriptionFunc = HRESULT(WINAPI * )(_In_ HANDLE hThread, _In_ PWSTR * ppszThreadDescription);
    static auto getThreadDescription = reinterpret_cast<GetThreadDescriptionFunc>(::GetProcAddress(::GetModuleHandleA("Kernel32.dll"), "GetThreadDescription"));

    if (!getThreadDescription) {
        std::ostringstream ss;
        ss << std::this_thread::get_id();
        return ss.str();
    } else {
        PWSTR data;
        HRESULT hr = getThreadDescription(GetCurrentThread(), &data);
        if (SUCCEEDED(hr) && data[0] != '\0') {
            char threadName[MAX_PATH];
            size_t numCharsConverted;
            errno_t charResult = wcstombs_s(&numCharsConverted, threadName, data, MAX_PATH - 1);
            if (charResult == 0) {
                LocalFree(data);
                std::ostringstream ss;
                ss << threadName;
                return ss.str();
            } else {
                return HJ2STR((uint64_t) GetCurrentThreadId());
            }
        } else {
            return HJ2STR((uint64_t) GetCurrentThreadId());
        }
    }
#else
    //if (!thread_name.empty()) {
    //    return thread_name;
    //}
    std::ostringstream ss;
    ss << std::this_thread::get_id();
    return ss.str();
#endif
}

std::string HJUtilitys::getTimeStr(const char* fmt, time_t time) {
    if (!time) {
        time = ::time(nullptr);
    }
    auto tm = HJTime::getLocalTime(time);
    char buffer[64];
    auto success = std::strftime(buffer, sizeof(buffer), fmt, &tm);
    return 0 == success ? std::string(fmt) : buffer;
}

std::string HJUtilitys::getTimeToString()
{
    std::string timeStr = HJUtilitys::getTimeStr("%Y_%m_%d_%H_%M_%S");
    uint64_t t = HJTime::getCurrentMicrosecond();
    std::string ums = HJUtilitys::formatString("_%03d_%03d", (t % HJ_US_PER_MS), ((t / HJ_US_PER_MS) % HJ_MS_PER_SEC)); //timeStr += ":" + HJ2STR(ms) + ":" + HJ2STR(us);
    return timeStr + ums;
}

/**
* 00:00:00:000
*/
std::string HJUtilitys::formatMSToString(int64_t t) {
    return HJUtilitys::formatString("%02d:%02d:%02d:%03d", ((t / (60 * 60 * HJ_MS_PER_SEC)) % 60), ((t / (60 * HJ_MS_PER_SEC)) % 60), ((t / HJ_MS_PER_SEC) % 60), (t % HJ_MS_PER_SEC));
}

std::tuple<std::string, std::string, std::string> HJUtilitys::parseRtmpUrl(std::string url)
{
    size_t question_mark_pos = url.find('?');
    std::string url_without_auth = url;
    std::string auth_param = "";
    std::string stream_name = "";
    std::string address = "";

    if (question_mark_pos != std::string::npos) {
        url_without_auth = url.substr(0, question_mark_pos);
        std::string query_string = url.substr(question_mark_pos + 1);

        size_t auth_key_pos = query_string.find("auth_key=");
        if (auth_key_pos != std::string::npos) {
            auth_param = query_string.substr(auth_key_pos + 9);
        }
    }

    size_t last_slash_pos = url_without_auth.find_last_of('/');
    if (last_slash_pos != std::string::npos) {
        stream_name = url_without_auth.substr(last_slash_pos + 1);
        address = url_without_auth.substr(0, last_slash_pos);
    }

    return { address, stream_name, auth_param };
}

void HJUtilitys::splitParam(const std::string& param, std::vector<std::pair<std::string, std::string>>& result)
{
    if (param.empty()) {
        return;
    }

    size_t equal_pos = param.find('=');
    std::string key;
    std::string value;

    if (equal_pos == std::string::npos) {
        key = param;
        value = "";
    }
    else {
        key = param.substr(0, equal_pos);
        value = param.substr(equal_pos + 1);
    }

    result.emplace_back(key, value);
}

std::vector<std::pair<std::string, std::string>> HJUtilitys::parseUrl(const std::string& url)
{
    std::vector<std::pair<std::string, std::string>> result;

    if (url.empty()) {
        return result;
    }

    size_t question_mark_pos = url.find('?');
    std::string core_url;
    std::string query_string;

    if (question_mark_pos == std::string::npos) {
        core_url = url;
        query_string = "";
    }
    else {
        core_url = url.substr(0, question_mark_pos);
        query_string = url.substr(question_mark_pos + 1);
    }

    result.emplace_back("url", core_url);
    if (query_string.empty()) {
        return result;
    }

    size_t start = 0;
    size_t ampersand_pos = query_string.find('&');
    while (ampersand_pos != std::string::npos) {
        std::string param = query_string.substr(start, ampersand_pos - start);
        splitParam(param, result);
        start = ampersand_pos + 1;
        ampersand_pos = query_string.find('&', start);
    }

    std::string last_param = query_string.substr(start);
    splitParam(last_param, result);

    return result;
}

std::string HJUtilitys::getCoreUrl(const std::string& url)
{
    if (url.empty()) {
        return "";
    }

    size_t question_mark_pos = url.find('?');
    std::string core_url = "";

    if (question_mark_pos == std::string::npos) {
        core_url = url;
    } else {
        core_url = url.substr(0, question_mark_pos);
    }

    return core_url;
}

NS_HJ_END
