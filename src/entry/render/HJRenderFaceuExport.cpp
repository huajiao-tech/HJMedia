#include "HJRenderFaceuExport.h"
#include "HJPrerequisites.h"
#include "HJEntryContext.h"
#include "HJError.h"
#include "HJRenderFaceuMgr.h"
#include "HJUtilitys.h"
#include "HJFacePointMgr.h"
#include "HJFLog.h"
#include "HJComEvent.h"
#include "HJFacePointsReal.h"

NS_HJ_USING

static void* priFaceuInit(HJFaceuListener i_listener, bool i_bUseEnv)
{
	HJRenderFaceuBaseMgr* faceMgr = nullptr;
	if (i_bUseEnv)
	{
		faceMgr = new HJRenderFaceuEnvMgr;
	}
	else
	{
		faceMgr = new HJRenderFaceuBaseMgr;
	}
	int i_err = faceMgr->init(i_listener);
	if (i_err < 0)
	{
		HJFLoge("faceu init failed");
		delete faceMgr;
		faceMgr = nullptr;
	}
	return faceMgr;
}

#if defined(ANDROID_LIB)
void* faceuInitTrans(HJFaceuListener i_listener)
{
	return priFaceuInit(i_listener, false);
}
#endif

void* faceuInit(HJFaceuCallback i_callback, bool i_bUseEnv)
{
	HJFLogi("faceu init enter i_bUseEnv:{}", i_bUseEnv);
	HJFaceuListener listener = [i_callback](const std::string& i_uniqueKey, const HJNotification::Ptr ntf)
		{
			int type = HJ_FACEU_NOTIFY_UNKNOWN;
			std::string msg = ntf->getMsg();
			switch (ntf->getID())
			{
			case HJVIDEORENDERGRAPH_EVENT_FACEU_COMPLETE:
				type = HJ_FACEU_NOTIFY_COMPLETE;
				break;
			}
			if (i_callback)
			{
				i_callback(i_uniqueKey.c_str(), type);
			}
			return HJ_OK;
		};
		HJFLogi("faceu init end");
		return priFaceuInit(listener, i_bUseEnv);
}

int faceuAdd(void* i_handle, const char* i_uniqueKey, const char* i_faceuUrl, bool i_bDebugPoints)
{
	HJFLogi("faceu add enter i_uniqueKey:{} i_faceuUrl:{} i_bDebugPoints:{}", i_uniqueKey, i_faceuUrl, i_bDebugPoints);
	int i_err = HJ_OK;
	do
	{
		HJRenderFaceuBaseMgr* faceu = (HJRenderFaceuBaseMgr*)i_handle;
		if (!faceu)
		{
			HJFLoge("faceu hanle is empty");
			i_err = HJErrInvalidParams;
			break;
		}
		std::string uniqueKey = std::string(i_uniqueKey);

		i_err = faceu->add(i_uniqueKey, i_faceuUrl, i_bDebugPoints);
		if (i_err < 0)
		{
			HJFLoge("faceu add failed");
			break;
		}
	} while (false);
	HJFLogi("faceu add end:{}", i_err);
	return i_err;
}
int faceuRemove(void* i_handle, const char* i_uniqueKey)
{
	HJFLogi("faceu remove enter i_uniqueKey:{} ", i_uniqueKey);
	int i_err = HJ_OK;
	do
	{
		HJRenderFaceuBaseMgr* faceu = (HJRenderFaceuBaseMgr*)i_handle;
		if (!faceu)
		{
			HJFLoge("faceu hanle is empty");
			i_err = HJErrInvalidParams;
			break;
		}
		i_err = faceu->remove(i_uniqueKey);
		if (i_err < 0)
		{
			HJFLoge("faceu add failed");
			break;
		}
	} while (false);
	HJFLogi("faceu remove end:{}", i_err);
	return i_err;
}
int faceuRemoveAll(void* i_handle)
{
	HJFLogi("faceu faceuRemoveAll enter");
	int i_err = HJ_OK;
	do
	{
		HJRenderFaceuBaseMgr* faceu = (HJRenderFaceuBaseMgr*)i_handle;
		if (!faceu)
		{
			HJFLoge("faceu hanle is empty");
			i_err = HJErrInvalidParams;
			break;
		}
		i_err = faceu->removeAll();
		if (i_err < 0)
		{
			HJFLoge("faceu add failed");
			break;
		}
	} while (false);
	HJFLogi("faceu faceuRemoveAll end:{}", i_err);
	return i_err;
}
int faceuRender(void* i_handle, HJFacePointInfo* i_faceInfo, unsigned char*& o_pRGBA)
{
	int i_err = HJ_OK;
	do
	{
		HJRenderFaceuBaseMgr* faceu = (HJRenderFaceuBaseMgr*)i_handle;
		if (!faceu)
		{
			HJFLoge("faceu hanle is empty");
			i_err = HJErrInvalidParams;
			break;
		}
		if (!i_faceInfo)
		{
            HJFLoge("faceu faceInfo is empty");
            i_err = HJErrInvalidParams;
			break;
		}

		HJMoreFacePointsReal::Ptr morePoints = HJMoreFacePointsReal::Create<HJMoreFacePointsReal>(i_faceInfo->width, i_faceInfo->height, (i_faceInfo->faceCount > 0));
		for (int i = 0; i < i_faceInfo->faceCount; i++)
		{
            HJSingleFacePointsReal::Ptr point = HJSingleFacePointsReal::Create<HJSingleFacePointsReal>();
			HJSingleFaceInfo* singleFaceInfo = &i_faceInfo->faces[i];
			point->setFaceRect(HJRectf{ singleFaceInfo->rect.x, singleFaceInfo->rect.y, singleFaceInfo->rect.w, singleFaceInfo->rect.h});

			for (int j = 0; j < 5; j++)
			{
				point->add(HJPointf{ singleFaceInfo->points[j].x, singleFaceInfo->points[j].y });
			}
			morePoints->add(point);
		}
		morePoints->setIsDebugPoints(faceu->getDebugPoints());

		//HJFacePointsReal::Ptr point = HJFacePointsReal::Create<HJFacePointsReal>(i_width, i_height, i_bContainFace);
		//for (int i = 0; i < i_nPointsCnt; i++)
		//{
		//	point->add(HJPointf{ points[i].x, points[i].y });
		//}
		//point->setIsDebugPoints(faceu->getDebugPoints());

		i_err = faceu->render(i_faceInfo->width, i_faceInfo->height, morePoints, o_pRGBA);
		if (i_err < 0)
        {
            HJFLoge("faceu render failed");
            break;
        }
	} while (false);
	return i_err;
}
void faceuDone(void* i_handle)
{
	HJFLogi("faceu done enter");
	do 
	{
		HJRenderFaceuBaseMgr* faceu = (HJRenderFaceuBaseMgr*)i_handle;
		if (!faceu)
		{
			HJFLoge("faceu hanle is empty");
			break;
		}
		faceu->done();
        delete faceu;
        faceu = nullptr;
	} while (false);
	HJFLogi("faceu done end");
}
