#ifndef _LZSANDBOX_HELPER_H_
#define _LZSANDBOX_HELPER_H_

/* before including this file, appropriately define the ZLIB_INCL_* */
#include "zlib.h"

#define LZOP_DEFLATEINIT2 0
#define LZOP_DEFLATE      1
#define LZOP_DEFLATEEND   2

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
};

int ef (const char * format, ...);

#endif /* !_LZSANDBOX_HELPER_H_ */
