#ifndef _JPLAYER_HLINKS_MANAGER_INTERFACE_H_
#define _JPLAYER_HLINKS_MANAGER_INTERFACE_H_

#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include <string.h>

typedef struct AVDictionary AVDictionary;
typedef struct URLContext URLContext;
/////////////////////////////////////////////////////////////////////////////
#if defined( __cplusplus )
extern "C" {
#endif

size_t hjhlink_create(URLContext* h, const char* url, int flags, AVDictionary** options, AVDictionary* params);
int hjhlink_destroy(size_t handle);
//
void hjhlink_done();

int hjhlink_accept(size_t handle,URLContext** c);
int hjhlink_handshake(size_t handle);
int hjhlink_read(size_t handle, uint8_t* buf, int size);
int hjhlink_write(size_t handle, const uint8_t* buf, int size);
int64_t hjhlink_seek(size_t handle, int64_t off, int whence);
int hjhlink_get_file_handle(size_t handle);
int hjhlink_get_short_seek(size_t handle);
int hjhlink_shutdown(size_t handle, int flags);

#if defined( __cplusplus )
}
#endif
#endif //_JPLAYER_HLINKS_MANAGER_INTERFACE_H_
