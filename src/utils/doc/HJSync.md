# HJSync

## 用途
- 封装 `std::shared_mutex` + `condition_variable_any`，提供生产者优先的读写锁。
- 给插件/线程/队列在读写状态上统一加锁、等待、唤醒。

## 主要接口
| 接口 | 参数 | 返回 | 线程限制/说明 |
| --- | --- | --- | --- |
| `wait(lock)` | `UNIQUE_LOCK& lock`：已持有的写锁 | `void` | 阻塞至唤醒；需要外部持有 `UNIQUE_LOCK`。|
| `wait(lock, timeout)` | `UNIQUE_LOCK& lock`；`timeout(ms)`：=0 无限等待，>0 超时 | `void` | 需要外部持有 `UNIQUE_LOCK`；`timeout<0` 抛 `invalid_argument`。|
| `notify()` | - | `void` | 唤醒一个等待线程。|
| `notifyAll()` | - | `void` | 唤醒所有等待线程。|
| `consLock(run)` | `run`：无参可调用对象 | `decltype(run())` | 读锁（共享锁）；等待所有生产者退出后执行 `run`。|
| `prodLock(run)` | `run`：无参可调用对象 | `decltype(run())` | 写锁（独占锁）；生产者计数+写锁保护，离开时自动唤醒等待的消费者。|
| `lock/tryLock/unlock` | - | - | `shared_mutex` 的直接封装，优先考虑 `prodLock/consLock`。|

## 生命周期与线程安全
- 默认构造即可使用，禁止拷贝/移动；内部使用原子计数和 shared_mutex 管理读写。
- 生产者优先：有生产者排队/持锁时，消费者在 `consLock` 中阻塞直到 `prodCount==0`。
- `wait/notify` 需配合 `UNIQUE_LOCK` 使用；`prodLock` 在 `run` 执行前后维护计数并负责唤醒。
- 无需显式释放，析构后资源自动释放，确保析构时无线程仍在使用。

## 常见用法
### 生产者/消费者
```cpp
HJSync sync;

// 生产者路径
sync.prodLock([&] {
    push_item();
    sync.notifyAll();
});

// 消费者路径
auto status = sync.consLock([&] {
    return pop_item();
});
```

### 等待条件
```cpp
UNIQUE_LOCK lock(sync.m);
while (!ready()) {
    sync.wait(lock);
}
consume();
```

### 清理
- 无需单独清理；停止使用时确保不再有线程持有/等待该锁。
- 若封装到类中，随类生命周期一起析构；必要时先用自定义标志使等待方退出。
