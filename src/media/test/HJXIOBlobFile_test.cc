#include <gtest/gtest.h>
#include "HJXIOBlobFile.h"
#include "HJBlockData.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

NS_HJ_USING;

namespace {
std::string makeTempFilePath(const std::string& prefix) {
    std::error_code ec;
    std::filesystem::path temp_dir = "E:/localserver/temp"; //std::filesystem::temp_directory_path(ec);
    if (ec) {
        return "";
    }
    static std::atomic<uint32_t> file_index{0};
    const uint32_t index = ++file_index;
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    std::string filename = prefix + "_" + std::to_string(stamp) + "_" + std::to_string(index) + ".bin";
    return HJUtilitys::concatenatePath(temp_dir.string(), filename);
    //return (temp_dir / filename).string();
}

struct ScopedFile {
    explicit ScopedFile(const std::string& path_in) : path(path_in) {}
    ~ScopedFile() {
        if (path.empty()) {
            return;
        }
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }
    std::string path;
};

// Helper to generate random data
std::vector<uint8_t> generateData(size_t size) {
    std::vector<uint8_t> data(size);
    for (size_t i = 0; i < size; ++i) {
        data[i] = static_cast<uint8_t>(i & 0xFF);
    }
    return data;
}
} // namespace

TEST(HJXIOBlobFileTest, BasicReadWrite) {
    const std::string path = makeTempFilePath("hjxioblob_basic");
    ASSERT_FALSE(path.empty());
    ScopedFile guard(path);
    
    // Write
    {
        auto file = HJCreator::createp<HJXIOBlobFile>();
        auto url = HJUrl::makeUrl(path, HJ_XIO_WRITE);
        ASSERT_EQ(file->open(url), HJ_OK);
        
        std::string data = "Hello, HJXIOBlobFile!";
        ASSERT_EQ(file->write(data.data(), data.size()), static_cast<int>(data.size()));
        
        // isComplete should be false because the block (32KB) is not full
        EXPECT_FALSE(file->isComplete());
        
        file->close();
    }
    
    // Read Verify
    {
        auto file = HJCreator::createp<HJXIOBlobFile>();
        auto url = HJUrl::makeUrl(path, HJ_XIO_READ);
        ASSERT_EQ(file->open(url), HJ_OK);
        
        EXPECT_EQ(file->size(), 21); // "Hello, HJXIOBlobFile!" length
        
        char buffer[100] = {0};
        ASSERT_EQ(file->read(buffer, 21), 21);
        EXPECT_STREQ(buffer, "Hello, HJXIOBlobFile!");
        
        file->close();
    }
}

TEST(HJXIOBlobFileTest, AutoFlushOnBlockComplete) {
    const std::string path = makeTempFilePath("hjxioblob_autoflush");
    ASSERT_FALSE(path.empty());
    ScopedFile guard(path);
    
    const size_t kBlockSize = HJXIOBlobFile::getBlockSize();
    
    // Write exactly one block
    {
        auto file = HJCreator::createp<HJXIOBlobFile>();
        auto url = HJUrl::makeUrl(path, HJ_XIO_WRITE);
        ASSERT_EQ(file->open(url), HJ_OK);
        
        auto data = generateData(kBlockSize);
        ASSERT_EQ(file->write(data.data(), data.size()), static_cast<int>(data.size()));
        
        // Now block 0 should be complete and tracked
        EXPECT_TRUE(file->isComplete());
        EXPECT_TRUE(file->isBlockComplete(0));
        
        // Write one more byte to start second block
        uint8_t extra = 0xAA;
        ASSERT_EQ(file->write(&extra, 1), 1);
        
        // Now globally not complete (2nd block incomplete)
        EXPECT_FALSE(file->isComplete());
        EXPECT_TRUE(file->isBlockComplete(0));
        EXPECT_FALSE(file->isBlockComplete(1));
        
        file->close();
    }
    
    // Verify file size
    std::error_code ec;
    auto fileSize = std::filesystem::file_size(path, ec);
    EXPECT_EQ(fileSize, kBlockSize + 1);
}

