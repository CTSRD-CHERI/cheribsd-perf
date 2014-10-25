#include "gzsandbox-libc.h"

int printfs (char * dst, const char * format, ...)
{
  int rc;
  va_list ap;
  va_start(ap, format);
  rc = vsnprintf(dst, format, INT_MAX, ap);
  va_end(ap);
  return rc;
}

int printfsn (char * dst, size_t size, const char * format, ...)
{
  int rc;
  va_list ap;
  va_start(ap, format);
  rc = vsnprintf(dst, format, size, ap);
  va_end(ap);
  return rc;
}

/* XXX: doesn't do bounds checking or return size */
int printfvsn (char * dst, size_t size, const char * format, va_list ap)
{
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
          int n = va_arg(ap, int);
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
        state = 0;
        break;
    }
    format++;
  }
  *dst = 0;
  return 0;
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

void * allocm (size_t size)
{
}

void reef (void * ptr)
{
}
