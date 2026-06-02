#pragma once

#include "HJPrerequisites.h"
#include <glad/gl.h> //#define GLAD_GL_IMPLEMENTATION  only implementation once;
#include <GLFW/glfw3.h>

NS_HJ_BEGIN

class HJGUICommon
{
public:  
	static void allignMonitorTopRight(GLFWwindow* i_window, int width, int height);
};

NS_HJ_END



