#ifndef _GZSANDBOX_HELPER_H_
#define _GZSANDBOX_HELPER_H_

#define GZSANDBOX_HELPER_OP_INIT            0
#define GZSANDBOX_HELPER_OP_GZCOMPRESS      1
#define GZSANDBOX_HELPER_OP_GZUNCOMPRESS    2

#include <machine/cheri.h>
#include <machine/cheric.h>
struct gz_params
{
  struct cheri_object infd;
  struct cheri_object outfd;
  off_t * gsizep;
  const char * origname;
  uint32_t mtime;
};

#endif /* !_GZSANDBOX_HELPER_H_ */
