#include "gtest/gtest.h"
#include "HJShareBlob.h"
#include "HJBlockData.h"

using namespace HJ;

class HJShareBlobTest : public testing::Test {
protected:
    HJDBFileInfo createTestDBInfo(int64_t total_length, int block_size = 32 * 1024) {
        HJDBFileInfo info;
        info.rid = "test_rid_001";
        info.url = "https://example.com/test.mp4";
        info.local_url = "E:/test/local/test.mp4";
        info.total_length = total_length;
        info.block_size = block_size;
        info.status = 0;
        info.ensure();  // 初始化 bitmap
        return info;
    }
};

//==========================================================================
// 初始化测试
//==========================================================================
TEST_F(HJShareBlobTest, InitWithEmptyBitmap) {
    // 所有块都是 HOLE 状态
    auto info = createTestDBInfo(100 * 1024);  // 100KB = 4 块 (3个完整 + 1个部分)
    
    auto blob = std::make_shared<HJShareBlob>(info);
    ASSERT_EQ(blob->init(), HJ_OK);
    
    EXPECT_EQ(blob->blockCount(), 4);
    
    // 所有块都是 HOLE
    for (size_t i = 0; i < blob->blockCount(); ++i) {
        EXPECT_EQ(blob->getBlockStatus(i), HJBlockData::BlockStatus::HOLE);
    }
}

TEST_F(HJShareBlobTest, InitWithPartialBitmap) {
    // 部分块已缓存
    auto info = createTestDBInfo(100 * 1024);  // 4 块
    
    // 模拟块 0 和块 2 已缓存
    info.setBlockStatus(0, true);
    info.setBlockStatus(2, true);
    
    // 添加对应的 span 信息
    HJDBSpanInfo span0;
    span0.block_start_idx = 0;
    span0.block_cnt = 1;
    span0.local_url = "E:/cache/block_0.bin";
    info.spans.push_back(span0);
    
    HJDBSpanInfo span2;
    span2.block_start_idx = 2;
    span2.block_cnt = 1;
    span2.local_url = "E:/cache/block_2.bin";
    info.spans.push_back(span2);
    
    auto blob = std::make_shared<HJShareBlob>(info);
    ASSERT_EQ(blob->init(), HJ_OK);
    
    EXPECT_EQ(blob->getBlockStatus(0), HJBlockData::BlockStatus::CACHED);
    EXPECT_EQ(blob->getBlockStatus(1), HJBlockData::BlockStatus::HOLE);
    EXPECT_EQ(blob->getBlockStatus(2), HJBlockData::BlockStatus::CACHED);
    EXPECT_EQ(blob->getBlockStatus(3), HJBlockData::BlockStatus::HOLE);
    
    // 验证 local_url
    EXPECT_EQ(blob->getBlockLocalUrl(0), "E:/cache/block_0.bin");
    EXPECT_EQ(blob->getBlockLocalUrl(2), "E:/cache/block_2.bin");
}

//==========================================================================
// getBlock 状态转换测试
//==========================================================================
TEST_F(HJShareBlobTest, GetBlockCached) {
    auto info = createTestDBInfo(64 * 1024);  // 2 块
    info.setBlockStatus(0, true);
    
    HJDBSpanInfo span;
    span.block_start_idx = 0;
    span.block_cnt = 1;
    span.local_url = "E:/cache/file.bin";
    info.spans.push_back(span);
    
    auto blob = std::make_shared<HJShareBlob>(info);
    blob->init();
    
    // CACHED 状态，直接返回，不改变状态
    auto block = blob->getBlock(0);
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(block->getType(), HJBlockData::BlockStatus::CACHED);
    
    // 再次获取，仍是 CACHED
    auto block2 = blob->getBlock(0);
    EXPECT_EQ(block2->getType(), HJBlockData::BlockStatus::CACHED);
    EXPECT_EQ(block.get(), block2.get());  // 应该是同一个对象
}

