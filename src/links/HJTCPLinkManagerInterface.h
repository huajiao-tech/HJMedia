#ifndef _HJ_TCP_LINK_MANAGER_INTERFACE_H_
#define _HJ_TCP_LINK_MANAGER_INTERFACE_H_
#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include <string.h>

typedef struct AVDictionary AVDictionary;
/////////////////////////////////////////////////////////////////////////////
#if defined( __cplusplus )
extern "C" {
#endif

size_t hjtcp_link_create(const char* url, int flags);
int hjtcp_link_unused(size_t handle);
int hjtcp_link_destroy(size_t handle);
//
void hjtcp_link_done();
//
int hjtcp_link_open(size_t handle, const char* url, int flags, AVDictionary** options);
int hjtcp_link_accept(size_t handle);
int hjtcp_link_read(size_t handle, uint8_t* buf, int size);
int hjtcp_link_write(size_t handle, const uint8_t* buf, int size);
int hjtcp_link_shutdown(size_t handle, int flags);
//int hjtcp_link_close(size_t handle);
int hjtcp_link_get_file_handle(size_t handle);
int hjtcp_link_get_window_size(size_t handle);

int hjtcp_link_is_alive(size_t handle);

#if defined( __cplusplus )
}
#endif

#endif //_HJ_TCP_LINK_MANAGER_INTERFACE_H_