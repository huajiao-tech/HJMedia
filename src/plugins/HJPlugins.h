#pragma once

#if defined(HarmonyOS)
#include "HJPluginAudioOHCapturer.h"
#include "HJPluginVideoOHEncoder.h"
#include "HJPluginAudioOHRender.h"
#include "HJPluginVideoOHDecoder.h"
#endif

#if defined(WINDOWS)
#include "HJPluginAudioWASRender.h"
#endif

#if defined(HJ_OS_IOS)
#include "HJPluginAudioIOSRender.h"
#endif

#if defined(HJ_OS_ANDROID)
#include "HJPluginAudioAARender.h"
#endif

#include "HJPluginRTMPMuxer.h"
#include "HJPluginFFMuxer.h"
#include "HJPluginFFDemuxer.h"

#include "HJPluginFDKAACEncoder.h"
#include "HJPluginAudioFFDecoder.h"
#include "HJPluginVideoFFDecoder.h"

#include "HJPluginAudioInput.h"
#include "HJPluginAudioMixer.h"
#include "HJPluginAudioResampler.h"
#include "HJPluginSpeechRecognizer.h"
#include "HJPluginSpeedControl.h"

#include "HJPluginAVInterleave.h"
#include "HJPluginAVDropping.h"

#include "HJPluginVideoRender.h"
#include "HJPluginAudioRender.h"

#include "HJTimeline.h"
