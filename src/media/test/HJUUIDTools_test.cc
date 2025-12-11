#include "gtest/gtest.h"
#include "../HJUUIDTools.h" // 假设CMake包含路径已正确设置
#include "HJFLog.h"

// 使用命名空间以简化测试代码
NS_HJ_USING;

// 为HJUUIDTools测试创建的Test Fixture
class HJUUIDToolsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Code here will be called immediately after the constructor (right
        // before each test).
    }

    void TearDown() override {
        // Code here will be called immediately after each test (right
        // before the destructor).
    }
};

// 测试 HJUUIDTools::generate_uuid
TEST_F(HJUUIDToolsTest, GenerateUUID) {
    // 1. 测试生成是否成功
    auto uuid_opt = HJUUIDTools::generate_uuid();
    ASSERT_TRUE(uuid_opt.has_value());
    HJUUID uuid1 = uuid_opt.value();

    // 2. 测试连续生成的两个UUID是否不同
    auto another_uuid_opt = HJUUIDTools::generate_uuid();
    ASSERT_TRUE(another_uuid_opt.has_value());
    HJUUID uuid2 = another_uuid_opt.value();

    EXPECT_NE(uuid1.data, uuid2.data);

    // 3. 测试版本号(v4)和变体(variant)是否正确设置
    // Version 4: (data[6] & 0xF0) == 0x40
    EXPECT_EQ((uuid1.data[6] & 0xF0), 0x40);
    // Variant 1: (data[8] & 0xC0) == 0x80
    EXPECT_EQ((uuid1.data[8] & 0xC0), 0x80);
}

// 测试派生和验证逻辑
TEST_F(HJUUIDToolsTest, DerivationAndVerification) {
    // 1. 准备初始数据
    auto uuid0_opt = HJUUIDTools::generate_uuid();
    ASSERT_TRUE(uuid0_opt.has_value());
    HJUUID uuid0 = uuid0_opt.value();
    const int64_t number = 123456789;

    // 2. 测试成功的派生和验证（正常路径）
    HJUUID uuid1 = HJUUIDTools::derive_uuid(uuid0, number);
    EXPECT_TRUE(HJUUIDTools::verify_uuid(uuid0, uuid1, number));

    // 3. 测试派生过程的确定性
    HJUUID uuid1_again = HJUUIDTools::derive_uuid(uuid0, number);
    EXPECT_EQ(uuid1.data, uuid1_again.data);

    // 4. 测试验证的失败场景
    // 使用错误的数字
    EXPECT_FALSE(HJUUIDTools::verify_uuid(uuid0, uuid1, 987654321));

    // 使用错误的基准UUID (uuid0)
    auto another_uuid0_opt = HJUUIDTools::generate_uuid();
    ASSERT_TRUE(another_uuid0_opt.has_value());
    HJUUID another_uuid0 = another_uuid0_opt.value();
    EXPECT_FALSE(HJUUIDTools::verify_uuid(another_uuid0, uuid1, number));

    // 使用被篡改过的派生UUID (uuid1)
    HJUUID tampered_uuid1 = uuid1;
    tampered_uuid1.data[0]++; // 轻微修改数据
    EXPECT_FALSE(HJUUIDTools::verify_uuid(uuid0, tampered_uuid1, number));
}

// 使用已知值测试派生逻辑的确定性（快照测试）
TEST_F(HJUUIDToolsTest, DerivationIsDeterministicWithKnownValues) {
    // 1. 创建一个固定的UUID0和数字
    HJUUID known_uuid0;
    std::fill(known_uuid0.data.begin(), known_uuid0.data.end(), 0xAA);
    const int64_t known_number = 42;

    // 2. 派生UUID
    HJUUID derived_uuid = HJUUIDTools::derive_uuid(known_uuid0, known_number);

    // 3. 这是基于已知输入预先计算出的期望值
    // sha256("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA000000000000002A")
    // = 21e9e0364ce4f365c95f5471132b3391...
    HJUUID expected_uuid;
    expected_uuid.data = {0x21, 0xe9, 0xe0, 0x36, 0x4c, 0xe4, 0xf3, 0x65,
                          0xc9, 0x5f, 0x54, 0x71, 0x13, 0x2b, 0x33, 0x91};

    EXPECT_EQ(derived_uuid.data, expected_uuid.data);

    // 4. 同时验证它
    EXPECT_TRUE(HJUUIDTools::verify_uuid(known_uuid0, expected_uuid, known_number));
}
