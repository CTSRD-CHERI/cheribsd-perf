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

#ifdef SB_COLLECT_STATS
extern int num_ccalls;
extern int num_sandboxes;
#endif /* SB_COLLECT_STATS */

#ifdef SABI_ONLY
/*
 * The compiled-against zlib uses mabi=sandbox,
 * but we don't, and neither does the user application.
 */

int ZEXPORT deflate (z_streamp strm, int flush)
{
	int rc;
	z_stream_cap strm_cap;
	__capability z_stream_cap *strm_cap_c;
	__capability void *in, *out;

	in = strm->next_in ?
    cheri_ptrperm(strm->next_in, strm->avail_in,
        CHERI_PERM_LOAD | CHERI_PERM_LOAD_CAP) : strm->next_in;
	out = strm->next_out ?
     cheri_ptrperm(strm->next_out, strm->avail_out,
        CHERI_PERM_STORE | CHERI_PERM_STORE_CAP) : strm->next_out;
	strm_cap.next_in = in;
	strm_cap.next_out = out;
	strm_cap.avail_in = strm->avail_in;
	strm_cap.avail_out = strm->avail_out;
	strm_cap.total_in = strm->total_in;
	strm_cap.total_out = strm->total_out;
	strm_cap.state = strm->state;
	strm_cap.data_type = strm->data_type;
	strm_cap.adler = strm->adler;
	strm_cap.reserved = strm->reserved;
	/* XXX: TODO: zalloc,zfree,opaque */
	strm_cap.zalloc = strm->zalloc;
	strm_cap.zfree = strm->zfree;
	strm_cap.opaque = strm->opaque;

	strm_cap_c = cheri_ptr(&strm_cap, sizeof strm_cap);

	rc = deflate_c(strm_cap_c, flush);
  
	strm->next_in = (void*)strm_cap.next_in;
	strm->next_out = (void*)strm_cap.next_out;
	strm->avail_in = strm_cap.avail_in;
	strm->avail_out = strm_cap.avail_out;
	strm->total_in = strm_cap.total_in;
	strm->total_out = strm_cap.total_out;
	strm->state = strm_cap.state;
	strm->data_type = strm_cap.data_type;
	strm->adler = strm_cap.adler;
	strm->reserved = strm_cap.reserved;
	/* XXX: TODO: zalloc,zfree,opaque */
	strm->zalloc = strm_cap.zalloc;
	strm->zfree = strm_cap.zfree;
	strm->opaque = strm_cap.opaque;

	return (rc);
}

