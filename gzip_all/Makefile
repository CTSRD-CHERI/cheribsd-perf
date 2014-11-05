.include "../cheridefs.mk"

CFLAGS+=-DNO_XZ_SUPPORT -DNO_BZIP2_SUPPORT

OPTCFLAGS+=-DSB_COLLECT_STATS

CFLAGS+=$(OPTCFLAGS)

# allows specifying BUFLEN by -B option
CFLAGS+=-DDYNAMIC_BUFLEN

GZIP_U_CFLAGS=-DSB_GZIP_TYPE=\"u\" -DSB_LIBZ_TYPE=\"u\" -I../libz_all/
GZIP_A_CFLAGS=-DSB_GZIP_TYPE=\"a\" -DSB_LIBZ_TYPE=\"u\" -DSB_GZIP_CAPSICUM -I../libsep/ -I../libz_all/
GZIP_H_CFLAGS=-DSB_GZIP_TYPE=\"h\" -DSB_LIBZ_TYPE=\"u\" -DSB_GZIP_LIBCHERI -I../libz_all/
GZIP_U_LIBZ_H1_CFLAGS=-DSB_GZIP_TYPE=\"u\" -DSB_LIBZ_TYPE=\"h1\" -I../libz_h2/
GZIP_U_LIBZ_HM_CFLAGS=-DSB_GZIP_TYPE=\"u\" -DSB_LIBZ_TYPE=\"hm\" -I../libz_h2/
GZIP_U_LIBZ_A_CFLAGS=-DSB_GZIP_TYPE=\"u\" -DSB_LIBZ_TYPE=\"a\" -I../libz_all/
GZIP_U_LIBZ_C_CFLAGS=-DSB_GZIP_TYPE=\"u\" -DSB_LIBZ_TYPE=\"c\" -I../libz_h2/
GZIP_A_LIBZ_C_CFLAGS=-DSB_GZIP_TYPE=\"a\" -DSB_LIBZ_TYPE=\"c\" -DSB_GZIP_CAPSICUM -I../libsep/ -I../libz_h2/

# Hmm, not completely unmodified then
GZIP_U_LIBZ_A_CFLAGS+=-DSB_LIBZ_CAPSICUM
GZIP_U_LIBZ_C_CFLAGS+=-DZLIB_CAP_ONLY
GZIP_A_LIBZ_C_CFLAGS+=-DZLIB_CAP_ONLY

GZIP_U_LDADD=../libz_all/libz_u.a
GZIP_A_LDADD=../libz_all/libz_u.a ../libsep/libsep.a
GZIP_H_LDADD=../libcheri_u/libcheri.a
GZIP_H_HELPER_LDADD=../libz_all/libz_u.a ../libc_cheri_u/libc_cheri.a
GZIP_U_LIBZ_H1_LDADD=../libz_h2/libzwrapper1.a
GZIP_U_LIBZ_HM_LDADD=../libz_h2/libzwrapperm.a
GZIP_U_LIBZ_A_LDADD=../libz_all/libz_a.a
GZIP_U_LIBZ_C_LDADD=../libz_h2/libz_c.a
GZIP_A_LIBZ_C_LDADD=../libz_h2/libz_c.a ../libsep/libsep.a

all: dep progs
		echo done

clean: clean-dep clean-progs
		echo done

progs: gzip_u gzip_a gzip_h gzip_u_libz_h1 gzip_u_libz_hm gzip_u_libz_a gzip_u_libz_c gzip_a_libz_c
		echo done

everything: clean all push

dep: libsep libz_u libz_a libcheri_u libc_cheri_u libz_h2 libz_c
		echo done

clean-dep: clean-libsep clean-libz_u clean-libz_a clean-libz_h2 clean-libz_c clean-libcheri_u clean-libc_cheri_u
		echo done

libz_u:
		cd ../libz_all && \
		OPTCFLAGS=$(OPTCFLAGS) $(MAKE) libz_u.a

clean-libz_u:
		cd ../libz_all && \
		OPTCFLAGS=$(OPTCFLAGS) $(MAKE) clean

libz_a:
		cd ../libz_all && \
		OPTCFLAGS=$(OPTCFLAGS) $(MAKE) libz_a.a

clean-libz_a:
		cd ../libz_all && \
		OPTCFLAGS=$(OPTCFLAGS) $(MAKE) clean

