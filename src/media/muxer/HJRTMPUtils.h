//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJCommons.h"

NS_HJ_BEGIN
//***********************************************************************************//
typedef enum HJRTMPEEvent
{
    HJRTMP_EVENT_NONE = 0,
    HJRTMP_EVENT_CONNECTED,
    HJRTMP_EVENT_CONNECT_FAILED,
    HJRTMP_EVENT_STREAM_CONNECTED,
    HJRTMP_EVENT_STREAM_CONNECT_FAIL,
    HJRTMP_EVENT_DISCONNECTED,
    HJRTMP_EVENT_DROP_FRAME,
    HJRTMP_EVENT_AUTOADJUST_BITRATE,
    HJRTMP_EVENT_SEND_Error,
	HJRTMP_EVENT_RECV_Error,
	HJRTMP_EVENT_LOW_BITRATE,
	HJRTMP_EVENT_RETRY,
	HJRTMP_EVENT_RETRY_TIMEOUT,
	HJRTMP_EVENT_NET_BITRATE,
	HJRTMP_EVENT_LIVE_INFO,
	HJRTMP_EVENT_DONE,
} HJRTMPEEvent;
HJEnumToStringFuncDecl(HJRTMPEEvent);

typedef enum HJColorPrimaries {
	HJCOL_PRI_RESERVED0 = 0,
	HJCOL_PRI_BT709 =
	1, ///< also ITU-R BT1361 / IEC 61966-2-4 / SMPTE RP 177 Annex B
	HJCOL_PRI_UNSPECIFIED = 2,
	HJCOL_PRI_RESERVED = 3,
	HJCOL_PRI_BT470M =
	4, ///< also FCC Title 47 Code of Federal Regulations 73.682 (a)(20)
	HJCOL_PRI_BT470BG =
	5, ///< also ITU-R BT601-6 625 / ITU-R BT1358 625 / ITU-R BT1700 625 PAL & SECAM
	HJCOL_PRI_SMPTE170M =
	6, ///< also ITU-R BT601-6 525 / ITU-R BT1358 525 / ITU-R BT1700 NTSC
	HJCOL_PRI_SMPTE240M =
	7, ///< identical to above, also called "SMPTE C" even though it uses D65
	HJCOL_PRI_FILM = 8,      ///< colour filters using Illuminant C
	HJCOL_PRI_BT2020 = 9,    ///< ITU-R BT2020
	HJCOL_PRI_SMPTE428 = 10, ///< SMPTE ST 428-1 (CIE 1931 XYZ)
	HJCOL_PRI_SMPTEST428_1 = HJCOL_PRI_SMPTE428,
	HJCOL_PRI_SMPTE431 = 11, ///< SMPTE ST 431-2 (2011) / DCI P3
	HJCOL_PRI_SMPTE432 =
	12, ///< SMPTE ST 432-1 (2010) / P3 D65 / Display P3
	HJCOL_PRI_EBU3213 =
	22, ///< EBU Tech. 3213-E (nothing there) / one of JEDEC P22 group phosphors
	HJCOL_PRI_JEDEC_P22 = HJCOL_PRI_EBU3213,
	HJCOL_PRI_NB ///< Not part of ABI
} HJColorPrimaries;
/**
 * Color Transfer Characteristic.
 * These values match the ones defined by ISO/IEC 23091-2_2019 subclause 8.2.
 */
typedef enum HJColorTransferCharacteristic {
	HJCOL_TRC_RESERVED0 = 0,
	HJCOL_TRC_BT709 = 1, ///< also ITU-R BT1361
	HJCOL_TRC_UNSPECIFIED = 2,
	HJCOL_TRC_RESERVED = 3,
	HJCOL_TRC_GAMMA22 =
	4, ///< also ITU-R BT470M / ITU-R BT1700 625 PAL & SECAM
	HJCOL_TRC_GAMMA28 = 5, ///< also ITU-R BT470BG
	HJCOL_TRC_SMPTE170M =
	6, ///< also ITU-R BT601-6 525 or 625 / ITU-R BT1358 525 or 625 / ITU-R BT1700 NTSC
	HJCOL_TRC_SMPTE240M = 7,
	HJCOL_TRC_LINEAR = 8, ///< "Linear transfer characteristics"
	HJCOL_TRC_LOG =
	9, ///< "Logarithmic transfer characteristic (100:1 range)"
	HJCOL_TRC_LOG_SQRT =
	10, ///< "Logarithmic transfer characteristic (100 * Sqrt(10) : 1 range)"
	HJCOL_TRC_IEC61966_2_4 = 11, ///< IEC 61966-2-4
	HJCOL_TRC_BT1361_ECG = 12,   ///< ITU-R BT1361 Extended Colour Gamut
	HJCOL_TRC_IEC61966_2_1 = 13, ///< IEC 61966-2-1 (sRGB or sYCC)
	HJCOL_TRC_BT2020_10 = 14,    ///< ITU-R BT2020 for 10-bit system
	HJCOL_TRC_BT2020_12 = 15,    ///< ITU-R BT2020 for 12-bit system
	HJCOL_TRC_SMPTE2084 =
	16, ///< SMPTE ST 2084 for 10-, 12-, 14- and 16-bit systems
	HJCOL_TRC_SMPTEST2084 = HJCOL_TRC_SMPTE2084,
	HJCOL_TRC_SMPTE428 = 17, ///< SMPTE ST 428-1
	HJCOL_TRC_SMPTEST428_1 = HJCOL_TRC_SMPTE428,
	HJCOL_TRC_ARIB_STD_B67 =
	18,   ///< ARIB STD-B67, known as "Hybrid log-gamma"
	HJCOL_TRC_NB ///< Not part of ABI
} HJColorTransferCharacteristic;

