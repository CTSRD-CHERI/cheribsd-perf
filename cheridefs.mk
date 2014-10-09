CHERI_SDK?=/scratch/cheri-tools/sdk
CHERI_SYSROOT?=/scratch/cheri-tools/Build_old/sdk
CC:=PATH=$(CHERI_SDK)/bin:$(CHERI_SYSROOT)/bin $(CHERI_SDK)/bin/clang
CFLAGS+=--sysroot=$(CHERI_SYSROOT)
CPOSTFLAGS+=-target cheri-unknown-freebsd -msoft-float -B$(CHERI_SYSROOT)
CHERI_SSH_HOST=oregano
