#include <gtest/gtest.h>
#include "HJDownloads.h"
#include "HJDataSourceKit.h"
#include "HJFileUtil.h"
#include "HJMediaUtils.h"
#include "HJXIOBlobFile.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <thread>
#include <vector>

NS_HJ_USING

namespace {
HJParams::Ptr makeLocalIOParams(const std::string& work_dir)
{
    HJCacheOptions cache_opts;
    cache_opts.emplace_back(work_dir, 10);
    auto params = HJCreates<HJParams>();
    (*params)["medias_dir"] = work_dir;
    (*params)["cache_opts"] = cache_opts;
    return params;
}

bool createDummyFile(const std::string& path, size_t size)
{
    std::string data(size, 'a');
    return HJFileUtil::saveFile(data, path, "wb");
}
} // namespace

class HJDownloadsTest : public ::testing::Test
{
protected:
    void TearDown() override
    {
        auto kit = HJDataSourceKit::getInstance();
        if (kit) {
            kit->done();
        }
        HJDataSourceKit::destoryInstance();
    }
};

TEST_F(HJDownloadsTest, DuplicateQueuedIgnored)
{
    const std::string work_dir = "test_downloads_dup";
    HJFileUtil::makeDir(work_dir);
    auto kit = HJDataSourceKit::getInstance();
    ASSERT_EQ(kit->init(makeLocalIOParams(work_dir), nullptr), HJ_OK);

    HJDownloads downloads(nullptr, 1);
    HJXIOFileInfo info;
    info.rid = "dup_rid";
    info.url = "invalid-url";
    info.local_url = "dup_download.mp4";
    info.priority = HJ_PRIORITY_NORMAL;

    EXPECT_EQ(downloads.download(info), HJ_OK);
    EXPECT_EQ(downloads.download(info), HJErrAlreadyExist);
    EXPECT_EQ(downloads.pendingCount(), 1u);

    kit->done();
    HJDataSourceKit::destoryInstance();
    HJFileUtil::removeFile(work_dir);
}

TEST_F(HJDownloadsTest, CancelQueuedRemovesTask)
{
    const std::string work_dir = "test_downloads_cancel_queue";
    HJFileUtil::makeDir(work_dir);
    auto kit = HJDataSourceKit::getInstance();
    ASSERT_EQ(kit->init(makeLocalIOParams(work_dir), nullptr), HJ_OK);

    HJDownloads downloads(nullptr, 1);
    HJXIOFileInfo info;
    info.rid = "cancel_rid";
    info.url = "invalid-url";
    info.local_url = "cancel_download.mp4";
    info.priority = HJ_PRIORITY_NORMAL;

    EXPECT_EQ(downloads.download(info), HJ_OK);
    EXPECT_EQ(downloads.cancel(info.rid), HJ_OK);
    EXPECT_EQ(downloads.pendingCount(), 0u);

    kit->done();
    HJDataSourceKit::destoryInstance();
    HJFileUtil::removeFile(work_dir);
}

TEST_F(HJDownloadsTest, PriorityOrdering)
{
    const std::string work_dir = "test_downloads_priority";
    HJFileUtil::makeDir(work_dir);
    auto kit = HJDataSourceKit::getInstance();
    ASSERT_EQ(kit->init(makeLocalIOParams(work_dir), nullptr), HJ_OK);

    HJDownloads downloads(nullptr, 1);
    std::mutex mutex;
    std::condition_variable cv;
    std::vector<std::string> start_order;

    downloads.setListener([&](const HJNotification::Ptr ntf) {
        if (ntf && ntf->getID() == HJ_LOCALIO_NOTIFY_CACHE_START) {
            std::lock_guard<std::mutex> lock(mutex);
            start_order.push_back(ntf->getValue<std::string>("rid"));
            if (start_order.size() >= 2) {
                cv.notify_one();
            }
        }
        return HJ_OK;
    });

    HJXIOFileInfo low_info;
    low_info.rid = "low_rid";
    low_info.url = "invalid-low";
    low_info.local_url = "low_priority.mp4";
    low_info.priority = HJ_PRIORITY_LOW;

    HJXIOFileInfo high_info;
    high_info.rid = "high_rid";
    high_info.url = "invalid-high";
    high_info.local_url = "high_priority.mp4";
    high_info.priority = HJ_PRIORITY_HIGH;

    downloads.start();
    EXPECT_EQ(downloads.download(low_info), HJ_OK);
    EXPECT_EQ(downloads.download(high_info), HJ_OK);

    std::unique_lock<std::mutex> lock(mutex);
    bool ok = cv.wait_for(lock, std::chrono::seconds(2), [&]() {
        return start_order.size() >= 2;
    });
    ASSERT_TRUE(ok);
    EXPECT_EQ(start_order[0], high_info.rid);
    EXPECT_EQ(start_order[1], low_info.rid);

    downloads.done();
    kit->done();
    HJDataSourceKit::destoryInstance();
    HJFileUtil::removeFile(work_dir);
}

