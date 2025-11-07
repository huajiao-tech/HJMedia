
#include "HJThreadPool.h"
#include "HJSleepto.h"
#include "HJFLog.h"
#include <iostream>
#include <atomic>
#include <vector>
#include <thread>
#include <random>

NS_HJ_USING

std::atomic<int> async_tasks_started = {0};
std::atomic<int> async_tasks_completed = {0};
std::atomic<int> sync_tasks_started = {0};
std::atomic<int> sync_tasks_completed = {0};

void random_sleep() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(1, 10);
    std::this_thread::sleep_for(std::chrono::milliseconds(distrib(gen)));
}

int async_task() {
    async_tasks_started++;
    random_sleep();
    async_tasks_completed++;
    return 0;
}

int sync_task() {
    sync_tasks_started++;
    random_sleep();
    sync_tasks_completed++;
    return 0;
}

void stress_worker(HJThreadPool::Ptr thread_pool) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, 1);

    while (!thread_pool->isQuit()) {
        if (distrib(gen) == 0) {
            thread_pool->async(async_task);
        } else {
            thread_pool->sync(sync_task);
        }
        random_sleep();
    }
}

int main() {
    const int num_worker_threads = 4;
    const int test_duration_seconds = 30;

    auto thread_pool = HJThreadPool::Create();

    std::vector<std::thread> worker_threads;

    for (int i = 0; i < num_worker_threads; ++i) {
        worker_threads.emplace_back(stress_worker, thread_pool);
    }

    auto start_time = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start_time < std::chrono::seconds(test_duration_seconds)) {
        std::random_device rd;
        std::mt19937 gen(rd());
        //std::uniform_int_distribution<> distrib(0, 2);
        std::uniform_int_distribution<> distrib(0, 1);

        int action = distrib(gen);
        if (action == 0) {
            thread_pool->start();
            HJFLogi("Main thread: Started thread pool");
        } else if (action == 1) {
            thread_pool->done();
            HJFLogi("Main thread: Stopped thread pool");
        }
        //else {
        //    bool pause_state = distrib(gen) == 0;
        //    thread_pool->setPause(pause_state);
        //    HJFLogi("Main thread: Set pause to {}", pause_state);
        //}
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    thread_pool->done();

    for (auto& t : worker_threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    std::cout << "Test finished." << std::endl;
    std::cout << "Async tasks started: " << async_tasks_started << std::endl;
    std::cout << "Async tasks completed: " << async_tasks_completed << std::endl;
    std::cout << "Sync tasks started: " << sync_tasks_started << std::endl;
    std::cout << "Sync tasks completed: " << sync_tasks_completed << std::endl;

    if (async_tasks_started != async_tasks_completed) {
        std::cerr << "Error: Not all async tasks completed!" << std::endl;
    }
    if (sync_tasks_started != sync_tasks_completed) {
        std::cerr << "Error: Not all sync tasks completed!" << std::endl;
    }

    return 0;
}