TEST_F(HJShareBlobTest, GetBlockHoleToLocked) {
    auto info = createTestDBInfo(64 * 1024);  // 2 块
    
    auto blob = std::make_shared<HJShareBlob>(info);
    blob->init();
    
    // HOLE 状态，获取后转为 LOCKED
    EXPECT_EQ(blob->getBlockStatus(0), HJBlockData::BlockStatus::HOLE);
    
    auto block = blob->getBlock(0);
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(block->getType(), HJBlockData::BlockStatus::LOCKED);
    EXPECT_EQ(blob->getBlockStatus(0), HJBlockData::BlockStatus::LOCKED);
}

TEST_F(HJShareBlobTest, GetBlockLockedReturnsAlone) {
    auto info = createTestDBInfo(64 * 1024);  // 2 块
    
    auto blob = std::make_shared<HJShareBlob>(info);
    blob->init();
    
    // 第一个播放器获取 -> LOCKED
    auto block1 = blob->getBlock(0);
    ASSERT_NE(block1, nullptr);
    EXPECT_EQ(block1->getType(), HJBlockData::BlockStatus::LOCKED);
    
    // 第二个播放器获取 -> ALONE (副本)
    auto block2 = blob->getBlock(0);
    ASSERT_NE(block2, nullptr);
    EXPECT_EQ(block2->getType(), HJBlockData::BlockStatus::ALONE);
    
    // 验证是不同的对象
    EXPECT_NE(block1.get(), block2.get());
    
    // 但 ID 相同
    EXPECT_EQ(block1->getID(), block2->getID());
    
    // 原始块仍然是 LOCKED
    EXPECT_EQ(blob->getBlockStatus(0), HJBlockData::BlockStatus::LOCKED);
}

//==========================================================================
// commitBlock 测试
//==========================================================================
TEST_F(HJShareBlobTest, CommitBlockSuccess) {
    auto info = createTestDBInfo(64 * 1024);
    
    auto blob = std::make_shared<HJShareBlob>(info);
    blob->init();
    
    // 获取块 -> LOCKED
    auto block = blob->getBlock(0);
    EXPECT_EQ(block->getType(), HJBlockData::BlockStatus::LOCKED);
    
    // 提交成功 -> CACHED
    std::string local_url = "E:/cache/block_0.bin";
    EXPECT_EQ(blob->commitBlock(0, local_url), HJ_OK);
    
    EXPECT_EQ(blob->getBlockStatus(0), HJBlockData::BlockStatus::CACHED);
    EXPECT_EQ(blob->getBlockLocalUrl(0), local_url);
    
    // 验证 db_info 位图已更新
    EXPECT_TRUE(blob->dbInfo().getBlockStatus(0));
}

TEST_F(HJShareBlobTest, CommitBlockNotLocked) {
    auto info = createTestDBInfo(64 * 1024);
    
    auto blob = std::make_shared<HJShareBlob>(info);
    blob->init();
    
    // 未锁定的块不能提交
    EXPECT_NE(blob->commitBlock(0, "E:/cache/block.bin"), HJ_OK);
}

//==========================================================================
// releaseBlock 测试
//==========================================================================
TEST_F(HJShareBlobTest, ReleaseBlockSuccess) {
    auto info = createTestDBInfo(64 * 1024);
    
    auto blob = std::make_shared<HJShareBlob>(info);
    blob->init();
    
    // 获取块 -> LOCKED
    auto block = blob->getBlock(0);
    EXPECT_EQ(block->getType(), HJBlockData::BlockStatus::LOCKED);
    
    // 释放 -> 回到 HOLE
    EXPECT_EQ(blob->releaseBlock(0), HJ_OK);
    EXPECT_EQ(blob->getBlockStatus(0), HJBlockData::BlockStatus::HOLE);
}

TEST_F(HJShareBlobTest, ReleaseBlockNotLocked) {
    auto info = createTestDBInfo(64 * 1024);
    
    auto blob = std::make_shared<HJShareBlob>(info);
    blob->init();
    
    // 未锁定的块不能释放
    EXPECT_NE(blob->releaseBlock(0), HJ_OK);
}

