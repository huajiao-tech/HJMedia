# HJOHDemuxer 实现说明

## 概述
HJOHDemuxer 是基于 HarmonyOS 原生解复用 API (OH_AVDemuxer 和 OH_AVSource) 实现的多媒体解复用器，用于替代基于 FFmpeg 的 HJFFDemuxer，以在 HarmonyOS 平台上获得更好的性能和兼容性。

## 重要说明
⚠️ **API 兼容性注意**: 本实现基于 HarmonyOS 原生多媒体 API，但实际的 API 接口可能在不同的 HarmonyOS 版本间有所差异。在实际使用前，请根据目标 HarmonyOS 版本的 SDK 文档调整相应的 API 调用。

## 主要修正的API问题

### 1. 函数签名调整
- `OH_AVSource_CreateWithURI`: 需要 `char*` 而非 `const char*` 参数
- `OH_AVDemuxer_CreateWithSource`: 函数名可能与文档不一致
- `OH_AVDemuxer_SelectTrack`: 替代 `OH_AVDemuxer_SelectTrackByID`

### 2. 常量定义
- 使用 `HJ_NOTS_VALUE` 替代 `AV_NOPTS_VALUE`
- 关键帧标志使用通用位标志 `0x01` 替代 `AVCODEC_BUFFER_FLAGS_SYNC_FRAME`

### 3. 轨道索引获取
由于 `OH_AVDemuxer_GetCurrentTrack` 可能不存在，改用预设的轨道索引进行判断。

## 架构设计

### 类继承关系
```
HJBaseDemuxer (基类)
    ↓
HJOHDemuxer (HarmonyOS 实现)
```

### 核心组件
- **OH_AVSource**: HarmonyOS 媒体源管理器，负责处理输入媒体文件或流
- **OH_AVDemuxer**: HarmonyOS 解复用器，负责分离音视频轨道
- **OH_AVBuffer**: HarmonyOS 缓冲区管理，用于数据传输
- **OH_AVFormat**: HarmonyOS 格式描述器，包含媒体参数信息

## 主要功能

### 1. 初始化流程 (init)
```cpp
int HJOHDemuxer::init(const HJMediaUrl::Ptr& mediaUrl)
```
- 创建 OH_AVSource 并关联媒体URL
- 创建 OH_AVDemuxer 并关联媒体源
- 解析媒体信息 (makeMediaInfo)
- 选择音视频轨道 (selectTracks)
- 分析媒体格式特征

### 2. 媒体信息解析 (makeMediaInfo)
```cpp
int HJOHDemuxer::makeMediaInfo()
```
- 获取轨道数量和源格式信息
- 遍历所有轨道，识别音频和视频轨道
- 根据 MIME 类型确定编解码器
- 提取分辨率、帧率、采样率等参数
- 创建对应的 HJVideoInfo 和 HJAudioInfo 对象

### 3. 帧数据获取 (getFrame)
```cpp
int HJOHDemuxer::getFrame(HJMediaFrame::Ptr& frame)
```
- 调用 OH_AVDemuxer_ReadSample 读取样本数据
- 获取缓冲区属性 (PTS、DTS、flags等)
- 确定当前轨道类型 (音频/视频)
- 创建对应的 HJMediaFrame 对象
- 处理时间戳和数据缓冲区

### 4. 定位功能 (seek)
```cpp
int HJOHDemuxer::seek(int64_t pos)
```
- 调用 OH_AVDemuxer_SeekToTime 进行时间定位
- 重置 EOF 状态
- 处理时间偏移计算

## 关键特性

### 1. 多轨道支持
- 自动识别音频和视频轨道
- 支持轨道选择和切换
- 处理多种编解码格式 (H.264/H.265, AAC/MP3/OPUS)

### 2. 时间戳管理
- 统一使用微秒时间基 (1/1000000)
- 支持时间偏移和同步
- 音频时间戳连续性检查

### 3. 错误处理
- HarmonyOS 错误码转换为 HJMedia 错误码
- EOF 检测和处理
- 异常状态恢复

### 4. 性能优化
- 缓冲区复用机制
- 轨道预选择减少解析开销
- 关键帧优先处理

## 与 HJFFDemuxer 的差异

| 特性 | HJFFDemuxer | HJOHDemuxer |
|------|-------------|-------------|
| 底层库 | FFmpeg | HarmonyOS Native API |
| 平台支持 | 跨平台 | HarmonyOS 专用 |
| 硬件加速 | 部分支持 | 原生优化 |
| 内存管理 | 手动管理 | 系统托管 |
| 格式支持 | 广泛 | HarmonyOS 支持格式 |

## 使用示例

```cpp
// 创建解复用器实例
auto demuxer = std::make_shared<HJOHDemuxer>();

// 创建媒体URL
auto mediaUrl = std::make_shared<HJMediaUrl>("file://test.mp4");
mediaUrl->setEnableVideo(true);
mediaUrl->setEnableAudio(true);

// 初始化
int result = demuxer->init(mediaUrl);
if (result == HJ_OK) {
    // 获取媒体信息
    auto mediaInfo = demuxer->getMediaInfo();
    
    // 读取帧数据
    HJMediaFrame::Ptr frame;
    while (demuxer->getFrame(frame) == HJ_OK) {
        // 处理帧数据
        processFrame(frame);
    }
    
    // 清理资源
    demuxer->done();
}
```

## 依赖项

### HarmonyOS Native API
- `multimedia/player_framework/native_avdemuxer.h`
- `multimedia/player_framework/native_avsource.h`
- `multimedia/player_framework/native_avcodec_base.h`
- `multimedia/player_framework/native_avformat.h`
- `multimedia/player_framework/native_avbuffer.h`
- `multimedia/player_framework/native_averrors.h`

### HJMedia 框架
- HJBaseDemuxer (基类)
- HJMediaFrame (帧数据结构)
- HJMediaInfo (媒体信息)
- HJVideoInfo/HJAudioInfo (流信息)

## 注意事项

1. **API 版本兼容性**: HarmonyOS Native API 可能在不同版本间有变化，需要根据实际版本调整
2. **内存管理**: OH_AVBuffer 等对象需要正确释放，避免内存泄漏
3. **线程安全**: 在多线程环境下使用时需要额外的同步机制
4. **错误处理**: 需要完善的错误处理和恢复机制

## 测试验证

项目包含基本的测试用例 `test_hjoh_demuxer.cpp`，用于验证主要功能的正确性。

## 扩展性

该实现为 HarmonyOS 平台的媒体处理提供了基础，可以根据需要扩展支持：
- 更多编解码格式
- 网络流媒体
- 实时流处理
- 高级错误恢复机制