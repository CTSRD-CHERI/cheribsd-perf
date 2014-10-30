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

#define	PROXIED_LZ_DEFLATEINIT2	1
#define	PROXIED_LZ_DEFLATE	2
#define	PROXIED_LZ_DEFLATEEND	3

static struct lc_sandbox	*lcsp;
static int			 lzsandbox_initialized;
static int			 lzsandbox_enabled;

static void
lzsandbox_initialize(void)
{
	if (lzsandbox_initialized)
		return;
	lzsandbox_initialized = 1;

  if (lch_startfn(lzsandbox, NULL,
      LCH_PERMIT_STDERR, &lcsp) < 0)
    err(-1, "lch_startfn");
}

struct host_lz_deflate_req {
  z_stream hzc_req_strm;
  int hzc_req_flush;
} __packed;

struct host_lz_deflate_rep {
  z_stream hzc_rep_strm;
  int hzc_rep_retval;
} __packed;

static int 
lz_deflate_insandbox(z_streamp strm, int flush)
{
	struct host_lz_deflate_req req;
	struct host_lz_deflate_rep rep;
	struct iovec iov_req[2], iov_rep[2];
	size_t len;

	bzero(&req, sizeof(req));
  memcpy(&req.hzc_req_strm, strm, sizeof(z_stream));
	iov_req[0].iov_base = &req;
	iov_req[0].iov_len = sizeof(req);
	iov_req[1].iov_base = strm->next_in;
	iov_req[1].iov_len = strm->avail_in;
	iov_rep[0].iov_base = &rep;
	iov_rep[0].iov_len = sizeof(rep);
	iov_rep[1].iov_base = strm->next_out;
	iov_rep[1].iov_len = strm->avail_out;
  struct host_rpc_params params;
  params.scb = lcsp;
  params.opno = PROXIED_LZ_DEFLATE;
  params.req = iov_req;
  params.reqcount = 2;
  params.req_fdp = NULL;
  params.req_fdcount = 0;
  params.rep = iov_rep;
  params.repcount = 2;
  params.replenp = &len;
  params.rep_fdp = NULL;
  params.rep_fdcountp = NULL;
  fprintf(stderr, "sending req (%zu + %zu)\n", params.req[0].iov_len, params.req[1].iov_len);
	if (lch_rpc_fix(&params) < 0)
		err(-1, "lch_rpc");
  fprintf(stderr, "req sent\n");
	if (len != sizeof(rep)+strm->avail_out)
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
  Bytef * next_in = (Bytef *) buffer + sizeof(req);
	struct iovec iov[2];

	if (len < sizeof(req))
		err(-1, "sandbox_lz_deflate_buffer: len %zu", len);

	bcopy(buffer, &req, sizeof(req));
	
  if (len != sizeof(req)+req.hzc_req_strm.avail_in)
		err(-1, "sandbox_lz_deflate_buffer: len %zu", len);

  /* XXX: expensive! (Maybe do this once if possible?) */
  /* allocate output buffer for deflate() */
  size_t avail_out = req.hzc_req_strm.avail_out;
  Bytef * next_out = malloc(avail_out);
  if (!next_out)
		err(-1, "malloc: %zu", avail_out);
  /* XXX: could bzero next_out, but trust is one way so we don't
   * care about leaking info here
   */

  /* deflate() indicates how far it's moved along in the next_in
   * buffer by incrementing the pointer. Therefore, we need to
   * figure out the corresponding difference on the host's end
   * after the call to deflate.
   */
  Bytef * host_next_in = req.hzc_req_strm.next_in;
  Bytef * host_next_out = req.hzc_req_strm.next_out;

  req.hzc_req_strm.next_in = next_in;
  req.hzc_req_strm.next_out = next_out;

	bzero(&rep, sizeof(rep));
	rep.hzc_rep_retval = zlib_deflate(&req.hzc_req_strm, req.hzc_req_flush);
	iov[0].iov_base = &rep;
	iov[0].iov_len = sizeof(rep);
	iov[1].iov_base = next_out;
	iov[1].iov_len = avail_out;

  memcpy(&rep.hzc_rep_strm, &req.hzc_req_strm, sizeof(z_stream));
  /* fix up the pointers */
  rep.hzc_rep_strm.next_in = host_next_in + (req.hzc_req_strm.next_in - next_in);
  rep.hzc_rep_strm.next_out = host_next_out + (req.hzc_req_strm.next_out - next_out);

	if (lcs_sendrpc(lchp, opno, seqno, iov, 2) < 0)
  {
    free(next_out);
		err(-1, "lcs_sendrpc");
  }
  free(next_out);
}

int
deflate_wrapper(z_streamp strm, int flush)
{
  printf("lzsandbox deflate wrapper.\n");
	lzsandbox_initialize();
  return (lz_deflate_insandbox(strm, flush));
}

/*
 * Main entry point for capability-mode 
 */
int lzsandbox(void * context)
{
	struct lc_host *lchp;
	uint32_t opno, seqno;
	u_char *buffer;
	size_t len;

printf("%d: in lzsandbox\n", getpid());
	if (lcs_get(&lchp) < 0)
		errx(-1, "libcapsicum sandbox binary");

	while (1) {
		if (lcs_recvrpc(lchp, &opno, &seqno, &buffer, &len) < 0) {
			if (errno == EPIPE)
      {
        printf("quit: EPIPE\n");
				_Exit(-1);
      }
			else
				err(-1, "lcs_recvrpc");
		}
printf("%d: received opno: %u\n", getpid(),opno);
		switch (opno) {
		case PROXIED_LZ_DEFLATE:
			sandbox_lz_deflate_buffer(lchp, opno, seqno, (char*)buffer,
			    len);
			break;
		default:
			errx(-1, "sandbox_workloop: unknown op %d", opno);
		}
		free(buffer);
	}
  (void) context;
  return 0;
}

#endif /* SB_LIBZ_CAPSICUM */
