#include "HJFLog.h"
#include "HJGUICommon.h"

NS_HJ_BEGIN

void HJGUICommon::allignMonitorTopRight(GLFWwindow* i_window, int width, int height)
{
	// Position the UI window at the top-right corner of the primary monitor
	// Align the window's outer top-right corner with the monitor work area top-right
	// (work area excludes taskbar and docks when supported)
	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	if (monitor)
	{
		int workX = 0, workY = 0, workW = 0, workH = 0;
		glfwGetMonitorWorkarea(monitor, &workX, &workY, &workW, &workH);
		// Fallback if work area is unavailable
		if (workW <= 0 || workH <= 0)
		{
			const GLFWvidmode* mode = glfwGetVideoMode(monitor);
			if (mode)
			{
				workX = 0;
				workY = 0;
				workW = mode->width;
				workH = mode->height;
			}
		}

		int frameLeft = 0, frameTop = 0, frameRight = 0, frameBottom = 0;
		glfwGetWindowFrameSize(i_window, &frameLeft, &frameTop, &frameRight, &frameBottom);

		// Compute content-area position so that the outer frame's top-right
		// aligns with the work area's top-right
		int posX = workX + workW - width - frameRight;
		int posY = workY + frameTop;
		glfwSetWindowPos(i_window, posX, posY);
	}
}

NS_HJ_END



