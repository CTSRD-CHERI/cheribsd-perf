#include <sys/types.h>
#include <sys/stat.h>

#include <machine/cheri.h>
#include <machine/cheric.h>

#include <cheri/cheri_enter.h>
#include <cheri/cheri_fd.h>
#include <cheri/cheri_invoke.h>
extern __capability void	*cheri_system_type;
#include <cheri/cheri_system.h>

//#include "gzsandbox-libc.h"

/* libc_cheri provides implementation */
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <zlib.h>

#include "gzsandbox-helper.h"

#define BUFLEN		(64 * 1024)
#define GZIP_MAGIC0	0x1F
#define GZIP_MAGIC1	0x8B
#define GZIP_OMAGIC1	0x9E

#define ORIG_NAME	0x08
#define OS_CODE		3	/* Unix */

static struct cheri_object stderrfd;

/* TODO: set this on sandbox init */
static const char * progname = "progname";

/* TODO: copy these flags on sandbox init */
static int	numflag = 6;		/* gzip -1..-9 value */
static	int	qflag;			/* quiet mode */
static	int	nflag;			/* don't save name/timestamp */
static	int	exit_value = 0;		/* exit value */

off_t
gz_compress(struct cheri_object in, struct cheri_object out, off_t *gsizep, const char *origname, uint32_t mtime);

static	void	maybe_err(const char *fmt, ...) __printflike(1, 2) __dead2;
static	void	maybe_warn(const char *fmt, ...) __printflike(1, 2);
static	void	maybe_warnx(const char *fmt, ...) __printflike(1, 2);

int
invoke(register_t op,
  __capability void * co_codecap_stderrfd,
  __capability void * co_datacap_stderrfd,
  __capability void * vparams);

ssize_t read_c (struct cheri_object fd, void * buf, size_t nbytes);
ssize_t write_c (struct cheri_object fd, const void * buf, size_t nbytes);
int fprintf_c (struct cheri_object fd, const char * restrict format, ...);
int vfprintf_c (struct cheri_object fd, const char * restrict format, va_list ap);
void vwarn (const char * restrict format, va_list ap);
void vwarnx (const char * restrict format, va_list ap);

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
void vwarn (const char * restrict format, va_list ap)
{
  fprintf_c(stderrfd, "%s: ", progname);
  if (format)
  {
    vfprintf_c(stderrfd, format, ap);
    fprintf_c(stderrfd, ": %s", strerror(errno));
  }
  fprintf_c(stderrfd, "\n");
}
void vwarnx (const char * restrict format, va_list ap)
{
  fprintf_c(stderrfd, "%s: ", progname);
  if (format)
  {
    vfprintf_c(stderrfd, format, ap);
  }
  fprintf_c(stderrfd, "\n");
}

int
invoke(register_t op,
  __capability void * co_codecap_stderrfd,
  __capability void * co_datacap_stderrfd,
  __capability void * vparams)
{
  __capability struct gz_params * params = vparams;
  /* reconstruct the cheri_objects */
  if (op == GZSANDBOX_HELPER_OP_INIT)
  {
    stderrfd.co_codecap = co_codecap_stderrfd;
    stderrfd.co_datacap = co_datacap_stderrfd;
    fprintf_c(stderrfd, "in invoke(), initialized.\n");
  }
  else if (op == GZSANDBOX_HELPER_OP_GZCOMPRESS ||
    op == GZSANDBOX_HELPER_OP_GZUNCOMPRESS)
  {
    if (op == GZSANDBOX_HELPER_OP_GZCOMPRESS)
      return gz_compress(params->infd, params->outfd,
        params->gsizep, params->origname, params->mtime);
    else if (op == GZSANDBOX_HELPER_OP_GZUNCOMPRESS)
      ;
  }
  return 0;
}

/* maybe print a warning */
void
maybe_warn(const char *fmt, ...)
{
	va_list ap;

	if (qflag == 0) {
		va_start(ap, fmt);
		vwarn(fmt, ap);
		va_end(ap);
	}
	if (exit_value == 0)
		exit_value = 1;
}

/* ... without an errno. */
void
maybe_warnx(const char *fmt, ...)
{
	va_list ap;

	if (qflag == 0) {
		va_start(ap, fmt);
		vwarnx(fmt, ap);
		va_end(ap);
	}
	if (exit_value == 0)
		exit_value = 1;
}

/* maybe print an error */
void
maybe_err(const char *fmt, ...)
{
	va_list ap;

	if (qflag == 0) {
		va_start(ap, fmt);
		vwarn(fmt, ap);
		va_end(ap);
	}
	exit(2);
}