//==========================================================================
// Span 合并测试
//==========================================================================
TEST_F(HJShareBlobTest, MergeSpansCreateNew) {
    auto info = createTestDBInfo(128 * 1024);  // 4 块
    
    auto blob = std::make_shared<HJShareBlob>(info);
    blob->init();
    
    // 获取并提交块 1
    blob->getBlock(1);
    blob->commitBlock(1, "E:/cache/file.bin");
    
    // 应该创建一个新 span
    auto span = blob->findSpanForBlock(1);
    ASSERT_TRUE(span.has_value());
    EXPECT_EQ(span->block_start_idx, 1);
    EXPECT_EQ(span->block_cnt, 1);
    EXPECT_EQ(span->local_url, "E:/cache/file.bin");
}

TEST_F(HJShareBlobTest, MergeSpansExtendForward) {
    auto info = createTestDBInfo(128 * 1024);  // 4 块
    
    auto blob = std::make_shared<HJShareBlob>(info);
    blob->init();
    
    // 提交块 1
    blob->getBlock(1);
    blob->commitBlock(1, "E:/cache/file.bin");
    
    // 提交块 2（紧跟块 1）- 应该合并
    blob->getBlock(2);
    blob->commitBlock(2, "E:/cache/file.bin");
    
    auto span = blob->findSpanForBlock(1);
    ASSERT_TRUE(span.has_value());
    EXPECT_EQ(span->block_start_idx, 1);
    EXPECT_EQ(span->block_cnt, 2);  // 合并后
}

TEST_F(HJShareBlobTest, MergeSpansExtendBackward) {
    auto info = createTestDBInfo(128 * 1024);  // 4 块
    
    auto blob = std::make_shared<HJShareBlob>(info);
    blob->init();
    
    // 先提交块 2
    blob->getBlock(2);
    blob->commitBlock(2, "E:/cache/file.bin");
    
    // 提交块 1（在块 2 前面）- 应该向后扩展
    blob->getBlock(1);
    blob->commitBlock(1, "E:/cache/file.bin");
    
    auto span = blob->findSpanForBlock(1);
    ASSERT_TRUE(span.has_value());
    EXPECT_EQ(span->block_start_idx, 1);
    EXPECT_EQ(span->block_cnt, 2);
}

TEST_F(HJShareBlobTest, MergeSpansBridge) {
    auto info = createTestDBInfo(128 * 1024);  // 4 块
    
    auto blob = std::make_shared<HJShareBlob>(info);
    blob->init();
    
    // 提交块 0
    blob->getBlock(0);
    blob->commitBlock(0, "E:/cache/file.bin");
    
    // 提交块 2
    blob->getBlock(2);
    blob->commitBlock(2, "E:/cache/file.bin");
    
    // 提交块 1 - 应该桥接块 0 和块 2
    blob->getBlock(1);
    blob->commitBlock(1, "E:/cache/file.bin");
    
    auto span = blob->findSpanForBlock(0);
    ASSERT_TRUE(span.has_value());
    EXPECT_EQ(span->block_start_idx, 0);
    EXPECT_EQ(span->block_cnt, 3);  // 0, 1, 2 合并
    
    // 验证 spans 数量应该是 1
    EXPECT_EQ(blob->dbInfo().spans.size(), 1);
}

TEST_F(HJShareBlobTest, MergeSpansDifferentUrl) {
    auto info = createTestDBInfo(128 * 1024);  // 4 块
    
    auto blob = std::make_shared<HJShareBlob>(info);
    blob->init();
    
    // 提交块 0 到文件 A
    blob->getBlock(0);
    blob->commitBlock(0, "E:/cache/file_a.bin");
    
    // 提交块 1 到文件 B - 不同文件，不应该合并
    blob->getBlock(1);
    blob->commitBlock(1, "E:/cache/file_b.bin");
    
    // 验证有两个 span
    EXPECT_EQ(blob->dbInfo().spans.size(), 2);
}

