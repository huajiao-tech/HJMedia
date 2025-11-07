#include "gtest/gtest.h"
#include "../db/MediaStorageManager.h"
#include <filesystem>
#include <fstream>
#include <chrono>

namespace fs = std::filesystem;

class MediaStorageManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 设置测试用的路径
        test_db_path = "./test_db.sqlite";
        test_media_root = "./test_media_root";

        // 清理上一次测试可能遗留的文件
        TearDown();

        // 创建测试媒体根目录
        fs::create_directories(test_media_root);

        // 实例化被测试的类
        manager = std::make_unique<MediaStorageManager>(test_db_path, test_media_root);
    }

    void TearDown() override {
        // manager的析构函数会释放数据库连接，所以先reset
        manager.reset();
        
        // 删除测试数据库和媒体目录
        if (fs::exists(test_db_path)) {
            fs::remove(test_db_path);
        }
        if (fs::exists(test_media_root)) {
            fs::remove_all(test_media_root);
        }
    }

    // 辅助函数：创建一个虚拟文件
    void CreateDummyFile(const fs::path& path) {
        std::ofstream outfile(path);
        outfile << "dummy content";
        outfile.close();
    }

    std::string test_db_path;
    std::string test_media_root;
    std::unique_ptr<MediaStorageManager> manager;
};

// 测试：添加和获取子文件夹
TEST_F(MediaStorageManagerTest, AddAndGetSubFolder) {
    SubFolderInfo folder = {"videos", 1, 1024, 0, 0};
    ASSERT_TRUE(manager->AddSubFolder(folder));

    auto retrieved_folder = manager->GetSubFolderInfo("videos");
    ASSERT_NE(retrieved_folder, nullptr);
    EXPECT_EQ(*retrieved_folder, folder);

    // 测试重复添加
    ASSERT_FALSE(manager->AddSubFolder(folder));
}

// 测试：初始化时的文件系统校验
TEST_F(MediaStorageManagerTest, InitializationValidation) {
    // Case 1: 正常情况
    SubFolderInfo folder = {"gifs", 2, 512, 0, 0};
    fs::create_directory(fs::path(test_media_root) / "gifs");
    manager->AddSubFolder(folder);
    ASSERT_NO_THROW(manager->Initialize());

    // Case 2: 数据库中的文件夹在磁盘上不存在
    SubFolderInfo ghost_folder = {"ghost", 3, 100, 0, 0};
    manager->AddSubFolder(ghost_folder);
    ASSERT_THROW(manager->Initialize(), std::runtime_error);

    // Case 3: 数据库中的文件在磁盘上不存在
    fs::create_directory(fs::path(test_media_root) / "ghost"); // 先创建文件夹以通过第一层校验
    FileInfo ghost_file = {1, "", "ghost.gif", 1, 10, 3, 0, 0, 0};
    manager->AddOrUpdateFile("ghost", ghost_file);
    manager->DeleteFile("ghost", 1); // 从DB删除，模拟不一致
    // 重新创建一个新的manager实例来触发校验
    manager.reset();
    manager = std::make_unique<MediaStorageManager>(test_db_path, test_media_root);
    // 在DB中加入文件记录，但磁盘上没有
    SubFolderInfo ghost_folder_from_db = *manager->GetSubFolderInfo("ghost");
    ghost_folder_from_db.file_count = 1;
    ghost_folder_from_db.total_size = 10;
    manager->AddSubFolder(ghost_folder_from_db);
    manager->AddOrUpdateFile("ghost", ghost_file);
    // 此时DB有记录，但磁盘没有ghost.gif
    ASSERT_THROW(manager->Initialize(), std::runtime_error);
}

// 测试：添加、更新和获取文件
TEST_F(MediaStorageManagerTest, AddUpdateAndGetFile) {
    SubFolderInfo folder = {"videos", 1, 1024, 0, 0};
    fs::create_directory(fs::path(test_media_root) / "videos");
    manager->AddSubFolder(folder);
    manager->Initialize();

    FileInfo file1 = {101, "url1", "file1.mp4", 1, 50, 1, 1000, 1000, 1};
    CreateDummyFile(fs::path(test_media_root) / "videos" / "file1.mp4");
    manager->AddOrUpdateFile("videos", file1);

    auto files = manager->GetFilesInFolder("videos");
    ASSERT_EQ(files.size(), 1);
    EXPECT_EQ(files[0], file1);

    auto folder_info = manager->GetSubFolderInfo("videos");
    EXPECT_EQ(folder_info->file_count, 1);
    EXPECT_EQ(folder_info->total_size, 50);

    // 更新文件
    FileInfo file1_updated = {101, "url1_new", "file1.mp4", 1, 60, 1, 1000, 1001, 2};
    manager->AddOrUpdateFile("videos", file1_updated);

    files = manager->GetFilesInFolder("videos");
    ASSERT_EQ(files.size(), 1);
    EXPECT_EQ(files[0], file1_updated);

    folder_info = manager->GetSubFolderInfo("videos");
    EXPECT_EQ(folder_info->file_count, 1);
    EXPECT_EQ(folder_info->total_size, 60);
}

