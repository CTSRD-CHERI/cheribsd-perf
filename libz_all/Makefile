# $FreeBSD$

.include "../cheridefs.mk"

LIBZ_U_CFLAGS=
LIBZ_A1_CFLAGS=-DSB_LIBZ_CAPSICUM -DLZ_SINGLE_SANDBOX -I../libsep/
LIBZ_AM_CFLAGS=-DSB_LIBZ_CAPSICUM -I../libsep/

CFLAGS+=$(OPTCFLAGS)

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
OBJS_A1=$(OBJS:S/.o/_a1.o/g)
OBJS_AM=$(OBJS:S/.o/_am.o/g)

.SUFFIXES: _a.c .c _u.o _a1.o _am.o .o

.c_u.o:
	$(CC) $(CFLAGS) $(LIBZ_U_CFLAGS) -c $< -o $@

.c_a1.o:
	$(CC) $(CFLAGS) $(LIBZ_A1_CFLAGS) -c $< -o $@

.c_am.o:
	$(CC) $(CFLAGS) $(LIBZ_AM_CFLAGS) -c $< -o $@

libz_u.a: $(OBJS_U)
	$(AR) -r libz_u.a $(OBJS_U)

.PHONY: libz_a
libz_a1.a: libz_a
libz_am.a: libz_a
libz_a: $(OBJS_A1) $(OBJS_AM)
	mkdir -p tmpdir
	cd tmpdir && \
	$(AR) -x ../../libsep/libsep.a && \
	cd ..
	$(AR) -r libz_a1.a $(OBJS_A1) tmpdir/*.o
	$(AR) -r libz_am.a $(OBJS_AM) tmpdir/*.o

clean:
	rm -f *.so *.o *.so.1 *.So *.po *.a *.gz
	rm -rf tmpdir

