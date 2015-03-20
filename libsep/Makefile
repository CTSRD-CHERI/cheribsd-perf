.include "../cheridefs.mk"
OBJS	=	sandbox.o sandbox_rpc.o

CLEANFILES = $(LIBS) $(OBJS)
NO_MAN=
CFLAGS += -Werror -Wall 
CFLAGS += -Wextra
CFLAGS += $(OPTCFLAGS)

OBJS_U=$(OBJS:S/.o/_u.o/g)
OBJS_SHMEM=$(OBJS:S/.o/_shmem.o/g)

LDFLAGS +=

all: libsep.a libsep_shmem.a

.SUFFIXES: _u.o _shmem.o

.c_u.o:
	$(CC) $(CFLAGS) -fpic -c $< -o $@

.c_shmem.o:
	$(CC) $(CFLAGS) -fpic -DGZ_SHMEM -c $< -o $@

libsep.a: $(OBJS_U)
	ar rcs ${.TARGET} ${.ALLSRC:.c=.o}

libsep_shmem.a: $(OBJS_SHMEM)
	ar rcs ${.TARGET} ${.ALLSRC:.c=.o}

clean:
		rm -f *.a *.so *.o *~ *.core core