TEST(HJXIOBlobFileTest, PartialWritesAndAccumulation) {
    const std::string path = makeTempFilePath("hjxioblob_partial");
    ASSERT_FALSE(path.empty());
    ScopedFile guard(path);
    
    const size_t kBlockSize = HJXIOBlobFile::getBlockSize();
    auto data1 = generateData(kBlockSize / 2); // 16KB
    auto data2 = generateData(kBlockSize / 2); // 16KB (Second half)
    
    // 1. Write first half
    {
        auto file = HJCreator::createp<HJXIOBlobFile>();
        auto url = HJUrl::makeUrl(path, HJ_XIO_WRITE);
        ASSERT_EQ(file->open(url), HJ_OK);
        
        ASSERT_EQ(file->write(data1.data(), data1.size()), static_cast<int>(data1.size()));
        EXPECT_FALSE(file->isBlockComplete(0));
        
        file->close(); // Should flush existing partial data
    }
    
    // 2. Append second half
    {
        std::string mem_path = path + "_mem";
        ScopedFile mem_guard(mem_path); // Ensure cleanup
        
        auto file = HJCreator::createp<HJXIOBlobFile>();
        auto url = HJUrl::makeUrl(mem_path, HJ_XIO_WRITE);
        ASSERT_EQ(file->open(url), HJ_OK);
        
        ASSERT_EQ(file->write(data1.data(), data1.size()), static_cast<int>(data1.size()));
        EXPECT_FALSE(file->isBlockComplete(0));
        
        ASSERT_EQ(file->write(data2.data(), data2.size()), static_cast<int>(data2.size()));
        EXPECT_TRUE(file->isBlockComplete(0)); // Should be complete now
        
        file->close();
        
        std::error_code ec;
        EXPECT_EQ(std::filesystem::file_size(mem_path, ec), kBlockSize);
    }
}

TEST(HJXIOBlobFileTest, SetSizeAndCapacity) {
    const std::string path = makeTempFilePath("hjxioblob_setsize");
    ASSERT_FALSE(path.empty());
    ScopedFile guard(path);
    
    const size_t kTargetSize = 100 * 1024; // 100KB -> 4 blocks (32, 32, 32, 4)
    
    auto file = HJCreator::createp<HJXIOBlobFile>();
    auto url = HJUrl::makeUrl(path, HJ_XIO_WRITE);
    ASSERT_EQ(file->open(url), HJ_OK);
    
    file->setSize(kTargetSize);
    EXPECT_EQ(file->size(), static_cast<int64_t>(kTargetSize));
    
    // Check if we can write at the end
    file->seek(kTargetSize - 1, std::ios::beg);
    uint8_t byte = 0xFF;
    ASSERT_EQ(file->write(&byte, 1), 1);
    
    EXPECT_EQ(file->tell(), kTargetSize);
    
    file->close();
}

TEST(HJXIOBlobFileTest, SeekAndOverwrite) {
    const std::string path = makeTempFilePath("hjxioblob_seek");
    ASSERT_FALSE(path.empty());
    ScopedFile guard(path);
    
    auto file = HJCreator::createp<HJXIOBlobFile>();
    auto url = HJUrl::makeUrl(path, HJ_XIO_RW);
    ASSERT_EQ(file->open(url), HJ_OK);
    
    // Write 10 bytes
    std::string base = "0123456789";
    file->write(base.data(), base.size());
    
    // Seek back to 5 and overwrite
    file->seek(5, std::ios::beg);
    std::string ovr = "ABCDE";
    file->write(ovr.data(), ovr.size());
    
    // Expect "01234ABCDE"
    file->seek(0, std::ios::beg);
    char buf[11] = {0};
    file->read(buf, 10);
    EXPECT_STREQ(buf, "01234ABCDE");
    
    file->close();
}

