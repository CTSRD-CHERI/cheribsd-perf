#include "zlib.h"
#include "lzsandbox-helper.h"

#include <machine/cheri.h>
#include <machine/cheric.h>
#include <cheri/cheri_fd.h>
#include <cheri/sandbox.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int			 lzsandbox_initialized;
static struct    sandbox_class * sbcp;
static struct    sandbox_object * sbop;
/* stderr for debugging only */
struct cheri_object stderrfd;

#define LZ_SANDBOX_BIN "lzsandbox-helper.bin"
#define LZ_SANDBOX_HEAP_SIZE (4*1048576)

/* if defined, a single sandbox instance is shared between all z_streams */
#define LZ_SINGLE_SANDBOX

/* returns non-zero iff s is a subcapability of b */
/* XXX: ignores permissions */
static int subcap (__capability void * s, __capability void * b);

static int lzsandbox_initialize (z_streamp strm);
static int lzsandbox_invoke (z_streamp strm, int opno, struct lzparams * params);

static int subcap (__capability void * s, __capability void * b)
{
  return
    cheri_getbase(s) >= cheri_getbase(b) &&
    cheri_getbase(s) <= cheri_getbase(b)+cheri_getlen(b) &&
    cheri_getlen(s) <= cheri_getlen(b) - (cheri_getbase(s) - cheri_getbase(b));
}

static int lzsandbox_initialize (z_streamp strm)
{
  if (!lzsandbox_initialized)
  {
    lzsandbox_initialized = 1;

    if (sandbox_class_new(LZ_SANDBOX_BIN, LZ_SANDBOX_HEAP_SIZE, &sbcp))
    {
      fprintf(stderr, "sandbox_class_new %s, %d\n", LZ_SANDBOX_BIN, LZ_SANDBOX_HEAP_SIZE);
      return -1;
    }
  
    /* stderr for debugging only */
    if (cheri_fd_new(STDERR_FILENO, &stderrfd) < 0)
    {
      fprintf(stderr, "cheri_fd_new: STDERR_FILENO\n");
      return -1;
    }
#ifdef LZ_SINGLE_SANDBOX
    if (sandbox_object_new(sbcp, &sbop))
    {
      fprintf(stderr, "sandbox_object_new");
      return -1;
    }
#endif /* LZ_SINGLE_SANDBOX */
  }

#ifndef LZ_SINGLE_SANDBOX
  if (sandbox_object_new(sbcp, &strm->sbop))
  {
    fprintf(stderr, "sandbox_object_new");
    return -1;
  }
#endif /* !LZ_SINGLE_SANDBOX */
  return 0;
}

static int lzsandbox_invoke (z_streamp strm, int opno, struct lzparams * params)
{
#ifdef LZ_SINGLE_SANDBOX
#define local_sbop sbop
#else /* LZ_SINGLE_SANDBOX */
#define local_sbop strm->sbop
#endif /* LZ_SINGLE_SANDBOX */
  return sandbox_object_cinvoke(local_sbop, opno, 
            0, 0, 0, 0, 0, 0, 0,
            stderrfd.co_codecap, stderrfd.co_datacap,
            cheri_ptrperm(params, sizeof(struct lzparams), CHERI_PERM_LOAD | CHERI_PERM_LOAD_CAP), cheri_zerocap(),
            cheri_zerocap(), cheri_zerocap(),
            cheri_zerocap(), cheri_zerocap());
}

