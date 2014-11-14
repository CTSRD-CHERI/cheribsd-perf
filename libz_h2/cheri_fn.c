#ifdef SB_LIBZ_EXT_ALLOC
#include <sys/types.h>
#include <sys/stat.h>

#include <machine/cheri.h>
#include <machine/cheric.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <cheri_class.h>
#include "cheri_fn.h"
#include <cheri_type.h>

CHERI_CLASS_DECL(cheri_fn);

static __capability void *cheri_fn_type;

struct cheri_fn {
  cheri_fn_proto cf_fn; /* underlying fn ptr */
};

static __attribute__ ((constructor)) void
cheri_fn_init(void)
{
	cheri_fn_type = cheri_type_alloc();
}

int
cheri_fn_new (cheri_fn_proto fn, struct cheri_object *cop)
{
  __capability void *codecap, *datacap;
  struct cheri_fn *cfn;

  cfn = calloc(1, sizeof(*cfn));
  if (cfn == NULL)
  {
    errno = ENOMEM;
    return -1;
  }
  cfn->cf_fn = fn;
  
  codecap = cheri_setoffset(cheri_getpcc(),
    (register_t)CHERI_CLASS_ENTRY(cheri_fn));
  cop->co_codecap = cheri_seal(codecap, cheri_fn_type);

  datacap = cheri_ptrperm(cfn, sizeof(*cfn), CHERI_PERM_GLOBAL |
    CHERI_PERM_LOAD | CHERI_PERM_LOAD_CAP |
    CHERI_PERM_STORE | CHERI_PERM_STORE_CAP);
  cop->co_datacap = cheri_seal(datacap, cheri_fn_type);
  return 0;
}

void
cheri_fn_revoke (struct cheri_object co)
{
  __capability struct cheri_fn *cfn;
  cfn = cheri_unseal(co.co_datacap, cheri_fn_type);
  cfn->cf_fn = NULL;
}

/* must only be called once all outstanding refs from sandboxes are deleted */
void
cheri_fn_destroy (struct cheri_object co)
{
  __capability struct cheri_fn *cfn;
  cfn = cheri_unseal(co.co_datacap, cheri_fn_type);
  free((void*)cfn);
}

/* in the ambient auth */
static struct cheri_fn_ret
_cheri_fn_call_c (__capability void *params)
{
  __capability struct cheri_fn *cfn;
  cfn = cheri_getidc();
  return cfn->cf_fn(params);
}

struct cheri_fn_ret
cheri_fn_enter (__capability void * c3)
{
  return _cheri_fn_call_c(c3);
}

/* XXX: re-prototype of cheri_invoke */
struct cheri_fn_ret	cheri_invoke(struct cheri_object fn_object, __capability void *c3)
			    __attribute__((cheri_ccall));

/* in the sandbox */
struct cheri_fn_ret cheri_fn_call_c (struct cheri_object fn_object, __capability void * params)
{
  return cheri_invoke(fn_object, params);
}
#endif /* SB_LIBZ_EXT_ALLOC */
