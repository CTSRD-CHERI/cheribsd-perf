.include "../cheridefs.mk"

CFLAGS+=-DNO_XZ_SUPPORT -DNO_BZIP2_SUPPORT

GZIP_U_CFLAGS=-DSB_GZIP_TYPE=\"u\" -DSB_LIBZ_TYPE=\"u\" -I../libz_all/
GZIP_A_CFLAGS=-DSB_GZIP_TYPE=\"a\" -DSB_LIBZ_TYPE=\"u\" -DSB_GZIP_CAPSICUM -I../libsep/ -I../libz_all/
GZIP_H_CFLAGS=-DSB_GZIP_TYPE=\"h\" -DSB_LIBZ_TYPE=\"u\" -DSB_GZIP_LIBCHERI  -I../libz_all/
GZIP_U_LIBZ_H_CFLAGS=-DSB_GZIP_TYPE=\"u\" -DSB_LIBZ_TYPE=\"h\" -I../libz_h2/
GZIP_U_LIBZ_A_CFLAGS=-DSB_GZIP_TYPE=\"u\" -DSB_LIBZ_TYPE=\"a\" -I../libz_all/

# Hmm, not completely unmodified then
GZIP_U_LIBZ_A_CFLAGS+=-DSB_LIBZ_CAPSICUM

GZIP_U_LDADD=../libz_all/libz_u.a
GZIP_A_LDADD=../libz_all/libz_u.a ../libsep/libsep.a
GZIP_H_LDADD=../libcheri_u/libcheri.a
GZIP_H_HELPER_LDADD=../libz_all/libz_u.a ../libc_cheri_u/libc_cheri.a
GZIP_U_LIBZ_H_LDADD=../libz_h2/libzwrapper.a
GZIP_U_LIBZ_A_LDADD=../libz_all/libz_a.a

all: dep progs
		echo done

clean: clean-dep clean-progs
		echo done

progs: gzip_u gzip_a gzip_h gzip_u_libz_h gzip_u_libz_a
		echo done

everything: clean all push

dep: libsep libz_u libz_a libcheri_u libc_cheri_u libz_h2
		echo done

clean-dep: clean-libsep clean-libz_u clean-libz_a clean-libz_h2 clean-libcheri_u clean-libc_cheri_u
		echo done

libz_u:
		cd ../libz_all && \
		$(MAKE) libz_u.a

clean-libz_u:
		cd ../libz_all && \
		$(MAKE) clean

libz_a:
		cd ../libz_all && \
		$(MAKE) libz_a.a

clean-libz_a:
		cd ../libz_all && \
		$(MAKE) clean

libz_h2:
		cd ../libz_h2 && \
		$(MAKE) -f Makefile-libz.CHERI all && \
		$(MAKE) -f Makefile-wrapper.CHERI all && \
		$(MAKE) -f Makefile-helper.CHERI all

clean-libz_h2:
		cd ../libz_h2 && \
		$(MAKE) -f Makefile-libz.CHERI clean && \
		$(MAKE) -f Makefile-wrapper.CHERI clean && \
		$(MAKE) -f Makefile-helper.CHERI clean

libsep:
		cd ../libsep && \
		$(MAKE) -f Makefile.CHERI all

clean-libsep:
		cd ../libsep && \
		$(MAKE) -f Makefile.CHERI clean

libcheri_u:
		cd ../libcheri_u && \
		$(MAKE) -f Makefile.CHERI all

clean-libcheri_u:
		cd ../libcheri_u && \
		$(MAKE) -f Makefile.CHERI clean

libc_cheri_u:
		cd ../libc_cheri_u && \
		touch cheri_invoke.So && \
		$(MAKE) -f Makefile.CHERI all

clean-libc_cheri_u:
		cd ../libc_cheri_u && \
		$(MAKE) -f Makefile.CHERI clean

