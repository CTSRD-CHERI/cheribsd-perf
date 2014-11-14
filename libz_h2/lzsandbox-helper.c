#include <sys/types.h>
#include <sys/stat.h>

#include <machine/cheri.h>
#include <machine/cheric.h>

#include <cheri/cheri_enter.h>
#include <cheri/cheri_fd.h>
#include <cheri/cheri_invoke.h>
extern __capability void	*cheri_system_type;
#include <cheri/cheri_system.h>

/* libc_cheri provides implementation */
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lzsandbox-helper.h"

static struct cheri_object stderrfd;

int
invoke(register_t op,
  __capability void * co_codecap_stderrfd,
  __capability void * co_datacap_stderrfd,
  __capability void * vparams);

ssize_t read_c (struct cheri_object fd, void * buf, size_t nbytes);
ssize_t write_c (struct cheri_object fd, const void * buf, size_t nbytes);
int fprintf_c (struct cheri_object fd, const char * restrict format, ...);
int vfprintf_c (struct cheri_object fd, const char * restrict format, va_list ap);
__capability void * malloc_c (size_t size);
void free_c (__capability void * ptr);
size_t strlen_c (__capability const char * str);

ssize_t read_c (struct cheri_object fd, void * buf, size_t nbytes)
{
  struct cheri_fd_ret rc;
  rc = cheri_fd_read_c(fd, cheri_ptrperm(buf, nbytes, CHERI_PERM_STORE));
  errno = rc.cfr_retval1;
  return rc.cfr_retval0;
}

ssize_t write_c (struct cheri_object fd, const void * buf, size_t nbytes)
{
  struct cheri_fd_ret rc;
  rc = cheri_fd_write_c(fd, cheri_ptrperm((void*)buf, nbytes, CHERI_PERM_LOAD));
  errno = rc.cfr_retval1;
  return rc.cfr_retval0;
}

int fprintf_c (struct cheri_object fd, const char * restrict format, ...)
{
  int rc;
  va_list ap;
  va_start(ap, format);
  rc = vfprintf_c(fd, format, ap);
  va_end(ap);
  return rc;
}
int vfprintf_c (struct cheri_object fd, const char * restrict format, va_list ap)
{
  char buf[512];
  int n;
  n = vsnprintf(buf, sizeof buf, format, ap);
  if (n < 0) return n;
  n++;
  return write_c(fd, buf, n);
}
__capability void * malloc_c (size_t size)
{
  void * ptr;
  ptr = malloc(size);
  if (!ptr) return NULL;
  return cheri_ptr(ptr, size);
}

void free_c (__capability void * ptr)
{
  free((void*)ptr);
}

size_t strlen_c (__capability const char * str)
{
  size_t len = 0;
  while (*str)
    str++, len++;
  return len;
}

int ef (const char * format, ...)
{
  int rc;
  va_list ap;
  va_start(ap, format);
  rc = vfprintf_c(stderrfd, format, ap);
  va_end(ap);
  return rc;
}

int
invoke(register_t op,
  __capability void * co_codecap_stderrfd,
  __capability void * co_datacap_stderrfd,
  __capability void * vparams)
{
  __asm__ __volatile__ ("cmove $c11, $c26" ::: "memory");
#pragma clang diagnostic push
#pragma clang diagnostic warning "-Winline-asm"
  __asm__ __volatile__ ("cmove $c0, $c26" ::: "memory");
#pragma clang diagnostic pop
  __capability struct lzparams * params = vparams;
  static int initialized = 0;

  if (!initialized)
  {
    initialized = 1;
    /* reconstruct the cheri_object */
    stderrfd.co_codecap = co_codecap_stderrfd;
    stderrfd.co_datacap = co_datacap_stderrfd;
    /*fprintf_c(stderrfd, "in invoke(), initialized.\n");*/
  }
  /*fprintf_c(stderrfd, "invoke: op=%d\n", (int) op);*/

  if (op == LZOP_DEFLATEINIT2)
    return deflateInit2_c(params->strm, params);
  else if (op == LZOP_DEFLATE)
    return deflate_c(params->strm, params->flush);
  else if (op == LZOP_DEFLATEEND)
    return deflateEnd(params->strm);
  else if (op == LZOP_DEFLATERESET)
    return deflateReset(params->strm);
  else if (op == LZOP_INFLATEINIT2)
    return inflateInit2_c(params->strm, params);
  else if (op == LZOP_INFLATE)
    return inflate_c(params->strm, params->flush);
  else if (op == LZOP_INFLATEEND)
    return inflateEnd(params->strm);
  else if (op == LZOP_CRC32)
    return crc32_c(params->crc, params->buf, params->len);
  else
  {
    fprintf_c(stderrfd, "invoke: unrecognized op: %d\n", (int) op);
    return -2;
  }

  return 0;
}
