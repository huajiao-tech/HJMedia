#include "HJFBOCtrlPool.h"
#include "HJFLog.h"

NS_HJ_BEGIN

HJOGFBOCtrl::Ptr HJFBOCtrlPool::acquire(const std::string& i_useName, int i_width, int i_height, bool i_bTransparent)
{
	return HJFlyweightPoolBase<HJOGFBOCtrl>::acquire([this, i_width, i_height, i_bTransparent]()
		{
			return priGetKey(i_width, i_height, i_bTransparent);
		},
		[this, i_useName, i_width, i_height, i_bTransparent]()
		{
			HJOGFBOCtrl::Ptr pFBOCtrl = HJOGFBOCtrl::Create();
			int i_err = pFBOCtrl->init(i_width, i_height, i_bTransparent);
			if (i_err < 0)
			{
				pFBOCtrl = nullptr;;
			}
			HJFLogi("pool total create cnt:{} userName:{} width:{} height:{} transparent:{}", m_createCnt, i_useName, i_width, i_height, i_bTransparent);
			return pFBOCtrl;
		});
}
void HJFBOCtrlPool::recovery(HJOGFBOCtrl::Ptr i_pFBOCtrl)
{
	HJFlyweightPoolBase<HJOGFBOCtrl>::recovery(i_pFBOCtrl, [this, i_pFBOCtrl]
		{
			return priGetKey(i_pFBOCtrl->getWidth(), i_pFBOCtrl->getHeight(), i_pFBOCtrl->isTransparency());
		});
}
void HJFBOCtrlPool::remove(int i_width, int i_height, bool i_bTransparent)
{
	HJFlyweightPoolBase<HJOGFBOCtrl>::remove([this, i_width, i_height, i_bTransparent]
		{
			return priGetKey(i_width, i_height, i_bTransparent);
		});
}

std::string HJFBOCtrlPool::priGetKey(int i_width, int i_height, bool i_bTransparent)
{
	return HJFMT("{}X{}X{}", i_width, i_height, i_bTransparent);
}

NS_HJ_END