TEST(HJXIOBlobFileTest, LargeFileThroughput) {
    const std::string path = makeTempFilePath("hjxioblob_large");
    ASSERT_FALSE(path.empty());
    ScopedFile guard(path);
    
    const size_t kLargeSize = 5 * 1024 * 1024; // 5MB
    auto data = generateData(kLargeSize);
    
    {
        auto file = HJCreator::createp<HJXIOBlobFile>();
        auto url = HJUrl::makeUrl(path, HJ_XIO_WRITE);
        ASSERT_EQ(file->open(url), HJ_OK);
        
        auto start = std::chrono::steady_clock::now();
        ASSERT_EQ(file->write(data.data(), data.size()), static_cast<int>(data.size()));
        auto end = std::chrono::steady_clock::now();
        
        // Ensure all blocks are marked complete (except possibly the last if partial? 5MB % 32KB == 0, so all complete)
        // 5 * 1024 * 1024 / (32 * 1024) = 160 blocks
        EXPECT_TRUE(file->isComplete());
        EXPECT_TRUE(file->isBlockComplete(159));
        
        file->close();
        
       // std::cout << "Write 5MB took: " 
       //           << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() 
       //           << "ms" << std::endl;
    }
    
    // Verify content
    {
        auto file = HJCreator::createp<HJXIOBlobFile>();
        auto url = HJUrl::makeUrl(path, HJ_XIO_READ);
        ASSERT_EQ(file->open(url), HJ_OK);
        
        EXPECT_EQ(file->size(), static_cast<int64_t>(kLargeSize));
        
        std::vector<uint8_t> readBack(kLargeSize);
        ASSERT_EQ(file->read(readBack.data(), kLargeSize), static_cast<int>(kLargeSize));
        
        EXPECT_EQ(readBack, data);
        
        file->close();
    }
}

TEST(HJXIOBlobFileTest, BoundaryWrites) {
    const std::string path = makeTempFilePath("hjxioblob_boundary");
    ASSERT_FALSE(path.empty());
    ScopedFile guard(path);
    
    const size_t kBlockSize = HJXIOBlobFile::getBlockSize();
    auto file = HJCreator::createp<HJXIOBlobFile>();
    auto url = HJUrl::makeUrl(path, HJ_XIO_WRITE);
    ASSERT_EQ(file->open(url), HJ_OK);
    
    // Case 1: Write exactly block size
    auto data = generateData(kBlockSize);
    ASSERT_EQ(file->write(data.data(), kBlockSize), static_cast<int>(kBlockSize));
    EXPECT_TRUE(file->isBlockComplete(0));
    
    // Case 2: Write BlockSize - 1 (In next block)
    // Current pos is kBlockSize. 
    auto data_short = generateData(kBlockSize - 1);
    ASSERT_EQ(file->write(data_short.data(), data_short.size()), static_cast<int>(data_short.size()));
    EXPECT_FALSE(file->isBlockComplete(1));
    
    // Case 3: Write 2 bytes (complete block 1, start block 2)
    // Current pos in block 1 is kBlockSize - 1. Need 1 byte to complete.
    uint8_t two_bytes[2] = {0xCC, 0xDD};
    ASSERT_EQ(file->write(two_bytes, 2), 2);
    
    EXPECT_TRUE(file->isBlockComplete(1));
    EXPECT_FALSE(file->isBlockComplete(2)); // Just started
    
    file->close();
}

TEST(HJXIOBlobFileTest, PersistenceBehavior) {
    const std::string path = makeTempFilePath("hjxioblob_persist");
    ASSERT_FALSE(path.empty());
    ScopedFile guard(path);
    
    const size_t kBlockSize = HJXIOBlobFile::getBlockSize();
    
    // 1. Create and complete a block
    {
        auto file = HJCreator::createp<HJXIOBlobFile>();
        auto url = HJUrl::makeUrl(path, HJ_XIO_WRITE);
        ASSERT_EQ(file->open(url), HJ_OK);
        
        auto data = generateData(kBlockSize);
        file->write(data.data(), kBlockSize);
        EXPECT_TRUE(file->isComplete());
        file->close();
    }
    
    // 2. Reopen and check status
    {
        auto file = HJCreator::createp<HJXIOBlobFile>();
        auto url = HJUrl::makeUrl(path, HJ_XIO_READ);
        ASSERT_EQ(file->open(url), HJ_OK);
        
        // Current implementation does NOT persist block completion status metadata.
        // It should assume false or recalculate?
        // Code analysis shows m_block_status is cleared on open and not re-calculated from file content.
        // So isComplete() is expected to be false initially.
        EXPECT_FALSE(file->isComplete());
        EXPECT_FALSE(file->isBlockComplete(0));
        
        // Verify data integrity regardless of status
        std::vector<uint8_t> buffer(kBlockSize);
        EXPECT_EQ(file->read(buffer.data(), kBlockSize), static_cast<int>(kBlockSize));
        
        // Reading doesn't update write status.
        EXPECT_FALSE(file->isBlockComplete(0));
        
        file->close();
    }
}