gzip_u.o:
		$(CC) $(CFLAGS) $(GZIP_U_CFLAGS) -c gzip.c -o gzip_u.o

gzip_a.o:
		$(CC) $(CFLAGS) $(GZIP_A_CFLAGS) -c gzip.c -o gzip_a.o

gzip_h.o:
		$(CC) $(CFLAGS) $(GZIP_H_CFLAGS) -c gzip.c -o gzip_h.o

gzip_u_libz_h.o:
		$(CC) $(CFLAGS) $(GZIP_U_LIBZ_H_CFLAGS) -c gzip.c -o gzip_u_libz_h.o

gzip_u_libz_a.o:
		$(CC) $(CFLAGS) $(GZIP_U_LIBZ_A_CFLAGS) -c gzip.c -o gzip_u_libz_a.o

gzsandbox_a.o:
		$(CC) $(CFLAGS) $(GZIP_A_CFLAGS) -c gzsandbox_a.c -o gzsandbox_a.o

gzsandbox_h.o:
		$(CC) $(CFLAGS) $(GZIP_H_CFLAGS) -c gzsandbox_h.c -o gzsandbox_h.o

gzsandbox-helper_h.o:
		$(CC) $(CFLAGS) $(GZIP_H_CFLAGS) -c gzsandbox-helper_h.c -o gzsandbox-helper_h.o

gzip_u: gzip_u.o libz_u
		$(CC) $(CFLAGS) -o gzip_u gzip_u.o $(GZIP_U_LDADD)

gzip_a: gzip_a.o gzsandbox_a.o libz_u libsep
		$(CC) $(CFLAGS) -o gzip_a gzip_a.o gzsandbox_a.o $(GZIP_A_LDADD)

gzip_h: gzip_h.o gzsandbox_h.o gzsandbox-helper_h.o libz_u libcheri_u libc_cheri_u
		$(CC) $(CFLAGS) -o gzip_h gzip_h.o gzsandbox_h.o $(GZIP_H_LDADD)
		$(CC) $(CFLAGS) -Wl,--script=$(CHERI_SANDBOX_LD) -nostdlib -o gzsandbox-helper_h gzsandbox-helper_h.o $(GZIP_H_HELPER_LDADD)
		$(OBJCOPY) -S -O binary gzsandbox-helper_h gzsandbox-helper_h.bin

gzip_u_libz_h: gzip_u_libz_h.o libz_h2
		$(CC) $(CFLAGS) -o gzip_u_libz_h gzip_u_libz_h.o $(GZIP_U_LIBZ_H_LDADD)

gzip_u_libz_a: gzip_u_libz_a.o libz_a
		$(CC) $(CFLAGS) -o gzip_u_libz_a gzip_u_libz_a.o $(GZIP_U_LIBZ_A_LDADD)

clean-progs:
		rm -f *.o gzip gzip_u gzip_a gzip_h gzip_u_libz_h gzip_u_libz_a gzsandbox-helper_h gzsandbox-helper_h.bin

push:
		$(CHERI_PUSH) gzip_u $(CHERI_PUSH_DIR)/gzip_u
		$(CHERI_PUSH) gzip_a $(CHERI_PUSH_DIR)/gzip_a
		$(CHERI_PUSH) gzip_h $(CHERI_PUSH_DIR)/gzip_h
		$(CHERI_PUSH) gzip_u_libz_h $(CHERI_PUSH_DIR)/gzip_u_libz_h
		$(CHERI_PUSH) gzip_u_libz_a $(CHERI_PUSH_DIR)/gzip_u_libz_a
		$(CHERI_PUSH) ../libz_h2/lzsandbox-helper.bin $(CHERI_PUSH_DIR)/lzsandbox-helper.bin
		$(CHERI_PUSH) gzsandbox-helper_h.bin $(CHERI_PUSH_DIR)/gzsandbox-helper_h.bin
		$(CHERI_PUSH) test.sh $(CHERI_PUSH_DIR)/test.sh
