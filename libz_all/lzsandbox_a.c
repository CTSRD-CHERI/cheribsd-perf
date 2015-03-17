#ifdef SB_LIBZ_CAPSICUM

#include <sys/cdefs.h>

#include <sys/types.h>
#include <sys/capability.h>
#include <sys/uio.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <libcapsicum8.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "zlib.h"

int	lzsandbox(void *);

#ifdef LZ_SINGLE_SANDBOX
#define local_lcsp	lcsp
#else
#define local_lcsp	strm->lcsp
#endif

#define	PROXIED_LZ_DEFLATEINIT2	1
#define	PROXIED_LZ_DEFLATE	2
#define	PROXIED_LZ_DEFLATEEND	3
#define	PROXIED_LZ_CRC32	4

static struct lc_sandbox	*lcsp;
static int			lzsandbox_initialized;
static int			lzsandbox_enabled;

static void
lzsandbox_initialize(z_streamp strm)
{
#ifdef LZ_SINGLE_SANDBOX
	if (lzsandbox_initialized)
		return;
	lzsandbox_initialized = 1;

	if (lch_startfn(lzsandbox, NULL,
	    LCH_PERMIT_STDERR, &lcsp) < 0)
		err(-1, "lch_startfn");
#else /* !LZ_SINGLE_SANDBOX */
	if (strm->lcsp == NULL) {
		if (lch_startfn(lzsandbox, NULL,
		    LCH_PERMIT_STDERR, &strm->lcsp) < 0)
			err(-1, "lch_startfn");
	}
#endif /* LZ_SINGLE_SANDBOX */
}

struct host_lz_deflate_req {
	z_stream	hzc_req_strm;
	int		hzc_req_flush;
} __packed;

struct host_lz_deflate_rep {
	z_stream	hzc_rep_strm;
	int		hzc_rep_retval;
} __packed;

static int 
lz_deflate_insandbox(z_streamp strm, int flush)
{
	struct host_lz_deflate_req req;
	struct host_lz_deflate_rep rep;
	struct host_rpc_params params;
	struct iovec iov_req[2], iov_rep[2];
	int iovcount;
	size_t len, repsz;

	iovcount = 0;
	bzero(&req, sizeof(req));
	memcpy(&req.hzc_req_strm, strm, sizeof(z_stream));
	req.hzc_req_flush = flush;
	iov_req[0].iov_base = &req;
	iov_req[0].iov_len = sizeof(req);
	iov_rep[0].iov_base = &rep;
	iov_rep[0].iov_len = sizeof(rep);
	iovcount++;
	repsz += sizeof(rep);
#ifndef GZ_SHMEM
	iov_req[1].iov_base = strm->next_in;
	iov_req[1].iov_len = strm->avail_in;
	iov_rep[1].iov_base = strm->next_out;
	iov_rep[1].iov_len = strm->avail_out;
	iovcount++;
	repsz += strm->avail_out;
#endif
	params.scb = local_lcsp;
	params.opno = PROXIED_LZ_DEFLATE;
	params.req = iov_req;
	params.reqcount = iovcount;
	params.req_fdp = NULL;
	params.req_fdcount = 0;
	params.rep = iov_rep;
	params.repcount = iovcount;
	params.replenp = &len;
	params.rep_fdp = NULL;
	params.rep_fdcountp = NULL;
	if (lch_rpc_fix(&params) < 0)
		err(-1, "lch_rpc");
	if (len != repsz)
		errx(-1, "lch_rpc len %zu", len);

	/* XXX: TODO: a check on strm to make sure sandbox hasn't caused buffer overflow */
	/* NOTE: sandbox never gets write access to next_in's contents, but can still cause damage */
	memcpy(strm, &rep.hzc_rep_strm, sizeof(z_stream));

	return (rep.hzc_rep_retval);
}