libz_h2:
		cd ../libz_h2 && \
		OPTCFLAGS=$(OPTCFLAGS) $(MAKE) -f Makefile-libz.CHERI libz_h.a && \
		OPTCFLAGS=$(OPTCFLAGS) $(MAKE) -f Makefile-wrapper.CHERI all && \
		OPTCFLAGS=$(OPTCFLAGS) $(MAKE) -f Makefile-helper.CHERI all

libz_c:
		cd ../libz_h2 && \
		OPTCFLAGS=$(OPTCFLAGS) $(MAKE) -f Makefile-libz.CHERI libz_c.a

clean-libz_h2:
		cd ../libz_h2 && \
		OPTCFLAGS=$(OPTCFLAGS) $(MAKE) -f Makefile-libz.CHERI clean && \
		OPTCFLAGS=$(OPTCFLAGS) $(MAKE) -f Makefile-wrapper.CHERI clean && \
		OPTCFLAGS=$(OPTCFLAGS) $(MAKE) -f Makefile-helper.CHERI clean

clean-libz_c:
		cd ../libz_h2 && \
		OPTCFLAGS=$(OPTCFLAGS) $(MAKE) -f Makefile-libz.CHERI clean

libsep:
		cd ../libsep && \
		OPTCFLAGS=$(OPTCFLAGS) $(MAKE) -f Makefile.CHERI all

clean-libsep:
		cd ../libsep && \
		OPTCFLAGS=$(OPTCFLAGS) $(MAKE) -f Makefile.CHERI clean

libcheri_u:
		cd ../libcheri_u && \
		OPTCFLAGS=$(OPTCFLAGS) $(MAKE) -f Makefile.CHERI all

clean-libcheri_u:
		cd ../libcheri_u && \
		OPTCFLAGS=$(OPTCFLAGS) $(MAKE) -f Makefile.CHERI clean

libc_cheri_u:
		cd ../libc_cheri_u && \
		touch cheri_invoke.So && \
		OPTCFLAGS=$(OPTCFLAGS) $(MAKE) -f Makefile.CHERI all

clean-libc_cheri_u:
		cd ../libc_cheri_u && \
		OPTCFLAGS=$(OPTCFLAGS) $(MAKE) -f Makefile.CHERI clean

gzip_u.o:
		$(CC) $(CFLAGS) $(GZIP_U_CFLAGS) -c gzip.c -o gzip_u.o

gzip_a.o:
		$(CC) $(CFLAGS) $(GZIP_A_CFLAGS) -c gzip.c -o gzip_a.o

gzip_h.o:
		$(CC) $(CFLAGS) $(GZIP_H_CFLAGS) -c gzip.c -o gzip_h.o

gzip_u_libz_h1.o:
		$(CC) $(CFLAGS) $(GZIP_U_LIBZ_H1_CFLAGS) -c gzip.c -o gzip_u_libz_h1.o

gzip_u_libz_hm.o:
		$(CC) $(CFLAGS) $(GZIP_U_LIBZ_HM_CFLAGS) -c gzip.c -o gzip_u_libz_hm.o

gzip_u_libz_a.o:
		$(CC) $(CFLAGS) $(GZIP_U_LIBZ_A_CFLAGS) -c gzip.c -o gzip_u_libz_a.o

gzip_u_libz_c.o:
		$(CC) $(CFLAGS) $(GZIP_U_LIBZ_C_CFLAGS) -c gzip.c -o gzip_u_libz_c.o

gzip_a_libz_c.o:
		$(CC) $(CFLAGS) $(GZIP_A_LIBZ_C_CFLAGS) -c gzip.c -o gzip_a_libz_c.o

gzsandbox_a.o:
		$(CC) $(CFLAGS) $(GZIP_A_CFLAGS) -c gzsandbox_a.c -o gzsandbox_a.o

gzsandbox_h.o:
		$(CC) $(CFLAGS) $(GZIP_H_CFLAGS) -c gzsandbox_h.c -o gzsandbox_h.o

gzsandbox-helper_h.o:
		$(CC) $(CFLAGS) $(GZIP_H_CFLAGS) -c gzsandbox-helper_h.c -o gzsandbox-helper_h.o

gzsandbox_a_libz_c.o:
		$(CC) $(CFLAGS) $(GZIP_A_LIBZ_C_CFLAGS) -c gzsandbox_a.c -o gzsandbox_a_libz_c.o

gzip_u: gzip_u.o libz_u
		$(CC) $(CFLAGS) -o gzip_u gzip_u.o $(GZIP_U_LDADD)

