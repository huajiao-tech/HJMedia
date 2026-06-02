#pragma once

#include "HJUIItemSR.h"
#include "HJFLog.h"
#include "imgui.h"
#include "HJInferenceContextExport.h"
#include "HJVisualDataUI.h"
#include "utils/HJFaceSRWrapper.h"
#include "HJTransferMediaData.h"
#include "HJUtilitys.h"
#include "HJError.h"
#include "HJOGCommon.h"
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <cmath>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

NS_HJ_BEGIN

namespace
{
constexpr float kNarrowComboWidth = 110.0f;
constexpr float kMediumComboWidth = 190.0f;
constexpr float kNarrowSliderWidth = 150.0f;

struct SRPresetSize
{
	const char* label;
	int width;
	int height;
};

constexpr SRPresetSize kFaceScalePresets[] = {
	{ "80x100", 80, 100 },
	{ "90x114", 90, 114 },
	{ "100x126", 100, 126 },
	{ "180x228", 180, 228 },
	{ "220x278", 220, 278 },
	{ "300x380", 300, 380 },
	{ "360x456", 360, 456 },
};

constexpr SRPresetSize kFullScalePresets[] = {
	{ "90x160", 90, 160 },
	{ "180x320", 180, 320 },
	{ "224x398", 224, 398 },
	{ "300x534", 300, 534 },
	{ "360x640", 360, 640 },
};

static HJFaceSRProcessPolicy getProcPolicyByIndex(int procPolicyIndex)
{
	switch (procPolicyIndex)
	{
	case 0:
		return HJFaceSRProcessPolicy_Mipmap;
	case 1:
		return HJFaceSRProcessPolicy_Bilinear;
	case 2:
		return HJFaceSRProcessPolicy_Bicubic;
	case 3:
		return HJFaceSRProcessPolicy_Lanczos3;
	case 4:
		return HJFaceSRProcessPolicy_Lanczos4;
	default:
		return HJFaceSRProcessPolicy_Mipmap;
	}
}

static bool isFaceMode(int srMode)
{
	return srMode == 1 || srMode == 2;
}

static bool usesPreScaleMode(int srMode)
{
	return srMode == 2 || srMode == 3;
}

static const SRPresetSize* getPresetListByMode(int srMode, int& outCount)
{
	if (srMode == 2)
	{
		outCount = (int)IM_ARRAYSIZE(kFaceScalePresets);
		return kFaceScalePresets;
	}
	if (srMode == 3)
	{
		outCount = (int)IM_ARRAYSIZE(kFullScalePresets);
		return kFullScalePresets;
	}
	outCount = 0;
	return nullptr;
}

static void resolvePreScaleForMode(int srMode, int facePresetIndex, int fullPresetIndex, int& outWidth, int& outHeight)
{
	outWidth = 0;
	outHeight = 0;
	int presetCount = 0;
	const SRPresetSize* presets = getPresetListByMode(srMode, presetCount);
	if (!presets || presetCount <= 0)
	{
		return;
	}

	int presetIndex = (srMode == 2) ? facePresetIndex : fullPresetIndex;
	presetIndex = (std::max)(0, (std::min)(presetIndex, presetCount - 1));
	outWidth = presets[presetIndex].width;
	outHeight = presets[presetIndex].height;
}

static std::string buildPadInfoText(int scaleW, int scaleH, int padLeft, int padRight, int padTop, int padBottom)
{
	const int padW = padLeft + padRight;
	const int padH = padTop + padBottom;
	if (padW > 0)
	{
		return std::to_string(padW) + "x" + std::to_string((std::max)(0, scaleH));
	}
	if (padH > 0)
	{
		return std::to_string((std::max)(0, scaleW)) + "x" + std::to_string(padH);
	}
	return "0x0";
}

static bool isTextureFSRType(int srWrapperType)
{
	return srWrapperType == 1;
}

static int uploadFrameToTexture(const HJTransferMediaData::Ptr& media, GLuint& ioTexture)
{
	if (!media)
	{
		return HJErrInvalidParams;
	}

	HJTransferMediaData::Ptr rgba = media;
	if (rgba->getConvertType() != HJConvertDataType_RGBA)
	{
		rgba = HJTransferMediaData::create(media, HJConvertDataType_RGBA);
	}
	if (!rgba || !rgba->getData(0) || rgba->getWidth() <= 0 || rgba->getHeight() <= 0)
	{
		return HJErrInvalidParams;
	}

	if (ioTexture == 0)
	{
		ioTexture = HJOGCommon::textureCreate();
	}
	HJOGCommon::textureUploadRGBA(ioTexture, rgba->getWidth(), rgba->getHeight(), rgba->getData(0));
	return HJ_OK;
}

static HJTransferMediaData::Ptr downloadTextureToRGBAFrame(GLuint textureId, int width, int height, int64_t timestamp)
{
#if !defined(WIN32_LIB)
	HJ_UNUSED(textureId);
	HJ_UNUSED(width);
	HJ_UNUSED(height);
	HJ_UNUSED(timestamp);
	return nullptr;
#else
	if (textureId == 0 || width <= 0 || height <= 0)
	{
		return nullptr;
	}

	std::vector<unsigned char> rgba((size_t)width * (size_t)height * 4);
	glBindTexture(GL_TEXTURE_2D, textureId);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
	glBindTexture(GL_TEXTURE_2D, 0);

	unsigned char* data[4] = { rgba.data(), nullptr, nullptr, nullptr };
	int stride[4] = { width * 4, 0, 0, 0 };
	return HJTransferMediaDataRGBA::Create<HJTransferMediaDataRGBA>(data, stride, width, height, timestamp);
#endif
}
}

