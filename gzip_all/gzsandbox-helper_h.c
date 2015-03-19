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

#include "gzsandbox-helper_h.h"

#ifdef DYNAMIC_BUFLEN
static size_t BUFLEN;
#else /* DYNAMIC_BUFLEN */
#define BUFLEN		(64 * 1024)
#endif /* DYNAMIC_BUFLEN */

#define GZIP_MAGIC0	0x1F
#define GZIP_MAGIC1	0x8B
#define GZIP_OMAGIC1	0x9E

#define HEAD_CRC	0x02
#define EXTRA_FIELD	0x04
#define ORIG_NAME	0x08
#define COMMENT		0x10

#define OS_CODE		3	/* Unix */

static struct cheri_object stderrfd;
#ifdef SB_COLLECT_STATS
__capability int * num_ccalls;
#endif /* SB_COLLECT_STATS */

/* TODO: set this on sandbox init */
static const char * progname = "progname";

/* TODO: copy these flags on sandbox init */
static int	numflag = 6;		/* gzip -1..-9 value */
static	int	nflag;			/* don't save name/timestamp */
static	int	qflag;			/* quiet mode */
static	int	tflag;			/* test */
static	int	exit_value = 0;		/* exit value */

off_t
gz_compress(struct cheri_object in, struct cheri_object out, __capability off_t *gsizep, __capability const char *origname, uint32_t mtime);
off_t
gz_uncompress(struct cheri_object in, struct cheri_object out, __capability char *pre, size_t prelen, __capability off_t *gsizep,
	      __capability const char *filename);

static	void	maybe_err(const char *fmt, ...) __printflike(1, 2) __dead2;
static	void	maybe_warn(const char *fmt, ...) __printflike(1, 2);
static	void	maybe_warnx(const char *fmt, ...) __printflike(1, 2);

__attribute__((cheri_ccallee))
__attribute__((cheri_method_suffix("_cap")))
__attribute__((cheri_method_class(cheri_gzip)))
int
cheri_gzip_invoke(register_t op,
  __capability void * co_codecap_stderrfd,
  __capability void * co_datacap_stderrfd,
  __capability void * vparams
#ifdef SB_COLLECT_STATS
  ,__capability int * param_num_ccalls
#endif /* SB_COLLECT_STATS */
);

ssize_t read_c (struct cheri_object fd, void * buf, size_t nbytes);
ssize_t write_c (struct cheri_object fd, const void * buf, size_t nbytes);
int fprintf_c (struct cheri_object fd, const char * restrict format, ...);
int vfprintf_c (struct cheri_object fd, const char * restrict format, va_list ap);
void vwarn (const char * restrict format, va_list ap);
void vwarnx (const char * restrict format, va_list ap);
__capability void * malloc_c (size_t size);
void free_c (__capability void * ptr);
size_t strlen_c (__capability const char * str);

int
invoke(void)
{

	printf("unimplemented\n");
	return (-1);
}

ssize_t read_c (struct cheri_object fd, void * buf, size_t nbytes)
{
  struct cheri_fd_ret rc;
#ifdef SB_COLLECT_STATS
  (*num_ccalls)++;
#endif /* SB_COLLECT_STATS */
  rc = cheri_fd_read_c(fd, cheri_ptrperm(buf, nbytes, CHERI_PERM_STORE));
  errno = rc.cfr_retval1;
  return rc.cfr_retval0;
}