gzip_a: gzip_a.o gzsandbox_a.o libz_u libsep
		$(CC) $(CFLAGS) -o gzip_a gzip_a.o gzsandbox_a.o $(GZIP_A_LDADD)

gzip_h: gzip_h.o gzsandbox_h.o gzsandbox-helper_h.o libz_u libcheri_u libc_cheri_u
		$(CC) $(CFLAGS) -o gzip_h gzip_h.o gzsandbox_h.o $(GZIP_H_LDADD)
		$(CC) $(CFLAGS) -Wl,--script=$(CHERI_SANDBOX_LD) -nostdlib -o gzsandbox-helper_h gzsandbox-helper_h.o $(GZIP_H_HELPER_LDADD)
		$(OBJCOPY) -S -O binary gzsandbox-helper_h gzsandbox-helper_h.bin

gzip_u_libz_h1: gzip_u_libz_h1.o libz_h2
		$(CC) $(CFLAGS) -o gzip_u_libz_h1 gzip_u_libz_h1.o $(GZIP_U_LIBZ_H1_LDADD)

gzip_u_libz_hm: gzip_u_libz_hm.o libz_h2
		$(CC) $(CFLAGS) -o gzip_u_libz_hm gzip_u_libz_hm.o $(GZIP_U_LIBZ_HM_LDADD)

gzip_u_libz_a: gzip_u_libz_a.o libz_a
		$(CC) $(CFLAGS) -o gzip_u_libz_a gzip_u_libz_a.o $(GZIP_U_LIBZ_A_LDADD)

gzip_u_libz_c: gzip_u_libz_c.o libz_c
		$(CC) $(CFLAGS) -o gzip_u_libz_c gzip_u_libz_c.o $(GZIP_U_LIBZ_C_LDADD)

gzip_a_libz_c: gzip_a_libz_c.o gzsandbox_a_libz_c.o libz_c libsep
		$(CC) $(CFLAGS) -o gzip_a_libz_c gzip_a_libz_c.o gzsandbox_a_libz_c.o $(GZIP_A_LIBZ_C_LDADD)

clean-progs:
		rm -f *.o gzip gzip_u gzip_a gzip_h gzip_u_libz_h1 gzip_u_libz_hm gzip_u_libz_a gzip_u_libz_c gzip_a_libz_c gzsandbox-helper_h gzsandbox-helper_h.bin
		rm -rf results

push: push-gzip_u push-gzip_a push-gzip_h push-gzip_u_libz_h1 push-gzip_u_libz_hm push-gzip_u_libz_a push-gzip_u_libz_c push-gzip_a_libz_c push-test

push-gzip_u:
		$(CHERI_PUSH) gzip_u $(CHERI_PUSH_DIR)/gzip_u

push-gzip_a:
		$(CHERI_PUSH) gzip_a $(CHERI_PUSH_DIR)/gzip_a

push-gzip_h:
		$(CHERI_PUSH) gzip_h $(CHERI_PUSH_DIR)/gzip_h
		$(CHERI_PUSH) gzsandbox-helper_h.bin $(CHERI_PUSH_DIR)/gzsandbox-helper_h.bin

push-gzip_u_libz_h1:
		$(CHERI_PUSH) gzip_u_libz_h1 $(CHERI_PUSH_DIR)/gzip_u_libz_h1
		$(CHERI_PUSH) ../libz_h2/lzsandbox-helper.bin $(CHERI_PUSH_DIR)/lzsandbox-helper.bin

push-gzip_u_libz_hm:
		$(CHERI_PUSH) gzip_u_libz_hm $(CHERI_PUSH_DIR)/gzip_u_libz_hm
		$(CHERI_PUSH) ../libz_h2/lzsandbox-helper.bin $(CHERI_PUSH_DIR)/lzsandbox-helper.bin

push-gzip_u_libz_a:
		$(CHERI_PUSH) gzip_u_libz_a $(CHERI_PUSH_DIR)/gzip_u_libz_a

push-gzip_u_libz_c:
		$(CHERI_PUSH) gzip_u_libz_c $(CHERI_PUSH_DIR)/gzip_u_libz_c

push-gzip_a_libz_c:
		$(CHERI_PUSH) gzip_a_libz_c $(CHERI_PUSH_DIR)/gzip_a_libz_c

push-test:
		$(CHERI_PUSH) test.sh $(CHERI_PUSH_DIR)/test.sh

pull-results:
		rm -rf results
		mkdir results
		cp post_process.sh results/
		cd results && \
		$(CHERI_PULL) && \
		./post_process.sh all