int ZEXPORT deflate (z_streamp strm, int flush)
{
  int rc;
  struct lzparams params;

  __capability void * in = cheri_ptrperm(strm->next_in_p, strm->avail_in, CHERI_PERM_LOAD);
  __capability void * out = cheri_ptrperm(strm->next_out_p, strm->avail_out, CHERI_PERM_STORE);

  memset(&params, 0, sizeof params);

  /* lzsandbox-helper and the user app (e.g. gzip) have a different view of z_streamp; this syncs them up */
  strm->next_in_c = in;
  strm->next_out_c = out;

  params.strm = cheri_ptrperm(strm,
    sizeof(z_stream), CHERI_PERM_LOAD | CHERI_PERM_LOAD_CAP | CHERI_PERM_STORE | CHERI_PERM_STORE_CAP | CHERI_PERM_STORE_LOCAL_CAP);
  params.flush = flush;

  rc = lzsandbox_invoke(strm, LZOP_DEFLATE, &params);

  /* restore next_in, next_out */
  strm->next_in_p = (void*)strm->next_in_c;
  strm->next_out_p = (void*)strm->next_out_c;

  /* XXX: not sure how this should work:
   * strm->next_in_c might have been modified to point to
   * some previously passed capability (e.g. an old next_out_c).
   * In this case, the sandbox could (on next invoke) read previous
   * output, etc.
   * Currently we check whether the capability falls within the
   * bounds it should, but this is manual and not nice.
   * Should other strm parameters be checked for sanity?
   */

  /* XXX: even with this check, there's nothing stopping the sandbox
   * from not increasing next_in. That is, we need to ensure
   * progress; otherwise, a sandbox could potentially cause
   * non-termination in the user application.
   */

  if (!subcap(cheri_ptr(strm->next_in_p, strm->avail_in), in) ||
      !subcap(cheri_ptr(strm->next_out_p, strm->avail_out), out))
  {
    fprintf(stderr, "invalid pointer or length returned by sandbox");
    exit(1);
  }

  /* XXX: if these are used by the user application, make them into
   * capabilities.
   */
  strm->msg = NULL;

  return rc;
}

int ZEXPORT inflate (z_streamp strm, int flush)
{
  (void)strm;
  (void)flush;
  return -1;
}

int ZEXPORT deflateInit2_ (z_streamp strm, int  level, int  method,
                           int windowBits, int memLevel,
                           int strategy, const char *version,
                           int stream_size)
{
  int rc;
  struct lzparams params;
  void * next_in = strm->next_in_p;
  void * next_out = strm->next_out_p;
  uInt avail_in = strm->avail_in;
  uInt avail_out = strm->avail_out;
  
  memset(&params, 0, sizeof params);
  lzsandbox_initialize(strm);
  params.strm = cheri_ptrperm(strm,
    sizeof(z_stream), CHERI_PERM_LOAD | CHERI_PERM_LOAD_CAP | CHERI_PERM_STORE); /* XXX: need to sync up cap ptrs? */
  params.level = level;
  params.method = method;
  params.windowBits = windowBits;
  params.memLevel = memLevel;
  params.strategy = strategy;
  params.version = cheri_ptrperm((void*)version, strlen(version)+1, CHERI_PERM_LOAD);
  params.stream_size = stream_size;

  rc = lzsandbox_invoke(strm, LZOP_DEFLATEINIT2, &params);

  /* XXX: see note in deflate() */
  strm->next_in_p = next_in;
  strm->avail_in = avail_in;
  strm->next_out_p = next_out;
  strm->avail_out = avail_out;
  strm->msg = NULL;

  return rc;
}

int ZEXPORT inflateInit2_ (z_streamp strm, int  windowBits,
                           const char *version, int stream_size)
{
  lzsandbox_initialize(strm);
  (void)strm;
  (void)windowBits;
  (void)version;
  (void)stream_size;
  return -1;
}

uLong ZEXPORT crc32 (uLong crc, const Bytef *buf, uInt len)
{
  struct lzparams params;
  memset(&params, 0, sizeof params);
  params.crc = crc;
  params.buf = buf ? cheri_ptrperm(buf, len, CHERI_PERM_LOAD) : buf;
  params.len = len;

  /* create a temporary sandbox, or use existing one;
   * lzsandbox_initialize and lzsandbox_invoke deal with this
   */
  z_stream strm;
  lzsandbox_initialize(&strm);
  return lzsandbox_invoke(&strm, LZOP_CRC32, &params);
  /* XXX: need some way of destroying this sandbox */
}

int ZEXPORT inflateEnd (z_streamp strm)
{
  (void)strm;
  return -1;
}

int ZEXPORT deflateEnd (z_streamp strm)
{
  struct lzparams params;
  memset(&params, 0, sizeof params);
  params.strm = cheri_ptrperm(strm,
    sizeof(z_stream), CHERI_PERM_LOAD | CHERI_PERM_STORE); /* no sync up of cap ptrs; deflateEnd doesn't need them */
  return lzsandbox_invoke(strm, LZOP_DEFLATEEND, &params);
}
