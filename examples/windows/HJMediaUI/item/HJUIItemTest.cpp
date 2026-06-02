#pragma once

#include "HJUIItemTest.h"
#include "HJFLog.h"
#include "imgui.h"

#if defined(HJ_ENABLE_RENDER_PRIO)
	#include "HJPrioCom.h"
	#include "HJPrioGraph.h"
#endif

#include "HJFaceuInfo.h"
#include "HJFacePointsInfo.h"
#include "HJRteGraph.h"
#include "HJRteUtils.h"
#include "HJReflCpp.h"
#include "HJReflCppJson.h"

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
	//graph->testTopToBottom();
	//graph->tesetBottomToTop();
}


class PointTest
{
public:
	float x = 0.f;
	float y = 1.f;
};

class Address {
public:
	std::string city;
	int zip = 0;
};
class Foo : public HJ::HJReflCppJsonInterpreter<Foo> {
public:
	int id = 0;
	std::string name;
	PointTest pt;
	HJ::HJPointf pos;
	HJ::HJRecti rect;
	Address addr;                 // Ç¶Ì×¶ÔÏó
	std::vector<std::shared_ptr<Address>> addrList; // Ç¶Ì×Êý×é
};

NS_HJ_END

REFL_AUTO_SIMPLE(HJ::PointTest, x, y)
REFL_AUTO_SIMPLE(HJ::Address, city, zip)
REFL_AUTO_SIMPLE(HJ::Foo, rect, pt, pos, id, name, addr, addrList)

NS_HJ_BEGIN


void HJUIItemTest::priTestReflCpp()
{
	Foo foo;
	foo.id = 10;
	foo.name = "HJ";
    foo.addr.city = "Hangzhou";
    foo.addr.zip = 100000;
	foo.pos.x = 11.f;
    foo.pos.y = 22.f;
	foo.pt.x = 1.f;
    foo.pt.y = 2.f;
	foo.rect.x = 1;
    foo.rect.y = 2;
	foo.rect.w = 640;
	foo.rect.h = 480;

	std::shared_ptr<Address> addr1 = std::make_shared<Address>();
	addr1->city = "Beijing";
	addr1->zip = 200000;
	foo.addrList.push_back(addr1);

	std::string out = foo.serial();

	Foo foo2;
	foo2.deserial(out);

	auto type = refl::reflect<HJ::PointTest>();
	// ±éÀú×Ö¶Î
	refl::util::for_each(type.members, [](auto member)
		{
		std::cout << member.name.c_str() << std::endl;
		});


	HJ::PointTest p;
	p.x = 3.5f;
	p.y = 9.0f;

	//auto type = refl::reflect<HJ::PointTest>();
	refl::util::for_each(type.members, [&](auto member)
		{
		// ¶Á×Ö¶Î
		auto value = member(p);
		std::cout << member.name.c_str() << " = " << value << std::endl;
		});
}
void HJUIItemTest::priTestPrioRender()
{
	do
	{
#if defined(HJ_ENABLE_RENDER_PRIO)
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
#endif
	} while (false);
}
void HJUIItemTest::priTestJson()
{
	const std::string json = R"({
	"uid": 123,
	"device": "windows",
	"timestamp": 10,
	"business": 1,
	"scene": {
		"value": 110,
		"sceneinfo": [
			{
				"id": 3,
				"value": 0
			},
			{
				"id": 4,
				"value": ""
			}
		],
		"doublePlainArray": [
			56.0,
			66.0
		]
	},
	"metrics": [
		{
			"id": 0,
			"value": 0
		},
		{
			"id": 1,
			"value": 0.5
		},
		{
			"id": 2,
			"value": ""
		}
	],
	"strPlainArray": [
		"abc",
		"Efg"
	],
	"intPlainArray": [
		5,
		6,
		8
	]
})";
	int i_err = 0;

	//HJJsonTestMonitor testMonitor;
	//i_err = testMonitor.init(json);
	//if (i_err < 0)
	//{
	//	
	//}

	//std::string newTest = testMonitor.initSerial();
 //   HJFLogi("newTest: {}", newTest.c_str());

	HJBaseParam::Ptr param = HJBaseParam::Create();
	HJ_CatchMapPlainSetVal(param, std::string, HJRteUtils::ParamUrlFaceu, "E:/video/gift/FaceU/2D/90237_1"); //90237_1  120067_1
	HJFaceuInfo faceuInfo;

	i_err = faceuInfo.parse(param);
	if (i_err < 0)
	{
		
	}

	const std::string pointsJson = R"(
[
	{
		"probability": 0.9999819993972778,
		"block": -1,
		"rect": {
			"left": 257,
			"top": 435,
			"width": 309,
			"height": 380
		},
		"pose": {
			"yaw": -26.128273010253908,
			"pitch": -19.960651397705079,
			"roll": 4.956053256988525
		},
		"points": [
			{
				"x": 390,
				"y": 580
			},
			{
				"x": 513,
				"y": 551
			},
			{
				"x": 507,
				"y": 626
			},
			{
				"x": 440,
				"y": 744
			},
			{
				"x": 528,
				"y": 718
			}
		]
	},
	{
		"probability": 0.3,
		"block": -1,
		"rect": {
			"left": 157,
			"top": 235,
			"width": 209,
			"height": 380
		},
		"pose": {
			"yaw": -26.128273010253908,
			"pitch": -19.960651397705079,
			"roll": 4.956053256988525
		},
		"points": [
			{
				"x": 390,
				"y": 580
			},
			{
				"x": 513,
				"y": 551
			},
			{
				"x": 507,
				"y": 626
			},
			{
				"x": 440,
				"y": 744
			},
			{
				"x": 528,
				"y": 718
			}
		]
	}
]
)";
	HJFacePointsInfo pointInfo;
	i_err = pointInfo.deserial(pointsJson);
	if (i_err < 0)
	{

	}

}

