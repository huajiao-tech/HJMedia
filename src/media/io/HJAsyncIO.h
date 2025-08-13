//***********************************************************************************//
//HJMedia FRAMEWORK SOURCE;
//AUTHOR:
//CREATE TIME:
//***********************************************************************************//
#pragma once
#include "HJFFUtils.h"

#if defined( __cplusplus )
extern "C" {
#endif

	typedef struct HJExasyncContext
	{
		AVClass*	cls;
		void*		m_asyncIO;
		char*		m_url;
		int			m_flags;
		//
		void*		m_opaque;
		void*		m_blob;
	} HJExasyncContext;
//***********************************************************************************//
	int exasync_open(URLContext* h, const char* arg, int flags, AVDictionary** options);
	int exasync_read(URLContext* h, unsigned char* buf, int size);
	int64_t exasync_seek(URLContext* h, int64_t pos, int whence);
	int exasync_close(URLContext* h);

#if defined( __cplusplus )
}
#endif