/* compress input to output. Return bytes read, -1 on error */
off_t
gz_compress(struct cheri_object in, struct cheri_object out, off_t *gsizep, const char *origname, uint32_t mtime)
{
	z_stream z;
	char *outbufp, *inbufp;
	off_t in_tot = 0, out_tot = 0;
	ssize_t in_size;
	int i, error;
	uLong crc;
#ifdef SMALL
	static char header[] = { GZIP_MAGIC0, GZIP_MAGIC1, Z_DEFLATED, 0,
				 0, 0, 0, 0,
				 0, OS_CODE };
#endif

	outbufp = malloc(BUFLEN);
	inbufp = malloc(BUFLEN);
  fprintf_c(stderrfd, "inbufp: %p\n", inbufp);
	if (outbufp == NULL || inbufp == NULL) {
		maybe_err("malloc failed");
		goto out;
	}

	memset(&z, 0, sizeof z);
	z.zalloc = Z_NULL;
	z.zfree = Z_NULL;
	z.opaque = 0;

#ifdef SMALL
	memcpy(outbufp, header, sizeof header);
	i = sizeof header;
#else
	if (nflag != 0) {
		mtime = 0;
		origname = "";
	}

  /* Avoid printf()-like functions because CHERI Clang/LLVM messes up varargs that spill to the stack */
  if (BUFLEN >= 10)
  {
    outbufp[0] = GZIP_MAGIC0;
    outbufp[1] = GZIP_MAGIC1;
    outbufp[2] = Z_DEFLATED;
    outbufp[3] = *origname ? ORIG_NAME : 0;
    outbufp[4] = mtime & 0xff;
    outbufp[5] = (mtime >> 8) & 0xff;
    outbufp[6] = (mtime >> 16) & 0xff;
    outbufp[7] = (mtime >> 24) & 0xff;
    outbufp[8] = numflag == 1 ? 4 : numflag == 9 ? 2 : 0;
    outbufp[9] = OS_CODE;
    i = 10+snprintf(&outbufp[10], BUFLEN-10, "%s", origname);
  }
  else
  {
    i = 0;
  }

	/*i = snprintf(outbufp, BUFLEN, "%c%c%c%c%c%c%c%c%c%c%s", 
		     GZIP_MAGIC0, GZIP_MAGIC1, Z_DEFLATED,
		     *origname ? ORIG_NAME : 0,
		     mtime & 0xff,
		     (mtime >> 8) & 0xff,
		     (mtime >> 16) & 0xff,
		     (mtime >> 24) & 0xff,
		     numflag == 1 ? 4 : numflag == 9 ? 2 : 0,
		     OS_CODE, origname);*/

	if (i >= BUFLEN)     
		/* this need PATH_MAX > BUFLEN ... */
		maybe_err("snprintf");
	if (*origname)
		i++;
#endif

	z.next_out = (unsigned char *)outbufp + i;
	z.avail_out = BUFLEN - i;

	error = deflateInit2(&z, numflag, Z_DEFLATED,
			     (-MAX_WBITS), 8, Z_DEFAULT_STRATEGY);
	if (error != Z_OK) {
		maybe_warnx("deflateInit2 failed");
		in_tot = -1;
		goto out;
	}

	crc = crc32(0L, Z_NULL, 0);
	for (;;) {
		if (z.avail_out == 0) {
			if (write_c(out, outbufp, BUFLEN) != BUFLEN) {
				maybe_warn("write");
				out_tot = -1;
				goto out;
			}

			out_tot += BUFLEN;
			z.next_out = (unsigned char *)outbufp;
			z.avail_out = BUFLEN;
		}

		if (z.avail_in == 0) {
			in_size = read_c(in, inbufp, BUFLEN);
			if (in_size < 0) {
				maybe_warn("read");
				in_tot = -1;
				goto out;
			}
			if (in_size == 0)
				break;

			crc = crc32(crc, (const Bytef *)inbufp, (unsigned)in_size);
			in_tot += in_size;
			z.next_in = (unsigned char *)inbufp;
			z.avail_in = in_size;
		}

		error = deflate(&z, Z_NO_FLUSH);
		if (error != Z_OK && error != Z_STREAM_END) {
			maybe_warnx("deflate failed");
			in_tot = -1;
			goto out;
		}
	}

	/* clean up */
	for (;;) {
		size_t len;
		ssize_t w;

		error = deflate(&z, Z_FINISH);
		if (error != Z_OK && error != Z_STREAM_END) {
			maybe_warnx("deflate failed");
			in_tot = -1;
			goto out;
		}

		len = (char *)z.next_out - outbufp;

		w = write_c(out, outbufp, len);
		if (w == -1 || (size_t)w != len) {
			maybe_warn("write");
			out_tot = -1;
			goto out;
		}
		out_tot += len;
		z.next_out = (unsigned char *)outbufp;
		z.avail_out = BUFLEN;

		if (error == Z_STREAM_END)
			break;
	}

	if (deflateEnd(&z) != Z_OK) {
		maybe_warnx("deflateEnd failed");
		in_tot = -1;
		goto out;
	}

  /* Avoid printf()-like functions because CHERI Clang/LLVM messes up varargs that spill to the stack */
  if (BUFLEN >= 8)
  {
    outbufp[0] = crc & 0xff;
    outbufp[1] = (crc >> 8) & 0xff;
    outbufp[2] = (crc >> 16) & 0xff;
    outbufp[3] = (crc >> 24) & 0xff;
    outbufp[4] = in_tot & 0xff;
    outbufp[5] = (in_tot >> 8) & 0xff;
    outbufp[6] = (in_tot >> 16) & 0xff;
    outbufp[7] = (in_tot >> 24) & 0xff;
    i = 8;
  }
  else
  {
    i = 0;
  }

	/*i = snprintf(outbufp, BUFLEN, "%c%c%c%c%c%c%c%c", 
		 (int)crc & 0xff,
		 (int)(crc >> 8) & 0xff,
		 (int)(crc >> 16) & 0xff,
		 (int)(crc >> 24) & 0xff,
		 (int)in_tot & 0xff,
		 (int)(in_tot >> 8) & 0xff,
		 (int)(in_tot >> 16) & 0xff,
		 (int)(in_tot >> 24) & 0xff);*/
	if (i != 8)
		maybe_err("snprintf");
	if (write_c(out, outbufp, i) != i) {
		maybe_warn("write");
		in_tot = -1;
	} else
		out_tot += i;

out:
	if (inbufp != NULL)
		free(inbufp);
	if (outbufp != NULL)
		free(outbufp);
	if (gsizep)
		*gsizep = out_tot;
	return in_tot;
}


