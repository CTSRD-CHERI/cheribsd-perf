#ifndef _GZSANDBOX_LIBC_H_
#define _GZSANDBOX_LIBC_H_

#include <errno.h>
#include <stdarg.h>

#define sprintf    printfs
#define snprintf   printfsn
#define vsnprintf  printfvsn
#define memcpy     cpymem
#define memset     setmem
#define strcpy     cpystr
#define strlen     lenstr
#define malloc     allocm
#define free       reef

int printfs (char * dst, const char * format, ...);
int printfsn (char * dst, size_t size, const char * format, ...);
int printfvsn (char * dst, size_t size, const char * format, va_list ap);
size_t lenstr (const char * str);
char * cpystr (char * dst, const char * src);
void * setmem (void * ptr, int value, size_t num);
void * cpymem (void * dst, const void * src, size_t num);
void * allocm (size_t size);
void reef (void * ptr);

#endif /* !_GZSANDBOX_LIBC_H_ */