HJUIItemSR::HJUIItemSR()
{
	m_visualDataUI = HJVisualDataUI::Create();

	HJEntryContextInfo infoInference;
	infoInference.logIsValid = true;
	infoInference.logDir = "e:/infoInference";
	infoInference.logLevel = HJLOGLevel_INFO;
	infoInference.logMode = HJLogLMode_CONSOLE | HJLLogMode_FILE;
	infoInference.logMaxFileSize = 5 * 1024 * 1024;
	infoInference.logMaxFileNum = 5;
	inferenceContextInit(infoInference);
}

HJUIItemSR::~HJUIItemSR()
{
	priDone();
	inferenceContextUnInit();
}

//bool HJUIItemSR::updateTitle()
//{
//	std::string info = HJFMT("Mouse: ({}, {}) FPS:{:.1f}", ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y, ImGui::GetIO().Framerate);
//	glfwSetWindowTitle((GLFWwindow*)getWindow(), info.c_str());
//	return true;
//}

int HJUIItemSR::priRun()
{
	int i_err = HJ_OK;
	do
	{
		resolvePreScaleForMode(m_srMode, m_faceScalePresetIndex, m_fullScalePresetIndex, m_preScaleWidth, m_preScaleHeight);
		if (m_visualDataUI)
		{
			i_err = m_visualDataUI->drawUI("SR VisualData");
			if (i_err < 0)
			{
				break;
			}

			HJTransferMediaData::Ptr frame = m_isPaused ? m_cachedPauseFrame : m_visualDataUI->acquire();
			const bool needRecovery = (!m_isPaused && frame != nullptr);
			if (frame)
			{
				m_actualInputWidth = frame->getWidth();
				m_actualInputHeight = frame->getHeight();

				HJTransferMediaData::Ptr srFrame = nullptr;
				HJTransferMediaData::Ptr frameForSR = frame;
				HJFaceSRProcessResult srProcessRet;
				const bool useTextureFSR = isTextureFSRType(m_srWrapperType);
				const int previewTargetW = (std::max)(1, frameForSR->getWidth() * (std::max)(1, m_srRatio));
				const int previewTargetH = (std::max)(1, frameForSR->getHeight() * (std::max)(1, m_srRatio));
				if (m_srEnabled && m_faceSR)
				{
					int srRet = HJ_OK;
					if (useTextureFSR)
					{
						srRet = uploadFrameToTexture(frameForSR, m_fsrInputTexture);
						if (srRet >= 0)
						{
							uint32_t outputTextureId = 0;
							int outputWidth = 0;
							int outputHeight = 0;
							srRet = m_faceSR->process(
								static_cast<uint32_t>(m_fsrInputTexture),
								frameForSR->getWidth(),
								frameForSR->getHeight(),
								outputTextureId,
								outputWidth,
								outputHeight,
								srProcessRet);
							if (srRet >= 0)
							{
								srFrame = downloadTextureToRGBAFrame(outputTextureId, outputWidth, outputHeight, frameForSR->getTimeStamp());
								if (!srFrame)
								{
									srRet = HJErrFatal;
								}
							}
						}
					}
					else
					{
						HJFaceSRProcessOption processOption;
						processOption.mode = (m_srMode == 0)
							? HJFaceSRProcessMode_Full
							: ((m_srMode == 2) ? HJFaceSRProcessMode_FaceScale
								: ((m_srMode == 3) ? HJFaceSRProcessMode_FullScale : HJFaceSRProcessMode_FaceOrigin));
						processOption.preScaleWidth = usesPreScaleMode(m_srMode) ? m_preScaleWidth : 0;
						processOption.preScaleHeight = usesPreScaleMode(m_srMode) ? m_preScaleHeight : 0;
						processOption.composeCanvasWidth = isFaceMode(m_srMode) ? previewTargetW : 0;
						processOption.composeCanvasHeight = isFaceMode(m_srMode) ? previewTargetH : 0;
						processOption.bFeather = isFaceMode(m_srMode) ? m_featherAlphaEnable : false;
						processOption.bEnablePostSRDisplayResize = m_enableNativePostSRDisplayResize;
						processOption.mixAlphaRatio = m_srAlphaRatio;
						processOption.policy = getProcPolicyByIndex(m_faceScaleProcPolicy);
						srRet = m_faceSR->process(frameForSR, processOption, srProcessRet);
						if (srRet >= 0)
						{
							srFrame = m_faceSR->takeLastOutput();
						}
					}
					if (srRet < 0)
					{
						HJFLogw("HJFaceSRWrapper process failed ret:{}", srRet);
					}
					m_srElapseMs = srProcessRet.srElapsedMs;
					m_lastFaceScaleW = srProcessRet.scaleW;
					m_lastFaceScaleH = srProcessRet.scaleH;
					m_lastSRFaceWidth = srFrame ? srFrame->getWidth() : 0;
					m_lastSRFaceHeight = srFrame ? srFrame->getHeight() : 0;
					m_lastFaceTargetDisplayW = srProcessRet.faceTargetDisplayWidth;
					m_lastFaceTargetDisplayH = srProcessRet.faceTargetDisplayHeight;
					m_lastPadLeft = srProcessRet.padLeft;
					m_lastPadRight = srProcessRet.padRight;
					m_lastPadTop = srProcessRet.padTop;
					m_lastPadBottom = srProcessRet.padBottom;
				}
				else
				{
					m_srElapseMs = 0;
					m_lastFaceScaleW = 0;
					m_lastFaceScaleH = 0;
					m_lastSRFaceWidth = 0;
					m_lastSRFaceHeight = 0;
					m_lastFaceTargetDisplayW = 0;
					m_lastFaceTargetDisplayH = 0;
					m_lastPadLeft = 0;
					m_lastPadRight = 0;
					m_lastPadTop = 0;
					m_lastPadBottom = 0;
					m_faceRectsUI.clear();
				}

				static bool s_windowInit = false;
				if (!s_windowInit)
				{
					cv::namedWindow("SR OpenCV Preview", cv::WINDOW_AUTOSIZE);
					s_windowInit = true;
				}

				cv::Mat showMat;
				auto toRgba = [](HJTransferMediaData::Ptr media, cv::Mat& rgba) -> bool
				{
					if (!media)
					{
						return false;
					}
					const int w = media->getWidth();
					const int h = media->getHeight();
					if (w <= 0 || h <= 0)
					{
						return false;
					}
					const HJConvertDataType type = media->getConvertType();
					if (type == HJConvertDataType_RGBA)
					{
						rgba = cv::Mat(h, w, CV_8UC4, media->getData(0), media->getStride(0)).clone();
						return !rgba.empty();
					}
					if (type == HJConvertDataType_RGB)
					{
						cv::Mat rgb(h, w, CV_8UC3, media->getData(0), media->getStride(0));
						cv::cvtColor(rgb, rgba, cv::COLOR_RGB2RGBA);
						return !rgba.empty();
					}
					return false;
				};
				auto toBgr = [&toRgba](HJTransferMediaData::Ptr media, cv::Mat& bgr) -> bool
				{
					cv::Mat rgba;
					if (!toRgba(media, rgba) || rgba.empty())
					{
						return false;
					}
					cv::cvtColor(rgba, bgr, cv::COLOR_RGBA2BGR);
					return !bgr.empty();
				};

				m_faceRectsUI.clear();
				if (srFrame)
				{
					const int displaySrMode = useTextureFSR ? 0 : m_srMode;
					cv::Mat srcBgr;
					cv::Mat srBgr;
					if (toBgr(frameForSR, srcBgr) && toBgr(srFrame, srBgr) && !srcBgr.empty() && !srBgr.empty())
					{
						if (isFaceMode(displaySrMode))
						{
							m_faceRectsUI.push_back(srProcessRet.faceRect);
							const int targetW = previewTargetW;
							const int targetH = previewTargetH;
							cv::Mat srcRgba;
							cv::Mat srRgba;
							if (toRgba(frameForSR, srcRgba) && toRgba(srFrame, srRgba) && !srcRgba.empty() && !srRgba.empty())
							{
								cv::Mat srcScaledRgba;
								cv::resize(srcRgba, srcScaledRgba, cv::Size(targetW, targetH), 0, 0, cv::INTER_LINEAR);
								cv::Mat faceSrRgba = srcScaledRgba.clone();

								const double sx = (double)targetW / (double)(std::max)(1, frameForSR->getWidth());
								const double sy = (double)targetH / (double)(std::max)(1, frameForSR->getHeight());
								int dstX = (int)std::lround(srProcessRet.faceRect.x * sx);
								int dstY = (int)std::lround(srProcessRet.faceRect.y * sy);
								int dstW = srProcessRet.faceTargetDisplayWidth > 0
									? srProcessRet.faceTargetDisplayWidth
									: (int)std::lround(srProcessRet.faceRect.w * sx);
								int dstH = srProcessRet.faceTargetDisplayHeight > 0
									? srProcessRet.faceTargetDisplayHeight
									: (int)std::lround(srProcessRet.faceRect.h * sy);
								if (dstW > 0 && dstH > 0)
								{
									dstX = (std::max)(0, (std::min)(dstX, faceSrRgba.cols - 1));
									dstY = (std::max)(0, (std::min)(dstY, faceSrRgba.rows - 1));
									if (dstX + dstW > faceSrRgba.cols)
									{
										dstW = faceSrRgba.cols - dstX;
									}
									if (dstY + dstH > faceSrRgba.rows)
									{
										dstH = faceSrRgba.rows - dstY;
									}
									if (dstW > 0 && dstH > 0)
									{
										cv::Mat patch = srRgba;
										if (patch.cols != dstW || patch.rows != dstH)
										{
											cv::resize(srRgba, patch, cv::Size(dstW, dstH), 0, 0, cv::INTER_LINEAR);
										}
										cv::Mat dstRoi = faceSrRgba(cv::Rect(dstX, dstY, dstW, dstH));
										for (int y = 0; y < dstH; ++y)
										{
											const cv::Vec4b* p = patch.ptr<cv::Vec4b>(y);
											cv::Vec4b* d = dstRoi.ptr<cv::Vec4b>(y);
											for (int x = 0; x < dstW; ++x)
											{
												const int a = p[x][3];
												const int invA = 255 - a;
												d[x][0] = (unsigned char)((p[x][0] * a + d[x][0] * invA + 127) / 255);
												d[x][1] = (unsigned char)((p[x][1] * a + d[x][1] * invA + 127) / 255);
												d[x][2] = (unsigned char)((p[x][2] * a + d[x][2] * invA + 127) / 255);
												d[x][3] = 255;
											}
										}
									}
								}

								cv::Mat srcScaledBgr;
								cv::Mat faceSrBgr;
								cv::cvtColor(srcScaledRgba, srcScaledBgr, cv::COLOR_RGBA2BGR);
								cv::cvtColor(faceSrRgba, faceSrBgr, cv::COLOR_RGBA2BGR);

								const std::string inputInfo = "Input " + std::to_string(frameForSR->getWidth()) + "x" + std::to_string(frameForSR->getHeight()) +
									" -> bilinear " + std::to_string(targetW) + "x" + std::to_string(targetH);
								const std::string srInfo = "FaceSR rect(" + std::to_string(srProcessRet.faceRect.x) + "," + std::to_string(srProcessRet.faceRect.y) + "," +
									std::to_string(srProcessRet.faceRect.w) + "," + std::to_string(srProcessRet.faceRect.h) + ")" +
									" target(" + std::to_string(dstW) + "," + std::to_string(dstH) + ")";
								const std::string srSizeInfo = "SR face(" +
									std::to_string(srRgba.cols) + "," + std::to_string(srRgba.rows) + ")";
								const std::string srPostInfo = "PostResize " +
									std::string(m_enableNativePostSRDisplayResize ? "nativeLanczos" : "opencvLinear");
								cv::putText(srcScaledBgr, inputInfo, cv::Point(12, 28),
									cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);
								cv::putText(faceSrBgr, srInfo, cv::Point(12, 28),
									cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 255, 255), 2, cv::LINE_AA);
								cv::putText(faceSrBgr, srSizeInfo, cv::Point(12, 56),
									cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);
								cv::putText(faceSrBgr, srPostInfo, cv::Point(12, 84),
									cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 200, 255), 2, cv::LINE_AA);
								cv::hconcat(srcScaledBgr, faceSrBgr, showMat);
							}
						}
						else
						{
							cv::Mat srcScaledRgba;
							cv::Mat srScaledRgba;
							if (toRgba(frameForSR, srcScaledRgba) && toRgba(srFrame, srScaledRgba) && !srcScaledRgba.empty() && !srScaledRgba.empty())
							{
								int targetW = srScaledRgba.cols;
								int targetH = srScaledRgba.rows;
								if (displaySrMode == 0)
								{
									targetW = (std::max)(1, frameForSR->getWidth() * (std::max)(1, m_srRatio));
									targetH = (std::max)(1, frameForSR->getHeight() * (std::max)(1, m_srRatio));
								}
								else if (displaySrMode == 3)
								{
									int contentW = m_lastFaceScaleW - m_lastPadLeft - m_lastPadRight;
									int contentH = m_lastFaceScaleH - m_lastPadTop - m_lastPadBottom;
									if (contentW <= 0 || contentH <= 0)
									{
										contentW = srScaledRgba.cols / (std::max)(1, m_srRatio);
										contentH = srScaledRgba.rows / (std::max)(1, m_srRatio);
									}
									targetW = (std::max)(1, contentW * (std::max)(1, m_srRatio));
									targetH = (std::max)(1, contentH * (std::max)(1, m_srRatio));
								}
								if (srcScaledRgba.cols != targetW || srcScaledRgba.rows != targetH)
								{
									cv::resize(srcScaledRgba, srcScaledRgba, cv::Size(targetW, targetH), 0, 0, cv::INTER_LINEAR);
								}
								if (srScaledRgba.cols != targetW || srScaledRgba.rows != targetH)
								{
									cv::resize(srScaledRgba, srScaledRgba, cv::Size(targetW, targetH), 0, 0, cv::INTER_LINEAR);
								}
								for (int y = 0; y < srScaledRgba.rows; ++y)
								{
									const cv::Vec4b* p = srScaledRgba.ptr<cv::Vec4b>(y);
									cv::Vec4b* d = srcScaledRgba.ptr<cv::Vec4b>(y);
									for (int x = 0; x < srScaledRgba.cols; ++x)
									{
										const int a = p[x][3];
										const int invA = 255 - a;
										d[x][0] = (unsigned char)((p[x][0] * a + d[x][0] * invA + 127) / 255);
										d[x][1] = (unsigned char)((p[x][1] * a + d[x][1] * invA + 127) / 255);
										d[x][2] = (unsigned char)((p[x][2] * a + d[x][2] * invA + 127) / 255);
										d[x][3] = 255;
									}
								}
								cv::Mat srcScaled;
								cv::Mat mixedBgr;
								cv::cvtColor(srcScaledRgba, mixedBgr, cv::COLOR_RGBA2BGR);
								cv::resize(srcBgr, srcScaled, cv::Size(mixedBgr.cols, mixedBgr.rows), 0, 0, cv::INTER_LINEAR);
								const std::string inputInfo = "Input " + std::to_string(frameForSR->getWidth()) + "x" + std::to_string(frameForSR->getHeight()) +
									" -> bilinear " + std::to_string(mixedBgr.cols) + "x" + std::to_string(mixedBgr.rows);
								const std::string srInfo = std::string(useTextureFSR ? "TextureFSR" : ((displaySrMode == 3) ? "FullScale" : "FullSR")) + " alphaMix " +
									std::to_string(mixedBgr.cols) + "x" + std::to_string(mixedBgr.rows);
								cv::putText(srcScaled, inputInfo, cv::Point(12, 28),
									cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);
								cv::putText(mixedBgr, srInfo, cv::Point(12, 28),
									cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 255, 255), 2, cv::LINE_AA);
								cv::hconcat(srcScaled, mixedBgr, showMat);
							}
						}
					}
				}
				else
				{
					cv::Mat srcBgr;
					if (toBgr(frameForSR, srcBgr) && !srcBgr.empty())
					{
						if (!m_srEnabled)
						{
							const int targetW = (std::max)(1, frameForSR->getWidth() * (std::max)(1, m_srRatio));
							const int targetH = (std::max)(1, frameForSR->getHeight() * (std::max)(1, m_srRatio));
							cv::Mat srcScaledBgr;
							cv::Mat compareBgr;
							cv::resize(srcBgr, srcScaledBgr, cv::Size(targetW, targetH), 0, 0, cv::INTER_LINEAR);
							compareBgr = srcScaledBgr.clone();

							const std::string inputInfo = "Input " + std::to_string(frameForSR->getWidth()) + "x" + std::to_string(frameForSR->getHeight()) +
								" -> bilinear " + std::to_string(targetW) + "x" + std::to_string(targetH);
							const std::string compareInfo = "No SR " + std::to_string(targetW) + "x" + std::to_string(targetH);
							cv::putText(srcScaledBgr, inputInfo, cv::Point(12, 28),
								cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);
							cv::putText(compareBgr, compareInfo, cv::Point(12, 28),
								cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 255, 255), 2, cv::LINE_AA);
							cv::hconcat(srcScaledBgr, compareBgr, showMat);
						}
						else
						{
							const std::string info = "Input " + std::to_string(frameForSR->getWidth()) + "x" + std::to_string(frameForSR->getHeight());
							cv::putText(srcBgr, info, cv::Point(12, 28),
								cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);
							showMat = srcBgr;
						}
					}
				}

				if (!showMat.empty())
				{
					cv::imshow("SR OpenCV Preview", showMat);
					cv::waitKey(1);
				}
				else
				{
					HJFLogw("SR preview empty: in_type:{} w:{} h:{} stride0:{}",
						(int)frame->getConvertType(), frame->getWidth(), frame->getHeight(), frame->getStride(0));
				}
				if (needRecovery)
				{
					m_visualDataUI->recovery(frame);
				}
			}
		}

		ImGui::Begin("SR Params");
		const bool useTextureFSR = isTextureFSRType(m_srWrapperType);
		ImGui::TextUnformatted("Inference Device");
		ImGui::RadioButton("cpu", &m_inferUseGPU, 0);
		ImGui::SameLine(0.0f, 24.0f);
		ImGui::RadioButton("gpu", &m_inferUseGPU, 1);
		ImGui::SameLine(0.0f, 24.0f);
		ImGui::Checkbox("Enable SR", &m_srEnabled);
		ImGui::Separator();
		const char* srModeItems[] = { "Full", "Face", "FaceScale", "FullScale" };
		ImGui::SetNextItemWidth(kNarrowComboWidth);
		if (useTextureFSR)
		{
			ImGui::BeginDisabled();
		}
		ImGui::Combo("SR Mode", &m_srMode, srModeItems, IM_ARRAYSIZE(srModeItems));
		if (useTextureFSR)
		{
			ImGui::EndDisabled();
		}
		ImGui::SameLine(0.0f, 18.0f);
		if (useTextureFSR || !isFaceMode(m_srMode))
		{
			ImGui::BeginDisabled();
		}
		if (ImGui::Checkbox("bFeather", &m_featherAlphaEnable))
		{
			// passed via process(...) every frame
		}
		if (useTextureFSR || !isFaceMode(m_srMode))
		{
			ImGui::EndDisabled();
		}

		ImGui::SameLine(0.0f, 18.0f);
		if (useTextureFSR || !isFaceMode(m_srMode))
		{
			ImGui::BeginDisabled();
		}
		ImGui::Checkbox("Native PostResize", &m_enableNativePostSRDisplayResize);
		if (useTextureFSR || !isFaceMode(m_srMode))
		{
			ImGui::EndDisabled();
		}

		ImGui::SameLine(0.0f, 18.0f);
		if (useTextureFSR || !usesPreScaleMode(m_srMode))
		{
			ImGui::BeginDisabled();
		}
		const char* procPolicyItems[] = {
			"Mipmap",
			"Bilinear",
			"Bicubic",
			"Lanczos3",
			"Lanczos4"
		};
		ImGui::SetNextItemWidth(kNarrowComboWidth);
		ImGui::Combo("ProcPolicy", &m_faceScaleProcPolicy, procPolicyItems, IM_ARRAYSIZE(procPolicyItems));
		if (useTextureFSR || !usesPreScaleMode(m_srMode))
		{
			ImGui::EndDisabled();
		}

		if (!useTextureFSR && usesPreScaleMode(m_srMode))
		{
			int presetCount = 0;
			const SRPresetSize* presets = getPresetListByMode(m_srMode, presetCount);
			int* presetIndex = (m_srMode == 2) ? &m_faceScalePresetIndex : &m_fullScalePresetIndex;
			if (presets && presetCount > 0)
			{
				*presetIndex = (std::max)(0, (std::min)(*presetIndex, presetCount - 1));
				std::vector<const char*> presetItems;
				presetItems.reserve((size_t)presetCount);
				for (int i = 0; i < presetCount; ++i)
				{
					presetItems.push_back(presets[i].label);
				}
				ImGui::SetNextItemWidth(kNarrowComboWidth);
				ImGui::Combo("PreScale", presetIndex, presetItems.data(), presetCount);
			}
		}
		resolvePreScaleForMode(m_srMode, m_faceScalePresetIndex, m_fullScalePresetIndex, m_preScaleWidth, m_preScaleHeight);
		ImGui::SetNextItemWidth(kNarrowSliderWidth);
		if (useTextureFSR)
		{
			ImGui::BeginDisabled();
		}
		ImGui::SliderFloat("SR Alpha", &m_srAlphaRatio, 0.0f, 1.0f, "%.2f");
		if (useTextureFSR)
		{
			ImGui::EndDisabled();
		}

		ImGui::TextUnformatted("SR Type");
		ImGui::RadioButton("RealESRGAN", &m_srWrapperType, 0);
		ImGui::SameLine(0.0f, 24.0f);
		ImGui::RadioButton("FSR", &m_srWrapperType, 1);
		ImGui::SameLine(0.0f, 24.0f);
		ImGui::RadioButton("RealCUGAN", &m_srWrapperType, 2);
		ImGui::SameLine(0.0f, 24.0f);
		ImGui::RadioButton("PlainUSR", &m_srWrapperType, 3);
		const bool showTextureFSRControls = isTextureFSRType(m_srWrapperType);
		if (m_srWrapperType == 0)
		{
			const char* srRealEsrModels[] = { "realesr-general-x4v3", "realesr-animevideov3-x2", "realesrgan-x2plus" };
			ImGui::SetNextItemWidth(kMediumComboWidth);
			ImGui::Combo("Model", &m_srRealEsr, srRealEsrModels, IM_ARRAYSIZE(srRealEsrModels));
			
			// Added Denoise dropdown for specific model
			if (m_srRealEsr == 0) // realesr-general-x4v3
			{
				const char* denoiseOptions[] = { "0.0", "0.2", "0.8", "0.5", "1.0" };
				ImGui::SetNextItemWidth(kNarrowComboWidth);
				ImGui::Combo("Denoise", &m_srRealEsrDenoiseIdx, denoiseOptions, IM_ARRAYSIZE(denoiseOptions));
			}
		}
		else if (m_srWrapperType == 2)
		{
			const char* srRealCUModels[] = { "conservative", "no-denoise" };
			ImGui::SetNextItemWidth(kMediumComboWidth);
			ImGui::Combo("Model", &m_srRealCU, srRealCUModels, IM_ARRAYSIZE(srRealCUModels));
		}
		ImGui::SetNextItemWidth(kNarrowSliderWidth);
		if (useTextureFSR)
		{
			ImGui::BeginDisabled();
		}
		ImGui::SliderInt("OutScale", &m_srRatio, 1, 4);
		if (useTextureFSR)
		{
			ImGui::EndDisabled();
		}

		ImGui::SetNextItemWidth(kNarrowSliderWidth);
		ImGui::SliderInt("Threads", &m_srThreadNums, 1, 4);

		if (showTextureFSRControls)
		{
			ImGui::Separator();
			ImGui::TextUnformatted("Texture FSR");
			ImGui::TextUnformatted("FSR path: input texture -> denoise -> SR -> output texture");
			ImGui::SetNextItemWidth(kNarrowSliderWidth);
			ImGui::SliderFloat("FSR Denoise", &m_fsrDenoiseStrength, 0.0f, 1.0f, "%.2f");
			ImGui::SetNextItemWidth(kNarrowSliderWidth);
			ImGui::SliderFloat("FSR Sharpness", &m_fsrSharpness, 0.0f, 1.0f, "%.2f");
			ImGui::Checkbox("FSR ExtraSharpen", &m_fsrEnableExtraSharpen);
			if (!m_fsrEnableExtraSharpen)
			{
				ImGui::BeginDisabled();
			}
			ImGui::SetNextItemWidth(kNarrowSliderWidth);
			ImGui::SliderFloat("FSR ExtraBoost", &m_fsrExtraSharpenBoost, 0.0f, 1.0f, "%.2f");
			if (!m_fsrEnableExtraSharpen)
			{
				ImGui::EndDisabled();
			}
		}

		ImGui::Text("Current: %s", m_inferUseGPU == 1 ? "GPU" : "CPU");
		ImGui::Text("Actual Input: %d x %d", m_actualInputWidth, m_actualInputHeight);
		if (!useTextureFSR && isFaceMode(m_srMode))
		{
			ImGui::Text("PostResize: %s", m_enableNativePostSRDisplayResize ? "nativeLanczos" : "opencvLinear");
		}
		if (usesPreScaleMode(m_srMode))
		{
			ImGui::Text("PreScale: %d x %d", m_preScaleWidth, m_preScaleHeight);
		}
		ImGui::Text("Last Scale: %d x %d  Target: %d x %d  Pad: %s",
			m_lastFaceScaleW, m_lastFaceScaleH,
			m_lastFaceTargetDisplayW, m_lastFaceTargetDisplayH,
			buildPadInfoText(m_lastFaceScaleW, m_lastFaceScaleH, m_lastPadLeft, m_lastPadRight, m_lastPadTop, m_lastPadBottom).c_str());
		ImGui::Text("Last SR Face: %d x %d", m_lastSRFaceWidth, m_lastSRFaceHeight);
		ImGui::Text("Face Rects: %d", (int)m_faceRectsUI.size());
		if (!m_faceRectsUI.empty())
		{
			const int showCount = (std::min)(10, (int)m_faceRectsUI.size());
			for (int i = 0; i < showCount; i++)
			{
				const HJUnifyWrapperRect& r = m_faceRectsUI[i];
				if (i == 0)
				{
					ImGui::Text("Face: {%d,%d,%d,%d}->scale(%d,%d) sr(%d,%d) target(%d,%d) post(%s) pad(%s)",
						r.x, r.y, r.w, r.h, m_lastFaceScaleW, m_lastFaceScaleH,
						m_lastSRFaceWidth, m_lastSRFaceHeight,
						m_lastFaceTargetDisplayW, m_lastFaceTargetDisplayH,
						m_enableNativePostSRDisplayResize ? "nativeLanczos" : "opencvLinear",
						buildPadInfoText(m_lastFaceScaleW, m_lastFaceScaleH, m_lastPadLeft, m_lastPadRight, m_lastPadTop, m_lastPadBottom).c_str());
				}
				else
				{
					ImGui::Text("Face[%d] x:%d y:%d w:%d h:%d", i, r.x, r.y, r.w, r.h);
				}
			}
			if ((int)m_faceRectsUI.size() > showCount)
			{
				ImGui::Text("... %d more", (int)m_faceRectsUI.size() - showCount);
			}
		}
		ImGui::Text("SR Elapse: %lld ms", (long long)m_srElapseMs);
		if (ImGui::Button("Apply"))
		{
			priReleasePausedFrame();
			m_isPaused = false;
			i_err = priResetSR();
			if (i_err < 0)
			{
				HJFLoge("SR reset failed ret:{}", i_err);
			}
			if (m_visualDataUI)
			{
				i_err = m_visualDataUI->applySource(HJConvertDataType_RGB);
				if (i_err < 0)
				{
					HJFLoge("SR visual apply failed ret:{}", i_err);
				}
			}
		}
		ImGui::SameLine();
		const char* pauseLabel = m_isPaused ? "Resume" : "Pause";
		if (ImGui::Button(pauseLabel))
		{
			if (m_isPaused)
			{
				priReleasePausedFrame();
				m_isPaused = false;
			}
			else
			{
				if (m_visualDataUI && !m_cachedPauseFrame)
				{
					m_cachedPauseFrame = m_visualDataUI->acquire();
				}
				m_isPaused = true;
			}
		}
		ImGui::End();
	} while (false);
	return i_err;
}
int HJUIItemSR::run()
{
	int i_err = 0;
	do
	{
		i_err = priRun();
		if (i_err < 0)
		{
			break;
		}
	} while (false);
	return i_err;
}

