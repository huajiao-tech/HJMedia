#pragma once

#if defined(HarmonyOS)
#include "HJPluginAudioOHCapturer.h"
#include "HJPluginVideoOHEncoder.h"
#include "HJPluginAudioOHRender.h"
#include "HJPluginVideoOHDecoder.h"
#endif

#if defined(WINDOWS)
#include "HJPluginAudioWORender.h"
#endif

#include "HJPluginRTMPMuxer.h"
#include "HJPluginFFMuxer.h"
#include "HJPluginFFDemuxer.h"

#include "HJPluginFDKAACEncoder.h"
#include "HJPluginAudioFFDecoder.h"
#include "HJPluginVideoFFDecoder.h"

#include "HJPluginAudioResampler.h"
#include "HJPluginSpeechRecognizer.h"
#include "HJPluginSpeedControl.h"

#include "HJPluginAVInterleave.h"
#include "HJPluginAVDropping.h"

#include "HJPluginVideoRender.h"

#include "HJTimeline.h"
