# $FreeBSD$

.include "../cheridefs.mk"

LIB=z
SHLIB_MAJOR=1
USE_CHERI=yes
OBJS+=		adler32.o
OBJS+=		compress.o
OBJS+=		crc32.o
OBJS+=		deflate.o
OBJS+=		gzclose.o
OBJS+=		gzlib.o
OBJS+=		gzread.o
OBJS+=		gzwrite.o
OBJS+=		infback.o
OBJS+=		inffast.o
OBJS+=		inflate.o
OBJS+=		inftrees.o
OBJS+=		trees.o
OBJS+=		uncompr.o
OBJS+=		zopen.o
OBJS+=		zutil.o

all: $(OBJS)
	$(AR) -r libz.a $(OBJS)

clean:
	rm -f lib$(LIB).a *.so *.o *.so.1 *.So *.po *.a *.gz