static void
sandbox_lz_deflate_buffer(struct lc_host *lchp, uint32_t opno,
    uint32_t seqno, char *buffer, size_t len)
{
	struct host_lz_deflate_req req;
	struct host_lz_deflate_rep rep;
	Bytef *next_in, *next_out, *host_next_in, *host_next_out;
	struct iovec iov[2];
	int iovcount;
	size_t avail_in, avail_out;

	iovcount = 0;
	if (len < sizeof(req))
		err(-1, "sandbox_lz_deflate_buffer: len %zu", len);

	bcopy(buffer, &req, sizeof(req));
	
#ifndef GZ_SHMEM
	if (len != sizeof(req) + req.hzc_req_strm.avail_in)
		err(-1, "sandbox_lz_deflate_buffer: len %zu", len);

	avail_out = req.hzc_req_strm.avail_out;
	next_in = (Bytef *)buffer + sizeof(req);

	/*
	 * XXX: Expensive! (Maybe do this once if possible?)
	 * Allocate output buffer for deflate().
	 */
	next_out = malloc(avail_out);
	if (!next_out)
		err(-1, "malloc: %zu", avail_out);
	/*
	 * XXX: could bzero next_out, but trust is one way so we don't
	 * care about leaking info here.
	 */

	/*
	 * deflate() indicates how far it's moved along in the next_in
	 * buffer by incrementing the pointer. Therefore, we need to
	 * figure out the corresponding difference on the host's end
	 * after the call to deflate.
	 */
	host_next_in = req.hzc_req_strm.next_in;
	host_next_out = req.hzc_req_strm.next_out;

	req.hzc_req_strm.next_in = next_in;
	req.hzc_req_strm.next_out = next_out;
#endif /* !GZ_SHMEM */

	bzero(&rep, sizeof(rep));
	rep.hzc_rep_retval = zlib_deflate(&req.hzc_req_strm,
	    req.hzc_req_flush);
	iov[0].iov_base = &rep;
	iov[0].iov_len = sizeof(rep);
	iovcount++;
	
#ifndef GZ_SHMEM
	iov[1].iov_base = next_out;
	iov[1].iov_len = avail_out;
	iovcount++;
#endif /* !GZ_SHMEM */

	memcpy(&rep.hzc_rep_strm, &req.hzc_req_strm, sizeof(z_stream));

#ifndef GZ_SHMEM
	/* Fix up the pointers. */
	rep.hzc_rep_strm.next_in = host_next_in +
	    (req.hzc_req_strm.next_in - next_in);
	rep.hzc_rep_strm.next_out = host_next_out +
	    (req.hzc_req_strm.next_out - next_out);
#endif /* !GZ_SHMEM */


	if (lcs_sendrpc(lchp, opno, seqno, iov, iovcount) < 0) {
#ifndef GZ_SHMEM
		free(next_out);
#endif
		err(-1, "lcs_sendrpc");
	}
#ifndef GZ_SHMEM
	free(next_out);
#endif
}

int
deflate_wrapper(z_streamp strm, int flush)
{

	lzsandbox_initialize(strm);
	return (lz_deflate_insandbox(strm, flush));
}

struct host_lz_deflateInit2_req {
	z_stream	hzc_req_strm;
	int	hzc_req_level;
	int	hzc_req_method;
	int	hzc_req_windowBits;
	int	hzc_req_memLevel;
	int	hzc_req_strategy;
	int	hzc_req_stream_size;
} __packed;

struct host_lz_deflateInit2_rep {
	z_stream	hzc_rep_strm;
	int		hzc_rep_retval;
} __packed;

static int 
lz_deflateInit2_insandbox(z_streamp strm, int level, int method,
    int windowBits, int memLevel, int strategy, const char *version,
    int stream_size)
{
	struct host_lz_deflateInit2_req req;
	struct host_lz_deflateInit2_rep rep;
	struct host_rpc_params params;
	struct iovec iov_req[2], iov_rep[2];
	size_t len;

	bzero(&req, sizeof(req));
	memcpy(&req.hzc_req_strm, strm, sizeof(z_stream));
	req.hzc_req_level = level;
	req.hzc_req_method = method;
	req.hzc_req_windowBits = windowBits;
	req.hzc_req_memLevel = memLevel;
	req.hzc_req_strategy = strategy;
	req.hzc_req_stream_size = stream_size;
	iov_req[0].iov_base = &req;
	iov_req[0].iov_len = sizeof(req);
	iov_req[1].iov_base = (void *)version;
	iov_req[1].iov_len = strlen(version) + 1;
	iov_rep[0].iov_base = &rep;
	iov_rep[0].iov_len = sizeof(rep);
	params.scb = local_lcsp;
	params.opno = PROXIED_LZ_DEFLATEINIT2;
	params.req = iov_req;
	params.reqcount = 2;
	params.req_fdp = NULL;
	params.req_fdcount = 0;
	params.rep = iov_rep;
	params.repcount = 1;
	params.replenp = &len;
	params.rep_fdp = NULL;
	params.rep_fdcountp = NULL;
	if (lch_rpc_fix(&params) < 0)
		err(-1, "lch_rpc");
	if (len != sizeof(rep))
		errx(-1, "lch_rpc len %zu", len);

	/*
	 * XXX: TODO: A check on strm to make sure sandbox hasn't caused
	 * buffer overflow. */
	memcpy(strm, &rep.hzc_rep_strm, sizeof(z_stream));

	return (rep.hzc_rep_retval);
}

