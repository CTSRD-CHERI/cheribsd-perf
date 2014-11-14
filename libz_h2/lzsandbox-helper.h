#ifndef _LZSANDBOX_HELPER_H_
#define _LZSANDBOX_HELPER_H_

/* before including this file, appropriately define the ZLIB_INCL_* */
#include "zlib.h"

#define LZOP_DEFLATEINIT2 0
#define LZOP_DEFLATE      1
#define LZOP_DEFLATEEND   2
#define LZOP_CRC32        3
#define LZOP_INFLATEINIT2 4
#define LZOP_INFLATE      5
#define LZOP_INFLATEEND   6
#define LZOP_DEFLATERESET 7

#include <machine/cheri.h>
#include <machine/cheric.h>

struct lzparams
{
  z_streamp_c strm;

  /* deflate_c, inflate_c */
  int flush;

  /* deflateInit2_c, inflateInit2_c */
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

#endif /* !_LZSANDBOX_HELPER_H_ */
