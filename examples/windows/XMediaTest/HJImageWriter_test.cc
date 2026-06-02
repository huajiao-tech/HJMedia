#include "gtest/gtest.h"

#include "HJImageWriter.h"

NS_HJ_USING;

TEST(HJImageWriterTest, initWithJPGSucceedsWithQualityMode) {
    auto image_writer = std::make_shared<HJImageWriter>();
    ASSERT_NE(image_writer, nullptr);

    EXPECT_EQ(image_writer->initWithJPG(320, 180, 0.8f), HJ_OK);
}
