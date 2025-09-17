#pragma once

#include "HJUIItemTest.h"
#include "HJFLog.h"
#include "imgui.h"

#include "HJPrioCom.h"
#include "HJPrioGraph.h"

#include "HJRteGraph.h"

NS_HJ_BEGIN


HJUIItemTest::HJUIItemTest()
{
}

HJUIItemTest::~HJUIItemTest()
{

}
void HJUIItemTest::priTestRteRender()
{
	HJRteGraph::Ptr graph = HJRteGraph::Create();
	graph->test();
}
void HJUIItemTest::priTestPrioRender()
{

	do
	{
		HJPrioGraph::Ptr graph = HJPrioGraph::Create();

		HJPrioCom::Ptr prioComA = HJPrioCom::Create();
		prioComA->setPriority(HJPrioComType_VideoBlur);
		prioComA->setInsName("blurA");

		graph->insert(prioComA);
		graph->insert(prioComA);
		HJPrioCom::Ptr prioComB = HJPrioCom::Create();
		prioComB->setPriority(HJPrioComType_VideoBlur);
		prioComB->setInsName("blurB");

		graph->insert(prioComB);

		HJPrioCom::Ptr prioComC = HJPrioCom::Create();
		prioComC->setPriority(HJPrioComType_GiftSeq2D);
		prioComC->setInsName("gift");

		graph->insert(prioComC);

		HJPrioCom::Ptr prioComD = HJPrioCom::Create();
		prioComD->setPriority(HJPrioComType_VideoBeauty);
		prioComD->setInsName("beauty");

		graph->insert(prioComD);

		HJPrioCom::Ptr prioComE = HJPrioCom::Create();
		prioComE->setPriority(HJPrioComType_VideoBlur);
		prioComE->setInsName("blurE");

		graph->insert(prioComE);

		graph->remove(prioComD);
		graph->remove(prioComD);
		graph->remove(prioComB);
		graph->remove(prioComC);

	} while (false);
}
int HJUIItemTest::run()
{
	int i_err = 0;
	do
	{
		ImGui::Begin("YuvParams");

		if (ImGui::Button("RteRender"))
		{
			priTestRteRender();
		}
		if (ImGui::Button("PrioRender"))
		{
			priTestPrioRender();
		}

		ImGui::End();
	} while (false);
	return i_err;
}


NS_HJ_END



