#pragma once

#include <string>
#include <functional>
#include "HJCommonInterface.h"

typedef enum HJPusherCodecType
{
    HJCodecUnknown = -1,
    HJCodecH264    = 27,    
    HJVCodecH265   = 173,
    HJCodecAAC     = 86018,
} HJPusherCodecType;

typedef enum HJPusherAudioSampleFormat
{
    AudioSampleFormat_NONE = -1,
    AudioSampleFormat_U8   = 0,         ///< unsigned 8 bits
    AudioSampleFormat_S16  = 1,         ///< signed 16 bits
    AudioSampleFormat_S32  = 2,         ///< signed 32 bits
    AudioSampleFormat_FLT  = 3,         ///< float
    AudioSampleFormat_DBL  = 4,         ///< double

    AudioSampleFormat_U8P  = 5,         ///< unsigned 8 bits, planar
    AudioSampleFormat_S16P = 6,         ///< signed 16 bits, planar
    AudioSampleFormat_S32P = 7,         ///< signed 32 bits, planar
    AudioSampleFormat_FLTP = 8,         ///< float, planar
    AudioSampleFormat_DBLP = 9,         ///< double, planar
    AudioSampleFormat_S64  = 10,         ///< signed 64 bits
    AudioSampleFormat_S64P = 11,        ///< signed 64 bits, planar

    AudioSampleFormat_NB   = 12           ///< Number of sample formats. DO NOT USE if linking dynamically
} HJPusherAudioSampleFormat;

typedef struct HJPusherPreviewInfo
{
    int videoWidth  = 0;
    int videoHeight = 0;
    int videoFps    = 30;
} HJPusherPreviewInfo;

typedef struct HJPusherVideoInfo
{
    int videoCodecId = HJCodecUnknown;
    int videoWidth  = 0;
    int videoHeight = 0;
    int videoBitrateBit = 0;
    int videoFramerate  = 30;
    int videoGopSize    = 60;
} HJPusherVideoInfo;

typedef struct HJPusherAudioInfo
{
    int audioCodecId = HJCodecUnknown;
    int audioBitrateBit = 0;
    
    int audioCaptureSampleRate = 0;
    int audioCaptureChannels   = 0;
    
    int audioEncoderSampleRate = 0;
    int audioEncoderChannels   = 0;
    
    int audioSampleFormat      = AudioSampleFormat_S16;
} HJPusherAudioInfo;

typedef struct HJPusherRTMPInfo
{
    std::string rtmpUrl = "";
    
} HJPusherRTMPInfo;

typedef struct HJPusherRecordInfo
{
    std::string recordUrl = "";
} HJPusherRecordInfo;

typedef struct HJPusherPNGSegInfo
{
    std::string pngSeqUrl = "";
} HJPusherPNGSegInfo;

typedef enum HJPusherNofityType
{
    HJ_PUSHER_NOTIFY_NONE = 0,
    HJ_PUSHER_NOTIFY_CONNECT_SUCCESS = 1,
    HJ_PUSHER_NOTIFY_CONNECT_FAILED = 2,
    HJ_PUSHER_NOTIFY_CONNECT_CLOSED = 3,
    HJ_PUSHER_NOTIFY_CONNECT_RETRY = 4,
    HJ_PUSHER_NOTIFY_ERROR = 5,
    HJ_PUSHER_NOTIFY_DROP_FRAME = 6,
    HJ_PUSHER_NOTIFY_AUTOADJUST_BITRATE = 7,
    HJ_PUSHER_NOTIFY_LIVE_INFO = 8,

    HJ_PUSHER_NOTIFY_ERROR_MUXER_INIT = 11,
    HJ_PUSHER_NOTIFY_ERROR_MUXER_WRITEFRAME = 12,
    HJ_PUSHER_NOTIFY_ERROR_CODEC_RUN = 13,
    HJ_PUSHER_NOTIFY_ERROR_CODEC_GETFRAME = 14,
    HJ_PUSHER_NOTIFY_ERROR_AUDIOFIFO_ADDFRAME = 15,
    HJ_PUSHER_NOTIFY_ERROR_CAPTURER_GETFRAME = 16,

    HJ_RENDER_NOTIFY_ERROR_DEFAULT = 100,
    HJ_RENDER_NOTIFY_ERROR_UPDATE = 101,
    HJ_RENDER_NOTIFY_ERROR_DRAW = 102,
    HJ_RENDER_NOTIFY_ERROR_VIDEOCAPTURE_INIT = 103,
    HJ_RENDER_NOTIFY_ERROR_PNGSEQ_INIT = 104,
    HJ_RENDER_NOTIFY_PNGSEQ_COMPLETE = 105,

} HJPusherNofityType;






