#include <sys/types.h>
#include <sys/stat.h>

#include <machine/cheri.h>
#include <machine/cheric.h>

#include <cheri/cheri_enter.h>
#include <cheri/cheri_fd.h>
#include <cheri/cheri_invoke.h>
extern __capability void	*cheri_system_type;
#include <cheri/cheri_system.h>

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
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

/* TODO: copy this flag on sandbox init */
static	int	nflag;			/* don't save name/timestamp */
/* TODO: copy this flag on sandbox init */
static int	numflag = 6;		/* gzip -1..-9 value */

int
invoke(register_t op,
  __capability void * co_codecap_stderrfd,
  __capability void * co_datacap_stderrfd,
  __capability void * vparams);

off_t
gz_compress(struct cheri_object in, struct cheri_object out, off_t *gsizep, const char *origname, uint32_t mtime);

#define CHERI_STR_PTR(str)  cheri_ptr((void*)(str), 1+lenstr((str)))

size_t lenstr (const char * str);
char * cpystr (char * dst, const char * src);
void * setmem (void * ptr, int value, size_t num);
void * cpymem (void * dst, const void * src, size_t num);
const char * i2a (int n);

ssize_t read_c (struct cheri_object fd, void * buf, size_t nbytes);
ssize_t write_c (struct cheri_object fd, const void * buf, size_t nbytes);

/* TODO: implement this */
static	void	maybe_err(const char *fmt, ...) __printflike(1, 2) __dead2;
static	void	maybe_warn(const char *fmt, ...) __printflike(1, 2);
static	void	maybe_warnx(const char *fmt, ...) __printflike(1, 2);
void
maybe_err(const char *fmt, ...)
{
  write_c(stderrfd, fmt, lenstr(fmt)+1);
  exit(2);
}
void
maybe_warn(const char *fmt, ...)
{
  write_c(stderrfd, fmt, lenstr(fmt)+1);
}
void
maybe_warnx(const char *fmt, ...)
{
  write_c(stderrfd, fmt, lenstr(fmt)+1);
}

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

const char * i2a (int n)
{
  static char s[50];
  int neg=0, i;
  if (n == 0)
  {
    s[0] = '0';
    s[1] = '\0';
    return s;
  }
  if (n < 0)
  {
    neg = 1;
    n = -n;
  }
  s[(sizeof s)-1] = '\0';
  for (i=(sizeof s)-2; n; n/=10, i--)
    s[i] = "0123456789"[n%10];
  if (neg) s[i] = '-';
  else i++;
  return &s[i];
}

#if 0
void printfs (char * dst, const char * format, ...);
void printfs (char * dst, const char * format, ...)
{
  va_list vl;
  va_start(vl, format);
  int state = 0;
  while (*format)
  {
    switch (state)
    {
      case 0: /* parsing next char */
        if (*format == '%') state = 1;
        else *(dst++) = *format;
        break;
      case 1: /* just parsed '%' */
        if (*format == 'u')
        {
          /* copy int to output */
          int n = va_arg(vl, int);
          if (!n) *(dst++) = '0';
          else
          {
            int l,i;
            for (l=0; n; n/=10, l++);
              dst[l] = "0123456789"[n%10];
            /* reverse */
            for (i=0; 0&&i<l/2; i++)
            {
              char c = dst[i];
              dst[i] = dst[l-i];
              dst[l-i] = c;
            }
            dst += l;
          }
        }
        else *(dst++) = *format;
        state = 0;
        break;
    }
    format++;
  }
  va_end(vl);
  *dst = 0;
}
#endif /* 0 */

size_t lenstr (const char * str)
{
  size_t len;
  len = 0;
  while (*str)
    str++, len++;
  return len;
}

char * cpystr (char * dst, const char * src)
{
  char * p;
  p = dst;
  while (*src)
    *(p++) = *(src++);
  *p = 0;
  return dst;
}

void * setmem (void * ptr, int value, size_t num)
{
  char * cptr = ptr;
  size_t i;
  for (i=0; i<num; i++)
    cptr[i] = value;
  return ptr;
}

void * cpymem (void * dst, const void * src, size_t num)
{
  char * cdst = dst;
  const char * csrc = src;
  size_t i;
  for (i=0; i<num; i++)
    cdst[i] = csrc[i];
  return cdst;
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
/*
  char strbuf[512];
  sprintf(strbuf, "return code from read(): %u*^\n", (int) i2a(n));
  write_c(stderrfd, strbuf, strlen(strbuf)+1);
*/
  return 0;
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
	if (outbufp == NULL || inbufp == NULL) {
		maybe_err("malloc failed");
		goto out;
	}

	setmem(&z, 0, sizeof z);
	z.zalloc = Z_NULL;
	z.zfree = Z_NULL;
	z.opaque = 0;

#ifdef SMALL
	cpymem(outbufp, header, sizeof header);
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


