#include <gtest/gtest.h>
#include "HJNetFile.h"
#include "HJFLog.h"

NS_HJ_BEGIN

class HJNetFileTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize network if needed, but ffurl usually handles it
    }

    void TearDown() override {
    }
};

TEST_F(HJNetFileTest, BasicOpenClose) {
    HJNetFile::Ptr net_file = HJNetFile::create();
    EXPECT_NE(net_file, nullptr);

    // Test with invalid URL
    HJUrl::Ptr invalid_url = std::make_shared<HJUrl>("");
    EXPECT_EQ(net_file->open(invalid_url), HJErrInvalidParams);

    // Test with valid dummy URL (may fail if no network or server)
    HJUrl::Ptr url = std::make_shared<HJUrl>("http://www.google.com");
    int res = net_file->open(url);
    if (res == HJ_OK) {
        EXPECT_NE(net_file->m_inner_ctx, nullptr);
        EXPECT_GE(net_file->size(), 0);
        net_file->close();
        EXPECT_EQ(net_file->m_inner_ctx, nullptr);
    } else {
        HJFLogw("HJNetFileTest: Open google.com failed (maybe no network), res: {}", res);
    }
}

TEST_F(HJNetFileTest, SeekAndTell) {
    HJNetFile::Ptr net_file = HJNetFile::create();
    HJUrl::Ptr url = std::make_shared<HJUrl>("http://www.baidu.com");
    int res = net_file->open(url);
    if (res == HJ_OK) {
        int64_t size = net_file->size();
        if (size > 0) {
            EXPECT_EQ(net_file->tell(), 0);
            net_file->seek(size / 2, SEEK_SET);
            EXPECT_GE(net_file->tell(), size / 2);
        }
        net_file->close();
    }
}

NS_HJ_END