template <typename... Args>
class AutoCleanEventBus 
{
private:
	using CallbackType = std::function<void(Args...)>;
	std::unordered_map<std::string, std::vector<std::weak_ptr<CallbackType>>> topicSubscribers;
	mutable std::mutex mutex_;
public:
	template<typename Callback>
	std::shared_ptr<CallbackType> subscribe(const std::string& topic, Callback&& callback)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		auto callbackPtr = std::make_shared<CallbackType>(std::forward<Callback>(callback));
		topicSubscribers[topic].emplace_back(callbackPtr); // save weak_ptr
		return callbackPtr; // return shared_ptr, the caller should hold a reference to it
	}
	
	void subscribe(const std::string& topic, std::shared_ptr<CallbackType> callbackptr)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		topicSubscribers[topic].emplace_back(std::move(callbackptr));
	}	
	
	void publish(const std::string& topic, Args... args) 
	{
		std::vector<std::shared_ptr<CallbackType>> activeCallbacks;
		{
			std::lock_guard<std::mutex> lock(mutex_);
			auto it = topicSubscribers.find(topic);
			if (it == topicSubscribers.end())
			{
				return;
			}
			auto& callbacks = it->second;
			auto callbackIt = callbacks.begin();

			while (callbackIt != callbacks.end()) 
			{
				if (auto callbackPtr = callbackIt->lock()) { // try weak_ptr to shared_ptr
					//(*callbackPtr)(args...); //not in lock
					activeCallbacks.push_back(std::move(callbackPtr));
					++callbackIt;
				}
				else 
				{
					// remove invalid weak_ptr
					callbackIt = callbacks.erase(callbackIt);
				}
			}
		}
		for (auto& callbackPtr : activeCallbacks)
		{
			(*callbackPtr)(args...);
		}
	}
};

class TestArguments {
public:
	TestArguments(int i_a, std::string i_b)
		: a(i_a), b(i_b)
	{
	}	
	int a;
	std::string b;
};

void HJUIItemTest::priTestSubscribe()
{

	AutoCleanEventBus<int, std::shared_ptr<TestArguments>> testBus;
	int k6 = 8;
	std::shared_ptr<std::function<void(int, std::shared_ptr<TestArguments>)>> v3 = std::make_shared<std::function<void(int, std::shared_ptr<TestArguments>)>>([k6](int a, std::shared_ptr<TestArguments> args)
		{
			std::cout << "v3 a: " << k6 << a << std::endl;
			std::cout << "v3 b: " << args->b << std::endl;
		});
	testBus.subscribe("testbus1", v3);

	testBus.publish("testbus1", 5, std::make_shared<TestArguments>(5, "hello"));


	int k1 = 2;
	int k2 = 3;
	std::shared_ptr<std::function<void(int, std::shared_ptr<TestArguments>)>> v1 = testBus.subscribe("testbus1", [k1](int a, std::shared_ptr<TestArguments> args)
	{
		std::cout << "v1 a: " << k1 << a << std::endl;
		std::cout << "v1 b: " << args->b << std::endl;
	});

	std::shared_ptr<std::function<void(int, std::shared_ptr<TestArguments>)>> v2 = testBus.subscribe("testbus1", [k2](int a, std::shared_ptr<TestArguments> args)
		{
			std::cout << "v2 a: " << k2 << a << std::endl;
			std::cout << "v2 b: " << args->b << std::endl;
		});


	testBus.publish("testbus1", 1, std::make_shared<TestArguments>(5, "hello"));


	AutoCleanEventBus<const std::string&> bus;

	{
		auto subscription1 = bus.subscribe("news", [](const std::string& msg) {
			std::cout << "abc: " << msg << std::endl;
			});
		bus.publish("news", "a");
	}

	bus.publish("news", "fafaf");

	auto subscription2 = bus.subscribe("news", [](const std::string& msg) {
		std::cout << "[??????2] ???????: " << msg << std::endl;
		});
	bus.publish("news", "e");

}
int HJUIItemTest::run()
{
	int i_err = 0;
	do
	{
		//ImGui::SetWindowSize(ImVec2(1500, 200));
		//ImGui::SetWindowPos(ImVec2(30, 30));
		ImGui::Begin("test");
		if (ImGui::Button("refl-cpp"))
		{
			priTestReflCpp();
		}

#if 0
		ImGui::Begin("YuvParams");



		ImGui::Button("testabc");

		ImGui::Button("testefg");

		if (ImGui::Button("RteRender"))
		{
			priTestRteRender();
		}
		
		if (ImGui::Button("Subscribe"))
		{
			priTestSubscribe();
		}
		if (ImGui::Button("Json"))
		{
			priTestJson();
		}
		
		if (ImGui::Button("PrioRender"))
		{
			priTestPrioRender();
		}
#endif

		ImGui::End();
	} while (false);
	return i_err;
}


NS_HJ_END



