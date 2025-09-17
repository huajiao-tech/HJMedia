#ifndef _HJ_TLS_OPENSSL_MANAGER_INTERFACE_H_
#define _HJ_TLS_OPENSSL_MANAGER_INTERFACE_H_
#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include <string.h>

typedef struct AVDictionary AVDictionary;
/////////////////////////////////////////////////////////////////////////////
#if defined( __cplusplus )
extern "C" {
#endif

	size_t hjtls_link_create(const char* url, int flags, AVDictionary** options);
	//int hjtls_link_unused(size_t handle);
	int hjtls_link_destroy(size_t handle);
	//
	void hjtls_link_done();
	////
	//int hjtls_link_open(size_t handle, const char* url, int flags, AVDictionary** options);
	int hjtls_link_read(size_t handle, uint8_t* buf, int size);
	int hjtls_link_write(size_t handle, const uint8_t* buf, int size);
	//int hjtls_link_close(size_t handle);
	int hjtls_link_get_file_handle(size_t handle);
	int hjtls_link_tls_get_short_seek(size_t handle);

	//
	void hjtls_link_set_proc_info(const char* category, const char* key, const int64_t value);
#if defined( __cplusplus )
}
#endif
#endif //_HJ_TLS_OPENSSL_MANAGER_INTERFACE_H_