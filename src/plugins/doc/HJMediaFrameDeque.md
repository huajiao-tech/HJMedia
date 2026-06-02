# HJMediaFrameDeque

音视频帧队列，维护音频累计时长/采样数与视频关键帧计数，用于解复用、丢帧策略等场景。

## 数据结构

- `FrameDequeInfo`：携带当前统计信息
  - `audioDuration`：音频累计时长（与帧时间基一致）
  - `audioSamples`：音频累计采样点数
  - `sampleRate`：最近音频帧的采样率
  - `videoKeyFrames`：视频关键帧数量
  - `videoFrames`：视频帧数量
  - `flush()`：清零上述字段

## 主要接口

| 方法 | 说明 | 参数 | 返回 | 线程限制 |
| --- | --- | --- | --- | --- |
| `size()` | 获取队列长度 | - | `size_t` 长度，`HJSTATUS_Done` 时返回 0 | 内部加锁，可跨线程调用 |
| `audioDuration()` | 当前音频累计时长 | - | `int64_t`，`HJSTATUS_Done` 时 0 | 内部加锁，可跨线程 |
| `deliver(frame, FrameDequeInfo* info=nullptr)` | 追加一帧并更新统计；`frame==nullptr` 返回 `false`（不再触发 flush） | 输入：`HJMediaFrame::Ptr`；可选输出：最新 `FrameDequeInfo` | `bool` 成功/失败 | 内部加锁，可跨线程；不要在锁内做耗时操作 |
| `receive(FrameDequeInfo* info=nullptr)` | 取出队头并更新统计 | 可选输出：最新 `FrameDequeInfo` | `HJMediaFrame::Ptr`；Done 时 `nullptr` | 内部加锁，可跨线程 |
| `preview(FrameDequeInfo* info=nullptr)` | 仅查看队头，不弹出 | 可选输出：最新 `FrameDequeInfo` | `HJMediaFrame::Ptr`；Done 时 `nullptr` | 内部加锁，可跨线程 |
| `dropFrames(i_audioDuration, FrameDequeInfo* info=nullptr)` | 按音频阈值 + 关键帧约束丢弃前部帧（压缩帧队列场景） | 阈值 `>0`；可选输出：最新 `FrameDequeInfo` | `bool` 成功/失败 | 内部加锁，可跨线程；遇到 flush/eof 即停；至少保留 1 个关键帧；丢视频后要求队头为关键/控制帧且音频达标才停；未丢视频且音频已达标可提前停 |
| `flush()` | 清空队列并清零统计 | - | void | 内部加锁，可跨线程 |

> 注意：`dropFrames` 仅对压缩帧队列有意义（保证音频 duration 随帧删减）；PCM 队列的 `audioDuration` 可能持续增长。

## 生命周期与线程安全

- 创建：`std::make_shared<HJMediaFrameDeque>(name, identify)`，无需额外初始化。
- 销毁：析构调用 `done()`；也可显式 `flush()` 清空。
- 状态：继承 `HJSyncObject`，`m_status == HJSTATUS_Done` 时查询方法返回零值/空帧，`deliver` 直接失败。
- 线程安全：所有队列操作在 `SYNCHRONIZED_SYNC_LOCK` 下，可多线程调用，但避免在锁内执行耗时工作。

## 常见用法

```cpp
// 创建
auto queue = std::make_shared<HJMediaFrameDeque>("demux-queue");

// 投递解复用后的帧（音视频混合）
HJMediaFrameDeque::FrameDequeInfo info{};
queue->deliver(audioFrame, &info);
queue->deliver(videoFrame, &info);

// 当音频累计时长超阈值时执行丢帧（保持队头为关键帧/控制帧，音频达标）
queue->dropFrames(targetAudioDuration, &info);

// 拉取消费
auto frame = queue->receive(&info);
if (frame && frame->isEofFrame()) {
    // 处理结束帧
}

// 清理
queue->flush();
queue.reset();
```