//==========================================================================
// 边界测试
//==========================================================================
TEST_F(HJShareBlobTest, GetBlockOutOfRange) {
    auto info = createTestDBInfo(64 * 1024);  // 2 块
    
    auto blob = std::make_shared<HJShareBlob>(info);
    blob->init();
    
    // 超出范围
    auto block = blob->getBlock(10);
    EXPECT_EQ(block, nullptr);
}

TEST_F(HJShareBlobTest, LastBlockValidSize) {
    // 最后一个块可能不是完整的 block_size
    auto info = createTestDBInfo(70 * 1024);  // 70KB = 2块 + 6KB
    
    auto blob = std::make_shared<HJShareBlob>(info);
    blob->init();
    
    EXPECT_EQ(blob->blockCount(), 3);
    
    auto block0 = blob->getBlock(0);
    auto block2 = blob->getBlock(2);
    
    EXPECT_EQ(block0->getValidSize(), 32 * 1024);
    EXPECT_EQ(block2->getValidSize(), 70 * 1024 - 64 * 1024);  // 6KB
}

//==========================================================================
// WriteUrl 管理测试
//==========================================================================
TEST_F(HJShareBlobTest, WriteUrlManagement) {
    auto info = createTestDBInfo(64 * 1024);
    
    auto blob = std::make_shared<HJShareBlob>(info);
    blob->init();
    
    // 第一次获取，应该创建一个新文件
    std::string url1 = blob->lockWriteUrl();
    EXPECT_FALSE(url1.empty());
    
    // 第二次获取，应该创建另一个新文件
    std::string url2 = blob->lockWriteUrl();
    EXPECT_FALSE(url2.empty());
    EXPECT_NE(url1, url2);
    
    // 释放 url1
    blob->unlockWriteUrl(url1);
    
    // 第三次获取，应该重用 url1
    std::string url3 = blob->lockWriteUrl();
    EXPECT_EQ(url3, url1);
    
    // 再次释放并获取
    blob->unlockWriteUrl(url3);
    std::string url4 = blob->lockWriteUrl();
    EXPECT_EQ(url4, url1);
}

//==========================================================================
// getHoleRanges 测试
//==========================================================================
TEST_F(HJShareBlobTest, GetHoleRangesFullHole) {
    auto info = createTestDBInfo(100 * 1024);  // 4 blocks [0, 32k), [32k, 64k), [64k, 96k), [96k, 100k)
    
    auto blob = std::make_shared<HJShareBlob>(info);
    blob->init();
    
    auto ranges = blob->getHoleRanges();
    ASSERT_EQ(ranges.size(), 1);
    EXPECT_EQ(ranges[0].begin, 0);
    EXPECT_EQ(ranges[0].end, 100 * 1024 - 1);
}

TEST_F(HJShareBlobTest, GetHoleRangesPartial) {
    auto info = createTestDBInfo(100 * 1024);
    info.setBlockStatus(1, true); // Block 1 is present
    
    auto blob = std::make_shared<HJShareBlob>(info);
    blob->init();
    
    auto ranges = blob->getHoleRanges();
    // Holes: Block 0, Block 2, Block 3
    // Block 2 and 3 should be merged
    
    ASSERT_EQ(ranges.size(), 2);
    
    // Range 1: Block 0 [0, 32k - 1]
    EXPECT_EQ(ranges[0].begin, 0);
    EXPECT_EQ(ranges[0].end, 32 * 1024 - 1);
    
    // Range 2: Block 2, 3 [64k, 100k - 1]
    EXPECT_EQ(ranges[1].begin, 64 * 1024);
    EXPECT_EQ(ranges[1].end, 100 * 1024 - 1);
}

TEST_F(HJShareBlobTest, GetHoleRangesNone) {
    auto info = createTestDBInfo(100 * 1024);
    for(int i=0; i<4; ++i) {
        info.setBlockStatus(i, true);
    }
    
    auto blob = std::make_shared<HJShareBlob>(info);
    blob->init();
    
    auto ranges = blob->getHoleRanges();
    EXPECT_TRUE(ranges.empty());
}
