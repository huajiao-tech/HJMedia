//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once

#if defined( __cplusplus )
extern "C" {
#endif
#define __STDC_CONSTANT_MACROS

#if defined( HarmonyOS)
#define register
#endif

#include <stdbool.h>
#include "config.h"
#include "libavutil/avutil.h"
#include "libavutil/opt.h"
#include "libavutil/channel_layout.h"
#include "libavutil/bprint.h"
#include "libavutil/samplefmt.h"
#include "libavutil/pixdesc.h"
#include "libavutil/hwcontext.h"
#include "libavutil/avassert.h"
#include "libavutil/imgutils.h"
#include "libavutil/buffer.h"
#include "libavutil/thread.h"
#include "libavutil/audio_fifo.h"
#include "libavutil/pixfmt.h"
#include "libavutil/file.h"
#include "libavutil/error.h"
//#include "libavutil/log.h"
#include "libavutil/mem.h"
#include "libavutil/eval.h"
#include "libavutil/display.h"
//
#include "libavformat/avformat.h"
#include "libavformat/url.h"
#include "libavformat/avc.h"
#include "libavformat/hevc.h"
#include "libavformat/avio.h"
#include "libavformat/avio_internal.h"
#include "libavformat/internal.h"
//
#include "libavcodec/avcodec.h"
#include "libavcodec/h264_ps.h"
#include "libavcodec/h264_parse.h"
#include "libavcodec/hevc/ps.h"
#include "libavcodec/hevc/parse.h"
#include "libavcodec/bsf.h"
#include "libavcodec/put_bits.h"
#include "libavcodec/get_bits.h"
//#include "libavcodec/golomb.h"
#include "libavcodec/h264.h"
#include "libavcodec/h264dsp.h"
#include "libavcodec/h264_sei.h"
#include "libavcodec/h2645_parse.h"
#include "libavcodec/h264data.h"
#include "libavcodec/mpegutils.h"
#include "libavcodec/parser.h"
#include "libavcodec/startcode.h"
#include "libavcodec/packet.h"
#include "libavcodec/atsc_a53.h"
#include "libavcodec/mpeg4audio.h"
//
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
//#include "libavdevice/avdevice.h"
//
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"

#include "libavcodec/mediacodec.h"

#if defined( HarmonyOS)
#undef register
#endif

#if defined( __cplusplus )
}
#endif

