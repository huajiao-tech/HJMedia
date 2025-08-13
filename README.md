# HJMedia

![](https://img.shields.io/badge/license-LGPL2.1-blue)

# 关于 HJMedia

- HJMedia是一个由花椒直播独立开发的跨平台多媒体框架（首先支持HarmonyOS平台），集成花椒多媒体多年的技术沉淀，结构简洁、模块独立，采用插件式方式开发，各插件支持热插拔，模块重启和状态切换由漂流瓶方式驱动。技术人员可根据需求快速集成 SDK 使用，也能基于源代码进行定制开发。

- HJMedia研发目的是让开发者能够快速搭建多媒体相关业务，并能根据自身需求进行简单扩展，以满足不同用户的多样化需求。它既可以作为学生学习的基础工具，也能供企业用于商业平台。

- HJMedia目前基于自有核心模块搭建的业务流包括推流器、播放器（进行中），可供开发者灵活调用。

| 平台      | 状态  |
| ------- | --- |
| Harmony | 完成  |

# 快速入门/演示

- Harmony - [快速入门](examples/harmony/README.md) / [演示](examples/harmony)
- Demo展示
  
  
  
   <img src="file:///E:/code/github/HJMedia/hjpusherDemo.gif" title="" alt="" data-align="center">

# 特性

- 推流器

| 特性                   | 描述                                    | 支持状态 | 备注                                                                          |
| -------------------- | ------------------------------------- | ---- | --------------------------------------------------------------------------- |
| 视频摄像头采集              | 支持摄像头采集                               | Y    | 摄像头在横屏、竖屏、前置、后置等任意状态下画面正确显示                                                 |
| 视频采集内容离屏后处理          | 支持摄像头采集画面进行GPU后处理                     | Y    | 离屏渲染（如视频内容增强）后可供后续的预览、推流、录制之用                                               |
| 音频PCM采集              | 支持设置声道数、采样率进行音频采集                     | Y    |                                                                             |
| 音频PCM拼帧              | 支持将采集的采样点及时戳按照编码器标准拼帧处理               | Y    | PCM采集不一定符合编码器要求的帧采样点数，需拼帧处理，比如AAC编码器Low Complexity需要1024采样点数，而音频采集不一定符合，需处理 |
| 音频AAC编码              | 支持PCM编码为AAC                           | Y    |                                                                             |
| 视频H.264硬编码           | 支持鸿蒙设置硬编码H.264能力                      | Y    |                                                                             |
| 视频H.265硬编码           | 支持鸿蒙设置硬编码H.265能力                      | Y    |                                                                             |
| 视频硬编码全局头及关键帧头        | 支持鸿蒙硬编码码流针对关键帧增加头信息的后处理               | Y    | 关键帧也增加头信息，便于后续推流、播放                                                         |
| RTMP推流视频格式           | 支持H.264、H2.65                         | Y    |                                                                             |
| RTMP推流音频格式           | 支持AAC                                 | Y    |                                                                             |
| RTMP推流音视频时戳交织        | 支持推流保持音视频交织递增的时间戳                     | Y    | 交织推流，便于后续推流、播放                                                              |
| 自适应码率控制              | 支持根据推流带宽动态调整视频编码码率                    |      |                                                                             |
| RTMP推流网络失败自动重试机制     | 支持网络连接失败时根据上层设置的重试间隔、最大重试次数进行重新建立连接推送 | Y    |                                                                             |
| RTMP推流网络拥塞根据帧优先级丢包策略 | 支持网络拥塞时根据帧的优先级进行丢包处理，保证推流尽可能顺畅        | Y    |                                                                             |
| RTMP推流过程中随时短视频录制     | 支持推流工程中MP4录制功能                        | Y    | 主播精彩瞬间可支持MP4录制视频保存                                                          |
| 信息统计                 | 统计推流状态及网络状态                           | Y    |                                                                             |
| 日志分析                 | 支持控制台日志及文件日志循环记录                      | Y    | 便于问题定位及异常分析                                                                 |

# 编译

- 依赖项
  
  HJMedia 使用 [**depsync**](https://github.com/domchen/depsync) 工具管理第三方依赖项。
  
  - ****请在项目根目录下运行以下脚本：****
    
    ```
    ./sync_deps.sh
    ```
    
    该脚本将自动安装必要工具并同步所有第三方代码库。

- 开发环境
  
  - Harmony
    
    - 使用 [DevEco Studio]([DevEco Studio-鸿蒙应用集成开发环境（IDE）-华为开发者联盟](https://developer.huawei.com/consumer/cn/deveco-studio/))。

# 最新更新

- [CHANGELOG](CHANGELOG.md)

# 支持

- 如果您有任何改进HJMedia的想法或建议，欢迎随时发起 [讨论](https://github.com/huajiao-tech/HJMedia/discussions)，提交 [问题](https://github.com/huajiao-tech/HJMedia/issues). 或创建 [拉取请求](https://github.com/huajiao-tech/HJMedia/pulls)。

# Licence

```
Copyright (c) 2025 huajiao
Licensed under LGPLv2.1 or later
```

#### 依赖许可证

- ffmpeg: LGPL v2.1+
- libRTMP: BSD 3-Clause License
- soundtouch: LGPL v2.1
- openssl: Apache License 2.0
- spdlog: MIT License
- minicoro: MIT License
- sonic: Apache License 2.0
- yyjson: MIT License
- miniaudio: MIT License
- stb: MIT License

### 法律法规

所有权及解释权均归花椒直播所有，您在产品中使用前应先咨询您的律师。
