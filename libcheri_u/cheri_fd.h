/*-
 * Copyright (c) 2014 Robert N. M. Watson
 * All rights reserved.
 *
 * This software was developed by SRI International and the University of
 * Cambridge Computer Laboratory under DARPA/AFRL contract (FA8750-10-C-0237)
 * ("CTSRD"), as part of the DARPA CRASH research programme.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _CHERI_FD_H_
#define	_CHERI_FD_H_

/*
 * Interfaces to create/revoke/destroy cheri_fd objects with ambient
 * authority.
 */
int	cheri_fd_new(int fd, struct cheri_object *cop);
void	cheri_fd_revoke(struct cheri_object co);
void	cheri_fd_destroy(struct cheri_object co);

/*
 * Method numbers used during invocation.
 */
#define	CHERI_FD_METHOD_FSTAT_C		1
#define	CHERI_FD_METHOD_LSEEK_C		2
#define	CHERI_FD_METHOD_READ_C		3
#define	CHERI_FD_METHOD_WRITE_C		4

/*
 * All methods return the following structure, which fits in register return
 * values for the calling convention.  In practice, retval0 is what we think
 * of as the normal return value for each method; retval1 holds an errno value
 * if retval0 == -1.  This is near-identical to the semantics of the kernel's
 * td_retval[0,1].
 */
struct cheri_fd_ret {
	register_t	cfr_retval0;	/* Actual return value. */
	register_t	cfr_retval1;	/* errno if cfr_retval0 == -1. */
};

/*
 * Methods that can be invoked on cheri_fd objects regardless of ambient
 * authority.
 */
struct stat;

#ifdef SABI_ONLY
struct cheri_fd_ret	cheri_fd_fstat_c(struct cheri_object fd_object,
			    struct stat *sb);
struct cheri_fd_ret	cheri_fd_lseek_c(struct cheri_object fd_object,
			    off_t offset, int whence);
struct cheri_fd_ret	cheri_fd_read_c(struct cheri_object fd_object,
			    void *buf);
struct cheri_fd_ret	cheri_fd_write_c(struct cheri_object fd_object,
			    const void *buf);
#else /* !SABI_ONLY */
struct cheri_fd_ret	cheri_fd_fstat_c(struct cheri_object fd_object,
			    __capability struct stat *sb_c);
struct cheri_fd_ret	cheri_fd_lseek_c(struct cheri_object fd_object,
			    off_t offset, int whence);
struct cheri_fd_ret	cheri_fd_read_c(struct cheri_object fd_object,
			    __capability void *buf_c);
struct cheri_fd_ret	cheri_fd_write_c(struct cheri_object fd_object,
			    __capability const void *buf_c);
#endif /* SABI_ONLY */

#endif /* !_CHERI_FD_H_ */
