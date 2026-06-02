#include "HJRenderGraphExport.h"
#include "HJPrerequisites.h"
#include "HJEntryContext.h"
#include "HJError.h"
#include "HJRenderFaceuMgr.h"
#include "HJUtilitys.h"
#include "HJFacePointMgr.h"
#include "HJFLog.h"
#include "HJComEvent.h"
#include "HJRteGraphProc.h"
#include "HJRenderCoreSingleton.h"
#include "HJTransferMediaData.h"

NS_HJ_USING

class HJRenderDataCatchInfo
{
public:
	HJ_DEFINE_CREATE(HJRenderDataCatchInfo);
	HJRenderDataCatchInfo() = default;
	virtual ~HJRenderDataCatchInfo() = default;

	HJRenderDataCatchInfo(int64_t i_time, int64_t i_systemTime, int64_t i_totalElapseCnt)
        : m_timeStamp(i_time), 
		m_systemTime(i_systemTime),
		m_totalElapseCnt(i_totalElapseCnt)
	{

	}
	int64_t m_timeStamp = 0;
	int64_t m_systemTime = 0;
	int64_t m_totalElapseCnt = 0;
};


HJRenderGraphWrapper::HJRenderGraphWrapper()
{

}
HJRenderGraphWrapper::~HJRenderGraphWrapper()
{
	if (m_graphConfig)
	{
		HJFLogi("HJRenderGraphWrapper::~HJRenderGraphWrapper done proc");
		m_graphConfig->done();
		m_graphConfig = nullptr;
		HJFLogi("HJRenderGraphWrapper::~HJRenderGraphWrapper done end");
	}
}   

