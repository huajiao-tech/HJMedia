#include "HJFacePointsReal.h"

NS_HJ_BEGIN

std::shared_ptr<HJMoreFacePointsReal> HJMoreFacePointsReal::cvtFrom(std::shared_ptr<HJFacePointsReal> i_pointreal)
{
	std::shared_ptr<HJMoreFacePointsReal> morePoints = nullptr;
	do 
	{
		if (!i_pointreal)
		{
			break;
		}

		morePoints = HJMoreFacePointsReal::Create<HJMoreFacePointsReal>(i_pointreal->width(), i_pointreal->height(), i_pointreal->isContainFace(), i_pointreal->getTimestamp());
		morePoints->setElapsedTime(i_pointreal->getElapsedTime());
		morePoints->setIsDebugPoints(i_pointreal->isDebugPoints());
		morePoints->setSystemTime(i_pointreal->getSystemTime());

		if (i_pointreal->isContainFace())
		{
			HJSingleFacePointsReal::Ptr singlePoint = HJSingleFacePointsReal::Create();
			const std::vector<HJPointf>& ptVecor = i_pointreal->getFilterPt();
			//only first five points
			for (int i = 0; i < 5; i++)
			{
				singlePoint->add(ptVecor[i]);
			}
			//face rect
			singlePoint->setFaceRect(ptVecor[5].x, ptVecor[5].y, ptVecor[8].x - ptVecor[5].x, ptVecor[8].y - ptVecor[5].y);
			morePoints->add(singlePoint);
		}

	} while (false);
	return morePoints;
}

std::shared_ptr<HJMoreFacePointsReal> HJMoreFacePointsReal::getFakePoints(bool i_bDebug)
{
	HJMoreFacePointsReal::Ptr morePtr = HJMoreFacePointsReal::Create<HJMoreFacePointsReal>(720, 1280, true);

	HJSingleFacePointsReal::Ptr singlePt = HJSingleFacePointsReal::Create();
	singlePt->setFaceRect(HJRectf{ 262.f, 307.f, (483.f - 262.f), (563.f - 307.f) });
	singlePt->add(HJPointf{ 343.f, 421.f });
	singlePt->add(HJPointf{ 435.f, 421.f });
	singlePt->add(HJPointf{ 397.f, 484.f });
	singlePt->add(HJPointf{ 349.f, 518.f });
	singlePt->add(HJPointf{ 422.f, 517.f });

	morePtr->setIsDebugPoints(i_bDebug);
	morePtr->add(singlePt);
	morePtr->setSystemTime(HJCurrentSteadyMS());
	return morePtr;
}

NS_HJ_END
