
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
gzsandbox_test(int infd_fileno, int outfd_fileno);
static void
gzsandbox_initialize(void);
#include "gzsandbox-helper.h"
#include <machine/cheri.h>
#include <machine/cheric.h>
#include <cheri/cheri_fd.h>
#include <cheri/sandbox.h>

#define GZIP_SANDBOX_BIN "gzsandbox-helper.bin"

void
gzsandbox_test(int infd_fileno, int outfd_fileno)
{
  register_t v;
  struct cheri_object infd, outfd, stderrfd;

  printf("initializing...\n");
  gzsandbox_initialize();

  /* create cheri_fd objects */
  if (cheri_fd_new(STDERR_FILENO, &stderrfd) < 0)
    err(-1, "cheri_fd_new: STDERR_FILENO");

  if (cheri_fd_new(infd_fileno, &infd) < 0)
    err(-1, "cheri_fd_new: infd_fileno");
  
  if (cheri_fd_new(outfd_fileno, &outfd) < 0)
    err(-1, "cheri_fd_new: outfd_fileno");
  
  printf("invoking...\n");
  v = sandbox_object_cinvoke(sbop, GZSANDBOX_HELPER_OP_GZCOMPRESS, 
            0, 0, 0, 0, 0, 0, 0,
            infd.co_codecap, infd.co_datacap,
            outfd.co_codecap, outfd.co_datacap,
            stderrfd.co_codecap, stderrfd.co_datacap,
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

