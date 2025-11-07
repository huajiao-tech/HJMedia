#pragma once

#include "HJPrerequisites.h"
#include "HJUIBaseItem.h"

NS_HJ_BEGIN

class HJUIItemTest : public HJUIBaseItem
{
public:
	    
	HJ_DEFINE_CREATE(HJUIItemTest);
    
	HJUIItemTest();
	virtual ~HJUIItemTest();

	virtual int run() override;
    
private:
	void priTestSubscribe();
	void priTestJson();
	void priTestPrioRender();
	void priTestRteRender();
};

NS_HJ_END



