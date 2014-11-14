#ifdef SB_LIBZ_EXT_ALLOC
#ifndef _CHERI_FN_H_
#define _CHERI_FN_H_

struct cheri_fn_ret {
  register_t czr_retval0;
  __capability void * czr_retcap0;
};

typedef struct cheri_fn_ret (*cheri_fn_proto) (__capability void * params);

/*
 * Ambient authority calls these.
 * cheri_fn_new: attach a new function to cheri_object
 * cheri_fn_revoke: revoke the capability to use a function
 * cheri_fn_destroy: destroy the cheri_object
 */
int cheri_fn_new (cheri_fn_proto fn, struct cheri_object *cop);
void cheri_fn_revoke(struct cheri_object co);
void cheri_fn_destroy(struct cheri_object co);

struct cheri_fn_ret cheri_fn_call_c (struct cheri_object fn_object, __capability void * params);

#endif /* _CHERI_FN_H_ */
#endif /* SB_LIBZ_EXT_ALLOC */
