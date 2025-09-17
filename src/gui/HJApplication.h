//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJUtils.h"
#include "HJLog.h"
#include "HJNotify.h"
#include "HJChore.h"
#include "HJNotify.h"
#include "HJWindow.h"

NS_HJ_BEGIN
//***********************************************************************************//
class HJApplication : public HJObject
{
public:
	using Ptr = std::shared_ptr<HJApplication>;
	explicit HJApplication();
	~HJApplication();

	virtual int init();
	virtual int run();
	virtual void done();
	//
	virtual int onInit() { return HJ_OK; }
	virtual int onRunBegin() { return HJ_OK; }
	virtual int onRunning() { return HJ_OK; }
	virtual int onRunEnd() { return HJ_OK; }
	virtual void onDone() { return; }
	//
	void addTask(HJRunnable run) {
		auto task = std::make_shared<HJTask>(std::move(run));
		m_tasks.push_back(task);
	}
protected:
	static void glfw_error_callback(int error, const char* description);
	//
	void executeTasks() {
		for (auto tsk : m_tasks) {
			(*tsk)();
		}
		m_tasks.clear();
	}
	
protected:
	HJRunState             m_runState = HJRun_NONE;
	HJWindow::Ptr			m_mainWindow = nullptr;
	HJList<HJTask::Ptr>	m_tasks;
};

NS_HJ_END