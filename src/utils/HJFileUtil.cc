//***********************************************************************************//
//HJediaKit FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include <cstdio>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <sys/stat.h>
#include "HJUtilitys.h"
#include "HJFLog.h"
#include "HJFileUtil.h"

namespace fs = std::filesystem;

NS_HJ_BEGIN
//***********************************************************************************//
bool HJFileUtil::create_path(const char* file, unsigned int mod) {
    try {
        fs::path path(file);
        // 递归创建目录
        if (!fs::create_directories(path)) {
            return fs::exists(path); // 目录已存在也返回true
        }
        // 设置权限（跨平台适配）
#ifdef _WIN32
        // Windows权限处理简化
#else
        fs::permissions(path, static_cast<fs::perms>(mod), fs::perm_options::replace);
#endif
        return true;
    } catch (const fs::filesystem_error& e) {
        HJFLogw("error, create path failed:{}", e.what());
        return false;
    }
}

bool HJFileUtil::makeDir(const char* file) {
#ifdef _WIN32
    return create_path(file, 0);
#else
    return create_path(file, S_IRWXU | S_IRWXG | S_IRWXO);
#endif
}

std::ofstream HJFileUtil::create_file(const char* file, const char* mode) {
    fs::path path(file);
    // 自动创建父目录
    if (!fs::exists(path.parent_path()) && !create_path(path.parent_path().string().c_str())) {
        return std::ofstream();
    }

    // 解析打开模式
    std::ios_base::openmode open_mode = std::ios::binary;
    if (strcmp(mode, "r") == 0) {
        open_mode |= std::ios::in;
    } else if (strcmp(mode, "w") == 0) {
        open_mode |= std::ios::out | std::ios::trunc;
    } else if (strcmp(mode, "a") == 0) {
        open_mode |= std::ios::out | std::ios::app;
    } else if (strcmp(mode, "rb") == 0) {
        open_mode |= std::ios::in;
    } else if (strcmp(mode, "wb") == 0) {
        open_mode |= std::ios::out | std::ios::trunc;
    } else if (strcmp(mode, "ab") == 0) {
        open_mode |= std::ios::out | std::ios::app;
    }

    return std::ofstream(path, open_mode);
}

bool HJFileUtil::is_dir(const char* path) {
    try {
        return fs::is_directory(fs::path(path));
    } catch (const fs::filesystem_error& e) {
        HJFLogw("error, is_dir failed:{}", e.what());
        return false;
    }
}

bool HJFileUtil::is_special_dir(const char* path) {
    fs::path p(path);
    return p.filename() == "." || p.filename() == "..";
}

int HJFileUtil::delete_file(const char* path) {
    try {
        return static_cast<int>(fs::remove_all(fs::path(path)));
    } catch (const fs::filesystem_error& e) {
        HJFLogw("error, delete_file failed:{}", e.what());
        return -1;
    }
}

bool HJFileUtil::fileExist(const char* path) {
    try {
        fs::path p(path);
        return fs::exists(p) && fs::is_regular_file(p);
    } catch (const fs::filesystem_error& e) {
        HJFLogw("error, fileExist failed:{}", e.what());
        return false;
    }
}

bool HJFileUtil::isDirExist(const std::string& dir)
{
    try {
        return fs::exists(dir) && fs::is_directory(dir);
    } catch (const fs::filesystem_error& e) {
        HJFLogw("error, dir exist failed:{}", e.what());
        return false;
    }
}

