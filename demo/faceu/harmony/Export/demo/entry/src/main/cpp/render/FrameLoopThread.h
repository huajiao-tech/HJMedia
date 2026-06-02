#ifndef FRAME_LOOP_THREAD_H
#define FRAME_LOOP_THREAD_H

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

// Simple std::thread based loop that runs a frame callback at a target fps
// and accepts async/sync messages executed on the same worker thread.
class FrameLoopThread {
public:
    using FrameCallback = std::function<void()>;
    using MessageCallback = std::function<void()>;
    using BeginCallback = std::function<void()>;
    using EndCallback = std::function<void()>;
    
    FrameLoopThread();
    ~FrameLoopThread();

    // Start the worker thread. If already running it will be restarted.
    void Start(double fps, FrameCallback frameCallback);
    void Stop();
    void SetFrameRate(double fps);

    void PostAsync(const MessageCallback& callback);
    void PostSync(const MessageCallback& callback);

    bool IsRunning() const;

    void setBeginCb(BeginCallback i_cb)
    {
        m_beginCb = i_cb;
    }
    void setEndCb(EndCallback i_cb)
    {
        m_endCb = i_cb;
    }
    
private:
    struct Task {
        MessageCallback callback;
        std::shared_ptr<std::promise<void>> promise;
    };

    void ThreadMain();
    void DrainTasks();
    std::chrono::milliseconds FrameDuration() const;

    std::thread worker_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<Task> tasks_;
    FrameCallback frameCallback_;
    std::atomic<bool> running_;
    std::atomic<double> frameIntervalMs_;
    BeginCallback m_beginCb = nullptr;
    EndCallback m_endCb = nullptr;
};

#endif // FRAME_LOOP_THREAD_H
