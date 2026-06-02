# Plugins Docs

## Purpose
This document is the entry point for the `src/plugins` documentation set.

It serves three audiences:

1. LLMs / coding agents
Purpose: quickly identify which plugin documents matter for the current task and avoid rereading unrelated source files.

2. Maintainers
Purpose: recover the plugin-layer architecture, dependency boundaries, and recommended reading order.

3. Other readers
Purpose: understand what kinds of plugins exist in this repository and where to read next.

## What The Plugins Layer Is
The `plugins` layer is the reusable media-processing layer under graph-level orchestration.

Typical responsibilities include:
- demux
- decode
- resample / format conversion
- render
- timeline coordination
- queueing / dropping / pacing helpers
- common plugin lifecycle and I/O wiring

The graph layer decides policy and composition.
The plugin layer performs the actual stage-specific work.

## Recommended Reading Strategy
Choose a reading path based on the task instead of reading every plugin doc.

### If you are new to the plugin system
1. [HJPlugin.md](/f:/Source/hjmedia/src/plugins/doc/HJPlugin.md)
2. [HJPluginCodec.md](/f:/Source/hjmedia/src/plugins/doc/HJPluginCodec.md)
3. [HJTimeline.md](/f:/Source/hjmedia/src/plugins/doc/HJTimeline.md)
4. Then continue into the specific audio or video chain you care about.

### If you are working on the MusicPlayer audio chain
1. [HJTimeline.md](/f:/Source/hjmedia/src/plugins/doc/HJTimeline.md)
2. [HJPluginDemuxer.md](/f:/Source/hjmedia/src/plugins/doc/HJPluginDemuxer.md)
3. [HJPluginAudioFFDecoder.md](/f:/Source/hjmedia/src/plugins/doc/HJPluginAudioFFDecoder.md)
4. [HJPluginAudioResampler.md](/f:/Source/hjmedia/src/plugins/doc/HJPluginAudioResampler.md)
5. [HJPluginAudioRender.md](/f:/Source/hjmedia/src/plugins/doc/HJPluginAudioRender.md)
6. [HJGraphMusicPlayer.md](/f:/Source/hjmedia/docs/architecture/HJGraphMusicPlayer.md)

### If you are working on video playback
1. [HJTimeline.md](/f:/Source/hjmedia/src/plugins/doc/HJTimeline.md)
2. [HJPluginDemuxer.md](/f:/Source/hjmedia/src/plugins/doc/HJPluginDemuxer.md)
3. [HJPluginVideoFFDecoder.md](/f:/Source/hjmedia/src/plugins/doc/HJPluginVideoFFDecoder.md)
4. [HJPluginVideoOHDecoder.md](/f:/Source/hjmedia/src/plugins/doc/HJPluginVideoOHDecoder.md)
5. [HJPluginVideoRender.md](/f:/Source/hjmedia/src/plugins/doc/HJPluginVideoRender.md)

## Core Foundation Docs
These documents explain the common plugin model and shared infrastructure.

- [HJPlugin.md](/f:/Source/hjmedia/src/plugins/doc/HJPlugin.md)
  - base plugin lifecycle, input/output wiring, task scheduling, and state transitions
- [HJPluginCodec.md](/f:/Source/hjmedia/src/plugins/doc/HJPluginCodec.md)
  - shared codec behavior used by decode-style plugins
- [HJTimeline.md](/f:/Source/hjmedia/src/plugins/doc/HJTimeline.md)
  - playback timeline semantics and listener behavior
- [HJMediaFrameDeque.md](/f:/Source/hjmedia/src/plugins/doc/HJMediaFrameDeque.md)
  - frame queue behavior and queue-side statistics / dropping support

## Audio Chain Docs
These documents are the main path for `HJGraphMusicPlayer` and audio playback work.

- [HJPluginDemuxer.md](/f:/Source/hjmedia/src/plugins/doc/HJPluginDemuxer.md)
  - source opening, demuxer generation control, seek/reset behavior, stream-opened metadata, EOF coordination
- [HJPluginAudioFFDecoder.md](/f:/Source/hjmedia/src/plugins/doc/HJPluginAudioFFDecoder.md)
  - audio decode specialization built on top of `HJPluginCodec`
- [HJPluginAudioResampler.md](/f:/Source/hjmedia/src/plugins/doc/HJPluginAudioResampler.md)
  - audio format conversion, repacking cadence, optional FIFO behavior
- [HJPluginAudioRender.md](/f:/Source/hjmedia/src/plugins/doc/HJPluginAudioRender.md)
  - render-side buffering, playback completion, timeline updates, PCM export
- [HJPluginAudioWASRender.md](/f:/Source/hjmedia/src/plugins/doc/HJPluginAudioWASRender.md)
  - Windows WASAPI-specific audio output implementation
- [HJPluginAudioOHRender.md](/f:/Source/hjmedia/src/plugins/doc/HJPluginAudioOHRender.md)
  - OH-platform-specific audio output implementation
- [HJPluginSpeedControl.md](/f:/Source/hjmedia/src/plugins/doc/HJPluginSpeedControl.md)
  - playback speed or pacing-related control logic

## Video Chain Docs
These documents matter for video playback and render paths.

- [HJPluginVideoFFDecoder.md](/f:/Source/hjmedia/src/plugins/doc/HJPluginVideoFFDecoder.md)
  - FFmpeg-based video decode path
- [HJPluginVideoOHDecoder.md](/f:/Source/hjmedia/src/plugins/doc/HJPluginVideoOHDecoder.md)
  - OH codec / hardware decode path
- [HJPluginVideoRender.md](/f:/Source/hjmedia/src/plugins/doc/HJPluginVideoRender.md)
  - video-frame consumption and output to render targets / shared memory
- [HJPluginAVDropping.md](/f:/Source/hjmedia/src/plugins/doc/HJPluginAVDropping.md)
  - drop-policy helper for A/V queue pressure and pacing situations

## How To Use This Index With Codex
Before editing plugin code, ask Codex to:
- identify which plugin chain is active for the task
- read only the foundation docs plus the relevant chain docs
- explain which graph owns the policy and which plugin owns the data-path behavior
- call out thread, EOF, flush, and lifecycle risks before changing code

## Good Prompt Template
```text
先阅读 src/plugins/doc/README.md，不要修改代码。
根据我的任务判断应该继续阅读哪些 plugin 文档，避免无关阅读。
输出：
1. 这次任务涉及哪些 plugin
2. 推荐阅读顺序
3. 哪些文档是基础前置
4. 哪些源码文件最后再看
5. 最容易踩错的线程/EOF/flush/生命周期风险
要求：先基于文档建立上下文，只有必要时再回到源码确认。
```

## Related Entry Docs
- [AGENTS.md](/f:/Source/hjmedia/AGENTS.md)
- [MusicPlayer Docs](/f:/Source/hjmedia/docs/Readme_MusicPlayer.md)
- [HJGraphMusicPlayer_AudioContextGuide.md](/f:/Source/hjmedia/docs/architecture/HJGraphMusicPlayer_AudioContextGuide.md)
- [HJThread README](/f:/Source/hjmedia/src/utils/HJThread/doc/README.md)
