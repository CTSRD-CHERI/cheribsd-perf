CHERIBSD?=/home/mbv21/trunk/tools/cheribsd
CHERI_SDK?=/home/mbv21/trunk/tools/sdk
CHERI_SYSROOT:=$(CHERI_SDK)/sysroot
CHERI_SDK_BIN:=$(CHERI_SDK)/bin
#CHERI_SANDBOX_LD:=$(CHERI_SYSROOT)/usr/libdata/ldscripts/sandbox.ld
CHERI_SANDBOX_LD:=${.CURDIR}/../libcheri_u/sandbox.ld
CFLAGS=
CFLAGS+=-O2

CC:=PATH=$(CHERI_SDK_BIN):$(CHERI_SDK)/bin $(CHERI_SDK_BIN)/clang
NM:=$(CHERI_SDK)/bin/nm
AS:=$(CHERI_SDK)/bin/as
OBJCOPY:=$(CHERI_SDK)/bin/objcopy
GIT_COMMIT_STRING!=git log | grep commit | head -n 1 | cut -d" " -f2 | sed s/^/\\\\\"/ | sed s/$$/\\\\\"/
CFLAGS+=--sysroot=$(CHERI_SYSROOT) -DGIT_COMMIT_STRING=$(GIT_COMMIT_STRING)
CPOSTFLAGS+=-target cheri-unknown-freebsd -msoft-float -B$(CHERI_SDK)

#CHERI_SSH_HOST:=oregano

# CHERI_PUSH is intended to be used in the form
# $(CHERI_PUSH) local_file $(CHERI_PUSH_DIR)/remote_file
#CHERI_PUSH=/home/mbv21/git-tmp/myncp/myncp 192.168.1.100 8888
#CHERI_PULL=/home/mbv21/git-tmp/myncp/myncp -1 8888

#CHERI_PUSH_DIR=/mnt2

#CHERI_PUSH=scp -i ~/.ssh/mbv21
#CHERI_PUSH_DIR=mbv21@cherrybox.sec.cl.cam.ac.uk:~/tmp
CHERI_PUSH=scp
CHERI_PUSH_DIR=pot:~/tmp
CHERI_PULL=$(CHERI_PUSH) $(CHERI_PUSH_DIR)/results.tar .

MACHINE_ARCH=mips64
MACHINE=mips
