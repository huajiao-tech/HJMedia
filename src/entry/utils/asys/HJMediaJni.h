#pragma once


#include <jni.h>
#include <android/log.h>
#include <android/api-level.h>
#include <android/native_window_jni.h>
#include <android/asset_manager_jni.h>
//#include <android_native_app_glue.h>

//////////////////////////////////////////////////////////////////////////
#if 1//MT_HAVE_LOG
#	define LOG_I(...) ((void)__android_log_print(ANDROID_LOG_INFO, HJ_LOG_TAG, __VA_ARGS__))
#	define LOG_V(...) ((void)__android_log_print(ANDROID_LOG_VERBOSE, HJ_LOG_TAG, __VA_ARGS__))
#	define LOG_E(...) ((void)__android_log_print(ANDROID_LOG_ERROR, HJ_LOG_TAG, __VA_ARGS__))
#	define LOG_D(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, HJ_LOG_TAG, __VA_ARGS__))
#	define LOG_W(...) ((void)__android_log_print(ANDROID_LOG_WARN, HJ_LOG_TAG, __VA_ARGS__))
#else
#	define LOG_I(...)
#	define LOG_V(...)
#	define LOG_E(...)
#	define LOG_D(...)
#	define LOG_W(...)
#endif

//////////////////////////////////////////////////////////////////////////

#define HJ_MEDIA_RENDER_DECL(nspre, sig) Java_com_HJMediasdk_entry_render_ ## nspre ##_## sig
#define HJ_MEDIA_PLAYER_DECL(nspre, sig) Java_com_HJMediasdk_entry_player_ ## nspre ##_## sig
#define HJ_MEDIA_INFERENCE_DECL(nspre, sig) Java_com_HJMediasdk_entry_inference_ ## nspre ##_## sig
//#define SL_RENDERFUN_DECL(nspre, sig) Java_com_slmedia_slrender_## nspre ##_## sig
//#define SL_OPENGLESRENDERFUN_DECL(nspre, sig) Java_com_slmedia_openglesrender_## nspre ##_## sig



