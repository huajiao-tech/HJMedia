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

	typedef struct HJFDataSourceIOContext
	{
		AVClass*	cls;
		void*		m_io;
		char*		m_url;
		int			m_flags;
		//
		void*		m_opaque;
	} HJFDataSourceIOContext;
//***********************************************************************************//
	int hjds_open(URLContext* h, const char* arg, int flags, AVDictionary** options);
	int hjds_read(URLContext* h, unsigned char* buf, int size);
	int64_t hjds_seek(URLContext* h, int64_t pos, int whence);
	int hjds_close(URLContext* h);

#if defined( __cplusplus )
}
#endif