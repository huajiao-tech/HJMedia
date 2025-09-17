//***********************************************************************************//
//HJediaKit FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include <string>
#include <functional>
#include <fstream>
#include <filesystem>
#include "HJCommons.h"
NS_HJ_BEGIN

//***********************************************************************************//
class HJFileUtil {
public:
    /**
     * 创建路径（递归创建目录）
     * @param file 路径
     * @param mod 权限（跨平台适配）
     * @return 是否创建成功
     */
    static bool create_path(const char* file, unsigned int mod = 0777);

    /**
     * 创建目录（调用create_path，适配平台权限）
     */
    static bool makeDir(const char* file);

    /**
     * 新建文件（自动创建父目录）
     * @param file 文件路径
     * @param mode 打开模式（如"w", "wb", "a"等）
     * @return 文件流（需判断是否有效）
     */
    static std::ofstream create_file(const char* file, const char* mode = "wb");

    /**
     * 判断是否为目录
     */
    static bool is_dir(const char* path);

    /**
     * 判断是否是特殊目录（. 或 ..）
     */
    static bool is_special_dir(const char* path);

    /**
     * 删除文件或目录（递归删除）
     * @return 成功删除的条目数（-1表示失败）
     */
    static int delete_file(const char* path);

    /**
     * 判断文件是否存在（常规文件）
     */
    static bool fileExist(const char* path);

    /**
     * 加载文件内容至string
     */
    static std::string loadFile(const char* path);

    /**
     * 读取文件内容（与loadFile合并实现）
     */
    static std::string readFile(const std::string& fileName);

    /**
     * 保存内容至文件
     * @param data 数据
     * @param path 路径
     * @param mode 写入模式
     * @return 是否成功
     */
    static bool saveFile(const std::string& data, const char* path, const std::string& mode = "wb");
    static bool saveFile(const char* data, size_t len, const char* path);

    /**
     * 获取父目录路径
     */
    static std::string parentDir(const std::string& path);

    /**
     * 计算绝对路径（处理../）
     * @param path 相对路径
     * @param current_path 当前目录
     * @param can_access_parent 是否允许访问父目录之外
     * @return 绝对路径（空表示失败）
     */
    static std::string absolutePath(const std::string& path, const std::string& current_path, bool can_access_parent = false);

    /**
     * 遍历目录
     * @param path 目录路径
     * @param cb 回调（返回false终止遍历）
     * @param enter_subdirectory 是否遍历子目录
     */
    static void scanDir(const std::string& path, const std::function<bool(const std::string&, bool)>& cb, bool enter_subdirectory = false);

    /**
     * 获取文件大小
     * @param fp 文件句柄
     * @param remain_size 是否返回剩余未读大小
     */
    static uint64_t fileSize(FILE* fp, bool remain_size = false);

    /**
     * 获取文件大小（通过路径）
     */
    static uint64_t fileSize(const char* path);
};

//***********************************************************************************//
NS_HJ_END