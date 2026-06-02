#pragma once

#include "HJComUtils.h"
#include "HJConvertUtils.h"
#include "HJReflCpp.h"
#include <chrono>
#include "HJAsyncCache.h"

namespace cv
{
    class Mat;
    class VideoCapture;
};

NS_HJ_BEGIN

#define HJ_VISUAL_DATA_TYPE_ENUM(M, EnumType) \
    M(EnumType, VisualDataType_None) \
    M(EnumType, VisualDataType_Video) \
    M(EnumType, VisualDataType_ImgSeq) \
    M(EnumType, VisualDataType_Image) \
    M(EnumType, VisualDataType_Camera)

enum class VisualDataType
{
    HJ_VISUAL_DATA_TYPE_ENUM(HJ_ENUM_DECLARE_ITEM, VisualDataType)
};

HJ_ENUM_STRING(VisualDataType, HJ_VISUAL_DATA_TYPE_ENUM)


class HJTimerThreadPool; 
class HJTransferMediaData;
class VisualDataBase : public HJBaseSharedObject
{
public:
    HJ_DEFINE_CREATE(VisualDataBase);
    VisualDataBase() = default;
    virtual ~VisualDataBase();

    static std::shared_ptr<VisualDataBase> createVisualData(VisualDataType i_type);

    virtual int init(HJBaseParam::Ptr i_param);

    virtual int restart()
    {
        return 0;
    }
    virtual std::shared_ptr<cv::Mat> read()
    {
        return nullptr;
    }

    int start();

    std::shared_ptr<HJTransferMediaData> acquire();
    void recovery(std::shared_ptr<HJTransferMediaData> i_data);


    VisualDataType getDataType() const
    {
        return m_dataType;
    }
    void setDataType(VisualDataType type)
    {
        m_dataType = type;
    }

    static std::string s_paramUrl;
	static std::string s_paramFps;
	static std::string s_paramLoop;
	static std::string s_paramOutputType;

    static std::string s_paramWidth;
	static std::string s_paramHeight;

protected:

    void done();

	HJConvertDataType m_outputType = HJConvertDataType_YUVNV12;
    std::string m_url = "";
    int m_fps = 30;
    bool m_bLoop = true;



private:
    std::shared_ptr<HJTransferMediaData> priCvtFrame(const std::shared_ptr<cv::Mat>& i_frame);
    int priRun();
    int priTryRestart();

	VisualDataType m_dataType = VisualDataType::VisualDataType_None;
    std::shared_ptr<HJTimerThreadPool> m_timerPool = nullptr;
    HJAsyncCache<std::shared_ptr<HJTransferMediaData>> m_asyncCache;
    bool m_bRestart = true;
};

NS_HJ_END
