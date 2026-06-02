#pragma once

#if defined(HarmonyOS)
	#include <EGL/egl.h>
	#include <EGL/eglext.h>
	#include <GLES3/gl3.h>
	#include <GLES2/gl2ext.h>
#elif defined(WIN32_LIB) || defined(MACOS_LIB)
	#include "glad/gl.h"
#elif defined(ANDROID_LIB)
	#include <GLES3/gl3.h>
	#include <GLES2/gl2ext.h>
#elif defined(IOS_LIB)
    #include <OpenGLES/ES3/gl.h>
    #include <OpenGLES/ES3/glext.h>
#endif

//fixme lfs after modify
#if !defined(HarmonyOS)
	using EGLSurface = void*;
	#define EGL_NO_SURFACE 0
#endif
