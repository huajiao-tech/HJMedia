# HJOHMuxer 实现总结

## 概述
基于 HJFFMuxer 的实现，使用 HarmonyOS 的 OH_AVMuxer API 创建了 HJOHMuxer 类，完成相同的多媒体复用功能。

## 实现文件
- `HJOHMuxer.h` - 头文件定义
- `HJOHMuxer.cc` - 实现文件

## 主要功能

### 1. 类结构
```cpp
class HJOHMuxer : public HJBaseMuxer
{
    // 继承自HJBaseMuxer，保持与HJFFMuxer相同的接口
    OH_AVMuxer* m_muxer;        // HarmonyOS复用器实例
    int32_t m_videoTrackId;     // 视频轨道ID
    int32_t m_audioTrackId;     // 音频轨道ID
    // ... 其他成员变量
};
```

### 2. 核心方法实现

#### 初始化方法
- `init(const HJMediaInfo::Ptr& mediaInfo, HJOptions::Ptr opts)` - 基于媒体信息初始化
- `init(const std::string url, int mediaTypes, HJOptions::Ptr opts)` - 基于URL初始化

#### 帧处理方法
- `addFrame(const HJMediaFrame::Ptr& frame)` - 添加帧到队列
- `writeFrame(const HJMediaFrame::Ptr& frame)` - 直接写入帧
- `done()` - 完成复用并停止

#### 内部核心方法
- `createMuxer()` - 创建OH_AVMuxer实例
- `addTrack(const HJStreamInfo::Ptr& info)` - 添加音视频轨道
- `writeSampleBuffer(const HJMediaFrame::Ptr& frame, int32_t trackId)` - 写入采样数据

### 3. HarmonyOS API 映射

| HJFFMuxer (FFmpeg) | HJOHMuxer (HarmonyOS) | 说明 |
|-------------------|----------------------|-----|
| `avformat_alloc_output_context2` | `OH_AVMuxer_Create` | 创建复用器 |
| `avformat_new_stream` | `OH_AVMuxer_AddTrack` | 添加轨道 |
| `avformat_write_header` | `OH_AVMuxer_Start` | 开始复用 |
| `av_interleaved_write_frame` | `OH_AVMuxer_WriteSampleBuffer` | 写入帧数据 |
| `av_write_trailer` | `OH_AVMuxer_Stop` | 结束复用 |
| `avformat_free_context` | `OH_AVMuxer_Destroy` | 销毁复用器 |

### 4. 格式支持
- **输出格式**: MP4 (AV_OUTPUT_FORMAT_MPEG_4), M4A (AV_OUTPUT_FORMAT_M4A)
- **视频编码**: H.264 (video/avc), H.265 (video/hevc)
- **音频编码**: AAC (audio/mp4a-latm), MP3 (audio/mpeg)

### 5. 关键特性

#### 延迟初始化
```cpp
#define HJ_MUXER_DELAY_INIT   1
// 支持延迟初始化，在第一个帧到达时才创建复用器
```

#### 时间戳处理
- 支持时间戳零点对齐 (`m_timestampZero`)
- DTS/PTS 校验和修正
- 起始时间偏移处理 (`m_startDTSOffset`)

#### 帧缓存机制
- 等待音视频第一帧到达后再开始复用
- 关键帧检查机制 (`m_keyFrameCheck`)
- 帧队列缓存 (`m_framesQueue`)

#### 错误处理
- 完整的错误码映射
- 错误回调支持 (`OnMuxerError`)
- 资源自动清理

### 6. 与HJFFMuxer的对比

#### 相同功能
- 相同的公共接口和方法签名
- 相同的媒体信息收集机制
- 相同的编码器集成方式
- 相同的时间戳处理逻辑

#### 差异点
1. **底层API**: FFmpeg AVFormatContext vs HarmonyOS OH_AVMuxer
2. **文件操作**: AVIO vs 文件描述符 (file descriptor)
3. **格式选择**: 基于扩展名 vs 枚举值
4. **轨道管理**: AVStream vs track ID
5. **数据写入**: AVPacket vs OH_AVBuffer

### 7. 编译依赖
```cpp
#include "multimedia/player_framework/native_avmuxer.h"
#include "multimedia/player_framework/native_avformat.h"
#include "multimedia/player_framework/native_avbuffer.h"
#include "multimedia/player_framework/native_avcodec_base.h"
#include "multimedia/player_framework/native_averrors.h"
```

## 使用示例
```cpp
// 创建复用器
HJOHMuxer::Ptr muxer = std::make_shared<HJOHMuxer>();

// 初始化
muxer->init("/path/to/output.mp4", HJMEDIA_TYPE_AV);

// 写入帧
muxer->addFrame(videoFrame);
muxer->addFrame(audioFrame);

// 完成
muxer->done();
```

## 注意事项
1. 需要HarmonyOS SDK支持相关多媒体API
2. 文件路径需要应用有写入权限
3. 支持的编码格式依赖于系统支持的编解码器
4. 时间戳单位转换（ms -> us）需要注意精度

## 状态
✅ 实现完成，代码已生成并可编译
✅ 保持与HJFFMuxer相同的接口规范
✅ 完整的HarmonyOS API集成
✅ 错误处理和资源管理