TEST_F(HJDownloadsTest, CompletedSkip)
{
    const std::string work_dir = "test_downloads_cache";
    HJFileUtil::makeDir(work_dir);

    auto kit = HJDataSourceKit::getInstance();
    ASSERT_EQ(kit->init(makeLocalIOParams(work_dir), nullptr), HJ_OK);

    const std::string url = "http://example.com/video.mp4";
    const std::string rid = HJMediaUtils::makeUrlRid(url);
    const std::string local_url = HJMediaUtils::makeLocalUrl(work_dir, url, rid);

    ASSERT_TRUE(HJFileUtil::saveFile("data", local_url, "wb"));

    HJDBFileInfo db_info(rid, url, local_url, 4, static_cast<int>(HJXIOBlobFile::getBlockSize()));
    db_info.status = static_cast<int>(HJFileStatus::COMPLETED);
    db_info.size = db_info.total_length;
    db_info.modify_time = HJCurrentSteadyMS();

    auto file_manager = kit->getFileManager();
    ASSERT_TRUE(file_manager != nullptr);
    EXPECT_EQ(file_manager->updateFileInfo(db_info), HJ_OK);

    HJDownloads downloads(nullptr, 1);
    HJXIOFileInfo info;
    info.rid = rid;
    info.url = url;
    info.local_url = local_url;
    info.priority = HJ_PRIORITY_NORMAL;

    EXPECT_EQ(downloads.download(info), HJ_EOF);

    kit->done();
    HJDataSourceKit::destoryInstance();
    HJFileUtil::removeFile(work_dir);
}

TEST_F(HJDownloadsTest, DestructionStressTest)
{
    const std::string work_dir = "test_downloads_stress";
    HJFileUtil::makeDir(work_dir);

    // Setup kit
    auto kit = HJDataSourceKit::getInstance();
    ASSERT_EQ(kit->init(makeLocalIOParams(work_dir), nullptr), HJ_OK);

    std::atomic<bool> stop_thread{false};
    
    // Background thread that keeps creating downloads 
    // This simulates tasks trying to be added while we are destroying
    std::thread worker([&]() {
        int counter = 0;
        while (!stop_thread.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            // Just dummy work to simulate load if we had access to the instance
             counter++;
             if (counter > 1000) counter = 0;
        }
    });

    // Stress test creation and destruction
    for (int i = 0; i < 50; ++i) {
        auto downloads = std::make_shared<HJDownloads>(nullptr, 4);
        downloads->start();
        
        // Add a bunch of tasks immediately
        for (int j = 0; j < 20; ++j) {
            HJXIOFileInfo info;
            info.rid = "stress_" + std::to_string(i) + "_" + std::to_string(j);
            info.url = "http://example.com/video" + std::to_string(j) + ".mp4";
            info.local_url = work_dir + "/" + info.rid + ".mp4"; // dummy path
            info.priority = HJ_PRIORITY_NORMAL;
            // It's okay if these fail (e.g. invalid path), we just want to stress the queue and locking
            downloads->download(info);
        }

        // Sleep a tiny bit to let some start
        std::this_thread::sleep_for(std::chrono::milliseconds(5));

        // Destruction happens here when shared_ptr goes out of scope or we reset
        downloads->done();
        downloads = nullptr;
    }

    stop_thread.store(true);
    worker.join();

    kit->done();
    HJDataSourceKit::destoryInstance();
    HJFileUtil::removeFile(work_dir);
}

TEST_F(HJDownloadsTest, CancelRunningJob)
{
    const std::string work_dir = "test_downloads_cancel_running";
    HJFileUtil::makeDir(work_dir);

    // Setup kit
    auto kit = HJDataSourceKit::getInstance();
    ASSERT_EQ(kit->init(makeLocalIOParams(work_dir), nullptr), HJ_OK);

    const std::string remote_path = HJUtilitys::concatenatePath(work_dir, "remote_cancel.bin");
    ASSERT_TRUE(createDummyFile(remote_path, 1024 * 1024));
    const std::string remote_abs_path = std::filesystem::absolute(remote_path).generic_string();
    std::string remote_url = "file://" + remote_abs_path;

    HJDownloads downloads(nullptr, 1);
    std::mutex progress_mutex;
    std::condition_variable progress_cv;
    std::condition_variable allow_cv;
    bool progress_seen = false;
    bool allow_progress = false;
    std::atomic<int> start_count{0};

    downloads.setListener([&](HJNotification::Ptr ntf) {
        if (!ntf) {
            return HJ_OK;
        }
        const int id = ntf->getID();
        if (id == HJ_LOCALIO_NOTIFY_CACHE_START) {
            start_count.fetch_add(1);
        } else if (id == HJ_LOCALIO_NOTIFY_CACHE_PROGRESS) {
            std::unique_lock<std::mutex> lock(progress_mutex);
            if (!progress_seen) {
                progress_seen = true;
                progress_cv.notify_one();
            }
            allow_cv.wait(lock, [&]() {
                return allow_progress;
            });
        }
        return HJ_OK;
    });

    downloads.start();

    HJXIOFileInfo info(remote_url, work_dir, "cancel_running_rid");
    info.priority = HJ_PRIORITY_NORMAL;
    ASSERT_EQ(downloads.download(info), HJ_OK);

    {
        std::unique_lock<std::mutex> lock(progress_mutex);
        bool ok = progress_cv.wait_for(lock, std::chrono::seconds(2), [&]() {
            return progress_seen;
        });
        ASSERT_TRUE(ok);
    }

    for (int i = 0; i < 10; ++i) {
        int res = downloads.cancel(info.rid);
        EXPECT_TRUE(res == HJ_OK || res == HJErrNotAlready);
    }

    {
        std::lock_guard<std::mutex> lock(progress_mutex);
        allow_progress = true;
    }
    allow_cv.notify_all();

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_EQ(start_count.load(), 1);

    downloads.done();
    kit->done();
    HJDataSourceKit::destoryInstance();
    HJFileUtil::removeFile(work_dir);
}
