CHERIBSD?=/scratch/cheribsd
CHERI_SDK?=/scratch/cherilibs5/trunk/tools/sdk
CHERI_SYSROOT:=$(CHERI_SDK)/sysroot
CHERI_SDK_BIN:=$(CHERI_SDK)/bin
CHERI_SANDBOX_LD:=$(CHERI_SYSROOT)/usr/libdata/ldscripts/sandbox.ld

CC:=PATH=$(CHERI_SDK_BIN):$(CHERI_SDK)/bin $(CHERI_SDK_BIN)/clang
NM:=$(CHERI_SDK)/bin/nm
AS:=$(CHERI_SDK)/bin/as
OBJCOPY:=$(CHERI_SDK)/bin/objcopy
GIT_COMMIT_STRING!=git log | grep commit | head -n 1 | cut -d" " -f2 | sed s/^/\\\\\"/ | sed s/$$/\\\\\"/
CFLAGS+=--sysroot=$(CHERI_SYSROOT) -DGIT_COMMIT_STRING=$(GIT_COMMIT_STRING)
CPOSTFLAGS+=-target cheri-unknown-freebsd -msoft-float -B$(CHERI_SDK)
CFLAGS+=-O2

CHERI_SSH_HOST:=oregano

# CHERI_PUSH is intended to be used in the form
# $(CHERI_PUSH) local_file $(CHERI_PUSH_DIR)/remote_file
CHERI_PUSH=/home/mbv21/git-tmp/myncp/myncp 192.168.1.100 8888
CHERI_PULL=/home/mbv21/git-tmp/myncp/myncp -1 8888

CHERI_PUSH_DIR=/mnt2

MACHINE_ARCH=mips64
MACHINE=mips
