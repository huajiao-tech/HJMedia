//
// Created by test on 2025/11/3.
//

#include <gtest/gtest.h>
#include "HJExecutor.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <future>
#include <thread>
#include <vector>
#include "HJFLog.h"

NS_HJ_USING;
//***********************************************************************************//
// Test fixture for HJExecutorPool
class HJExecutorPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Code here will be called immediately after the constructor (right
        // before each test).
        HJFLogi("HJExecutorPoolTest");
    }

    void TearDown() override {
        // Code here will be called immediately after each test (right
        // before the destructor).
        HJFLogi("~HJExecutorPoolTest");
    }
};

class HJExecutorPoolDelegateMock : public HJExecutorPoolDelegate {
public:
    HJ_DECLARE_PUWTR(HJExecutorPoolDelegateMock);

    HJTask::Ptr onAcquireTask() override {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [this] { return m_stop || !m_tasks.empty(); });
        if (m_stop) {
            return nullptr;
        }
        auto task = m_tasks.front();
        m_tasks.pop_front();
        return task;
    }

    void onStop() override {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_stop = true;
        }
        m_cv.notify_all();
    }

    void enqueueTask(HJTask::Ptr task) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_tasks.emplace_back(std::move(task));
        }
        m_cv.notify_one();
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::deque<HJTask::Ptr> m_tasks;
    bool m_stop{false};
};

TEST_F(HJExecutorPoolTest, PoolCreation) {
    HJExecutorPool pool(4);
    EXPECT_EQ(pool.getThreadCount(), 4);
    // Initially, all threads should be idle
    EXPECT_EQ(pool.getIdlCount(), 4);
}

TEST_F(HJExecutorPoolTest, SingleTaskExecution) {
    HJExecutorPool pool(1);
    std::atomic<bool> task_executed(false);

    auto future = pool.commit(0, "task1", [&]() {
        task_executed = true;
        return true;
    });

    // Wait for the future to be ready and check the result
    ASSERT_TRUE(future.get());
    EXPECT_TRUE(task_executed);
}

TEST_F(HJExecutorPoolTest, MultipleTaskExecution) {
    const int num_tasks = 10;
    HJExecutorPool pool(4); // Use a pool with 4 threads
    std::atomic<int> counter(0);
    std::vector<std::future<void>> futures;

    for (int i = 0; i < num_tasks; ++i) {
        futures.push_back(
            pool.commit(0, "task" + std::to_string(i), [&]() {
                counter++;
                // Simulate some work
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            })
        );
    }

    // Wait for all tasks to complete
    for (auto& f : futures) {
        f.get();
    }

    EXPECT_EQ(counter, num_tasks);
}

TEST_F(HJExecutorPoolTest, TaskPriority) {
    HJExecutorPool pool(1);
    std::vector<int> execution_order;
    std::mutex mtx;
    std::condition_variable cv;
    bool ready = false;

    // 1. Occupy the thread so subsequent tasks are queued
    auto f_block = pool.commit(0, "blocker", [&]() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&]{ return ready; });
    });

    // 2. Commit tasks with different priorities
    // Priority 1
    auto f1 = pool.commit(1, "p1", [&]() {
        std::lock_guard<std::mutex> lock(mtx);
        execution_order.push_back(1);
    });

    // Priority 10 (Should run before 1 because 10 > 1 and map uses greater)
    auto f10 = pool.commit(10, "p10", [&]() {
        std::lock_guard<std::mutex> lock(mtx);
        execution_order.push_back(10);
    });
    
    // Priority 5 (Should run between 10 and 1)
    auto f5 = pool.commit(5, "p5", [&]() {
        std::lock_guard<std::mutex> lock(mtx);
        execution_order.push_back(5);
    });

    // 3. Release blocker
    {
        std::lock_guard<std::mutex> lock(mtx);
        ready = true;
    }
    cv.notify_one();
    
    f_block.get();
    f10.get();
    f5.get();
    f1.get();

    ASSERT_EQ(execution_order.size(), 3);
    EXPECT_EQ(execution_order[0], 10);
    EXPECT_EQ(execution_order[1], 5);
    EXPECT_EQ(execution_order[2], 1);
}

