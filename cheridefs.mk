CHERI_SDK?=/scratch/cheri-tools/sdk
CHERI_SYSROOT?=/scratch/cheri-tools/Build_old/sdk
CC:=PATH=$(CHERI_SDK)/bin:$(CHERI_SYSROOT)/bin $(CHERI_SDK)/bin/clang
GIT_COMMIT_STRING!=git log | grep commit | head -n 1 | cut -d" " -f2 | sed s/^/\\\\\"/ | sed s/$$/\\\\\"/
CFLAGS+=--sysroot=$(CHERI_SYSROOT) -DGIT_COMMIT_STRING=$(GIT_COMMIT_STRING)
CPOSTFLAGS+=-target cheri-unknown-freebsd -msoft-float -B$(CHERI_SYSROOT)
CHERI_SSH_HOST:=oregano

