#include <sys/types.h>
#include <sys/stat.h>

#include <machine/cheri.h>
#include <machine/cheric.h>

#include <cheri/cheri_enter.h>
#include <cheri/cheri_fd.h>
#include <cheri/cheri_invoke.h>
extern __capability void	*cheri_system_type;
#include <cheri/cheri_system.h>

#include <stdarg.h>

int
invoke(register_t op,
  __capability void * co_codecap_infd,
  __capability void * co_datacap_infd,
  __capability void * co_codecap_outfd,
  __capability void * co_datacap_outfd,
  __capability void * co_codecap_stderrfd,
  __capability void * co_datacap_stderrfd);

#define CHERI_STR_PTR(str)  cheri_ptr((void*)(str), 1+lenstr((str)))

size_t lenstr (const char * str);
char * cpystr (char * dst, const char * src);
const char * i2a (int n);

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
        format++;
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
            for (i=0; i<l/2; i++)
            {
              char c = dst[i];
              dst[i] = dst[l-i];
              dst[l-i] = c;
            }
            dst += l;
          }
        }
        break;
    }
  }
  va_end(vl);
  *dst = 0;
}

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

static char buf[512];

int
invoke(register_t op,
  __capability void * co_codecap_infd,
  __capability void * co_datacap_infd,
  __capability void * co_codecap_outfd,
  __capability void * co_datacap_outfd,
  __capability void * co_codecap_stderrfd,
  __capability void * co_datacap_stderrfd)
{
  struct cheri_fd_ret rc;
  /* reconstruct the cheri_objects */
  struct cheri_object infd, outfd, stderrfd;
  infd.co_codecap = co_codecap_infd;
  infd.co_datacap = co_datacap_infd;
  outfd.co_codecap = co_codecap_outfd;
  outfd.co_datacap = co_datacap_outfd;
  stderrfd.co_codecap = co_codecap_stderrfd;
  stderrfd.co_datacap = co_datacap_stderrfd;

  rc = cheri_fd_read_c(infd, cheri_ptr(buf, sizeof buf));
  int n = rc.cfr_retval0;

  rc = cheri_fd_write_c(stderrfd, CHERI_STR_PTR("return code from read():"));
  
  rc = cheri_fd_write_c(stderrfd, CHERI_STR_PTR(i2a(n)));

  rc = cheri_fd_write_c(stderrfd, CHERI_STR_PTR("\n"));

  rc = cheri_fd_write_c(outfd, cheri_ptr(buf, n));
  
  rc = cheri_fd_write_c(stderrfd, CHERI_STR_PTR(i2a(rc.cfr_retval0)));

  /* no stdlib */ /*fprintf(stderr, "in invoke()!\n");*/
  /*cheri_system_puts(CHERI_STR_PTR("in invoke()!\n"));*/
  (void) op;
  return 0x5678;
}

