#include "gtest/gtest.h"
#include "HJBlobSource.h"
#include "HJDataSourceKit.h"
#include "HJLocalFileManager.h"
#include "HJFileUtil.h"
#include "HJFLog.h"

using namespace HJ;

#define HJ_TEST_ENV 1

class HJBlobSourceTest : public testing::Test {
public:
    std::string m_work_dir;
    std::string m_local_file;
    std::string m_test_rid = "test_rid_001";
    std::string m_test_category = "test_cat";
    std::string m_remote_file_path;
    size_t      m_file_size = 8341747; // 100KB
    HJLocalFileManager::Ptr m_fileManager;

protected:
    void SetUp() override {

        m_work_dir = "E:/movies/localio/gift";
        m_local_file = HJUtilitys::concatenatePath(m_work_dir, "11950537121191245240_ok.mp4");
        m_fileManager = HJDataSourceKit::getInstance()->getFileManager();
        // Create a temporary work directory
        // m_work_dir = HJUtilitys::concatenateDir(HJUtilitys::getWorkDir(), "test_blob_source_env");
        // HJFileUtil::removeDir(m_work_dir);
        // HJFileUtil::createDir(m_work_dir);

        // // Create a dummy remote file
        // m_remote_file_path = HJUtilitys::concatenatePath(m_work_dir, "remote_source.bin");
        // createDummyFile(m_remote_file_path, m_file_size);

        // // Init LocalIOKit
        // auto params = std::make_shared<HJParams>();
        // // Assuming "work_dir" is the key to set root directory
        // params->addStorage("work_dir", m_work_dir); 
        // HJDataSourceKit::getInstance()->init(params, nullptr);
    }

    void TearDown() override {
        // HJDataSourceKit::getInstance()->done();
        // HJFileUtil::removeDir(m_work_dir);
    }

    void createDummyFile(const std::string& path, size_t size) {
        FILE* fp = fopen(path.c_str(), "wb");
        if (fp) {
            std::vector<uint8_t> buffer(1024);
            for (size_t i = 0; i < buffer.size(); ++i) {
                buffer[i] = static_cast<uint8_t>(i % 256);
            }
            
            size_t written = 0;
            while (written < size) {
                size_t to_write = std::min(size - written, buffer.size());
                fwrite(buffer.data(), 1, to_write, fp);
                written += to_write;
            }
            fclose(fp);
        }
    }

    HJXIOFileInfo createTestInfo(const std::string& rid) {
        std::string url = "https://file-2.huajiao.com/data/webp/5b226800134dc73169da56f040dd36bb/ts/a5ccf608f7d50fac6746a797620492b7.mp4";
        // return HJLocalIOUtils::makeXIOFileInfo(url, m_work_dir, rid);
        auto xinfo = m_fileManager->checkXIOFile(url, m_work_dir, rid);

        return *xinfo;
    }
};

#if HJ_TEST_ENV
TEST_F(HJBlobSourceTest, TestOpenInvalid) {
    HJXIOFileInfo info;
    // Empty info
    auto source = std::make_shared<HJBlobSource>(info);
    EXPECT_NE(source->open(), HJ_OK);
    
    //HJXIOFileInfo info = createTestInfo("");
    // Still missing rid/local_url
    //EXPECT_NE(source->open(), HJ_OK);
}

TEST_F(HJBlobSourceTest, TestOpenSuccess) {
    HJXIOFileInfo info = createTestInfo("");
    
    HJFileUtil::removeFile(info.local_url);

    auto source = std::make_shared<HJBlobSource>(info);
    EXPECT_EQ(source->open(), HJ_OK);
    //EXPECT_EQ(source->size(), (int64_t)m_file_size);
    EXPECT_FALSE(source->isComplete());
    
    source->close();
}
#endif //HJ_TEST_ENV

