#include "HJDataConvert.h"
#include "HJFLog.h"
#include "libyuv.h"
#include <cstdint>
#include "stb_image_write.h"
#include "HJSPBuffer.h"

NS_HJ_BEGIN

HJVec4i HJDataConvert::cvtGetScaleOffset(HJDataScaleType i_type, int i_srcW, int i_srcH, int i_dstW, int i_dstH)
{
	int width = i_srcW;
	int height = i_srcH;
	int dw = i_dstW;
	int dh = i_dstH;
	
	int lhs = width * dh;   
	int rhs = height * dw;  
	switch (i_type)
	{
	case HJDataScaleType::Fit:
	{
		if (rhs < lhs)
		{
			dh = (dw * height) / width;
			dw = i_dstW;
		}
		else
		{
			dw = (dh * width) / height;
			dh = i_dstH;
		}
	}
		break;
	case HJDataScaleType::Clip:
	{
		if (rhs < lhs)
		{
			dw = (dh * width) / height;
			dh = i_dstH;
		}
		else
		{
			dh = (dw * height) / width;
			dw = i_dstW;
		}
	}
		break;
	case HJDataScaleType::Origin:
	{
		dw = i_srcW;
		dh = i_srcH;
	}
		break;
	case HJDataScaleType::Full:
	{
		dw = i_dstW;
		dh = i_dstH;
	}
		break;
	default:
		break;
	}
	return HJVec4i{dw, dh, (i_dstW - dw) / 2, (i_dstH - dh) / 2 };
}

void HJDataConvert::cvtSaveToImage(HJConvertDataType i_type, unsigned char* i_data[4], int i_nPitch[4], int i_nWidth, int i_nHeight, const std::string& i_pFilePath)
{
    int w = i_nWidth;
    int h = i_nHeight;
	HJSPBuffer::Ptr rgb = nullptr;
	unsigned char* pValue = nullptr;
	int nChannel = (i_type == HJConvertDataType_RGBA) ? 4 : 3;

	if (i_type == HJConvertDataType_RGBA)
    {
		pValue = i_data[0];
    }
	else
	{
		rgb = HJSPBuffer::create(w * h * nChannel);
		if (i_type == HJConvertDataType_YUVNV12)
		{
			unsigned char* pY = i_data[0];
			unsigned char* pUV = i_data[1];
			if (!pY || !pUV || w <= 0 || h <= 0)
			{
				return;
			}
			//libyuv::NV12ToRAW to r g b;    libyuv::NV12ToRGB24 to b g r;
			if (libyuv::NV12ToRAW(pY, i_nPitch[0], pUV, i_nPitch[1], rgb->getBuf(), w * nChannel, w, h) != 0)
			{
				HJFLoge("NV12ToRGB24 failed");
				return;
			}
		}
		if (i_type == HJConvertDataType_RGB)
		{
			memcpy(rgb->getBuf(), i_data[0], w * h * nChannel);
		}
		pValue = rgb->getBuf();
	}
	

	if (stbi_write_png(i_pFilePath.c_str(), w, h, nChannel, pValue, w * nChannel) == 0)
	{
		HJFLoge("write png failed {}", i_pFilePath);
	}
	else
	{
		HJFLogi("saved {}", i_pFilePath);
	}
}


NS_HJ_END
