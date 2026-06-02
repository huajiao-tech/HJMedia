//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include <mutex>
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
        std::lock_guard<std::mutex> lock(m_task_mutex);
		m_tasks.push_back(task);
	}
protected:
	static void glfw_error_callback(int error, const char* description);
	//
	void executeTasks() {
        HJList<HJTask::Ptr> pending_tasks;
        {
            std::lock_guard<std::mutex> lock(m_task_mutex);
            pending_tasks.swap(m_tasks);
        }
		for (const auto& tsk : pending_tasks) {
			(*tsk)();
		}
	}
	
protected:
	HJRunState             m_runState = HJRun_NONE;
	HJWindow::Ptr			m_mainWindow = nullptr;
	HJList<HJTask::Ptr>	m_tasks;
    std::mutex              m_task_mutex;
    bool                    m_imgui_ready = false;
    bool                    m_implot_ready = false;
    bool                    m_glfw_backend_ready = false;
    bool                    m_opengl_backend_ready = false;
};

NS_HJ_END