int HJRenderGraphWrapper::priSetParamEGLContext(const std::shared_ptr<HJ::HJBaseParam>& i_param)
{
	int i_err = HJ_OK;
	do
	{
		HJRenderMakeCurrent makeCurrentCb = [this](bool i_bMake)
			{
				if (i_bMake)
				{
					HJRenderCoreSingleton::Instance().makeContext();
				}
				else
				{
					HJRenderCoreSingleton::Instance().makeCurrentNULL();
				}
				return HJ_OK;
			};
		HJ_CatchMapSetVal(i_param, HJRenderMakeCurrent, makeCurrentCb);
	} while (false);
    return i_err;
}
int HJRenderGraphWrapper::priSetParamCb(const std::shared_ptr<HJ::HJBaseParam>& i_param)
{
	int i_err = HJ_OK;
	do
	{
		HJListener renderListener = [&](const HJNotification::Ptr ntf) -> int
			{
				if (!ntf)
				{
					return HJ_OK;
				}
				HJFLogi("graph video capture notify id:{}, msg:{}", HJVideoRenderGraphEventToString((HJVideoRenderGraphEvent)ntf->getID()), ntf->getMsg());
				std::string msg = ntf->getMsg();
				int type = HJEntryContext::renderNotifyIdMap(ntf->getID());
				if (msg.empty())
				{
					msg = HJVideoRenderGraphEventToString((HJVideoRenderGraphEvent)ntf->getID());
				}
				if (m_listener && (HJVIDEORENDERGRAPH_EVENT_NONE != type))
				{
					m_listener(type, 0, msg);
				}
				return HJ_OK;
			};
		(*i_param)["renderListener"] = (HJListener)renderListener;
	} while (false);
	return i_err;
}
void HJRenderGraphWrapper::priCatchQueueIn(int64_t i_time, int64_t i_totalElapseCount)
{
	HJRenderDataCatchInfo::Ptr info = HJRenderDataCatchInfo::Create<HJRenderDataCatchInfo>(i_time, HJCurrentSteadyMS(), i_totalElapseCount);
	m_inputCatchQueue.push_back(std::move(info));
	
}
void HJRenderGraphWrapper::priCatchQueueOut(int64_t& o_timeStamp, int64_t& o_totalElapseCount)
{
	if (!m_inputCatchQueue.empty())
	{
		//if pbo find change width or height, pbo reset not output callback, so m_inputCatchQueue can more than 2
		while (m_inputCatchQueue.size() > 2)
		{
			m_inputCatchQueue.pop_front();
		}
		HJRenderDataCatchInfo::Ptr info = m_inputCatchQueue.front();
		//HJFLogi("timeQueue pboout timestamp:{} systemTime:{} size:{} ", info->m_timeStamp, info->m_systemTime, (int)m_inputCatchQueue.size());
		m_inputCatchQueue.pop_front();

		////not use this, HJRenderGraphWrapper reset every time, if not reset, the last pbo frame is out of data, so drop;
		//int64_t curTime = HJCurrentSteadyMS();
		//int64_t elapse = abs(curTime - info->m_systemTime);
		//if (elapse > 200)
		//{
		//	HJFLogw("pboout time diff:{}", elapse);
		//	return HJ_WOULD_BLOCK;
		//}
		o_timeStamp = info->m_timeStamp;
		o_totalElapseCount = info->m_totalElapseCnt;
	}
}
int HJRenderGraphWrapper::priSetParamInAndOut(const std::shared_ptr<HJ::HJBaseParam>& i_param)
{
	int i_err = HJ_OK;
	do
	{
		HJTransferMediaDataGetCb getMDataCb = [this](std::shared_ptr<HJTransferMediaData>& o_data)
			{						
#if 0
				if ((m_statInDataIdx % 5) == 0)
				{
					HJFLogi("sleep");
					HJ_SLEEP(50);
				}
#endif

				HJ_LOCK(m_mutex);
				if (!m_inputQueue.empty())
				{
					m_statInDataIdx++;
					o_data = *m_inputQueue.begin();
					m_inputQueue.pop_front();

					//not use replicate frame, because the TotalElapseCount is used, if the replicate frame is used, the TotalElapseCount is not correct, not call this
					//if (m_inputQueue.size() > 1)
					//{
					//	m_inputQueue.pop_front();
					//}

					int64_t nTotalElapseCount = 0;
					HJ_CatchMapPlainGetVal(o_data, int64_t, "TotalElapseCount", nTotalElapseCount);
					priCatchQueueIn(o_data->getTimeStamp(), nTotalElapseCount);

					HJFPERLog5i("input nTotalElapseCount:{} width:{} height:{} timestamp:{} SourceEmptyIdx:{}", nTotalElapseCount, o_data->getWidth(), o_data->getHeight(), o_data->getTimeStamp(), m_statSourceEmptyIdx);
				}
				else
				{
					//if return nullptr, then graph is not draw, modify HJRteGraphProc
					o_data = nullptr;
					m_statSourceEmptyIdx++;
					//HJFLogi("HJRenderGraphWrapper::getMDataCb null");
				}
			};
		HJ_CatchMapSetVal(i_param, HJTransferMediaDataGetCb, getMDataCb);

		HJMediaDataReaderCb pboReadCb = [this](HJSPBuffer::Ptr i_buffer, int width, int height)
			{
				int64_t otimestamp = 0;
				int64_t totalElapseCnt = 0;
				
				//HJFLogi("timeQueue pboout outIdx:{}", m_statOutDataIdx);
				priCatchQueueOut(otimestamp, totalElapseCnt);
				if (m_outCb)
				{
					std::shared_ptr<HJUnifyWrapperData> data = std::make_shared<HJUnifyWrapperData>();
					HJTransferMediaData::Ptr transferData = nullptr;
					if (m_outDataType == HJUnifyWrapperDataType_NV12)
					{
						unsigned char* i_data[4] = { i_buffer->getBuf(), nullptr, nullptr, nullptr };
						int i_stride[4] = { width * 4, 0, 0, 0 };
						transferData = HJTransferMediaData::create(HJConvertDataType_RGBA, i_data, i_stride, width, height, otimestamp, HJConvertDataType_YUVNV12);

						data->dataType = HJUnifyWrapperDataType_NV12;
						for (int i = 0; i < 4; i++)
						{
							data->data[i] = transferData->getData(i);
							data->nPitch[i] = transferData->getStride(i);
						}
					}
					else if (m_outDataType == HJUnifyWrapperDataType_RGBA)
					{
						data->dataType = HJUnifyWrapperDataType_RGBA;
						data->nPitch[0] = width * 4;
						data->data[0] = i_buffer->getBuf();
					}
					else
					{
						HJFLoge("not support out type {}", (int)m_outDataType);
						return HJ_OK;
					}
					data->width = width;
					data->height = height;
					data->timeStamp = otimestamp;
					data->nElapseCount = (int)(totalElapseCnt - m_totalOutputCnt);
					m_totalOutputCnt = totalElapseCnt;
					m_outCb(data);
					m_statOutDataIdx++;
					if (data->nElapseCount <= 0)
					{
						HJFLoge("the error, obs encode use this, the timestamp add elapsecount, so two timestamp is replicate, error");
					}
					HJFPERLog5i("output width:{} height{} offsetElapseCnt:{} timestamp:{}", width, height, data->nElapseCount, otimestamp);
				}
				return 0;
			};
		HJ_CatchMapSetVal(i_param, HJMediaDataReaderCb, pboReadCb);
	} while (false);
	return i_err;
}
int HJRenderGraphWrapper::init(HJRenderWrapperListener i_listener, HJRenderGraphOutCb i_cb, int i_fps, bool i_bManulDrive, const std::string& i_graphInfo, HJUnifyWrapperDataType i_outDataType)
{
	int i_err = HJ_OK;
	do
	{
		m_outCb = i_cb;
		m_outDataType = i_outDataType;
		m_listener = i_listener;
		if (!m_graphConfig)
		{
			m_graphConfig = HJRteGraphProcConfigSetup::Create();

			HJBaseParam::Ptr param = HJBaseParam::Create();

			HJ_CatchMapPlainSetVal(param, int, HJBaseParam::s_paramFps, i_fps);
			HJ_CatchMapPlainSetVal(param, bool, HJBaseParam::s_paramIsManulDrive, i_bManulDrive);
			HJ_CatchMapPlainSetVal(param, std::string, "graphConfigInfo", i_graphInfo);
			HJ_CatchMapPlainSetVal(param, bool, "bMainSourceRenderForce", true);

			priSetParamEGLContext(param);
			priSetParamCb(param);
			priSetParamInAndOut(param);			

			i_err = m_graphConfig->init(param);
			if (i_err < 0)
			{
				HJFLoge("HJRenderGraphWrapper::init error:{}", i_err);
				break;
			}
		}
	} while (false);
	return i_err;
}
//void done();

