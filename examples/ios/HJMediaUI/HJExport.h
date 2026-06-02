#pragma once

#if defined(WIN32)
	#ifdef HJ_EXPORT_SET
		#define HJExport __declspec(dllexport)
	#else
		#define HJExport __declspec(dllimport)
	#endif
#elif defined(ANDROID_LIB) || defined(IOS_LIB) || defined(MACSO_LIB) || defined(HarmonyOS)
	#define HJExport __attribute__ ((visibility("default")))
#else
	#define HJExport
#endif

