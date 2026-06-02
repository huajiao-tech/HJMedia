#pragma once


#include "HJPrerequisites.h"
#include "HJOGUtils.h"
#include "HJOGFBOCtrl.h"
#include "HJComUtils.h"
#include "HJFlyweightPoolBase.h"

NS_HJ_BEGIN

class HJFBOCtrlPool : public HJFlyweightPoolBase<HJOGFBOCtrl>
{
public:
	HJ_DEFINE_CREATE(HJFBOCtrlPool);
	HJFBOCtrlPool() = default;
    virtual ~HJFBOCtrlPool() = default;
	
	HJOGFBOCtrl::Ptr acquire(const std::string &i_userName, int i_width, int i_height, bool i_bTransparent = true);
	void recovery(HJOGFBOCtrl::Ptr i_pFBOCtrl);
	void remove(int i_width, int i_height, bool i_bTransparent = true);

private:
	std::string priGetKey(int i_width, int i_height, bool i_bTransparent);
};

NS_HJ_END



