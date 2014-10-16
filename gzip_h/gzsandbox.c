
#include <sys/cdefs.h>

#include <sys/types.h>
#include <sys/capability.h>
#include <sys/uio.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "gzip.h"

off_t
gz_uncompress_wrapper(int in, int out, char *pre, size_t prelen,
    off_t *gsizep, const char *filename)
{
  return -1;
  (void) in;
  (void) out;
  (void) pre;
  (void) prelen;
  (void) gsizep;
  (void) filename;
}

off_t
gz_compress_wrapper(int in, int out, off_t *gsizep, const char *origname,
    uint32_t mtime)
{
  return -1;
  (void) in;
  (void) out;
  (void) gsizep;
  (void) origname;
  (void) mtime;
}

