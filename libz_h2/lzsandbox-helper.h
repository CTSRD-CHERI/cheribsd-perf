#ifndef _LZSANDBOX_HELPER_H_
#define _LZSANDBOX_HELPER_H_

/* before including this file, appropriately define the ZLIB_INCL_* */
#include "zlib.h"

#define LZOP_DEFLATEINIT2 0
#define LZOP_DEFLATE      1
#define LZOP_DEFLATEEND   2
#define LZOP_CRC32        3

#include <machine/cheri.h>
#include <machine/cheric.h>

struct lzparams
{
  z_streamp_c strm;

  /* deflate_c */
  int flush;

  /* deflateInit2_c */
  int  level;
  int  method;
  int  windowBits;
  int  memLevel;
  int  strategy;
  __capability const char *version;
  int stream_size;

  /* crc32 */
  uLong crc;
  __capability const Bytef * buf;
  uInt len;
};

int ef (const char * format, ...);

void * bufcpy_c_fromcap (void * dst, __capability const void * src, size_t len);
__capability void * bufcpy_c_tocap (__capability void * dst, const void * src, size_t len);
__capability void * bufcpy_c (__capability void * dst, __capability const void * src, size_t len);

#endif /* !_LZSANDBOX_HELPER_H_ */