static void
sandbox_lz_deflateInit2_buffer(struct lc_host *lchp, uint32_t opno,
    uint32_t seqno, char *buffer, size_t len)
{
	struct host_lz_deflateInit2_req req;
	struct host_lz_deflateInit2_rep rep;
	Bytef * next_in;
	struct iovec iov[2];
	const char * version;

	next_in = (Bytef *)buffer + sizeof(req);
	if (len < sizeof(req))
		err(-1, "sandbox_lz_deflateInit2_buffer: len %zu", len);

	bcopy(buffer, &req, sizeof(req));

	/*
	 * The version argument to deflateInit2 is located after
	 * req hdr.
	 */
	if (buffer[len - 1] != 0)
		err(-1, "sandbox_lz_deflateInit2_buffer: "
		    "string not terminated");
	version = buffer + sizeof(req);

	bzero(&rep, sizeof(rep));
	rep.hzc_rep_retval = zlib_deflateInit2_(&req.hzc_req_strm,
	    req.hzc_req_level, req.hzc_req_method, req.hzc_req_windowBits,
	    req.hzc_req_memLevel, req.hzc_req_strategy, version,
	    req.hzc_req_stream_size);
	iov[0].iov_base = &rep;
	iov[0].iov_len = sizeof(rep);

	memcpy(&rep.hzc_rep_strm, &req.hzc_req_strm, sizeof(z_stream));

	if (lcs_sendrpc(lchp, opno, seqno, iov, 1) < 0)
		err(-1, "lcs_sendrpc");
}

int
deflateInit2_wrapper(z_streamp strm, int level, int method,
    int windowBits, int memLevel, int strategy, const char *version,
    int stream_size)
{
	lzsandbox_initialize(strm);
	return (lz_deflateInit2_insandbox(strm, level, method,
	    windowBits, memLevel, strategy, version, stream_size));
}

struct host_lz_deflateEnd_req {
	z_stream	hzc_req_strm;
} __packed;

struct host_lz_deflateEnd_rep {
	z_stream	hzc_rep_strm;
	int	hzc_rep_retval;
} __packed;

static int 
lz_deflateEnd_insandbox(z_streamp strm)
{
	struct host_lz_deflateEnd_req req;
	struct host_lz_deflateEnd_rep rep;
	struct host_rpc_params params;
	struct iovec iov_req, iov_rep;
	size_t len;

	bzero(&req, sizeof(req));
	memcpy(&req.hzc_req_strm, strm, sizeof(z_stream));
	iov_req.iov_base = &req;
	iov_req.iov_len = sizeof(req);
	iov_rep.iov_base = &rep;
	iov_rep.iov_len = sizeof(rep);
	params.scb = local_lcsp;
	params.opno = PROXIED_LZ_DEFLATEEND;
	params.req = &iov_req;
	params.reqcount = 1;
	params.req_fdp = NULL;
	params.req_fdcount = 0;
	params.rep = &iov_rep;
	params.repcount = 1;
	params.replenp = &len;
	params.rep_fdp = NULL;
	params.rep_fdcountp = NULL;
	if (lch_rpc_fix(&params) < 0)
		err(-1, "lch_rpc");
	if (len != sizeof(rep))
		errx(-1, "lch_rpc len %zu", len);

	/*
	 * XXX: TODO: a check on strm to make sure sandbox hasn't
	 * changed it maliciously.
	 */
	memcpy(strm, &rep.hzc_rep_strm, sizeof(z_stream));

	return (rep.hzc_rep_retval);
}

static void
sandbox_lz_deflateEnd_buffer(struct lc_host *lchp, uint32_t opno,
    uint32_t seqno, char *buffer, size_t len)
{
	struct host_lz_deflateEnd_req req;
	struct host_lz_deflateEnd_rep rep;
	struct iovec iov;

	if (len != sizeof(req))
		err(-1, "sandbox_lz_deflateEnd_buffer: len %zu", len);

	bcopy(buffer, &req, sizeof(req));

	bzero(&rep, sizeof(rep));
	rep.hzc_rep_retval = zlib_deflateEnd(&req.hzc_req_strm);
	iov.iov_base = &rep;
	iov.iov_len = sizeof(rep);

	memcpy(&rep.hzc_rep_strm, &req.hzc_req_strm, sizeof(z_stream));

	if (lcs_sendrpc(lchp, opno, seqno, &iov, 1) < 0)
		err(-1, "lcs_sendrpc");
}

int
deflateEnd_wrapper(z_streamp strm)
{

	lzsandbox_initialize(strm);
	return (lz_deflateEnd_insandbox(strm));
}

#ifdef SB_LIBZ_CRC32
struct host_lz_crc32_req {
	Bytef	*hzc_req_buf;
	uLong	hzc_req_crc;
	uInt	hzc_req_len;
} __packed;

