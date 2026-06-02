#include "HJRenderCoreSingleton.h"
#include "HJError.h"
#include "HJFLog.h"

#if defined(WIN32_LIB) || defined(MACOS_LIB)
	#define GLAD_GL_IMPLEMENTATION  //only implemntation once, another use gl function, not use GLAD_GL_IMPLEMENTATION
	#include <glad/gl.h>

	#define GLFW_INCLUDE_NONE
	#include <GLFW/glfw3.h>
#endif

NS_HJ_BEGIN

HJRenderCoreSingleton& HJRenderCoreSingleton::Instance()
{
    static HJRenderCoreSingleton instance;
    return instance;
}
static void error_callback(int error, const char* description)
{
	HJFLoge("Error: {}", description);
}
GLFWwindow* HJRenderCoreSingleton::getWindow() const
{
    return m_window;
}
int HJRenderCoreSingleton::makeContext()
{
	int i_err = HJ_OK;
	do
	{
#if defined(WIN32_LIB) || defined(MACOS_LIB)
		if (!m_window)
		{
			i_err = HJErrNotInited;
			HJFLoge("window not inited, error");
			break;
		}
		glfwMakeContextCurrent(m_window);
#endif
	} while (false);
    return i_err;
}
void HJRenderCoreSingleton::makeCurrentNULL()
{
#if defined(WIN32_LIB) || defined(MACOS_LIB)
    glfwMakeContextCurrent(NULL);
#endif
}

int HJRenderCoreSingleton::init()
{
    int i_err = HJ_OK;
    do
    {
#if defined(WIN32_LIB) || defined(MACOS_LIB)
		if (!m_window)
		{
			HJFLogi("coreInit");

			glfwSetErrorCallback(error_callback);
			if (!glfwInit())
			{
				i_err = -1;
				HJFLoge("glfwInit error");
				break;
			}
#if defined(MACOS_LIB)
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
			glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);           // Required on Mac
#else
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // windows support GLFW_OPENGL_COMPAT_PROFILE, but MAC only support GLFW_OPENGL_CORE_PROFILE
#endif
			glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

			glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_NATIVE_CONTEXT_API); //windows use WGL , other use EGL
			m_window = glfwCreateWindow(1, 1, "HJRenderCoreSingleton", NULL, NULL);
			if (!m_window)
			{
				glfwTerminate();
				i_err = -1;
				HJFLoge("create window error");
				break;
			}

			glfwMakeContextCurrent(m_window);
			//must after make current 
			bool err = gladLoadGL(glfwGetProcAddress) == 0;
			if (err)
			{
				i_err = -1;
				HJFLoge("gladLoadGL error");
				break;
			}

			glfwSwapInterval(1);
			glfwMakeContextCurrent(NULL);
			HJFLogi("create window");
		}
#endif
    } while (false);
    return i_err;
}
void HJRenderCoreSingleton::done()
{
#if defined(WIN32_LIB) || defined(MACOS_LIB)
	if (m_window)
	{
        HJFLogi("done enter");
		glfwDestroyWindow(m_window);
		glfwTerminate();
		m_window = nullptr;
		HJFLogi("done end");
	}
#endif
}

NS_HJ_END
