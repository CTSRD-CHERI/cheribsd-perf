/*-
 * Copyright (c) 2012-2014 Robert N. M. Watson
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

#ifndef _SANDBOX_INTERNAL_H_
#define	_SANDBOX_INTERNAL_H_

extern int	sb_verbose;

/*
 * Description of a 'sandbox class': an instance of code that may be sandboxed
 * and invoked, along with statistics/monitoring information, etc.
 *
 * NB: For now, support up to 'SANDBOX_CLASS_METHOD_COUNT' sets of method
 * statistics, which will be indexed by method number.  If the requested
 * method number isn't in range, use the catchall entry instead.
 *
 * XXXRW: Ideally, we would load this data from the target ELF rather than
 * letting the caller provide it.
 */
#define	SANDBOX_CLASS_METHOD_COUNT	32
struct sandbox_class {
	char			*sbc_path;
	int			 sbc_fd;
	struct stat		 sbc_stat;
	size_t			 sbc_sandboxlen;
	struct sandbox_class_stat	*sbc_sandbox_class_statp;
	struct sandbox_method_stat	*sbc_sandbox_method_nonamep;
	struct sandbox_method_stat	*sbc_sandbox_methods[
					    SANDBOX_CLASS_METHOD_COUNT];
};

/*-
 * Description of a 'sandbox object' or 'sandbox instance': an in-flight
 * combination of code and data.  Currently, due to compiler limitations, we
 * must conflate 'code', 'heap', and 'stack', but eventually would like to
 * allow one VM mapping of code to serve many object instances.  This would
 * also ease supporting multithreaded objects.
 *
 * TODO:
 * - Add atomically set flag and assertion to ensure single-threaded entry to
 *   the sandbox.
 * - Once the compiler supports it, move the code memory mapping and
 *   capability out of sandbox_object into sandbox_class.
 */
struct sandbox_object {
	struct sandbox_class	*sbo_sandbox_classp;
	void			*sbo_mem;
	register_t		 sbo_sandboxlen;
	register_t		 sbo_heapbase;
	register_t		 sbo_heaplen;
	struct cheri_object	 sbo_cheri_object;
	struct cheri_object	 sbo_cheri_system_object;
	struct sandbox_object_stat	*sbo_sandbox_object_statp;
};

/*
 * A classic 'sandbox' is actually a combination of a sandbox class and a
 * sandbox object.  We continue to support this model as it is used in some
 * CHERI demo and test code.
 */
struct sandbox {
	struct sandbox_class	*sb_sandbox_classp;
	struct sandbox_object	*sb_sandbox_objectp;
};

int	sandbox_object_load(struct sandbox_class *sbcp,
	    struct sandbox_object *sbop);
void	sandbox_object_unload(struct sandbox_class *sbcp,
	    struct sandbox_object *sbop);

#endif /* !_SANDBOX_INTERNAL_H_ */
