#include "FrameLoopThread.h"

#include <chrono>
#include <utility>

FrameLoopThread::FrameLoopThread() : running_(false), frameIntervalMs_(16.6667) {}

FrameLoopThread::~FrameLoopThread() {
    Stop();
}

void FrameLoopThread::Start(double fps, FrameCallback frameCallback) {
    Stop();
    frameCallback_ = std::move(frameCallback);
    SetFrameRate(fps);
    running_.store(true);
    worker_ = std::thread(&FrameLoopThread::ThreadMain, this);
}

void FrameLoopThread::Stop() {
    if (!running_.load()) {
        return;
    }
    running_.store(false);
    cv_.notify_all();
    if (worker_.joinable()) {
        worker_.join();
    }
}

void FrameLoopThread::SetFrameRate(double fps) {
    // Fallback to 60fps when invalid values are provided.
    if (fps <= 0) {
        fps = 60.0;
    }
    frameIntervalMs_.store(1000.0 / fps);
    cv_.notify_all();
}

void FrameLoopThread::PostAsync(const MessageCallback& callback) {
    if (!callback) {
        return;
    }
    if (!running_.load()) {
        callback();
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks_.push({callback, nullptr});
    }
    cv_.notify_all();
}

void FrameLoopThread::PostSync(const MessageCallback& callback) {
    if (!callback) {
        return;
    }
    if (!running_.load()) {
        callback();
        return;
    }

    auto promise = std::make_shared<std::promise<void>>();
    auto future = promise->get_future();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks_.push({callback, promise});
    }
    cv_.notify_all();
    future.wait();
}

bool FrameLoopThread::IsRunning() const {
    return running_.load();
}

void FrameLoopThread::ThreadMain() {
    
    if (m_beginCb)
    {
        m_beginCb();
    }
    while (running_.load()) {
        auto frameStart = std::chrono::steady_clock::now();
        DrainTasks();

        if (frameCallback_) {
            frameCallback_();
        }

        auto frameDuration = FrameDuration();
        auto frameEnd = std::chrono::steady_clock::now();
        auto sleepDuration = frameDuration - std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd - frameStart);

        if (sleepDuration.count() > 0) {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait_for(lock, sleepDuration, [this]() {
                return !running_.load() || !tasks_.empty();
            });
        }
    }

    // Flush remaining tasks so sync posts are unblocked during shutdown.
    DrainTasks();
    
    if (m_endCb)
    {
        m_endCb();    
    }
}

void FrameLoopThread::DrainTasks() {
    std::queue<Task> pending;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::swap(pending, tasks_);
    }

    while (!pending.empty()) {
        Task task = std::move(pending.front());
        pending.pop();
        if (task.callback) {
            task.callback();
        }
        if (task.promise) {
            task.promise->set_value();
        }
    }
}

std::chrono::milliseconds FrameLoopThread::FrameDuration() const {
    double intervalMs = frameIntervalMs_.load();
    if (intervalMs < 1.0) {
        intervalMs = 1.0;
    }
    return std::chrono::milliseconds(static_cast<int64_t>(intervalMs));
}
