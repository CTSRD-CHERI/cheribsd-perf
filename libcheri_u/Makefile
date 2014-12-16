# $FreeBSD$

.include "../cheridefs.mk"
CFLAGS+=	-DNO_SANDBOX

SRCS=	cheri_invoke.S			\
	cheri_enter.c			\
	cheri_fd.c			\
	cheri_system.c			\
	cheri_type.c			\
	libcheri_classes.S		\
	libcheri_stat.c			\
	sandbox.c			\
	sandbox_loader.c		\

INCS=	cheri_class.h			\
	cheri_enter.h			\
	cheri_fd.h			\
	cheri_invoke.h			\
	cheri_memcpy.h			\
	cheri_system.h			\
	cheri_type.h			\
	sandbox.h			\
	sandboxasm.h

CFLAGS+=	-I${.CURDIR}/${MACHINE_ARCH}

all: libcheri.a

.PATH: ${.CURDIR}/${MACHINE_ARCH}

OBJS:=$(SRCS:S/.c/.o/g)
OBJS:=$(OBJS:S/.S/.o/g)

libcheri.a: $(OBJS)
	$(AR) -r libcheri.a $(OBJS)

clean:
	rm -f *.a *.o *.so *.So *.so.1 *.po

