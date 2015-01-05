# $FreeBSD$

.include "../cheridefs.mk"

# "_u" targets: unmodified
# "_j" targets: CCall replaced with CJALR
# "_sabi" targets: complete removal of sandboxing, but still using the sandbox ABI

SRCS=	cheri_enter.c			\
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

CFLAGS_SABI=	-DSABI_ONLY -mabi=sandbox

all: libcheri_u.a libcheri_j.a libcheri_sabi.a

.PATH: ${.CURDIR}/${MACHINE_ARCH}

OBJS:=$(SRCS:S/.c/.o/g)
OBJS:=$(OBJS:S/.S/.o/g)

OBJS_U=$(OBJS) cheri_invoke_u.o
OBJS_J=$(OBJS) cheri_invoke_j.o
OBJS_SABI=$(OBJS:S/.o/_sabi.o/g)

.SUFFIXES: _sabi.o

.c_sabi.o: $<
	$(CC) $(CFLAGS) $(CFLAGS_SABI) -c -o $@ $<

.S_sabi.o: $<
	$(CC) $(CFLAGS) $(CFLAGS_SABI) -c -o $@ $<

cheri_invoke_u.o: mips64/cheri_invoke.S
	$(CC) $(CFLAGS) -o cheri_invoke_u.o -c mips64/cheri_invoke.S

cheri_invoke_j.o: mips64/cheri_invoke.S
	$(CC) $(CFLAGS) -DNO_CCALL -o cheri_invoke_j.o -c mips64/cheri_invoke.S

libcheri_u.a: $(OBJS_U)
	$(AR) -r libcheri_u.a $(OBJS_U)

libcheri_j.a: $(OBJS_J)
	$(AR) -r libcheri_j.a $(OBJS_J)

libcheri_sabi.a: $(OBJS_SABI)
	$(AR) -r libcheri_sabi.a $(OBJS_SABI)

clean:
	rm -f *.a *.o *.so *.So *.so.1 *.po

