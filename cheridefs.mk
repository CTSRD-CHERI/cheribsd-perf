CHERI_SDK?=/scratch/cherilibs2/trunk/tools/sdk
CHERI_SDK_BIN:=$(CHERI_SDK)/tools/bin
CHERI_SANDBOX_LD:=$(CHERI_SDK)/usr/libdata/ldscripts/sandbox.ld

CC:=PATH=$(CHERI_SDK_BIN):$(CHERI_SDK)/bin $(CHERI_SDK_BIN)/clang
NM:=$(CHERI_SDK)/bin/nm
GIT_COMMIT_STRING!=git log | grep commit | head -n 1 | cut -d" " -f2 | sed s/^/\\\\\"/ | sed s/$$/\\\\\"/
CFLAGS+=--sysroot=$(CHERI_SDK) -DGIT_COMMIT_STRING=$(GIT_COMMIT_STRING)
CPOSTFLAGS+=-target cheri-unknown-freebsd -msoft-float -B$(CHERI_SDK)

CHERI_SSH_HOST:=oregano