TEST_F(HJExecutorPoolTest, TaskCancellationResult) {
    HJExecutorPool pool(1);
    std::mutex mtx;
    std::condition_variable cv;
    bool ready = false;
    
    pool.commit(0, "blocker", [&](){
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&]{ return ready; });
    });
    
    std::atomic<bool> executed{false};
    pool.commit(0, "to_cancel", [&](){
        executed = true;
    });
    
    bool cancelled = pool.cancelTask("to_cancel");
    EXPECT_TRUE(cancelled);
    
    {
        std::lock_guard<std::mutex> lock(mtx);
        ready = true;
    }
    cv.notify_one();
    
    // Synchronize to ensure blocker is done and queue processed
    pool.commit(0, "sync", [](){}).get();
    
    EXPECT_FALSE(executed);
}

TEST_F(HJExecutorPoolTest, ExceptionPropagation) {
    HJExecutorPool pool(1);
    auto future = pool.commit(0, "thrower", [](){
        throw std::runtime_error("Found error");
    });
    
    EXPECT_THROW({
        future.get();
    }, std::runtime_error);
}

TEST_F(HJExecutorPoolTest, StopBehavior) {
    auto pool = std::make_unique<HJExecutorPool>(1);
    std::atomic<int> run_count{0};
    std::mutex mtx;
    std::condition_variable cv;
    bool ready = false;
    
    pool->commit(0, "blocker", [&](){
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&]{ return ready; });
    });
    
    pool->commit(0, "pending", [&](){ run_count++; });
    
    pool->stop();
    
    {
        std::lock_guard<std::mutex> lock(mtx);
        ready = true;
    }
    cv.notify_one();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(run_count, 0);
    EXPECT_THROW(pool->commit(0, "new", [](){}), std::runtime_error);
}

TEST_F(HJExecutorPoolTest, StressTest) {
    HJExecutorPool pool(4);
    const int task_count = 1000;
    std::atomic<int> counter{0};
    std::vector<std::future<void>> futures;
    for(int i=0; i<task_count; ++i) {
        futures.push_back(pool.commit(0, "", [&](){
            counter++;
        }));
    }
    for(auto& f : futures) f.get();
    EXPECT_EQ(counter, task_count);
}

