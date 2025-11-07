//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include <cerrno>
#include <string>
#include <unordered_map>
#include "HJMacros.h"

NS_HJ_BEGIN
//***********************************************************************************//
typedef int32_t HJRet;

#define HJ_OK                  0x00000000
#define HJErrDefault           (HJ_OK - 4000000)
#define HJResDefault           (HJ_OK + 1000000)
#define HJDefErrID(id)         (HJErrDefault - id)
#define HJDefResID(id)         (HJ_OK + id)

//Result
#define HJ_EOF                 HJDefResID(1)
#define HJ_BOF                 HJDefResID(2)
#define HJ_WOULD_BLOCK         HJDefResID(3)
#define HJ_ECHO                HJDefResID(4)
#define HJ_IDLE                HJDefResID(5)
#define HJ_AGAIN               HJDefResID(6)
#define HJ_RESTART             HJDefResID(7)
#define HJ_MORE_DATA           HJDefResID(8)
#define HJ_REDIRECT            HJDefResID(9)
#define HJ_CLOSE               HJDefResID(11)
#define HJ_STOP                HJDefResID(12)
#define HJ_INITED              HJDefResID(13)

//Base Error
#define HJErrFatal             HJDefErrID(1)
#define HJErrExcep             HJDefErrID(2)
#define HJErrMemAlloc          HJDefErrID(3)
#define HJErrNewObj            HJDefErrID(4)
#define HJErrInvalidParams     HJDefErrID(5)
#define HJErrInvalidResult     HJDefErrID(6)
#define HJErrNotSupport        HJDefErrID(7)
#define HJErrTimeOut           HJDefErrID(8)
#define HJErrNotImp            HJDefErrID(9)
#define HJErrInvalidPath       HJDefErrID(10)
#define HJErrCacheFull         HJDefErrID(11)
#define HJErrAlreadyExist      HJDefErrID(12)
#define HJErrNotAlready        HJDefErrID(13)
#define HJErrInvalidData       HJDefErrID(14)
#define HJErrNotFind           HJDefErrID(15)
#define HJErrInvalid           HJDefErrID(16)
#define HJErrInvalidUrl        HJDefErrID(17)
#define HJErrFull              HJDefErrID(18)
#define HJErrNotMInfo          HJDefErrID(19)
#define HJErrNoticeCenter      HJDefErrID(20)
#define HJErrExecutor          HJDefErrID(21)
#define HJErrTaskCreate        HJDefErrID(22)
#define HJErrCoTaskCreate      HJDefErrID(23)
#define HJErrTaskExist         HJDefErrID(24)
#define HJErrCoTaskExist       HJDefErrID(25)
#define HJErrNotExist          HJDefErrID(26)
#define HJErrCreateThread      HJDefErrID(27)
#define HJErrInvalidCert       HJDefErrID(28)
#define HJErrAlreadyInited     HJDefErrID(29)
#define HJErrAlreadyDone       HJDefErrID(30)
#define HJErrNotInited         HJDefErrID(31)
#define HJErrInvalidFile       HJDefErrID(32)

//ffmpeg
#define HJErrFFNewIC           HJDefErrID(1000)
#define HJErrFFLoadUrl         HJDefErrID(1001)
#define HJErrFFFindInfo        HJDefErrID(1002)
#define HJErrFFSeek            HJDefErrID(1003)
#define HJErrFFNewAVCtx        HJDefErrID(1004)
#define HJErrFFCodec           HJDefErrID(1005)
#define HJErrFFAVFrame         HJDefErrID(1006)
#define HJErrFFGetFrame        HJDefErrID(1007)
#define HJErrENOSPC            HJDefErrID(1008)
#define HJErrEINVAL            HJDefErrID(1009)
#define HJErrFIFOREAD          HJDefErrID(1010)
#define HJErrETIME             HJDefErrID(1011)
#define HJErrParseData         HJDefErrID(1012)
#define HJErrFFAVPacket        HJDefErrID(1013)

//render
#define HJErrRender            HJDefErrID(2000)
#define HJErrCreateRender      HJDefErrID(2001)
#define HJErrCreateTexture     HJDefErrID(2002)
#define HJErrUploadTexture     HJDefErrID(2003)
#define HJErrCreateADevice     HJDefErrID(2004)
#define HJErrAContext          HJDefErrID(2005)

//codec
#define HJErrCodecCreate       HJDefErrID(3000)
#define HJErrCodecInit         HJDefErrID(3001)
#define HJErrHWDevice          HJDefErrID(3002)
#define HJErrHWFrameCtx        HJDefErrID(3003)
#define HJErrESParse           HJDefErrID(3004)
#define HJErrCodecParams       HJDefErrID(3005)
#define HJErrCodecEncode       HJDefErrID(3006)
#define HJErrCodecDecode       HJDefErrID(3007)
#define HJErrCodecConfig       HJDefErrID(3008)
#define HJErrCodecCallback     HJDefErrID(3009)
#define HJErrCodecPrepare      HJDefErrID(3010)
#define HJErrCodecStart        HJDefErrID(3011)
#define HJErrCodecBuffer       HJDefErrID(3012)

//IO
#define HJErrIO                HJDefErrID(4000)
#define HJErrIOOpen            HJDefErrID(4001)
#define HJErrIOSTAT            HJDefErrID(4002)
#define HJErrIOFail            HJDefErrID(4003)
#define HJErrIOSeek            HJDefErrID(4004)
#define HJErrIORead            HJDefErrID(4005)
#define HJErrIOWrite           HJDefErrID(4006)
#define HJErrIOHandShake       HJDefErrID(4007)
#define HJErrIOReply           HJDefErrID(4008)

//server
#define HJErrServer            HJDefErrID(5000)
#define HJErrServerInit        HJDefErrID(5001)
#define HJErrServerOpen        HJDefErrID(5002)

//JSON
#define HJErrJSONRead          HJDefErrID(6000)
#define HJErrJSONWrite         HJDefErrID(6001)
#define HJErrJSONValue         HJDefErrID(6002)
#define HJErrJSONAddValue      HJDefErrID(6003)

//Muxer
#define HJErrMuxer             HJDefErrID(7000)
#define HJErrMuxerOpen         HJDefErrID(7001)
#define HJErrMuxerDTS          HJDefErrID(7002)

//net
#define HJErrNetUnkown         HJDefErrID(8000)
#define HJErrNetConnectInit    HJDefErrID(8001)
#define HJErrNetConnectFailed  HJDefErrID(8002)
#define HJErrNetInvalidStream  HJDefErrID(8003)
#define HJErrNetNonBlocking    HJDefErrID(8004)
#define HJErrNetDisconnected   HJDefErrID(8005)
#define HJErrNetRecv           HJDefErrID(8006)
#define HJErrNetSend           HJDefErrID(8007)
#define HJErrNetDeadlineExceeded   HJDefErrID(8008)   //DEADLINE_EXCEEDED

//rtmp
#define HJErrRTMPUnkown        HJDefErrID(9000)
#define HJErrRTMPTag           HJDefErrID(9001)
#define HJErrRTMPADTag         HJDefErrID(9002)
#define HJErrRTMPWrite         HJDefErrID(9003)
#define HJErrRTMPRetry         HJDefErrID(9004)

//***********************************************************************************//
const std::string HJErrorCodeToMsg(const int32_t code);
#define HJErr2Msg(code) HJErrorCodeToMsg(code)

NS_HJ_END