/**
 * YUV colorspace type.
 * These values match the ones defined by ISO/IEC 23091-2_2019 subclause 8.3.
 */
typedef enum HJColorSpace {
	HJCOL_SPC_RGB =
	0, ///< order of coefficients is actually GBR, also IEC 61966-2-1 (sRGB), YZX and ST 428-1
	HJCOL_SPC_BT709 =
	1, ///< also ITU-R BT1361 / IEC 61966-2-4 xvYCC709 / derived in SMPTE RP 177 Annex B
	HJCOL_SPC_UNSPECIFIED = 2,
	HJCOL_SPC_RESERVED =
	3, ///< reserved for future use by ITU-T and ISO/IEC just like 15-255 are
	HJCOL_SPC_FCC =
	4, ///< FCC Title 47 Code of Federal Regulations 73.682 (a)(20)
	HJCOL_SPC_BT470BG =
	5, ///< also ITU-R BT601-6 625 / ITU-R BT1358 625 / ITU-R BT1700 625 PAL & SECAM / IEC 61966-2-4 xvYCC601
	HJCOL_SPC_SMPTE170M =
	6, ///< also ITU-R BT601-6 525 / ITU-R BT1358 525 / ITU-R BT1700 NTSC / functionally identical to above
	HJCOL_SPC_SMPTE240M =
	7, ///< derived from 170M primaries and D65 white point, 170M is derived from BT470 System M's primaries
	HJCOL_SPC_YCGCO =
	8, ///< used by Dirac / VC-2 and H.264 FRext, see ITU-T SG16
	HJCOL_SPC_YCOCG = HJCOL_SPC_YCGCO,
	HJCOL_SPC_BT2020_NCL =
	9, ///< ITU-R BT2020 non-constant luminance system
	HJCOL_SPC_BT2020_CL = 10, ///< ITU-R BT2020 constant luminance system
	HJCOL_SPC_SMPTE2085 = 11, ///< SMPTE 2085, Y'D'zD'x
	HJCOL_SPC_CHROMA_DERIVED_NCL =
	12, ///< Chromaticity-derived non-constant luminance system
	HJCOL_SPC_CHROMA_DERIVED_CL =
	13, ///< Chromaticity-derived constant luminance system
	HJCOL_SPC_ICTCP = 14, ///< ITU-R BT.2100-0, ICtCp
	HJCOL_SPC_NB          ///< Not part of ABI
} HJColorSpace;

typedef enum HJVideoColorspace {
	HJ_VIDEO_CS_DEFAULT,
	HJ_VIDEO_CS_601,
	HJ_VIDEO_CS_709,
	HJ_VIDEO_CS_SRGB,
	HJ_VIDEO_CS_2100_PQ,
	HJ_VIDEO_CS_2100_HLG,
} HJVideoColorspace;

typedef enum HJRTMPSentStatus
{
	HJ_RTMP_SENT_NONE		= 0x00,
	HJ_RTMP_SENT_META		= 0x01,
	HJ_RTMP_SENT_EXTRA_META = 0x02,
	HJ_RTMP_SENT_A_HEADER	= 0x04,
	HJ_RTMP_SENT_V_HEADER	= 0x08,
	HJ_RTMP_SENT_HDR_HEADER = 0x10,
	HJ_RTMP_SENT_FRAME		= 0x20,
	HJ_RTMP_SENT_FOOTER		= 0x40,
	HJ_RTMP_SENT_RESERVED	= 0x100,
}HJRTMPSentStatus;

//***********************************************************************************//
class HJRTMPUtils
{
public:
	static const std::string STORE_KEY_LOCALURL;
	static const std::string STORE_KEY_SOCKET_BUF_SIZE;
	static const std::string STORE_KEY_RETRY_TIME_LIMITED;
	static const std::string STORE_KEY_DROP_ENABLE;
	static const std::string STORE_KEY_DROP_THRESHOLD;
	static const std::string STORE_KEY_DROP_PFRAME_THRESHOLD;
	static const std::string STORE_KEY_DROP_IFRAME_THRESHOLD;
	//
	static const std::string STORE_KEY_LOWBR_TIMEOUT_ENABLE;
	static const std::string STORE_KEY_LOWBR_TIMEOUT_LIMITED;
	static const std::string STORE_KEY_LOWBR_LIMITED;
	//
	static const std::string STORE_KEY_BITRATE;
	static const std::string STORE_KEY_LOW_BITRATE;
	static const std::string STORE_KEY_ADJUST_INTERVAL;
};


NS_HJ_END