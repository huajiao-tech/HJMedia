#pragma once

#include "HJCommonInterface.h"
#include "HJExport.h"
#include <memory>
#include <mutex>
#include <deque>

namespace HJ
{
    class HJRteGraphProcConfigSetup;
    class HJTransferMediaData;
    class HJBaseParam;
}
class HJRenderDataCatchInfo;

using HJRenderGraphOutCb = std::function<void(std::shared_ptr<HJUnifyWrapperData> o_output)>;
using HJRenderWrapperListener = std::function<void(int i_type, int i_ret, const std::string& i_msgInfo)>;

class HJExport HJRenderGraphWrapper
{ 
public:
	HJRenderGraphWrapper();
	~HJRenderGraphWrapper();

	int init(HJRenderWrapperListener i_listener, HJRenderGraphOutCb i_cb, int i_fps, bool i_bManulDrive, const std::string &i_graphInfo, HJUnifyWrapperDataType i_outDataType);

	int nodeEnable(const std::string& i_classType, const std::string& i_insName, bool i_bEnable, const std::string& i_info);
    int nodeCreate(const std::string& classStyle, const std::string& insName, const std::string& role, bool enable, bool isMainSource, const std::string& i_resourceUrl, const std::string& i_dependsOn);
    int nodeConnect(const std::string& i_srcClassStyle, const std::string& i_srcInsName, const std::string& i_dstClassStyle, const std::string& i_dstInsName,
        const std::string& i_shaderStyle, const std::string& i_linkInfo);
    int nodeDelete(const std::string& i_classStyle, const std::string& i_insName, bool i_relink);

    void setFaceInfo(const std::string& i_sourceInsName, const std::string& i_faceInfo);

	int render(std::shared_ptr<HJUnifyWrapperData> i_input);

private:
    int priSetParamEGLContext(const std::shared_ptr<HJ::HJBaseParam>& i_param);
    int priSetParamCb(const std::shared_ptr<HJ::HJBaseParam>& i_param);
    int priSetParamInAndOut(const std::shared_ptr<HJ::HJBaseParam>& i_param);

    void priCatchQueueIn(int64_t i_time, int64_t i_totalElapseCount);
    void priCatchQueueOut(int64_t &o_time, int64_t &o_totalElapseCount);
    
    std::shared_ptr<HJ::HJRteGraphProcConfigSetup> m_graphConfig = nullptr;
    HJRenderWrapperListener m_listener = nullptr;
    std::mutex m_mutex;
    std::deque<std::shared_ptr<HJ::HJTransferMediaData>> m_inputQueue;
    HJRenderGraphOutCb m_outCb = nullptr;

    std::deque<std::shared_ptr<HJRenderDataCatchInfo>> m_inputCatchQueue;
    HJUnifyWrapperDataType m_outDataType = HJUnifyWrapperDataType_Unknown;

    int64_t m_statDriveIdx = 0;
    int64_t m_statInDataIdx = 0;
    int64_t m_statOutDataIdx = 0;
    int64_t m_statSourceEmptyIdx = 0;
    int64_t m_dropSourceIdx = 0;

    int64_t m_lastSystemTime = 0;
    int64_t m_totalInputCnt = 0;
    int64_t m_totalOutputCnt = 0;
};