// 测试：删除文件
TEST_F(MediaStorageManagerTest, DeleteFile) {
    SubFolderInfo folder = {"videos", 1, 1024, 0, 0};
    fs::create_directory(fs::path(test_media_root) / "videos");
    manager->AddSubFolder(folder);
    manager->Initialize();

    FileInfo file1 = {101, "url1", "file1.mp4", 1, 50, 1, 1000, 1000, 1};
    CreateDummyFile(fs::path(test_media_root) / "videos" / "file1.mp4");
    manager->AddOrUpdateFile("videos", file1);

    ASSERT_TRUE(manager->DeleteFile("videos", 101));

    auto files = manager->GetFilesInFolder("videos");
    EXPECT_TRUE(files.empty());

    auto folder_info = manager->GetSubFolderInfo("videos");
    EXPECT_EQ(folder_info->file_count, 0);
    EXPECT_EQ(folder_info->total_size, 0);

    // 测试删除不存在的文件
    ASSERT_FALSE(manager->DeleteFile("videos", 999));
}

// 测试：删除子文件夹
TEST_F(MediaStorageManagerTest, DeleteSubFolder) {
    SubFolderInfo folder = {"videos", 1, 1024, 0, 0};
    fs::create_directory(fs::path(test_media_root) / "videos");
    manager->AddSubFolder(folder);
    manager->Initialize();

    ASSERT_TRUE(manager->DeleteSubFolder("videos"));
    EXPECT_EQ(manager->GetSubFolderInfo("videos"), nullptr);
    EXPECT_TRUE(manager->GetAllSubFolderInfos().empty());

    // 验证表是否被删除 (尝试访问会抛异常)
    ASSERT_THROW(manager->GetFilesInFolder("videos"), std::exception);
}

// 测试：超出容量限制的清理逻辑
TEST_F(MediaStorageManagerTest, CleanupLogic) {
    SubFolderInfo folder = {"videos", 1, 100, 0, 0}; // max_size = 100
    fs::create_directory(fs::path(test_media_root) / "videos");
    manager->AddSubFolder(folder);
    manager->Initialize();

    FileInfo file1 = {1, "", "f1.mp4", 1, 40, 1, 1000, 1000, 5}; // 最老，但使用次数最多
    FileInfo file2 = {2, "", "f2.mp4", 1, 40, 1, 1001, 1001, 2}; // 次老，使用次数最少
    FileInfo file3 = {3, "", "f3.mp4", 1, 40, 1, 1002, 1002, 3}; // 最新

    CreateDummyFile(fs::path(test_media_root) / "videos" / "f1.mp4");
    CreateDummyFile(fs::path(test_media_root) / "videos" / "f2.mp4");
    CreateDummyFile(fs::path(test_media_root) / "videos" / "f3.mp4");

    manager->AddOrUpdateFile("videos", file1);
    manager->AddOrUpdateFile("videos", file2);
    
    // 添加 file3, 总大小将为 120 > 100，触发清理
    auto deleted_files = manager->AddOrUpdateFile("videos", file3);

    // 应该删除一个文件以满足大小限制 (120 - 40 = 80 <= 100)
    ASSERT_EQ(deleted_files.size(), 1);
    // 根据排序规则（创建时间优先，然后使用次数），file2 应该被删除
    EXPECT_EQ(deleted_files[0].rid, 2);

    auto current_files = manager->GetFilesInFolder("videos");
    ASSERT_EQ(current_files.size(), 2);

    auto folder_info = manager->GetSubFolderInfo("videos");
    EXPECT_EQ(folder_info->file_count, 2);
    EXPECT_EQ(folder_info->total_size, 80); // 40 (file1) + 40 (file3)
}