int ZEXPORT inflate (z_streamp strm, int flush)
{
	int rc;
	z_stream_cap strm_cap;
	__capability z_stream_cap *strm_cap_c;
	__capability void *in, *out;

	in = strm->next_in ?
    cheri_ptrperm(strm->next_in, strm->avail_in,
        CHERI_PERM_LOAD | CHERI_PERM_LOAD_CAP) : strm->next_in;
	out = strm->next_out ?
     cheri_ptrperm(strm->next_out, strm->avail_out,
        CHERI_PERM_STORE | CHERI_PERM_STORE_CAP) : strm->next_out;
	strm_cap.next_in = in;
	strm_cap.next_out = out;
	strm_cap.avail_in = strm->avail_in;
	strm_cap.avail_out = strm->avail_out;
	strm_cap.total_in = strm->total_in;
	strm_cap.total_out = strm->total_out;
	strm_cap.state = strm->state;
	strm_cap.data_type = strm->data_type;
	strm_cap.adler = strm->adler;
	strm_cap.reserved = strm->reserved;
	/* XXX: TODO: zalloc,zfree,opaque */
	strm_cap.zalloc = strm->zalloc;
	strm_cap.zfree = strm->zfree;
	strm_cap.opaque = strm->opaque;

	strm_cap_c = cheri_ptr(&strm_cap, sizeof strm_cap);

	strm->next_in = (void*)strm_cap.next_in;
	strm->next_out = (void*)strm_cap.next_out;
	rc = inflate_c(strm_cap_c, flush);

	strm->avail_in = strm_cap.avail_in;
	strm->avail_out = strm_cap.avail_out;
	strm->total_in = strm_cap.total_in;
	strm->total_out = strm_cap.total_out;
	strm->state = strm_cap.state;
	strm->data_type = strm_cap.data_type;
	strm->adler = strm_cap.adler;
	strm->reserved = strm_cap.reserved;
	/* XXX: TODO: zalloc,zfree,opaque */
	strm->zalloc = strm_cap.zalloc;
	strm->zfree = strm_cap.zfree;
	strm->opaque = strm_cap.opaque;


	return (rc);
}
int ZEXPORT deflateInit2_ (z_streamp strm, int level, int method,
                           int windowBits, int memLevel,
                           int strategy, const char *version,
                           int stream_size)
{
	int rc;
	z_stream_cap strm_cap;
	struct lzparams params;
	__capability z_stream_cap *strm_cap_c;
	__capability struct lzparams *params_c;
	
	stream_size = sizeof(z_stream_cap);
	memset(&strm_cap, 0, sizeof(strm_cap));
	strm_cap_c = cheri_ptr(&strm_cap, sizeof strm_cap);
	params_c = cheri_ptr(&params, sizeof params);

	params.strm = strm_cap_c;
	params.level = level;
	params.method = method;
	params.windowBits = windowBits;
	params.memLevel = memLevel;
	params.strategy = strategy;
	params.version = cheri_ptr(&version, strlen(version));
	params.stream_size = stream_size;

	rc = inflateInit2_c(strm_cap_c, params_c);

	/* XXX: check that these are the only things that deflateInit2 might want to modify */
	strm->state = strm_cap.state;
	strm->data_type = strm_cap.data_type;
	strm->adler = strm_cap.adler;
	strm->reserved = strm_cap.reserved;
	strm->zalloc = strm_cap.zalloc;
	strm->zfree = strm_cap.zfree;
	strm->opaque = strm_cap.opaque;

	return (rc);
}

int ZEXPORT inflateInit_ (z_streamp strm,
                           const char *version, int stream_size)
{
/* XXX: #define to get this to compile */
#define DEF_WBITS 15
  return inflateInit2_(strm, DEF_WBITS, version, stream_size);
}

int ZEXPORT inflateInit2_ (z_streamp strm, int  windowBits,
                           const char *version, int stream_size)
{
	int rc;
	z_stream_cap strm_cap;
	struct lzparams params;
	__capability z_stream_cap *strm_cap_c;
	__capability struct lzparams *params_c;
	
	stream_size = sizeof(z_stream_cap);
	memset(&strm_cap, 0, sizeof(strm_cap));
	strm_cap_c = cheri_ptr(&strm_cap, sizeof strm_cap);
	params_c = cheri_ptr(&params, sizeof params);

	params.strm = strm_cap_c;
	params.windowBits = windowBits;
	params.version = cheri_ptr(&version, strlen(version));
	params.stream_size = stream_size;

	rc = inflateInit2_c(strm_cap_c, params_c);

	/* XXX: check that these are the only things that deflateInit2 might want to modify */
	strm->state = strm_cap.state;
	strm->data_type = strm_cap.data_type;
	strm->adler = strm_cap.adler;
	strm->reserved = strm_cap.reserved;
	strm->zalloc = strm_cap.zalloc;
	strm->zfree = strm_cap.zfree;
	strm->opaque = strm_cap.opaque;

	return (rc);
}

uLong ZEXPORT crc32 (uLong crc, const Bytef *buf, uInt len)
{
	__capability const Bytef *buf_c;

	buf_c = buf ? cheri_ptr((void*)buf, len) : buf;
	return (crc32_c(crc, buf_c, len));
}

int ZEXPORT inflateEnd (z_streamp strm)
{
	z_stream_cap strm_cap;
	__capability z_stream_cap *strm_cap_c;

	/* XXX: check that these are the only things that inflateEnd might need... */
	strm_cap.next_in = NULL;
	strm_cap.next_out = NULL;
	strm_cap.avail_in = strm->avail_in;
	strm_cap.avail_out = strm->avail_out;
	strm_cap.total_in = strm->total_in;
	strm_cap.total_out = strm->total_out;
	strm_cap.state = strm->state;

	strm_cap_c = cheri_ptr(&strm_cap, sizeof strm_cap);

	return (inflateEnd_c(strm_cap_c));
}

