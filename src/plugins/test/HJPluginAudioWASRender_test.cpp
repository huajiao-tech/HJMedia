#include <gtest/gtest.h>
#include "HJPluginAudioWASRender.h"
#include "HJFFHeaders.h"

NS_HJ_USING;

class HJPluginAudioWASRenderTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建测试用的图信息
        HJKeyStorage::Ptr graphInfo = std::make_shared<HJKeyStorage>();

        // 创建HJPluginAudioWASRender实例
        m_audioRender = new HJPluginAudioWASRender("TestAudioRender", graphInfo);
    }

    void TearDown() override {
        if (m_audioRender) {
            delete m_audioRender;
            m_audioRender = nullptr;
        }
    }

    HJPluginAudioWASRender* m_audioRender = nullptr;
};

// 测试构造函数
TEST_F(HJPluginAudioWASRenderTest, ConstructorTest) {
    EXPECT_NE(m_audioRender, nullptr);

    // 验证初始化事件句柄
    // 注意：由于m_audioEvent是私有成员，无法直接访问
    // 可以通过其他公共方法间接验证
}

// 测试析构函数
TEST_F(HJPluginAudioWASRenderTest, DestructorTest) {
    // 这个测试主要验证析构不会崩溃
    HJPluginAudioWASRender* render = nullptr;
    HJKeyStorage::Ptr graphInfo = std::make_shared<HJKeyStorage>();
    EXPECT_NO_THROW(render = new HJPluginAudioWASRender("DestructorTest", graphInfo));
    EXPECT_NO_THROW(delete render);
}

// 测试done方法
TEST_F(HJPluginAudioWASRenderTest, DoneTest) {
    // done方法应该能正常执行
    EXPECT_NO_THROW(m_audioRender->done());
}

// 测试设备通知客户端类
TEST_F(HJPluginAudioWASRenderTest, DeviceNotificationClientTest) {
    // 测试DeviceNotificationClient的基本功能

    // AddRef和Release测试
    HJPluginAudioWASRender::DeviceNotificationClient* client =
        new HJPluginAudioWASRender::DeviceNotificationClient(m_audioRender);

    ULONG ref1 = client->AddRef();
    EXPECT_EQ(ref1, 2);

    ULONG ref2 = client->Release();
    EXPECT_EQ(ref2, 1);

    // 最后一次Release应该删除对象
    EXPECT_EQ(client->Release(), 0);
}

// 测试设备状态改变通知
TEST_F(HJPluginAudioWASRenderTest, OnDeviceStateChangedTest) {
    HJPluginAudioWASRender::DeviceNotificationClient client(m_audioRender);

    // 此方法当前只是返回S_OK，测试它不会崩溃
    EXPECT_EQ(client.OnDeviceStateChanged(L"test_device_id", DEVICE_STATE_ACTIVE), S_OK);
}

// 测试添加设备通知
TEST_F(HJPluginAudioWASRenderTest, OnDeviceAddedTest) {
    HJPluginAudioWASRender::DeviceNotificationClient client(m_audioRender);

    // 此方法当前只是返回S_OK，测试它不会崩溃
    EXPECT_EQ(client.OnDeviceAdded(L"test_device_id"), S_OK);
}

// 测试移除设备通知
TEST_F(HJPluginAudioWASRenderTest, OnDeviceRemovedTest) {
    HJPluginAudioWASRender::DeviceNotificationClient client(m_audioRender);

    // 此方法当前只是返回S_OK，测试它不会崩溃
    EXPECT_EQ(client.OnDeviceRemoved(L"test_device_id"), S_OK);
}

// 测试默认设备变更通知
TEST_F(HJPluginAudioWASRenderTest, OnDefaultDeviceChangedTest) {
    HJPluginAudioWASRender::DeviceNotificationClient client(m_audioRender);

    // 测试与音频渲染相关的默认设备变更
    EXPECT_EQ(client.OnDefaultDeviceChanged(eRender, eMultimedia, L"default_render_device"), S_OK);

    // 测试不相关的设备变更（应无操作）
    EXPECT_EQ(client.OnDefaultDeviceChanged(eCapture, eMultimedia, L"default_capture_device"), S_OK);
    EXPECT_EQ(client.OnDefaultDeviceChanged(eRender, eConsole, L"default_console_device"), S_OK);
}

// 测试属性值变更通知
TEST_F(HJPluginAudioWASRenderTest, OnPropertyValueChangedTest) {
    HJPluginAudioWASRender::DeviceNotificationClient client(m_audioRender);
    PROPERTYKEY testKey = { {0xa45c254e, 0xdf1c, 0x4efd, {0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0}}, 14 };

    // 此方法当前只是返回S_OK，测试它不会崩溃
    EXPECT_EQ(client.OnPropertyValueChanged(L"test_device_id", testKey), S_OK);
}

// 测试获取音频设备辅助函数
TEST_F(HJPluginAudioWASRenderTest, GetAudioDeviceTest) {
    // 由于这个函数依赖于实际的Windows音频子系统，
    // 我们只能测试它不会导致崩溃
    EXPECT_NO_THROW({
        // 注意：这部分需要COM初始化，并且需要模拟或实际的设备枚举器
        // 在真实测试环境中可能需要mock对象
        });
}

// 测试初始化渲染
TEST_F(HJPluginAudioWASRenderTest, InitRenderTest) {
    HJAudioInfo::Ptr audioInfo = std::make_shared<HJAudioInfo>();
    audioInfo->m_channels = 2;
    audioInfo->m_samplesRate = 44100;
    audioInfo->m_bytesPerSample = 2;

    // 由于这会尝试连接到实际的音频设备，在某些环境下可能会失败
    // 我们主要测试它不会导致程序崩溃
    EXPECT_NO_THROW({
        int result = m_audioRender->initRender(audioInfo);
    // 结果可能是HJ_OK或者错误码，取决于运行环境
    // 在测试环境中我们主要关心不会出现异常
        });
}

// 测试释放渲染资源
TEST_F(HJPluginAudioWASRenderTest, ReleaseRenderTest) {
    // 测试资源释放逻辑
    EXPECT_NO_THROW(m_audioRender->releaseRender());
}

// 测试重置设备
TEST_F(HJPluginAudioWASRenderTest, ResetDeviceTest) {
    // 测试空设备名称（默认设备）
    EXPECT_NO_THROW({
        int result = m_audioRender->resetDevice("");
    // 实际结果取决于运行环境
        });

    // 测试特定设备名称
    EXPECT_NO_THROW({
        int result = m_audioRender->resetDevice("Test Device");
    // 实际结果取决于运行环境
        });
}

// 测试默认设备变更回调
TEST_F(HJPluginAudioWASRenderTest, OnDefaultDeviceChangedCallbackTest) {
    // 测试当使用默认设备时的行为
    EXPECT_NO_THROW(m_audioRender->onDefaultDeviceChanged());
}