std::string HJFileUtil::loadFile(const char* path) {
    try {
        std::ifstream ifs(path, std::ios::binary);
        if (!ifs) {
            HJFLogw("error, loadFile: open failed:{}", path ? path : "null");
            return "";
        }
        // 预分配内存提升性能
        auto size = fs::file_size(fs::path(path));
        std::string content;
        content.reserve(size);
        content.assign(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
        return content;
    } catch (const fs::filesystem_error& e) {
        HJFLogw("error, loadFile failed:{}", e.what());
        return "";
    }
}

std::string HJFileUtil::readFile(const std::string& fileName) {
    return loadFile(fileName.c_str()); // 合并实现，避免冗余
}

bool HJFileUtil::saveFile(const std::string& data, const char* path, const std::string& mode) {
    fs::path p(path);
    // 确保父目录存在
    if (!fs::exists(p.parent_path()) && !create_path(p.parent_path().string().c_str())) {
        return false;
    }

    std::ios_base::openmode open_mode = std::ios::binary;
    if (mode == "wb") {
        open_mode |= std::ios::out | std::ios::trunc;
    } else if (mode == "ab") {
        open_mode |= std::ios::out | std::ios::app;
    } else if (mode == "rb+") {
        open_mode |= std::ios::in | std::ios::out;
    }

    std::ofstream ofs(p, open_mode);
    if (!ofs) {
        HJFLogw("error, saveFile: open failed - {}", path);
        return false;
    }
    ofs.write(data.data(), data.size());
    return ofs.good();
}

bool HJFileUtil::saveFile(const char* data, size_t len, const char* path) {
    if (!data || len == 0 || !path) return false;
    return saveFile(std::string(data, len), path);
}

std::string HJFileUtil::parentDir(const std::string& path) {
    fs::path p(path);
    auto parent = p.parent_path();
    return parent.empty() ? "" : parent.string();
}

std::string HJFileUtil::absolutePath(const std::string& path, const std::string& current_path, bool can_access_parent) {
    try {
        fs::path p(path);
        fs::path current(current_path);

        // 处理当前路径为相对路径的情况
        if (current.is_relative()) {
            current = fs::absolute(current);
        }

        // 组合路径并简化（处理../和./）
        fs::path abs_path = current / p;
        abs_path = abs_path.lexically_normal();

        // 检查是否越权访问父目录之外
        if (!can_access_parent) {
            fs::path root = current.lexically_normal();
            auto root_str = abs_path.lexically_relative(root).string();
            //if (root_str.starts_with("..")) {
            if (root_str.size() >= 2 && root_str[0] == '.' && root_str[1] == '.') {
                return "";
            }
        }

        return abs_path.string();
    } catch (const fs::filesystem_error& e) {
        HJFLogw("error, absolutePath failed:{}", e.what());
        return "";
    }
}

void HJFileUtil::scanDir(const std::string& path_in, const std::function<bool(const std::string&, bool)>& cb, bool enter_subdirectory) {
    try {
        fs::path path(path_in);
        if (!fs::is_directory(path)) return;

        for (const auto& entry : fs::directory_iterator(path)) {
            std::string entry_path = entry.path().string();
            bool is_dir = entry.is_directory();
            
            // 跳过隐藏文件（.开头）和特殊目录
            if (entry.path().filename().string().front() == '.' || is_special_dir(entry_path.c_str())) {
                continue;
            }

            if (!cb(entry_path, is_dir)) {
                break; // 回调返回false，终止遍历
            }

            if (is_dir && enter_subdirectory) {
                scanDir(entry_path, cb, enter_subdirectory); // 递归子目录
            }
        }
    } catch (const fs::filesystem_error& e) {
        HJFLogw("error, scanDir failed:{}", e.what());
    }
}

uint64_t HJFileUtil::fileSize(FILE* fp, bool remain_size) {
    if (!fp) return 0;

#if defined(_WIN32) || defined(_WIN64)
    auto current = _ftelli64(fp);
    _fseeki64(fp, 0, SEEK_END);
    auto end = _ftelli64(fp);
#else
    auto current = ftell(fp);
    fseek(fp, 0, SEEK_END);
    auto end = ftell(fp);
#endif
    fseek(fp, current, SEEK_SET); // 恢复位置

    return remain_size ? (end - current) : end;
}

uint64_t HJFileUtil::fileSize(const char* path) {
    try {
        return fs::file_size(fs::path(path));
    } catch (const fs::filesystem_error& e) {
        HJFLogw("error, fileSize failed:{}", e.what());
        return 0;
    }
}

//***********************************************************************************//
NS_HJ_END