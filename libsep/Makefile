CC		?=	clang
LIBS 	=	libsep.so
OBJS	=	*.o

CLEANFILES = $(LIBS) $(OBJS)
NO_MAN=
CFLAGS += -Werror -Wall 
CFLAGS += -Wextra

LDFLAGS +=

all: libsep.so libsep.a

libsep.so:	sandbox.c sandbox_rpc.c
	$(CC) $(CFLAGS) -fpic -c ${.ALLSRC}
	$(CC) -shared -o ${.TARGET} ${.ALLSRC:.c=.o}

libsep.a:	sandbox.c sandbox_rpc.c
	$(CC) $(CFLAGS) -fpic -c ${.ALLSRC}
	ar rcs ${.TARGET} ${.ALLSRC:.c=.o}

clean:
		rm -f *.a *.so *.o *~ *.core core
