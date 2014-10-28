#include "zlib.h"
#include "lzsandbox-helper.h"

#include <machine/cheri.h>
#include <machine/cheric.h>
#include <cheri/cheri_fd.h>
#include <cheri/sandbox.h>

#include <unistd.h>
#include <stdio.h>

static int			 lzsandbox_initialized;
static struct    sandbox_class * sbcp;
static struct    sandbox_object * sbop;
/* XXX: stderr for debugging only */
struct cheri_object stderrfd;

#define LZ_SANDBOX_BIN "lzsandbox-helper.bin"
#define LZ_SANDBOX_HEAP_SIZE (4*1048576)

/* if defined, a single sandbox instance is shared between all z_streams */
#define LZ_SINGLE_SANDBOX

static int lzsandbox_initialize (z_streamp strm);
static int lzsandbox_invoke (z_streamp strm, int opno, struct lzparams * params);

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
  
    /* XXX: stderr for debugging only */
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
            cheri_zerocap(), cheri_zerocap(),
            cheri_ptrperm(params, sizeof(struct lzparams), CHERI_PERM_LOAD | CHERI_PERM_LOAD_CAP), cheri_zerocap(),
            cheri_zerocap(), cheri_zerocap(),
            cheri_zerocap(), cheri_zerocap());
}

int ZEXPORT deflate (z_streamp strm, int flush)
{
  struct lzparams params;
  /* lzsandbox-helper and the user app (e.g. gzip) have a different view of z_streamp; this syncs them up */
  strm->next_in_c = cheri_ptrperm(strm->next_in_p, strm->avail_in, CHERI_PERM_LOAD);
  strm->next_out_c = cheri_ptrperm(strm->next_out_p, strm->avail_out, CHERI_PERM_STORE);
  params.strm = cheri_ptrperm(strm,
    sizeof(z_streamp), CHERI_PERM_LOAD | CHERI_PERM_LOAD_CAP);
  params.flush = flush;
  return lzsandbox_invoke(strm, LZOP_DEFLATE, &params);
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
  lzsandbox_initialize(strm);
  (void)strm;
  (void)level;
  (void)method;
  (void)windowBits;
  (void)memLevel;
  (void)strategy;
  (void)version;
  (void)stream_size;
  return -1;
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
  (void)crc;
  (void)buf;
  (void)len;
  return -1;
}

int ZEXPORT inflateEnd (z_streamp strm)
{
  (void)strm;
  return -1;
}

int ZEXPORT deflateEnd (z_streamp strm)
{
  (void)strm;
  return -1;
}
