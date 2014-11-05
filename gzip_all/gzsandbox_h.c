
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

#ifdef SB_COLLECT_STATS
extern int num_ccalls;
extern int num_sandboxes;
#endif /* SB_COLLECT_STATS */

static int			 gzsandbox_initialized;
static struct    sandbox_class * sbcp;
static struct    sandbox_object * sbop;

#include "gzsandbox-helper_h.h"
#include <machine/cheri.h>
#include <machine/cheric.h>
#include <cheri/cheri_fd.h>
#include <cheri/sandbox.h>

#define GZIP_SANDBOX_BIN "gzsandbox-helper_h.bin"

static void
gzsandbox_initialize(void);

static void
gzsandbox_initialize(void)
{
  struct cheri_object stderrfd;
  struct gz_init_params params;

	if (gzsandbox_initialized)
		return;
	gzsandbox_initialized = 1;

  if (sandbox_class_new(GZIP_SANDBOX_BIN, 4*1048576, &sbcp))
    err(-1, "sandbox_class_new %s", GZIP_SANDBOX_BIN);

  if (sandbox_object_new(sbcp, &sbop))
    err(-1, "sandbox_object_new");
#ifdef SB_COLLECT_STATS
    num_sandboxes++;
#endif /* SB_COLLECT_STATS */

  if (cheri_fd_new(STDERR_FILENO, &stderrfd) < 0)
    err(-1, "cheri_fd_new: STDERR_FILENO");

  params.numflag = numflag;
  params.nflag = nflag;
  params.qflag = qflag;
  params.tflag = tflag;
#ifdef DYNAMIC_BUFLEN
  params.BUFLEN = BUFLEN;
#endif /* DYNAMIC_BUFLEN */

#ifdef SB_COLLECT_STATS
  num_ccalls++;
#endif /* SB_COLLECT_STATS */
  
  if (sandbox_object_cinvoke(sbop, GZSANDBOX_HELPER_OP_INIT, 
            0, 0, 0, 0, 0, 0, 0,
            stderrfd.co_codecap, stderrfd.co_datacap,
            cheri_ptrperm(&params, sizeof params, CHERI_PERM_LOAD),
#ifdef SB_COLLECT_STATS
            cheri_ptrperm(&num_ccalls, sizeof num_ccalls, CHERI_PERM_LOAD | CHERI_PERM_STORE),
#else /* SB_COLLECT_STATS */
            cheri_zerocap(),
#endif /* SB_COLLECT_STATS */
            cheri_zerocap(), cheri_zerocap(),
            cheri_zerocap(), cheri_zerocap()))
    err(-1, "sandbox_object_cinvoke");
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
  gzsandbox_initialize();
  struct gz_params params;
  memset(&params, 0, sizeof params);
  if (cheri_fd_new(in, &params.infd) < 0)
    err(-1, "cheri_fd_new: in");
  if (cheri_fd_new(out, &params.outfd) < 0)
    err(-1, "cheri_fd_new: out");
  params.gsizep = cheri_ptrperm(gsizep, sizeof *gsizep, CHERI_PERM_STORE);
  params.origname = cheri_ptrperm((void*)origname, strlen(origname)+1, CHERI_PERM_LOAD);
  params.mtime = mtime;
#ifdef SB_COLLECT_STATS
  num_ccalls++;
#endif /* SB_COLLECT_STATS */
  return sandbox_object_cinvoke(sbop, GZSANDBOX_HELPER_OP_GZCOMPRESS, 
            0, 0, 0, 0, 0, 0, 0,
            cheri_zerocap(), cheri_zerocap(),
            cheri_ptrperm(&params, sizeof params, CHERI_PERM_LOAD | CHERI_PERM_LOAD_CAP), cheri_zerocap(),
            cheri_zerocap(), cheri_zerocap(),
            cheri_zerocap(), cheri_zerocap());
}

