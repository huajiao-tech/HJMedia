//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#include "gtest/gtest.h"
#include "HJDownloadJob.h"
#include "HJDataSourceKit.h"
#include "HJFileUtil.h"
#include "HJMediaDBManager.h"
#include <future>

NS_HJ_BEGIN

class HJDownloadJobTest : public testing::Test {
protected:
    void SetUp() override {
        // 初始化LocalIOKit以确保FileManager可用
        auto kit = HJDataSourceKit::getInstance();

        m_downloadDir = "E:/movies/localio/gift";
        if (!HJFileUtil::isDirExist(m_downloadDir)) {
            HJFileUtil::makeDir(m_downloadDir);
        }
    }

    void TearDown() override {
        // HJFileUtil::removeDir(m_downloadDir);
    }

    HJXIOFileInfo createTestInfo(const std::string& rid) {
        std::string url = "https://file-2.huajiao.com/data/webp/5b226800134dc73169da56f040dd36bb/ts/a5ccf608f7d50fac6746a797620492b7.mp4";
        return HJXIOFileInfo(url, m_downloadDir, rid);

        //HJXIOFileInfo info;
        //info.rid = rid;
        //info.url = "http://static.yximgs.com/udata/pkg/KS-Android-KSYMediaPlayer/ksyplayer_demo/test.mp4"; // 指向一个真实存在的测试文件
        ////info.url = "http://127.0.0.1:8080/test_file"; // 如果有本地测试服务器更好
        //info.local_url = m_downloadDir + "/" + rid + ".mp4";
        //info.category = "test_cat";
        //return info;
    }

    std::string m_downloadDir;
};

// 1. 无效参数测试
TEST_F(HJDownloadJobTest, InvalidParams) {
    HJXIOFileInfo info; // Empty
    auto job = HJCreates<HJDownloadJob>(info, nullptr);
    int res = job->run();
    EXPECT_EQ(res, HJErrInvalidParams);
}

// 2. 完整下载测试
TEST_F(HJDownloadJobTest, DownloadComplete) {
    auto info = createTestInfo("job_complete_test");
    
    // 清理旧文件
    if (HJFileUtil::isFileExist(info.local_url)) {
        HJFileUtil::removeFile(info.local_url);
    }
    std::string hjtdPath = info.local_url + ".hjtd";
    if (HJFileUtil::isFileExist(hjtdPath)) {
        HJFileUtil::removeFile(hjtdPath);
    }

    std::promise<int> progressPromise;
    auto listener = [&](HJNotification::Ptr ntf) -> int {
        if (ntf->getID() == HJ_LOCALIO_NOTIFY_CACHE_PROGRESS) {
            int p = ntf->getValue<int>("percent");
            if (p >= 100) {
                 progressPromise.set_value(p);
            }
        }
        return HJ_OK;
    };

    auto job = HJCreates<HJDownloadJob>(info, listener);
    int res = job->run();
    
    EXPECT_EQ(res, HJ_OK);
    EXPECT_TRUE(HJFileUtil::isFileExist(info.local_url));
    EXPECT_FALSE(HJFileUtil::isFileExist(hjtdPath)); // 完成后临时文件应被重命名或删除
    EXPECT_TRUE(job->isComplete());
    EXPECT_EQ(job->getProgress(), 100);
}

// 3. 断点续传测试
TEST_F(HJDownloadJobTest, ResumableDownload) {
    auto info = createTestInfo("job_resume_test");
    
    // 清理旧文件
    if (HJFileUtil::isFileExist(info.local_url)) {
        HJFileUtil::removeFile(info.local_url);
    }
    std::string hjtdPath = info.local_url + ".hjtd";
    if (HJFileUtil::isFileExist(hjtdPath)) {
        HJFileUtil::removeFile(hjtdPath);
    }

    // 阶段1: 启动下载并中途取消
    {
        auto job = HJCreates<HJDownloadJob>(info, nullptr);
        
        std::thread t([&job]() {
            job->run();
        });
        
        // 让它跑一会，确保下载了一些块
        std::this_thread::sleep_for(std::chrono::milliseconds(200)); 
        job->done(); // 触发取消
        if (t.joinable()) {
            t.join();
        }
    }
    
    // 验证: 应该生成了临时文件 .hjtd
    EXPECT_TRUE(HJFileUtil::isFileExist(hjtdPath));
    
    // 阶段2: 再次启动同一个任务 (恢复下载)
    {
        auto job = HJCreates<HJDownloadJob>(info, nullptr);
        int res = job->run();
        
        EXPECT_EQ(res, HJ_OK);
        EXPECT_TRUE(HJFileUtil::isFileExist(info.local_url));
        EXPECT_FALSE(HJFileUtil::isFileExist(hjtdPath));
        EXPECT_TRUE(job->isComplete());
    }
}

// 4. 重复下载已完成的任务
TEST_F(HJDownloadJobTest, AlreadyCompleted) {
    auto info = createTestInfo("job_already_completed_test");
    
    // 先完成一次下载
    {
        auto job = HJCreates<HJDownloadJob>(info, nullptr);
        job->run();
        EXPECT_TRUE(job->isComplete());
    }

    // 再次启动
    {
        auto job = HJCreates<HJDownloadJob>(info, nullptr);
        int res = job->run();
        EXPECT_EQ(res, HJ_OK);
        // 应该迅速返回，且不需要重新下载
    }
}

// 5. 进度回调测试
TEST_F(HJDownloadJobTest, ProgressCallback) {
    auto info = createTestInfo("job_progress_test");
    
    // 清理
    if (HJFileUtil::isFileExist(info.local_url)) HJFileUtil::removeFile(info.local_url);

    bool receivedProgress = false;
    bool receivedStart = false;
    bool receivedComplete = false;
    
    auto listener = [&](HJNotification::Ptr ntf) -> int {
        switch (ntf->getID()) {
            case HJ_LOCALIO_NOTIFY_CACHE_START:
                receivedStart = true;
                break;
            case HJ_LOCALIO_NOTIFY_CACHE_PROGRESS:
                receivedProgress = true;
                break;
            case HJ_LOCALIO_NOTIFY_CACHE_COMPLETED: // 假设有这个通知
                receivedComplete = true;
                break;
        }
        return HJ_OK;
    };

    auto job = HJCreates<HJDownloadJob>(info, listener);
    int res = job->run();
    
    EXPECT_EQ(res, HJ_OK);
    EXPECT_TRUE(receivedStart);
    EXPECT_TRUE(receivedProgress);
    // EXPECT_TRUE(receivedComplete); // 视具体实现是否发送完成通知而定
}

// 6. 启动后立即取消
TEST_F(HJDownloadJobTest, StartAndCancelImmediate) {
    auto info = createTestInfo("job_cancel_immediate");
    auto job = HJCreates<HJDownloadJob>(info, nullptr);
    
    job->done(); // 先调用done
    int res = job->run(); // 再调用run
    
    // 预期：因为已经abort了，run应该立即返回取消错误
    EXPECT_EQ(res, HJErrNetCanceled);
}


NS_HJ_END
