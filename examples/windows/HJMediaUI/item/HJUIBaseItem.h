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
    HJUIItemType_FaceuTool,
	HJUIItemType_NodeEditor, 
	HJUIItemType_RendGraphWrapper,
	HJUIItemType_SR,
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

	static HJUIBaseItem::Ptr createItem(HJUIItemType type, void *i_window);


	virtual int run();
	virtual int renderEveryStart();
	virtual int renderEveryEnd();
	virtual bool updateTitle()
	{
		return false;
	}

	void setWindow(void *i_window)
	{
		m_window = i_window;
	}
	void *getWindow()
	{
		return m_window;
	}

protected:
	HJUIItemState m_state = HJUIItemState_idle;
	bool m_vqRun = false;
private:
    void *m_window = nullptr;
};

NS_HJ_END