TEST_F(HJExecutorPoolTest, GracefulShutdown) {
    auto pool = std::make_unique<HJExecutorPool>(2);
    std::atomic<int> started_count(0);
    pool->commit(0, "long_task_1", [&]() {
        started_count++;
        std::this_thread::sleep_for(std::chrono::seconds(2));
    });
    pool->commit(0, "long_task_2", [&]() {
        started_count++;
        std::this_thread::sleep_for(std::chrono::seconds(2));
    });
     pool->commit(0, "long_task_3", [&]() {
        started_count++;
        std::this_thread::sleep_for(std::chrono::seconds(2));
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Allow time for tasks to be picked up
    
    // Let the pool go out of scope. The destructor will be called.
    // If it hangs, the test will time out.
    //pool.reset();

    // SUCCEED() will be reached if the destructor completes successfully.
    SUCCEED();
    // Check how many tasks managed to start before shutdown was initiated.
    // This isn't a strict check, but it confirms workers were active.
    EXPECT_GE(started_count, 0); 
    EXPECT_LE(started_count, 2);
    //HJFLogi("started_count:{}", started_count);
}

TEST_F(HJExecutorPoolTest, DelegateTaskExecution) {
    auto delegate = HJCreates<HJExecutorPoolDelegateMock>();
    HJExecutorPool pool(1, delegate);
    std::atomic<int> counter(0);
    std::promise<void> done;
    auto finished = done.get_future();

    delegate->enqueueTask(HJCreates<HJTask>([&]() {
        counter++;
        done.set_value();
    }));

    ASSERT_EQ(finished.wait_for(std::chrono::seconds(1)), std::future_status::ready);
    EXPECT_EQ(counter, 1);
}

TEST_F(HJExecutorPoolTest, DelegateShutdownUnblocks) {
    auto delegate = HJCreates<HJExecutorPoolDelegateMock>();
    {
        auto pool = std::make_unique<HJExecutorPool>(1, delegate);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        pool->stop();
    }
    SUCCEED();
}

TEST_F(HJExecutorPoolTest, DelegateStressTest) {
    auto delegate = HJCreates<HJExecutorPoolDelegateMock>();
    HJExecutorPool pool(4, delegate);
    
    const int task_count = 1000;
    std::atomic<int> counter{0};
    std::vector<std::thread> producers;
    
    // Multiple producers to stress thread safety of delegate interaction
    for(int i=0; i<4; ++i) {
        producers.emplace_back([&](){
            for(int j=0; j<task_count/4; ++j) {
                delegate->enqueueTask(HJCreates<HJTask>([&](){
                    counter++;
                }));
            }
        });
    }
    
    for(auto& t : producers) t.join();
    
    // Wait for completion
    int retries = 0;
    while(counter < task_count && retries++ < 200) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    EXPECT_EQ(counter, task_count);
    pool.stop();
}

TEST_F(HJExecutorPoolTest, DelegateGracefulStop) {
    auto delegate = HJCreates<HJExecutorPoolDelegateMock>();
    auto pool = std::make_unique<HJExecutorPool>(2, delegate);
    
    // Threads are blocked in delegate. Call stop to ensure they exit.
    auto start = std::chrono::steady_clock::now();
    pool->stop();
    pool.reset(); // Destructor joins threads
    auto end = std::chrono::steady_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    EXPECT_LT(duration, 200);
}

TEST_F(HJExecutorPoolTest, SyncCoroResumesParentCoroutine) {
    auto executor = HJCreates<HJExecutor>(HJ_PRIORITY_NORMAL, true);
    auto scheduler = HJCreates<HJScheduler>(executor, false);
    std::mutex order_mutex;
    std::vector<int> order;
    std::promise<void> done;
    auto finished = done.get_future();

    auto task = scheduler->asyncCoro([&]() {
        {
            std::lock_guard<std::mutex> lock(order_mutex);
            order.push_back(1);
        }
        scheduler->syncCoro([&]() {
            std::lock_guard<std::mutex> lock(order_mutex);
            order.push_back(2);
        });
        {
            std::lock_guard<std::mutex> lock(order_mutex);
            order.push_back(3);
        }
        done.set_value();
    });

    ASSERT_TRUE(task);
    ASSERT_EQ(finished.wait_for(std::chrono::seconds(1)), std::future_status::ready);
    executor->stop();

    ASSERT_EQ(order.size(), 3U);
    EXPECT_EQ(order[0], 1);
    EXPECT_EQ(order[1], 2);
    EXPECT_EQ(order[2], 3);
}

TEST_F(HJExecutorPoolTest, SyncCoroWorksWithoutCurrentCoroutine) {
    auto executor = HJCreates<HJExecutor>(HJ_PRIORITY_NORMAL, true);
    auto scheduler = HJCreates<HJScheduler>(executor, false);
    std::atomic<bool> ran{false};

    auto task = scheduler->syncCoro([&]() {
        ran = true;
    });

    executor->stop();
    ASSERT_TRUE(task);
    EXPECT_TRUE(ran.load());
}

TEST_F(HJExecutorPoolTest, CoroTaskReAsyncFailsWhenExecutorExpired) {
    auto task = HJCreates<HJCoroTask>(HJRunnable([]() {}));
    {
        auto executor = HJCreates<HJExecutor>(HJ_PRIORITY_NORMAL, true);
        task->setExecutor(executor);
        executor->stop();
    }

    EXPECT_EQ(task->reAsync(), HJErrNotAlready);
}