TEST_F(HJBlobSourceTest, TestReadFromRemoteAndCache) {
    HJXIOFileInfo info = createTestInfo("");

    auto source = std::make_shared<HJBlobSource>(info);
    ASSERT_EQ(source->open(), HJ_OK);

    // Read full file
    std::vector<uint8_t> buffer(m_file_size);
    int read_bytes = source->read(buffer.data(), m_file_size);
    EXPECT_EQ(read_bytes, (int)m_file_size);
    
    // Verify content (pattern is i % 256)
    //for (size_t i = 0; i < m_file_size; ++i) {
    //    EXPECT_EQ(buffer[i], static_cast<uint8_t>(i % 256));
    //}
    
    // Check status - might not be complete immediately due to async write or check
    // But since we read everything synchronously in single thread (HJBlobSource seems sync mostly or locks),
    // let's check positions.
    EXPECT_EQ(source->tell(), (int64_t)m_file_size);
    EXPECT_TRUE(source->eof());
    
    source->close();
    
    EXPECT_TRUE(HJFileUtil::compareFile(m_local_file, info.local_url));

    // Re-open to check if it thinks it's complete or cached
    // Note: HJBlobSource updates DB. HJLocalFileManager relies on DB.
    // If we re-open, we should see it as complete if persistence happened.
    info.tmp_url = "";
    info.tmp_rid = "";
    auto source2 = std::make_shared<HJBlobSource>(info);
    ASSERT_EQ(source2->open(), HJ_OK);
    EXPECT_TRUE(source2->isComplete()); // Should be complete after full read + close
    
    source2->close();
}

TEST_F(HJBlobSourceTest, TestSeek) {
    HJXIOFileInfo info = createTestInfo("");

    auto source = std::make_shared<HJBlobSource>(info);
    ASSERT_EQ(source->open(), HJ_OK);

    // Seek to middle
    size_t target_pos = m_file_size / 2;
    int seek_res = source->seek(target_pos, std::ios::beg);
    EXPECT_EQ(seek_res, 0);
    EXPECT_EQ(source->tell(), (int64_t)target_pos);
    
    // Read from middle
    size_t read_len = 100;
    std::vector<uint8_t> buffer(read_len);
    int read_bytes = source->read(buffer.data(), read_len);
    EXPECT_EQ(read_bytes, (int)read_len);
    
    // Verify content
    //for (size_t i = 0; i < read_len; ++i) {
    //    EXPECT_EQ(buffer[i], static_cast<uint8_t>((target_pos + i) % 256));
    //}
    
    source->close();
}

TEST_F(HJBlobSourceTest, TestSeek2) {
    HJXIOFileInfo info = createTestInfo("");

    HJRange64i range0{0, m_file_size/2};
    HJRange64i range2{ m_file_size - range0.end, m_file_size -1};
    //HJRange64i range2{ m_file_size * 2/3, m_file_size -1 };
    {
        auto source = std::make_shared<HJBlobSource>(info);
        ASSERT_EQ(source->open(), HJ_OK);

        HJRange64i range = range0;

        int seek_res = source->seek(range.begin, std::ios::beg);
        EXPECT_EQ(seek_res, 0);
        EXPECT_EQ(source->tell(), (int64_t)range.begin);

        // Read from middle
        size_t read_len = range.end - range.begin + 1;
        std::vector<uint8_t> buffer(read_len);
        int read_bytes = source->read(buffer.data(), read_len);
        EXPECT_EQ(read_bytes, (int)read_len);

        // Verify content
        //for (size_t i = 0; i < read_len; ++i) {
        //    EXPECT_EQ(buffer[i], static_cast<uint8_t>((target_pos + i) % 256));
        //}

        if (!source->isComplete()) {
            auto idxs = source->getIncompleteBlocks();
            HJFLogi("Incomplete blocks");
        }

        source->close();
    }

    {
        auto source = std::make_shared<HJBlobSource>(info);
        ASSERT_EQ(source->open(), HJ_OK);

        HJRange64i range = range2;
        // Seek to middle
        //size_t target_pos = m_file_size / 2;
        int seek_res = source->seek(range.begin, std::ios::beg);
        EXPECT_EQ(seek_res, 0);
        EXPECT_EQ(source->tell(), (int64_t)range.begin);

        // Read from middle
        size_t read_len = range.end - range.begin + 1;
        std::vector<uint8_t> buffer(read_len);
        int read_bytes = source->read(buffer.data(), read_len);
        EXPECT_EQ(read_bytes, (int)read_len);

        // Verify content
        //for (size_t i = 0; i < read_len; ++i) {
        //    EXPECT_EQ(buffer[i], static_cast<uint8_t>((target_pos + i) % 256));
        //}
        if (!source->isComplete()) {
            auto idxs = source->getIncompleteBlocks();
            HJFLogi("Incomplete blocks");
        }

        source->close();

    }

    return;
}