ssize_t write_c (struct cheri_object fd, const void * buf, size_t nbytes)
{
  struct cheri_fd_ret rc;
#ifdef SB_COLLECT_STATS
  (*num_ccalls)++;
#endif /* SB_COLLECT_STATS */
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

int
cheri_gzip_invoke(register_t op,
  __capability void * co_codecap_stderrfd,
  __capability void * co_datacap_stderrfd,
  __capability void * vparams
#ifdef SB_COLLECT_STATS
  ,__capability int * param_num_ccalls
#endif /* SB_COLLECT_STATS */
)
{
  //__asm__ __volatile__ ("cmove $c11, $c26" ::: "memory");
#pragma clang diagnostic push
#pragma clang diagnostic warning "-Winline-asm"
  //__asm__ __volatile__ ("cmove $c0, $c26" ::: "memory");
  __asm__ __volatile__ ("cmove $c0, $c11" ::: "memory");
#pragma clang diagnostic pop
  static int initialized = 0;
  /* reconstruct the cheri_objects */
  if (!initialized)
  {
    if (op == GZSANDBOX_HELPER_OP_INIT)
    {
      __capability struct gz_init_params * params = vparams;
      stderrfd.co_codecap = co_codecap_stderrfd;
      stderrfd.co_datacap = co_datacap_stderrfd;
      numflag = params->numflag;
      nflag = params->nflag;
      qflag = params->qflag;
      tflag = params->tflag;
#ifdef DYNAMIC_BUFLEN
      BUFLEN = params->BUFLEN;
#endif /* DYNAMIC_BUFLEN */
#ifdef SB_COLLECT_STATS
      num_ccalls = param_num_ccalls;
#endif /* SB_COLLECT_STATS */
      initialized = 1;
    }
  }
  else if (op == GZSANDBOX_HELPER_OP_GZCOMPRESS ||
    op == GZSANDBOX_HELPER_OP_GZUNCOMPRESS)
  {
    __capability struct gz_params * params = vparams;
    if (op == GZSANDBOX_HELPER_OP_GZCOMPRESS)
      return gz_compress(
        params->infd, params->outfd, params->gsizep, params->origname,
        params->mtime);
    else if (op == GZSANDBOX_HELPER_OP_GZUNCOMPRESS)
      return gz_uncompress(
        params->infd, params->outfd, params->pre, params->prelen,
        params->gsizep, params->filename);
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
gz_compress(struct cheri_object in, struct cheri_object out, __capability off_t *gsizep, __capability const char *origname, uint32_t mtime)
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
    const char * noname = "";
		mtime = 0;
		origname = cheri_ptrperm((void*)noname, 1, CHERI_PERM_LOAD);
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
    /* XXX: can't now convert origname into a pointer and use legacy MIPS load/store, because it's not within $c0 */
    /*i = 10+snprintf(&outbufp[10], BUFLEN-10, "%s", (void*)origname);*/
    size_t len = strlen_c(origname);
    if (BUFLEN >= 10+len+1)
    {
      memcpy_c(cheri_ptrperm(&outbufp[10], BUFLEN-10, CHERI_PERM_STORE), origname, len+1);
    }
    i = 10+len;
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

	crc = crc32_c(0L, Z_NULL, 0);
	for (;;) {
		if (z.avail_out == 0) {
			if (write_c(out, (void*)outbufp, BUFLEN) != BUFLEN) {
				maybe_warn("write");
				out_tot = -1;
				goto out;
			}

			out_tot += BUFLEN;
			z.next_out = (unsigned char *)outbufp;
			z.avail_out = BUFLEN;
		}

		if (z.avail_in == 0) {
			in_size = read_c(in, (void*)inbufp, BUFLEN);
			if (in_size < 0) {
				maybe_warn("read");
				in_tot = -1;
				goto out;
			}
			if (in_size == 0)
				break;

			crc = crc32_c(crc, (const Bytef *)inbufp, (unsigned)in_size);
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

		w = write_c(out, (void*)outbufp, len);
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

	if (deflateEnd_c(&z) != Z_OK) {
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
	if (write_c(out, (void*)outbufp, i) != i) {
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

/*
 * uncompress input to output then close the input.  return the
 * uncompressed size written, and put the compressed sized read
 * into `*gsizep'.
 */
off_t
gz_uncompress(struct cheri_object in, struct cheri_object out, __capability char *pre, size_t prelen, __capability off_t *gsizep,
	      __capability const char *filename)
{
	z_stream z;
	char *outbufp, *inbufp;
	off_t out_tot = -1, in_tot = 0;
	uint32_t out_sub_tot = 0;
	enum {
		GZSTATE_MAGIC0,
		GZSTATE_MAGIC1,
		GZSTATE_METHOD,
		GZSTATE_FLAGS,
		GZSTATE_SKIPPING,
		GZSTATE_EXTRA,
		GZSTATE_EXTRA2,
		GZSTATE_EXTRA3,
		GZSTATE_ORIGNAME,
		GZSTATE_COMMENT,
		GZSTATE_HEAD_CRC1,
		GZSTATE_HEAD_CRC2,
		GZSTATE_INIT,
		GZSTATE_READ,
		GZSTATE_CRC,
		GZSTATE_LEN,
	} state = GZSTATE_MAGIC0;
	int flags = 0, skip_count = 0;
	int error = Z_STREAM_ERROR, done_reading = 0;
	uLong crc = 0;
	ssize_t wr;
	int needmore = 0;

#define ADVANCE()       { z.next_in++; z.avail_in--; }

	if ((outbufp = malloc(BUFLEN)) == NULL) {
		maybe_err("malloc failed");
		goto out2;
	}
	if ((inbufp = malloc(BUFLEN)) == NULL) {
		maybe_err("malloc failed");
		goto out1;
	}

	memset(&z, 0, sizeof z);
	z.avail_in = prelen;
	z.next_in = (unsigned char *)pre;
	z.avail_out = BUFLEN;
	z.next_out = (unsigned char *)outbufp;
	z.zalloc = NULL;
	z.zfree = NULL;
	z.opaque = 0;

	in_tot = prelen;
	out_tot = 0;

	for (;;) {
		if ((z.avail_in == 0 || needmore) && done_reading == 0) {
			ssize_t in_size;

			if (z.avail_in > 0) {
				memmove(inbufp, z.next_in, z.avail_in);
			}
			z.next_in = (unsigned char *)inbufp;
			in_size = read_c(in, z.next_in + z.avail_in,
			    BUFLEN - z.avail_in);

			if (in_size == -1) {
				maybe_warn("failed to read stdin");
				goto stop_and_fail;
			} else if (in_size == 0) {
				done_reading = 1;
			}

			z.avail_in += in_size;
			needmore = 0;

			in_tot += in_size;
		}
		if (z.avail_in == 0) {
			if (done_reading && state != GZSTATE_MAGIC0) {
				maybe_warnx("%s: unexpected end of file",
					    (void*) filename);
				goto stop_and_fail;
			}
			goto stop;
		}
		switch (state) {
		case GZSTATE_MAGIC0:
			if (*z.next_in != GZIP_MAGIC0) {
				if (in_tot > 0) {
					maybe_warnx("%s: trailing garbage "
						    "ignored", (void*) filename);
					goto stop;
				}
				maybe_warnx("input not gziped (MAGIC0)");
				goto stop_and_fail;
			}
			ADVANCE();
			state++;
			out_sub_tot = 0;
			crc = crc32_c(0L, Z_NULL, 0);
			break;

		case GZSTATE_MAGIC1:
			if (*z.next_in != GZIP_MAGIC1 &&
			    *z.next_in != GZIP_OMAGIC1) {
				maybe_warnx("input not gziped (MAGIC1)");
				goto stop_and_fail;
			}
			ADVANCE();
			state++;
			break;

		case GZSTATE_METHOD:
			if (*z.next_in != Z_DEFLATED) {
				maybe_warnx("unknown compression method");
				goto stop_and_fail;
			}
			ADVANCE();
			state++;
			break;

		case GZSTATE_FLAGS:
			flags = *z.next_in;
			ADVANCE();
			skip_count = 6;
			state++;
			break;

		case GZSTATE_SKIPPING:
			if (skip_count > 0) {
				skip_count--;
				ADVANCE();
			} else
				state++;
			break;

		case GZSTATE_EXTRA:
			if ((flags & EXTRA_FIELD) == 0) {
				state = GZSTATE_ORIGNAME;
				break;
			}
			skip_count = *z.next_in;
			ADVANCE();
			state++;
			break;

		case GZSTATE_EXTRA2:
			skip_count |= ((*z.next_in) << 8);
			ADVANCE();
			state++;
			break;

		case GZSTATE_EXTRA3:
			if (skip_count > 0) {
				skip_count--;
				ADVANCE();
			} else
				state++;
			break;

		case GZSTATE_ORIGNAME:
			if ((flags & ORIG_NAME) == 0) {
				state++;
				break;
			}
			if (*z.next_in == 0)
				state++;
			ADVANCE();
			break;

		case GZSTATE_COMMENT:
			if ((flags & COMMENT) == 0) {
				state++;
				break;
			}
			if (*z.next_in == 0)
				state++;
			ADVANCE();
			break;

		case GZSTATE_HEAD_CRC1:
			if (flags & HEAD_CRC)
				skip_count = 2;
			else
				skip_count = 0;
			state++;
			break;

		case GZSTATE_HEAD_CRC2:
			if (skip_count > 0) {
				skip_count--;
				ADVANCE();
			} else
				state++;
			break;

		case GZSTATE_INIT:
			if (inflateInit2(&z, -MAX_WBITS) != Z_OK) {
				maybe_warnx("failed to inflateInit");
				goto stop_and_fail;
			}
			state++;
			break;

		case GZSTATE_READ:
			error = inflate(&z, Z_FINISH);
			switch (error) {
			/* Z_BUF_ERROR goes with Z_FINISH... */
			case Z_BUF_ERROR:
				if (z.avail_out > 0 && !done_reading)
					continue;

			case Z_STREAM_END:
			case Z_OK:
				break;

			case Z_NEED_DICT:
				maybe_warnx("Z_NEED_DICT error");
				goto stop_and_fail;
			case Z_DATA_ERROR:
				maybe_warnx("data stream error");
				goto stop_and_fail;
			case Z_STREAM_ERROR:
				maybe_warnx("internal stream error");
				goto stop_and_fail;
			case Z_MEM_ERROR:
				maybe_warnx("memory allocation error");
				goto stop_and_fail;

			default:
				maybe_warn("unknown error from inflate(): %d",
				    error);
			}
			wr = BUFLEN - z.avail_out;

			if (wr != 0) {
				crc = crc32_c(crc, (const Bytef *)outbufp, (unsigned)wr);
				if (
#ifndef SMALL
				    /* don't write anything with -t */
				    tflag == 0 &&
#endif
				    write_c(out, outbufp, wr) != wr) {
					maybe_warn("error writing to output");
					goto stop_and_fail;
				}

				out_tot += wr;
				out_sub_tot += wr;
			}

			if (error == Z_STREAM_END) {
				inflateEnd_c(&z);
				state++;
			}

			z.next_out = (unsigned char *)outbufp;
			z.avail_out = BUFLEN;

			break;
		case GZSTATE_CRC:
			{
				uLong origcrc;

				if (z.avail_in < 4) {
					if (!done_reading) {
						needmore = 1;
						continue;
					}
					maybe_warnx("truncated input");
					goto stop_and_fail;
				}
				origcrc = ((unsigned)z.next_in[0] & 0xff) |
					((unsigned)z.next_in[1] & 0xff) << 8 |
					((unsigned)z.next_in[2] & 0xff) << 16 |
					((unsigned)z.next_in[3] & 0xff) << 24;
				if (origcrc != crc) {
					maybe_warnx("invalid compressed"
					     " data--crc error");
					goto stop_and_fail;
				}
			}

			z.avail_in -= 4;
			z.next_in += 4;

			if (!z.avail_in && done_reading) {
				goto stop;
			}
			state++;
			break;
		case GZSTATE_LEN:
			{
				uLong origlen;

				if (z.avail_in < 4) {
					if (!done_reading) {
						needmore = 1;
						continue;
					}
					maybe_warnx("truncated input");
					goto stop_and_fail;
				}
				origlen = ((unsigned)z.next_in[0] & 0xff) |
					((unsigned)z.next_in[1] & 0xff) << 8 |
					((unsigned)z.next_in[2] & 0xff) << 16 |
					((unsigned)z.next_in[3] & 0xff) << 24;

				if (origlen != out_sub_tot) {
					maybe_warnx("invalid compressed"
					     " data--length error");
					goto stop_and_fail;
				}
			}
				
			z.avail_in -= 4;
			z.next_in += 4;

			if (error < 0) {
				maybe_warnx("decompression error");
				goto stop_and_fail;
			}
			state = GZSTATE_MAGIC0;
			break;
		}
		continue;
stop_and_fail:
		out_tot = -1;
stop:
		break;
	}
	if (state > GZSTATE_INIT)
		inflateEnd_c(&z);

	free(inbufp);
out1:
	free(outbufp);
out2:
	if (gsizep)
		*gsizep = in_tot;
	return (out_tot);
}