int ZEXPORT deflateEnd (z_streamp strm)
{
	z_stream_cap strm_cap;
	__capability z_stream_cap *strm_cap_c;

	/* XXX: check that these are the only things that deflateEnd might need... */
	strm_cap.next_in = NULL;
	strm_cap.next_out = NULL;
	strm_cap.avail_in = strm->avail_in;
	strm_cap.avail_out = strm->avail_out;
	strm_cap.total_in = strm->total_in;
	strm_cap.total_out = strm->total_out;
	strm_cap.state = strm->state;

	strm_cap_c = cheri_ptr(&strm_cap, sizeof strm_cap);

	return (deflateEnd_c(strm_cap_c));
}

int ZEXPORT deflateReset (z_streamp strm)
{
	z_stream_cap strm_cap;
	__capability z_stream_cap *strm_cap_c;

	/* XXX: check that these are the only things that deflateReset might need... */
	strm_cap.next_in = NULL;
	strm_cap.next_out = NULL;
	strm_cap.avail_in = strm->avail_in;
	strm_cap.avail_out = strm->avail_out;
	strm_cap.total_in = strm->total_in;
	strm_cap.total_out = strm->total_out;
	strm_cap.state = strm->state;

	strm_cap_c = cheri_ptr(&strm_cap, sizeof strm_cap);

	return (deflateReset_c(strm_cap_c));
}
#else /* !SABI_ONLY */

static int			 lzsandbox_initialized;
static struct    sandbox_class * sbcp;
static struct    sandbox_object * sbop;
/* stderr for debugging only */
struct cheri_object stderrfd;

#ifdef NO_CCALL
#define LZ_SANDBOX_BIN "lzsandbox-helper_j.bin"
#else
#define LZ_SANDBOX_BIN "lzsandbox-helper_u.bin"
#endif
#define LZ_SANDBOX_HEAP_SIZE (4*1048576)

/* returns non-zero iff s is a subcapability of b */
/* XXX: ignores permissions */
static int subcap (__capability void * s, __capability void * b);

static int lzsandbox_initialize (z_streamp strm);
static int lzsandbox_invoke (z_streamp strm, int opno,
  struct lzparams * params);

static int subcap (__capability void * s, __capability void * b)
{
  return
    ((void*)s == NULL && (void*)b == NULL) || (
    cheri_getbase(s) >= cheri_getbase(b) &&
    cheri_getbase(s) <= cheri_getbase(b)+cheri_getlen(b) &&
    cheri_getlen(s) <= cheri_getlen(b) -
      (cheri_getbase(s) - cheri_getbase(b)));
}

