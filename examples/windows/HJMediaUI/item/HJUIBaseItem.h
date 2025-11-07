#pragma once

#include "HJPrerequisites.h"
#include "HJComUtils.h"

NS_HJ_BEGIN

typedef enum HJUIItemType
{
	HJUIItemType_None = 0,
	HJUIItemType_PlayerCom,
	HJUIItemType_PlayerPlugin,
	HJUIItemType_YuvRender,
	HJUIItemType_SharedMemory,
	HJUIItemType_Test,
	
} HJUIItemType;

typedef enum HJUIItemState
{
	HJUIItemState_idle = 0,
	HJUIItemState_ready,
	HJUIItemState_run,
} HJUIItemState;

class HJUIBaseItem : public HJBaseObject
{
public:
	    
	HJ_DEFINE_CREATE(HJUIBaseItem);
    
	HJUIBaseItem();
	virtual ~HJUIBaseItem();

	static HJUIBaseItem::Ptr createItem(HJUIItemType type);


	virtual int run();
protected:
	HJUIItemState m_state = HJUIItemState_idle;
private:
    
};

NS_HJ_END



