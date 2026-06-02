#pragma once

#include "HJUIBaseItem.h"
#include "HJFLog.h"
#include "HJUIItemFaceuTool.h"

#if !defined(HJ_MEDIA_GRAPHIC_UI)
#include "HJUIItemYuvRender.h"
#include "HJUIItemPlayerCom.h"
#include "HJUIItemPlayerPlugin.h"
#include "HJUIItemTest.h"
#include "HJUIItemNodeEditor.h"
#include "HJUIItemRendGraphWrapper.h"
#include "HJUIItemSR.h"
#if defined(WIN32_LIB)
	#include "HJUIItemSharedMemory.h"
#endif
#endif
NS_HJ_BEGIN

//////////////////////////////
HJUIBaseItem::HJUIBaseItem()
{

}
HJUIBaseItem::~HJUIBaseItem()
{

}

int HJUIBaseItem::run()
{
	return 0;
}
int HJUIBaseItem::renderEveryStart()
{
	return 0;
}
int HJUIBaseItem::renderEveryEnd()
{
	return 0;
}
HJUIBaseItem::Ptr HJUIBaseItem::createItem(HJUIItemType type, void* i_window)
{
	HJUIBaseItem::Ptr item = nullptr;
	switch (type)
	{
	case HJ::HJUIItemType_None:
		break;

#if !defined(HJ_MEDIA_GRAPHIC_UI)
	case HJ::HJUIItemType_YuvRender:
		item = HJUIItemYuvRender::Create();
		break;
	case HJ::HJUIItemType_PlayerCom:
		item = HJUIItemPlayerCom::Create();
		break;
	case HJ::HJUIItemType_PlayerPlugin:
		item = HJUIItemPlayerPlugin::Create();
		break;
	case HJ::HJUIItemType_Test:
		item = HJUIItemTest::Create();
		break;
	case HJ::HJUIItemType_SharedMemory:
	#if defined(WIN32_LIB)
		item = HJUIItemSharedMemory::Create();
	#endif
		break;
	case HJ::HJUIItemType_NodeEditor:
		item = HJUIItemNodeEditor::Create();
		break;
	case HJ::HJUIItemType_RendGraphWrapper:
		item = HJUIItemRendGraphWrapper::Create();
		break;
	case HJ::HJUIItemType_SR:
		item = HJUIItemSR::Create();
		break;
#endif
	case HJ::HJUIItemType_FaceuTool:
		item = HJUIItemFaceuTool::Create();
		break;
	default:
		break;
	}
	if (item)
	{
		item->setWindow(i_window);
	}
	return item;
}

NS_HJ_END