static int lzsandbox_initialize (z_streamp strm)
{
  if (!lzsandbox_initialized)
  {
    lzsandbox_initialized = 1;

    if (sandbox_class_new(LZ_SANDBOX_BIN,
          LZ_SANDBOX_HEAP_SIZE, &sbcp))
    {
      fprintf(stderr, "sandbox_class_new %s, %d\n",
        LZ_SANDBOX_BIN, LZ_SANDBOX_HEAP_SIZE);
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
#ifdef SB_COLLECT_STATS
    num_sandboxes++;
#endif /* SB_COLLECT_STATS */
#endif /* LZ_SINGLE_SANDBOX */
  }

#ifndef LZ_SINGLE_SANDBOX
  if (!strm->sbop)
  {
    if (sandbox_object_new(sbcp, &strm->sbop))
    {
      fprintf(stderr, "sandbox_object_new");
      return -1;
    }
#ifdef SB_COLLECT_STATS
    num_sandboxes++;
#endif /* SB_COLLECT_STATS */
  }
#endif /* !LZ_SINGLE_SANDBOX */
  return 0;
}

static int lzsandbox_invoke (z_streamp strm, int opno,
  struct lzparams * params)
{
#ifdef LZ_SINGLE_SANDBOX
#define local_sbop sbop
#else /* LZ_SINGLE_SANDBOX */
#define local_sbop strm->sbop
#endif /* LZ_SINGLE_SANDBOX */

#ifdef SB_COLLECT_STATS
  num_ccalls++;
#endif /* SB_COLLECT_STATS */
  
  return sandbox_object_cinvoke(local_sbop, opno, 
            0, 0, 0, 0, 0, 0, 0,
            stderrfd.co_codecap, stderrfd.co_datacap,
            cheri_ptrperm(params, sizeof(struct lzparams),
                CHERI_PERM_LOAD | CHERI_PERM_LOAD_CAP),
            cheri_zerocap(),
            cheri_zerocap(), cheri_zerocap(),
            cheri_zerocap(), cheri_zerocap());
}

int ZEXPORT deflate (z_streamp strm, int flush)
{
  int rc;
  struct lzparams params;
  z_stream_cap strm_cap;
  void * sbop = strm->sbop;

  /*XXX: need CHERI_PERM_LOAD_CAP and CHERI_PERM_STORE_CAP for
   * memcpy_c in zlib due to current CLC/CSC semantics that require
   * the permission regardless of the tag bit status.
   */
  __capability void * in = strm->next_in ?
    cheri_ptrperm(strm->next_in, strm->avail_in,
        CHERI_PERM_LOAD | CHERI_PERM_LOAD_CAP) : strm->next_in;
  __capability void * out = strm->next_out ?
     cheri_ptrperm(strm->next_out, strm->avail_out,
        CHERI_PERM_STORE | CHERI_PERM_STORE_CAP) : strm->next_out;

  memset(&params, 0, sizeof params);
  memset(&strm_cap, 0, sizeof strm_cap);

  /* lzsandbox-helper and the user app (e.g. gzip) have a different
   * view of z_streamp; this syncs them up.
   */
  strm_cap.next_in = in;
  strm_cap.next_out = out;
  strm_cap.avail_in = strm->avail_in;
  strm_cap.avail_out = strm->avail_out;
  strm_cap.total_in = strm->total_in;
  strm_cap.total_out = strm->total_out;
  strm_cap.state = strm->state;
  strm_cap.data_type = strm->data_type;
  strm_cap.adler = strm->adler;
  strm_cap.reserved = strm->reserved;
  /* XXX: TODO: zalloc,zfree,opaque */
  strm_cap.zalloc = strm->zalloc;
  strm_cap.zfree = strm->zfree;
  strm_cap.opaque = strm->opaque;

  params.strm = cheri_ptrperm(&strm_cap, sizeof strm_cap,
    CHERI_PERM_LOAD | CHERI_PERM_LOAD_CAP |
    CHERI_PERM_STORE | CHERI_PERM_STORE_CAP |
    CHERI_PERM_STORE_LOCAL_CAP);
  params.flush = flush;

  rc = lzsandbox_invoke(strm, LZOP_DEFLATE, &params);

  /* restore next_in, next_out */
  strm->next_in = (void*)strm_cap.next_in;
  strm->next_out = (void*)strm_cap.next_out;
  strm->avail_in = strm_cap.avail_in;
  strm->avail_out = strm_cap.avail_out;
  strm->total_in = strm_cap.total_in;
  strm->total_out = strm_cap.total_out;
  strm->state = strm_cap.state;
  strm->data_type = strm_cap.data_type;
  strm->adler = strm_cap.adler;
  strm->reserved = strm_cap.reserved;
  /* XXX: TODO: zalloc,zfree,opaque */
  strm->zalloc = strm_cap.zalloc;
  strm->zfree = strm_cap.zfree;
  strm->opaque = strm_cap.opaque;

  /* XXX: not sure how this should work:
   *
   * strm->next_in_c might have been modified to point to
   * some previously passed capability (e.g. an old next_out_c).
   *
   * In this case, the sandbox could (on next invoke) read previous
   * output, etc.
   *
   * Currently we check whether the capability falls within the bounds
   * it should, but this is manual and not nice.
   *
   * Should other strm parameters be checked for sanity?
   */

  /* XXX: even with this check, there's nothing stopping the sandbox
   * from not increasing next_in. That is, we need to ensure progress;
   * otherwise, a sandbox could potentially cause non-termination
   * in the user application.
   */

  if (!subcap(strm->next_in ? cheri_ptr(strm->next_in, strm->avail_in) : strm->next_in, in) ||
      !subcap(strm->next_out ? cheri_ptr(strm->next_out, strm->avail_out) : strm->next_out, out))
  {
    fprintf(stderr, "invalid pointer or length returned by sandbox");
    exit(1);
  }

  return rc;
}

int ZEXPORT inflate (z_streamp strm, int flush)
{
  int rc;
  struct lzparams params;
  z_stream_cap strm_cap;
  void * sbop = strm->sbop;

  __capability void * in = strm->next_in ?
    cheri_ptrperm(strm->next_in, strm->avail_in,
        CHERI_PERM_LOAD | CHERI_PERM_LOAD_CAP) : strm->next_in;
  __capability void * out = strm->next_out ?
     cheri_ptrperm(strm->next_out, strm->avail_out,
        CHERI_PERM_STORE | CHERI_PERM_STORE_CAP) : strm->next_out;

  memset(&params, 0, sizeof params);
  memset(&strm_cap, 0, sizeof strm_cap);

  strm_cap.next_in = in;
  strm_cap.next_out = out;
  strm_cap.avail_in = strm->avail_in;
  strm_cap.avail_out = strm->avail_out;
  strm_cap.total_in = strm->total_in;
  strm_cap.total_out = strm->total_out;
  strm_cap.state = strm->state;
  strm_cap.data_type = strm->data_type;
  strm_cap.adler = strm->adler;
  strm_cap.reserved = strm->reserved;
  /* XXX: TODO: zalloc,zfree,opaque */
  strm_cap.zalloc = strm->zalloc;
  strm_cap.zfree = strm->zfree;
  strm_cap.opaque = strm->opaque;

  params.strm = cheri_ptrperm(&strm_cap, sizeof strm_cap,
    CHERI_PERM_LOAD | CHERI_PERM_LOAD_CAP |
    CHERI_PERM_STORE | CHERI_PERM_STORE_CAP |
    CHERI_PERM_STORE_LOCAL_CAP);
  params.flush = flush;

  rc = lzsandbox_invoke(strm, LZOP_INFLATE, &params);

  /* restore next_in, next_out */
  strm->next_in = (void*)strm_cap.next_in;
  strm->next_out = (void*)strm_cap.next_out;
  strm->avail_in = strm_cap.avail_in;
  strm->avail_out = strm_cap.avail_out;
  strm->total_in = strm_cap.total_in;
  strm->total_out = strm_cap.total_out;
  strm->state = strm_cap.state;
  strm->data_type = strm_cap.data_type;
  strm->adler = strm_cap.adler;
  strm->reserved = strm_cap.reserved;
  /* XXX: TODO: zalloc,zfree,opaque */
  strm->zalloc = strm_cap.zalloc;
  strm->zfree = strm_cap.zfree;
  strm->opaque = strm_cap.opaque;

  if (!subcap(strm->next_in ? cheri_ptr(strm->next_in, strm->avail_in) : strm->next_in, in) ||
      !subcap(strm->next_out ? cheri_ptr(strm->next_out, strm->avail_out) : strm->next_out, out))
  {
    fprintf(stderr, "invalid pointer or length returned by sandbox");
    exit(1);
  }

  return rc;
}

int ZEXPORT deflateInit2_ (z_streamp strm, int level, int method,
                           int windowBits, int memLevel,
                           int strategy, const char *version,
                           int stream_size)
{
  int rc;
  struct lzparams params;
  z_stream_cap strm_cap;
  void * next_in = strm->next_in;
  void * next_out = strm->next_out;
  uInt avail_in = strm->avail_in;
  uInt avail_out = strm->avail_out;
  void * sbop;

  stream_size = sizeof(z_stream_cap);

#ifdef SB_LIBZ_EXT_ALLOC
  wrap_alloc_fn(strm);
#endif /* SB_LIBZ_EXT_ALLOC */
  
  memset(&params, 0, sizeof params);
  memset(&strm_cap, 0, sizeof strm_cap);
  lzsandbox_initialize(strm);
  sbop = strm->sbop;
  params.strm = cheri_ptrperm(&strm_cap, sizeof strm_cap,
    CHERI_PERM_LOAD | CHERI_PERM_LOAD_CAP | CHERI_PERM_STORE | CHERI_PERM_STORE_CAP);
  
  params.level = level;
  params.method = method;
  params.windowBits = windowBits;
  params.memLevel = memLevel;
  params.strategy = strategy;
  params.version = cheri_ptrperm((void*)version, strlen(version)+1,
    CHERI_PERM_LOAD);
  params.stream_size = stream_size;

  rc = lzsandbox_invoke(strm, LZOP_DEFLATEINIT2, &params);

  /* XXX: check that these are the only things that deflateInit2 might want to modify */
  strm->state = strm_cap.state;
  strm->data_type = strm_cap.data_type;
  strm->adler = strm_cap.adler;
  strm->reserved = strm_cap.reserved;
  strm->zalloc = strm_cap.zalloc;
  strm->zfree = strm_cap.zfree;
  strm->opaque = strm_cap.opaque;

#ifdef SB_LIBZ_EXT_ALLOC
  restore_alloc_fn(strm);
#endif /* SB_LIBZ_EXT_ALLOC */

  return rc;
}

int ZEXPORT inflateInit_ (z_streamp strm,
                           const char *version, int stream_size)
{
/* XXX: #define to get this to compile */
#define DEF_WBITS 15
  return inflateInit2_(strm, DEF_WBITS, version, stream_size);
}

int ZEXPORT inflateInit2_ (z_streamp strm, int  windowBits,
                           const char *version, int stream_size)
{
  int rc;
  struct lzparams params;
  z_stream_cap strm_cap;
  void * next_in = strm->next_in;
  void * next_out = strm->next_out;
  uInt avail_in = strm->avail_in;
  uInt avail_out = strm->avail_out;
  void * sbop;

  stream_size = sizeof(z_stream_cap);

#ifdef SB_LIBZ_EXT_ALLOC
  wrap_alloc_fn(strm);
#endif /* SB_LIBZ_EXT_ALLOC */
  
  memset(&params, 0, sizeof params);
  memset(&strm_cap, 0, sizeof strm_cap);
  lzsandbox_initialize(strm);
  sbop = strm->sbop;
  params.strm = cheri_ptrperm(&strm_cap, sizeof strm_cap,
    CHERI_PERM_LOAD | CHERI_PERM_LOAD_CAP | CHERI_PERM_STORE | CHERI_PERM_STORE_CAP);
  
  params.windowBits = windowBits;
  params.version = cheri_ptrperm((void*)version, strlen(version)+1,
    CHERI_PERM_LOAD);
  params.stream_size = stream_size;

  rc = lzsandbox_invoke(strm, LZOP_INFLATEINIT2, &params);

  strm->state = strm_cap.state;
  strm->data_type = strm_cap.data_type;
  strm->adler = strm_cap.adler;
  strm->reserved = strm_cap.reserved;
  strm->zalloc = strm_cap.zalloc;
  strm->zfree = strm_cap.zfree;
  strm->opaque = strm_cap.opaque;

#ifdef SB_LIBZ_EXT_ALLOC
  restore_alloc_fn(strm);
#endif /* SB_LIBZ_EXT_ALLOC */

  return rc;
}

uLong ZEXPORT crc32 (uLong crc, const Bytef *buf, uInt len)
{
  uLong crc_result;
  struct lzparams params;
  memset(&params, 0, sizeof params);
  params.crc = crc;
  params.crc_result = cheri_ptrperm(&crc_result, sizeof crc_result, CHERI_PERM_STORE);
  params.buf = buf ? cheri_ptrperm((void*)buf, len,
    CHERI_PERM_LOAD) : buf;
  params.len = len;
  params.strm = NULL;

  /* create a temporary sandbox, or use existing one;
   * lzsandbox_initialize and lzsandbox_invoke deal with this
   */
  static int initialized = 0;
  static z_stream strm;
  if (!initialized)
  {
    memset(&strm, 0, sizeof strm);
    lzsandbox_initialize(&strm);
    initialized = 1;
  }
  /* XXX: need some way of destroying this sandbox */
  lzsandbox_invoke(&strm, LZOP_CRC32, &params);
  return crc_result;
}

int ZEXPORT inflateEnd (z_streamp strm)
{
  struct lzparams params;
  z_stream_cap strm_cap;
  memset(&params, 0, sizeof params);
  memset(&strm_cap, 0, sizeof strm_cap);
  /* XXX: check that these are the only things that inflateEnd might need... */
  strm_cap.next_in = NULL;
  strm_cap.next_out = NULL;
  strm_cap.avail_in = strm->avail_in;
  strm_cap.avail_out = strm->avail_out;
  strm_cap.total_in = strm->total_in;
  strm_cap.total_out = strm->total_out;
  strm_cap.state = strm->state;
  /* XXX: zalloc, zfree, opaque */
  strm_cap.zalloc = strm->zalloc;
  strm_cap.zfree = strm->zfree;
  strm_cap.opaque = strm->opaque;
  strm_cap.data_type = strm->data_type;
  strm_cap.adler = strm->adler;
  strm_cap.reserved = strm->reserved;
  params.strm = cheri_ptrperm(&strm_cap,
    sizeof(z_stream_cap), CHERI_PERM_LOAD | CHERI_PERM_STORE | CHERI_PERM_LOAD_CAP | CHERI_PERM_STORE_CAP);
  return lzsandbox_invoke(strm, LZOP_INFLATEEND, &params);
}

int ZEXPORT deflateEnd (z_streamp strm)
{
  struct lzparams params;
  z_stream_cap strm_cap;
  memset(&params, 0, sizeof params);
  memset(&strm_cap, 0, sizeof strm_cap);
  /* XXX: check that these are the only things that deflateEnd might need... */
  strm_cap.next_in = NULL;
  strm_cap.next_out = NULL;
  strm_cap.avail_in = strm->avail_in;
  strm_cap.avail_out = strm->avail_out;
  strm_cap.total_in = strm->total_in;
  strm_cap.total_out = strm->total_out;
  strm_cap.state = strm->state;
  /* XXX: zalloc, zfree, opaque */
  strm_cap.zalloc = strm->zalloc;
  strm_cap.zfree = strm->zfree;
  strm_cap.opaque = strm->opaque;
  strm_cap.data_type = strm->data_type;
  strm_cap.adler = strm->adler;
  strm_cap.reserved = strm->reserved;
  params.strm = cheri_ptrperm(&strm_cap,
    sizeof(z_stream_cap), CHERI_PERM_LOAD | CHERI_PERM_STORE | CHERI_PERM_LOAD_CAP | CHERI_PERM_STORE_CAP);
  return lzsandbox_invoke(strm, LZOP_DEFLATEEND, &params);
}

int ZEXPORT deflateReset (z_streamp strm)
{
  struct lzparams params;
  z_stream_cap strm_cap;
  memset(&params, 0, sizeof params);
  memset(&strm_cap, 0, sizeof strm_cap);
  /* XXX: check that these are the only things that deflateEnd might need... */
  strm_cap.next_in = NULL;
  strm_cap.next_out = NULL;
  strm_cap.avail_in = strm->avail_in;
  strm_cap.avail_out = strm->avail_out;
  strm_cap.total_in = strm->total_in;
  strm_cap.total_out = strm->total_out;
  strm_cap.state = strm->state;
  /* XXX: zalloc, zfree, opaque */
  strm_cap.zalloc = strm->zalloc;
  strm_cap.zfree = strm->zfree;
  strm_cap.opaque = strm->opaque;
  strm_cap.data_type = strm->data_type;
  strm_cap.adler = strm->adler;
  strm_cap.reserved = strm->reserved;
  params.strm = cheri_ptrperm(&strm_cap,
    sizeof(z_stream_cap), CHERI_PERM_LOAD | CHERI_PERM_STORE | CHERI_PERM_LOAD_CAP | CHERI_PERM_STORE_CAP);
  return lzsandbox_invoke(strm, LZOP_DEFLATERESET, &params);
}
#endif /* SABI_ONLY */
