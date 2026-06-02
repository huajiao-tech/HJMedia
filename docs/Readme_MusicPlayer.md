# MusicPlayer Docs

## Purpose
This document is the entry point for the `HJGraphMusicPlayer` documentation set.

It serves three audiences:

1. LLMs / coding agents
Purpose: build stable context before editing graph, plugin, or wrapper code.

2. Maintainers
Purpose: quickly recover the architecture, thread model, audio chain, and lifecycle semantics after not touching this module for a while.

3. Other readers
Purpose: understand what the music player does, what it depends on, and where to read next.

## What This Module Is
`HJGraphMusicPlayer` is the pure-audio player graph in `hjmedia`.

Its main responsibilities are:
- assemble the audio playback chain
- expose playback control APIs
- maintain playback timeline access
- coordinate repeat, seek, and final EOF behavior at graph level

Core implementation:
- [HJGraphMusicPlayer.h](/f:/Source/hjmedia/src/graphs/HJGraphMusicPlayer.h)
- [HJGraphMusicPlayer.cpp](/f:/Source/hjmedia/src/graphs/HJGraphMusicPlayer.cpp)

Detailed architecture document:
- [HJGraphMusicPlayer.md](/f:/Source/hjmedia/docs/architecture/HJGraphMusicPlayer.md)

## Core Audio Chain
The main playback chain is:

`demuxer -> audio decoder -> audio resampler -> audio render`

Key module docs:
- [HJPluginDemuxer.md](/f:/Source/hjmedia/src/plugins/doc/HJPluginDemuxer.md)
- [HJPluginAudioFFDecoder.md](/f:/Source/hjmedia/src/plugins/doc/HJPluginAudioFFDecoder.md)
- [HJPluginAudioResampler.md](/f:/Source/hjmedia/src/plugins/doc/HJPluginAudioResampler.md)
- [HJPluginAudioRender.md](/f:/Source/hjmedia/src/plugins/doc/HJPluginAudioRender.md)
- [HJTimeline.md](/f:/Source/hjmedia/src/plugins/doc/HJTimeline.md)

## Threading Prerequisite
This player depends heavily on the `HJThread` subsystem.

Read these before touching seek, teardown, handler posting, delayed tasks, or render/control thread coordination:
- [HJThread README](/f:/Source/hjmedia/src/utils/HJThread/doc/README.md)
- [HJLooperThread.md](/f:/Source/hjmedia/src/utils/HJThread/doc/HJLooperThread.md)
- [HJLooper.md](/f:/Source/hjmedia/src/utils/HJThread/doc/HJLooper.md)
- [HJHandler.md](/f:/Source/hjmedia/src/utils/HJThread/doc/HJHandler.md)
- [HJMessageQueue.md](/f:/Source/hjmedia/src/utils/HJThread/doc/HJMessageQueue.md)
- [HJMessage.md](/f:/Source/hjmedia/src/utils/HJThread/doc/HJMessage.md)

## Recommended Reading Order
If this is the first time reading the music player, use this order:

1. [MusicPlayer Audio Context Guide](/f:/Source/hjmedia/docs/architecture/HJGraphMusicPlayer_AudioContextGuide.md)
2. [HJGraphMusicPlayer.md](/f:/Source/hjmedia/docs/architecture/HJGraphMusicPlayer.md)
3. [HJThread README](/f:/Source/hjmedia/src/utils/HJThread/doc/README.md)
4. [HJTimeline.md](/f:/Source/hjmedia/src/plugins/doc/HJTimeline.md)
5. [HJPluginDemuxer.md](/f:/Source/hjmedia/src/plugins/doc/HJPluginDemuxer.md)
6. [HJPluginAudioFFDecoder.md](/f:/Source/hjmedia/src/plugins/doc/HJPluginAudioFFDecoder.md)
7. [HJPluginAudioResampler.md](/f:/Source/hjmedia/src/plugins/doc/HJPluginAudioResampler.md)
8. [HJPluginAudioRender.md](/f:/Source/hjmedia/src/plugins/doc/HJPluginAudioRender.md)
9. [HJGraphMusicPlayer.cpp](/f:/Source/hjmedia/src/graphs/HJGraphMusicPlayer.cpp)

## Wrapper Layers
Relevant wrapper / bridge code:
- [HJMusicPlayerJni.cpp](/f:/Source/hjmedia/src/entry/player/asys/HJMusicPlayerJni.cpp)
- [HJMusicPlayer.h](/f:/Source/hjmedia/src/entry/player/isys/HJMusicPlayer.h)
- [HJMusicPlayer.mm](/f:/Source/hjmedia/src/entry/player/isys/HJMusicPlayer.mm)

## Good Tasks For Codex
Good first tasks:
- summarize thread ownership and control flow
- trace `openURL -> demux -> decode -> resample -> render`
- audit seek / repeat / EOF semantics
- review lifecycle boundaries such as pause/resume/close/done
- generate regression checklist or design notes

Avoid jumping straight into:
- changing final EOF semantics
- changing timeline ownership
- changing render callback lifecycle
- redefining `close()` / stop semantics without first auditing wrappers and graph teardown