TEST_F(HJBlobSourceTest, TestMultiBlockDownload) {
    // 1. 准备测试数据
    // 创建一个跨越多个块的文件 (例如 3.5 个块大小)
    size_t block_size = HJBlockData::kBlockSize; // 32KB
    size_t test_file_size = block_size * 3 + block_size / 2;
    std::string remote_file_name = "multi_block_source.bin";
    std::string remote_file_path = HJUtilitys::concatenatePath(m_work_dir, remote_file_name);
    
    // 生成带随机内容的文件
    createDummyFile(remote_file_path, test_file_size);
    
    // 2. 构造本地测试路径
    std::string local_rid = "multi_block_rid";
    std::string local_file_url = HJUtilitys::concatenatePath(m_work_dir, "multi_block_local.bin");
    
    // 确保旧文件被清理
    HJFileUtil::removeFile(local_file_url);

    // 3. 创建文件信息
    // 注意：这里我们通过 "file://" 协议让 HJBlobSource 能够读取刚才创建的本地 temp 文件作为 "远程" 源
    // 实际业务中可能是 http/https，但 HJBlobSource 底层是 HJXIOContext，支持 file://
    std::string source_url = "file://" + remote_file_path; 
    
    auto xinfo = m_fileManager->checkXIOFile(source_url, m_work_dir, local_rid);
    ASSERT_TRUE(xinfo.has_value());
    //ASSERT_EQ(xinfo->block_size, (int)block_size);

    // 4. 打开 BlobSource
    auto source = std::make_shared<HJBlobSource>(*xinfo);
    ASSERT_EQ(source->open(), HJ_OK);
    
    // 验证基本信息
    EXPECT_EQ(source->size(), (int64_t)test_file_size);
    EXPECT_FALSE(source->isComplete());

    // 5. 分块循环读取
    std::vector<uint8_t> read_buffer(block_size);
    size_t total_read = 0;
    
    // 我们模拟播放器的行为，每次读一个 block 大小，或者是更小的 buffer
    // 这里为了测试边界对齐，直接用 block size
    while (total_read < test_file_size) {
        size_t to_read = std::min(block_size, test_file_size - total_read);
        
        int bytes_read = source->read(read_buffer.data(), to_read);
        
        ASSERT_GT(bytes_read, 0) << "Read failed at offset " << total_read;
        ASSERT_EQ(bytes_read, (int)to_read);
        
        total_read += bytes_read;
        
        // 简单验证一下读取位置
        EXPECT_EQ(source->tell(), (int64_t)total_read);
    }
    
    // 6. 验证完成状态
    EXPECT_TRUE(source->eof());
    
    // 关闭文件（触发最后的 flush 和完成状态保存）
    source->close();
    
    // 再次打开检查 isComplete
    // 注意：HJBlobSource 在读取过程中会后台写入，close 时会 flush。
    // 这里重新 checkXIOFile 或者直接检查 DB 状态
    auto source2 = std::make_shared<HJBlobSource>(*xinfo);
    ASSERT_EQ(source2->open(), HJ_OK);
    EXPECT_TRUE(source2->isComplete()) << "File should be marked as complete after full download";
    source2->close();

    // 7. 验证文件完整性（二进制比较）
    // xinfo->tmp_url 指向实际存储的物理文件路径
    EXPECT_TRUE(HJFileUtil::compareFile(remote_file_path, xinfo->tmp_url)) 
        << "Downloaded file content mismatch!\nSrc: " << remote_file_path 
        << "\nDst: " << xinfo->tmp_url;
}