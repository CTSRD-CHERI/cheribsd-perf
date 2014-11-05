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

  __capability off_t * gsizep;

  /* gz_compress params */
  __capability const char * origname;
  uint32_t mtime;

  /* gz_uncompress params */
  __capability char * pre;
  size_t prelen;
  __capability const char * filename;
};

struct gz_init_params
{
  int numflag;
  int nflag;
  int qflag;
  int tflag;
#ifdef DYNAMIC_BUFLEN
  size_t BUFLEN;
#endif /* DYNAMIC_BUFLEN */
};

#endif /* !_GZSANDBOX_HELPER_H_ */
