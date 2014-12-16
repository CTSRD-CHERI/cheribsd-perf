#
# $FreeBSD$
#

.include "../../cheridefs.mk"
CFLAGS+=-DNO_SANDBOX

SRCS=	crt_invoke.S				\
	crt_rtld.S				\
	crtbeginC.c				\
	crtendC.c
FILES=	crt_invoke_u.o				\
	crt_rtld_u.o				\
	crtbeginC_u.o				\
	crtendC_u.o

crt_invoke_u.o:
	$(CC) $(CFLAGS) -c crt_invoke.S -o crt_invoke_u.o

crt_rtld_u.o:
	$(CC) $(CFLAGS) -c crt_rtld.S -o crt_rtld_u.o

crtbeginC_u.o:
	$(CC) $(CFLAGS) -c crtbeginC.c -o crtbeginC_u.o

crtendC_u.o:
	$(CC) $(CFLAGS) -c crtendC.c -o crtendC_u.o

all: $(FILES)
	echo built crt*
clean:
	rm -f $(FILES) *.o
