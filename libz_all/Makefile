# $FreeBSD$

.include "../cheridefs.mk"

LIBZ_U_CFLAGS=
LIBZ_A_CFLAGS=-DSB_LIBZ_CAPSICUM -I../libsep/

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
OBJS+=		lzsandbox_a.o

OBJS_U=$(OBJS:S/.o/_u.o/g)
OBJS_A=$(OBJS:S/.o/_a.o/g)

.SUFFIXES: _a.c .c _u.o _a.o .o

.c_u.o:
	$(CC) $(CFLAGS) $(LIBZ_U_CFLAGS) -c $< -o $@

.c_a.o:
	$(CC) $(CFLAGS) $(LIBZ_A_CFLAGS) -c $< -o $@

libz_u.a: $(OBJS_U)
	$(AR) -r libz_u.a $(OBJS_U)

libz_a.a: $(OBJS_A)
	mkdir -p tmpdir
	cd tmpdir && \
	$(AR) -x ../../libsep/libsep.a && \
	cd ..
	$(AR) -r libz_a.a $(OBJS_A) tmpdir/*.o

clean:
	rm -f *.so *.o *.so.1 *.So *.po *.a *.gz
	rm -rf tmpdir