struct host_lz_crc32_rep {
	uLong	hzc_rep_retval;
} __packed;

/* strm only used if !LZ_SINGLE_SANDBOX. */
static uLong
lz_crc32_insandbox(z_streamp strm, uLong crc, const Bytef *buf, uInt len)
{
	struct host_lz_crc32_req req;
	struct host_lz_crc32_rep rep;
	struct host_rpc_params params;
	struct iovec iov_req[2], iov_rep;
	size_t replen;
	int iovcount;

	iovcount = 0;
	bzero(&req, sizeof(req));
	req.hzc_req_crc = crc;
	req.hzc_req_len = len;
	iov_req[0].iov_base = &req;
	iov_req[0].iov_len = sizeof(req);
	iovcount++;
#ifdef GZ_SHMEM
	req.hzc_req_buf = buf;
#else
	iov_req[1].iov_base = (void *)buf;
	iov_req[1].iov_len = len;
	iovcount++;
#endif
	iov_rep.iov_base = &rep;
	iov_rep.iov_len = sizeof(rep);
	params.scb = local_lcsp;
	params.opno = PROXIED_LZ_CRC32;
	params.req = iov_req;
	params.reqcount = iovcount;
	params.req_fdp = NULL;
	params.req_fdcount = 0;
	params.rep = &iov_rep;
	params.repcount = 1;
	params.replenp = &replen;
	params.rep_fdp = NULL;
	params.rep_fdcountp = NULL;
	if (lch_rpc_fix(&params) < 0)
		err(-1, "lch_rpc");
	if (replen != sizeof(rep))
		errx(-1, "lch_rpc len %zu", replen);
	return (rep.hzc_rep_retval);
}

static void
sandbox_lz_crc32_buffer(struct lc_host *lchp, uint32_t opno,
    uint32_t seqno, char *buffer, size_t len)
{
	struct host_lz_crc32_req req;
	struct host_lz_crc32_rep rep;
	Bytef *buf;
	struct iovec iov;

	if (len < sizeof(req))
		err(-1, "sandbox_lz_crc32_buffer: len %zu", len);

	bcopy(buffer, &req, sizeof(req));

#ifdef GZ_SHMEM
	buf = req.hzc_req_buf;
#else
	buf = (Bytef *)buffer + sizeof(req);
	if (len != sizeof(req) + req.hzc_req_len)
		err(-1, "sandbox_lz_crc32_buffer: len %zu", len);
#endif

	bzero(&rep, sizeof(rep));
	rep.hzc_rep_retval = zlib_crc32(req.hzc_req_crc,
	    buf, req.hzc_req_len);
	iov.iov_base = &rep;
	iov.iov_len = sizeof(rep);

	if (lcs_sendrpc(lchp, opno, seqno, &iov, 1) < 0)
		err(-1, "lcs_sendrpc");
}

uLong
crc32_wrapper(uLong crc, const Bytef *buf, uInt len)
{
	static int initialized;
	static z_stream strm;

	if (!initialized) {
		memset(&strm, 0, sizeof strm);
		lzsandbox_initialize(&strm);
		initialized = 1;
	}
	return (lz_crc32_insandbox(&strm, crc, buf, len));
}
#endif /* SB_LIBZ_CRC32 */

/*
 * Main entry point for capability-mode.
 */
int
lzsandbox(void *context)
{
	struct lc_host *lchp;
	uint32_t opno, seqno;
	u_char *buffer;
	size_t len;

	if (lcs_get(&lchp) < 0)
		errx(-1, "libcapsicum sandbox binary");

	for (;;) {
		if (lcs_recvrpc(lchp, &opno, &seqno, &buffer, &len) < 0) {
			if (errno == EPIPE)
				_Exit(-1);
			else
				err(-1, "lcs_recvrpc");
		}
		switch (opno) {
		case PROXIED_LZ_DEFLATE:
			sandbox_lz_deflate_buffer(lchp, opno, seqno,
			    (char *)buffer, len);
			break;
		case PROXIED_LZ_DEFLATEINIT2:
			sandbox_lz_deflateInit2_buffer(lchp, opno, seqno,
			    (char *)buffer, len);
			break;
		case PROXIED_LZ_DEFLATEEND:
			sandbox_lz_deflateEnd_buffer(lchp, opno, seqno,
			    (char *)buffer, len);
			break;
#ifdef SB_LIBZ_CRC32
		case PROXIED_LZ_CRC32:
			sandbox_lz_crc32_buffer(lchp, opno, seqno,
			    (char *)buffer, len);
			break;
#endif
		default:
			errx(-1, "sandbox_workloop: unknown op %d", opno);
		}
		free(buffer);
	}

	(void)context;
	return (0);
}
#endif /* SB_LIBZ_CAPSICUM */
