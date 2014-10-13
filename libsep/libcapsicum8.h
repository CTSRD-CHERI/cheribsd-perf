#ifndef _LIBCAPSICUM8_H_
#define _LIBCAPSICUM8_H_

/* Compatibility layer between the new (FreeBSD 10+) and old
 * (FreeBSD 8.0) libcapsicum API, implemented via libsep.
 */

#ifdef IN_LIBSEP_LIB
#include "sandbox.h"
#else
#include <sandbox.h>
#endif

#define lc_sandbox          sandbox_cb
#define lc_host             sandbox_cb

/* lch_startfn is akin to lch_start and lch_startfd, which took a
 * filename and file descriptor, respectively. fn_sandbox is called
 * after entering capability mode. The flags are ignored. lcsp is
 * allocated with malloc().
 */
int
lch_startfn (int (*fn_sandbox) (void *), void *context, u_int flags, struct lc_sandbox **lcsp);

#define LCH_PERMIT_STDERR   0

int
lcs_get (struct lc_host ** lchpp);

#define lch_recv            host_recv
#define lch_recv_rights     host_recv_rights
#define lch_rpc             host_rpc
#define lch_rpc_rights      host_rpc_rights
#define lch_send            host_send
#define lch_send_rights     host_send_rights

#define lcs_recv            sandbox_recv
#define lcs_recv_rights     sandbox_recv_rights
#define lcs_recvrpc         sandbox_recvrpc
#define lcs_recvrpc_rights  sandbox_recvrpc_rights
#define lcs_send            sandbox_send
#define lcs_send_rights     sandbox_send_rights
#define lcs_sendrpc         sandbox_sendrpc
#define lcs_sendrpc_rights  sandbox_sendrpc_rights

#endif /* !_LIBCAPSICUM8_H_ */