int HJRenderGraphWrapper::nodeEnable(const std::string& i_classType, const std::string& i_insName, bool i_bEnable, const std::string& i_info)
{
	if (m_graphConfig)
	{
		m_graphConfig->nodeEnable(i_classType, i_insName, i_bEnable, i_info);
	}
    return 0;
}
int HJRenderGraphWrapper::nodeCreate(const std::string& classStyle, const std::string& insName, const std::string& role, bool enable, bool isMainSource, const std::string& i_resourceUrl, const std::string &i_dependsOn)
{
	if (m_graphConfig)
	{
		m_graphConfig->nodeCreate(classStyle, insName, role, enable, isMainSource, i_resourceUrl, i_dependsOn);
	}
	return 0;
}
int HJRenderGraphWrapper::nodeConnect(const std::string& i_srcClassStyle, const std::string& i_srcInsName, const std::string& i_dstClassStyle, const std::string& i_dstInsName,
	const std::string& i_shaderStyle, const std::string& i_linkInfo)
{
	if (m_graphConfig)
	{
		m_graphConfig->nodeConnect(i_srcClassStyle, i_srcInsName, i_dstClassStyle, i_dstInsName, i_shaderStyle, i_linkInfo);
	}
    return 0;
}
int HJRenderGraphWrapper::nodeDelete(const std::string& i_classStyle, const std::string& i_insName, bool i_relink)
{
    if (m_graphConfig)
    {
        m_graphConfig->nodeDelete(i_classStyle, i_insName, i_relink);
    }
    return 0;
}

void HJRenderGraphWrapper::setFaceInfo(const std::string& i_sourceInsName, const std::string& i_faceInfo)
{
	if (m_graphConfig)
	{
		m_graphConfig->setFaceInfo(i_sourceInsName, i_faceInfo);
	}
}

int HJRenderGraphWrapper::render(std::shared_ptr<HJUnifyWrapperData> i_input)
{
	int i_err = HJ_OK;
	do
	{
		int nQueueSize = 0;
		HJ::HJTransferMediaData::Ptr data = nullptr;
		HJConvertDataType type = HJConvertDataType_None;
		switch (i_input->dataType)
		{
		case HJUnifyWrapperDataType_NV12:
			type = HJConvertDataType_YUVNV12;
			data = HJ::HJTransferMediaDataYUVNV12::Create<HJ::HJTransferMediaDataYUVNV12>(i_input->data, i_input->nPitch, i_input->width, i_input->height, i_input->timeStamp);
			break;
		default:
			i_err = HJErrInvalidParams;
			return i_err;
		}
		m_totalInputCnt += i_input->nElapseCount;

		if (data)
		{			
			HJ_CatchMapPlainSetVal(data, int64_t, "TotalElapseCount", m_totalInputCnt);
			HJ_LOCK(m_mutex);
			//if (i_input->bUltraLowLatency)
			//{
			//	m_dropSourceIdx += (int64_t)m_inputQueue.size();
			//	m_inputQueue.clear();
			//}
			//else
			//{
				//if producer is to fast, the render runs at a low speed, alleviating frame loss to a certain extent
				while (m_inputQueue.size() > i_input->nLatencyCnt)
				{
					m_inputQueue.pop_front();
					m_dropSourceIdx++;
				}
			//}
			m_inputQueue.push_back(std::move(data));
			nQueueSize = (int)m_inputQueue.size();
		}

		if (m_graphConfig)
		{
			int64_t systemTime = HJCurrentSteadyMS();
			//HJFLogi("avSync lastTimediff:{} totalCnt:{} nQueueSize:{}", (systemTime - m_lastSystemTime), m_totalInputCnt, nQueueSize);
			m_statDriveIdx++;

			HJFPERLog5i("manualDrive nLatencyCnt:{} nQueueSize:{} obsTotalCnt:{} diriveIdx:{} drive-in:{} drive-out:{} lastTimediff:{} obsDrop:{} EmptyCnt:{} InDropeCnt:{} Emptyratio:{:.3f} InDropRatio:{:.3f} outRatio:{:.3f}", i_input->nLatencyCnt, nQueueSize, m_totalInputCnt, m_statDriveIdx, (m_statDriveIdx - m_statInDataIdx), (m_statDriveIdx - m_statOutDataIdx),(systemTime - m_lastSystemTime), (m_totalInputCnt - m_statDriveIdx), m_statSourceEmptyIdx, m_dropSourceIdx, (double)m_statSourceEmptyIdx / m_statDriveIdx, (double)m_dropSourceIdx / m_statDriveIdx, (double)m_statOutDataIdx / m_statDriveIdx);
			i_err = m_graphConfig->manualDrive();
			if (i_err < 0)
			{
				break;
			}
			m_lastSystemTime = systemTime;
		}
	} while (false);
	return i_err;
}


