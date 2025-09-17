#pragma once

#include "HJUIBaseItem.h"
#include "HJFLog.h"
#include "HJUIItemYuvRender.h"
#include "HJUIItemPlayerCom.h"
#include "HJUIItemPlayerPlugin.h"
#include "HJUIItemTest.h"

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

HJUIBaseItem::Ptr HJUIBaseItem::createItem(HJUIItemType type)
{
	HJUIBaseItem::Ptr item = nullptr;
	switch (type)
	{
	case HJ::HJUIItemType_None:
		break;
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
	default:
		break;
	}
	return item;
}

NS_HJ_END



