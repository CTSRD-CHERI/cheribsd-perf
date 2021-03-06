#-
# Copyright (c) 2012-2014 Robert N. M. Watson
# All rights reserved.
#
# This software was developed by SRI International and the University of
# Cambridge Computer Laboratory under DARPA/AFRL contract (FA8750-10-C-0237)
# ("CTSRD"), as part of the DARPA CRASH research programme.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#

#include <machine/cherireg.h>

#.set mips64
.set noreorder
#.set nobopt
.set noat

#
# CHERI sandbox runtime startup code -- begins execution inside a sandbox and
# is responsible for bootstrapping MIPS-compiled C code (without CHERI
# extensions).
#
# Expectations on the CCall mechanism
#    $c1, $c2 contain the sealed invoked code and data capability
#    $pcc contains the unsealed invoked code capability
#    $idc contains the unsealed invoked data capability
#
# Expectations on the caller
#    $a0-$a7 will contain non-capability arguments
#    $c3-$c10 will contain capability arguments
#
# Expectations on the class code and instance setup:
#    $c0, $pcc contain access to (100% overlapped) sandbox code and data
#
# Things that will be set up for the callee C code
#    $c0, $c11 will be set up to match the class
#    $c11 will have have the global bit cleared
#    $sp, $fp will point into an in-sandbox stack, relative to $c11
#
# __invoke_vector will jump to the 'invoke' symbol and arrange a CReturn when
# that function returns.  This code also implements 'abort' and '_exit'
# symbols that can be used to fall back out quickly and efficiently.
#
# On return:
#    $v0, $v1 will contain non-capability return values
#    $c3 will contain a capability return value
#
# Sandbox heap information is extracted from the sandbox metadata structure.
# $c26 is assumed to have room for a stack at the top, although its length is
# currently undefined.
#
# For now, assume:
# (1) The caller has not set up the general-purpose register context, that's
#     our job.
# (2) The goal is not to set up the CHERI calling convention .. yet; we will
#     run only native MIPS64 code, or hand-crafted CHERI assembler.  Thus
#     must change before we can run Clang-compiled CHERI code.
#
	.text
	.global __invoke_vector
	.ent __invoke_vector
__invoke_vector:

	#
	# Set up in-sandbox stack; assume that suitable space for a stack has
	# been made at the top of the sandbox address space described by IDC.
	#
	# XXXRW: We make the stack capability ephemeral, and disallow storing
	# ephemeral capabilities via $c0 here, rather than in the TCB.  This
	# is to ensure software is ready for proper stack handling.  We can
	# remove these candperms once that is present.
	#
	dli	$t0, (~CHERI_PERM_GLOBAL)
#ifdef NO_CCALL
	# c26 is not set up yet; callee needs to save (for now)
	candperm	$c11, $c2, $t0
	cgetlen	$sp, $c2
#else
	candperm	$c11, $c26, $t0
	cgetlen	$sp, $c26
#endif
	move	$fp, $sp

#ifdef NO_CCALL
	# Save $c26 (IDC), $c24 (RA)
	daddiu $sp, -64
	csc $c24, $sp, 32($c11)
	csc $c26, $sp, 0($c11)
	cmove $c26, $c2
#endif

	#
	# Install $idc as $c0.  From here on out, we can do MIPS fetches and
	# stores to memory inside the sandbox, and $c0-relative capability
	# fetches and stores.  We are not allowed to store non-global
	# capabilities to the sandbox-local heap (currently).
	#
	dli	$t0, (~CHERI_PERM_STORE_LOCAL_CAP)
	candperm	$c0, $c26, $t0

	# undo those ephemeral perms for now, otherwise things break inside libz
	# (libz likes to store things from the passed parameters in the stack to
	# its internal structures in heap-allocated memory)
	cmove $c11, $c26
	cmove $c0, $c26

	#
	# The compiler is responsible for scrubbing unused argument registers
	# (since only it has the information required to do so).  CCall is
	# responsible for scrubbing all other registers.
	#

	#
	# Set up global pointer.
	#
	dla	$gp, _gp

	#
	# Call the MIPS/CHERI-ABI C "invoke" function.
	#
	dla	$t9, invoke
	jalr	$t9
	nop			# Branch-delay slot

creturn_target:
	#
	# The compiler is responsible for scrubbing unused return-value
	# registers # (since only it has the information required to do so).
	# CReturn is responsible for scrubbing all other registers.
	#

	#
	# Return to caller.
	#
#ifdef NO_CCALL
	# restore $c26, $c24 and return
	clc $c24, $sp, 32($c11)
	clc $c26, $sp, 0($c11)
	daddiu $sp, 64
	cjr $c24
#else
	creturn
#endif
	.end __invoke_vector

	.global abort
	.ent abort
abort:
	#
	# In general, the compiler should be scrubbing unused return values,
	# as  only it has the information to do so.  However, in the case of
	# abort(), that responsibility lies with the runtime: we explicitly
	# set $v0, and clear $v1/$c3.
	#
	cfromptr	$c3, $c3, $zero
	dli	$v0, -2
	b	creturn_target
	dli	$v1, 0		# Branch-delay slot
	.end abort

	.global _exit
	.ent _exit
_exit:
	cfromptr	$c3, $c3, $zero
	move	$v0, $a0
	b	creturn_target
	dli	$v1, 0		# Branch-delay slot
	.end _exit

	.data