void HJUIItemSR::priDone()
{
	priReleasePausedFrame();
	if (m_faceSR)
	{
		m_faceSR->done();
	}
	if (m_fsrInputTexture != 0)
	{
		HJOGCommon::textureDestroy(m_fsrInputTexture);
		m_fsrInputTexture = 0;
	}
	m_faceSR = nullptr;
	m_isPaused = false;
	m_faceRectsUI.clear();
	m_visualDataUI = nullptr;
}

void HJUIItemSR::priReleasePausedFrame()
{
	if (m_visualDataUI && m_cachedPauseFrame)
	{
		m_visualDataUI->recovery(m_cachedPauseFrame);
	}
	m_cachedPauseFrame = nullptr;
}

int HJUIItemSR::priResetSR()
{
	int i_err = HJ_OK;
	do
	{
		resolvePreScaleForMode(m_srMode, m_faceScalePresetIndex, m_fullScalePresetIndex, m_preScaleWidth, m_preScaleHeight);
		m_faceSR = std::make_shared<HJFaceSRWrapper>();
		if (!m_faceSR)
		{
			i_err = HJErrNotInited;
			break;
		}

		::HJVideoSRWrapperOption option;
		option.ncnnUseGPU = (m_inferUseGPU == 1);
		option.ncnnThreadNums = m_srThreadNums;
		option.ncnnScale = m_srRatio;
		option.textureFsrScale = m_srRatio;
		option.textureFsrDenoiseStrength = m_fsrDenoiseStrength;
		option.textureFsrSharpness = m_fsrSharpness;
		option.textureFsrEnableExtraSharpen = m_fsrEnableExtraSharpen;
		option.textureFsrExtraSharpenBoost = m_fsrExtraSharpenBoost;
		
		// NCNN denoise is only for RealESRGAN. Texture FSR uses its dedicated fields below.
		float denoiseValues[] = { 0.0f, 0.2f, 0.8f, 0.5f, 1.0f };
		if (m_srRealEsrDenoiseIdx >= 0 && m_srRealEsrDenoiseIdx < 5)
		{
			option.ncnnRealESRGANDenose = denoiseValues[m_srRealEsrDenoiseIdx];
		}
		else
		{
			option.ncnnRealESRGANDenose = 0.5f;
		}

		switch (m_srRealEsr)
		{
		case 1:
			option.ncnnRealESRGANType = "realesr-animevideov3-x2";
			break;
		case 2:
			option.ncnnRealESRGANType = "realesrgan-x2plus";
			break;
		case 0:
			option.ncnnRealESRGANType = "realesr-general-x4v3";
			break;
		default:
			break;
		}
		option.ncnnRealCUGANType = (m_srRealCU == 1) ? "no-denoise" : "conservative";

		::HJVideoSRWrapperType srType = HJVideoSRWrapperType_NCNNPLAINUSR;
		switch (m_srWrapperType)
		{
		case 0:
			srType = HJVideoSRWrapperType_NCNNREALESRGAN;
			break;
		case 1:
			srType = HJVideoSRWrapperType_TEXTUREFSR;
			break;
		case 2:
			srType = HJVideoSRWrapperType_NCNNREALCUGAN;
			break;
		case 3:
			srType = HJVideoSRWrapperType_NCNNPLAINUSR;
			break;
		}
		if (srType == HJVideoSRWrapperType_TEXTUREFSR)
		{
			option.textureFsrScale = m_srRatio;
		}
		::HJFaceDetectWrapperOption detectOption;
		detectOption.ncnnScrfdUseGPU = option.ncnnUseGPU;
		detectOption.ncnnScrfdThreadNums = (std::max)(1, (std::min)(2, option.ncnnThreadNums));

		i_err = m_faceSR->init(
			HJUtilitys::concatenatePath(HJUtilitys::exeDir(), "resource/model"),
			HJFaceDetectWrapperType_NCNNSCRFD,
			srType,
			detectOption,
			option);
		if (i_err < 0)
		{
			m_faceSR = nullptr;
			break;
		}
		HJFLogi("SR reset ok type:{} useGPU:{} realesr:{} realcu:{} ratio:{} threads:{} denoise:{}({}) fsrDenoise:{:.2f} fsrSharpness:{:.2f} fsrExtra:{} fsrBoost:{:.2f} procPolicy:{} preScale:{}x{} usePreScale:{} srAlpha:{:.2f} postResize:{}",
			(int)srType, option.ncnnUseGPU, option.ncnnRealESRGANType, option.ncnnRealCUGANType, option.ncnnScale, option.ncnnThreadNums,
			option.ncnnRealESRGANDenose, (srType == HJVideoSRWrapperType_TEXTUREFSR) ? "fsr" : "ncnn",
			option.textureFsrDenoiseStrength, option.textureFsrSharpness, option.textureFsrEnableExtraSharpen, option.textureFsrExtraSharpenBoost,
			usesPreScaleMode(m_srMode) ? (int)getProcPolicyByIndex(m_faceScaleProcPolicy) : -1,
			m_preScaleWidth, m_preScaleHeight,
			usesPreScaleMode(m_srMode),
			m_srAlphaRatio,
			m_enableNativePostSRDisplayResize);
	} while (false);
	return i_err;
}

NS_HJ_END



