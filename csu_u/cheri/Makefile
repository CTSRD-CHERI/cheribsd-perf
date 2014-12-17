#
# $FreeBSD$
#

.include "../../cheridefs.mk"

# "_u" targets: unmodified
# "_j" targets: CCall replaced with CJALR

CFLAGS_U=$(CFLAGS)
CFLAGS_J=$(CFLAGS) -DNO_CCALL

SRCS=	crt_invoke.S				\
	crt_rtld.S				\
	crtbeginC.c				\
	crtendC.c
FILES_U=	crt_invoke_u.o				\
	crt_rtld_u.o				\
	crtbeginC_u.o				\
	crtendC_u.o
FILES_J=	crt_invoke_j.o				\
	crt_rtld_j.o				\
	crtbeginC_j.o				\
	crtendC_j.o

crt_invoke_u.o:
	$(CC) $(CFLAGS_U) -c crt_invoke.S -o crt_invoke_u.o

crt_rtld_u.o:
	$(CC) $(CFLAGS_U) -c crt_rtld.S -o crt_rtld_u.o

crtbeginC_u.o:
	$(CC) $(CFLAGS_U) -c crtbeginC.c -o crtbeginC_u.o

crtendC_u.o:
	$(CC) $(CFLAGS_U) -c crtendC.c -o crtendC_u.o

crt_invoke_j.o:
	$(CC) $(CFLAGS_J) -c crt_invoke.S -o crt_invoke_j.o

crt_rtld_j.o:
	$(CC) $(CFLAGS_J) -c crt_rtld.S -o crt_rtld_j.o

crtbeginC_j.o:
	$(CC) $(CFLAGS_J) -c crtbeginC.c -o crtbeginC_j.o

crtendC_j.o:
	$(CC) $(CFLAGS_J) -c crtendC.c -o crtendC_j.o

FILES=$(FILES_U) $(FILES_J)

all: $(FILES) $(FILES)
	echo built crt*
clean:
	rm -f $(FILES) *.o
