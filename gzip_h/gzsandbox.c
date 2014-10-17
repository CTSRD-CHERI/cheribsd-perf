
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

static int			 gzsandbox_initialized;
static struct    sandbox_class * sbcp;
static struct    sandbox_object * sbop;

void
gzsandbox_test(void);
static void
gzsandbox_initialize(void);
#include "gzsandbox-helper.h"
#include <machine/cheri.h>
#include <machine/cheric.h>
#include <cheri/cheri_fd.h>
#include <cheri/sandbox.h>

#define GZIP_SANDBOX_BIN "gzsandbox-helper.bin"

void
gzsandbox_test(void)
{
  register_t v;
  struct cheri_object stderr_fd;

  printf("initializing...\n");
  gzsandbox_initialize();

  /* create cheri_fd for STDERR */
  if (cheri_fd_new(STDERR_FILENO, &stderr_fd) < 0)
    err(-1, "cheri_fd_new: stderr");
  
  printf("invoking...\n");
  v = sandbox_object_cinvoke(sbop, GZSANDBOX_HELPER_OP_GZCOMPRESS, 
            0, 0, 0, 0, 0, 0, 0,
            stderr_fd.co_codecap, stderr_fd.co_datacap, cheri_zerocap(),
            cheri_zerocap(), cheri_zerocap(), cheri_zerocap(),
            cheri_zerocap(), cheri_zerocap());
  printf("invoked.\n");
}

static void
gzsandbox_initialize(void)
{
	if (gzsandbox_initialized)
		return;
	gzsandbox_initialized = 1;

  if (sandbox_class_new(GZIP_SANDBOX_BIN, 4*1048576, &sbcp))
    err(-1, "sandbox_class_new %s", GZIP_SANDBOX_BIN);

  if (sandbox_object_new(sbcp, &sbop))
    err(-1, "sandbox_object_new");
}

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

