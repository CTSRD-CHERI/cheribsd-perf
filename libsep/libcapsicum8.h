#ifndef _LIBCAPSICUM8_H_
#define _LIBCAPSICUM8_H_

/* Compatibility layer between the new (FreeBSD 10+) and old (FreeBSD 8.0) libcapsicum API, implemented via libsep. */ 

#define lc_sandbox      sandbox_cb

#define lch_recv        host_recv
#define lch_recv_rights host_recv_rights
#define lch_rpc         host_rpc
#define lch_rpc_rights  host_rpc_rights
#define lch_send        host_send
#define lch_send_rights host_send_rights

#define lcs_recv        sandbox_recv
#define lcs_recv_rights sandbox_recv_rights
#define lcs_recv_rpc    sandbox_recv_rpc
#define lcs_send        sandbox_send
#define lcs_send_rights sandbox_send_rights
#define lcs_send_rpc    sandbox_send_rpc

#endif /* !_LIBCAPSICUM8_H_ */
