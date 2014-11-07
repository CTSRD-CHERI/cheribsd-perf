#
# $FreeBSD$
#

.include "../../cheridefs.mk"

SRCS=	crt_sb.S				\
	crtbeginC.c				\
	crtendC.c
FILES=	crt_sb_u.o				\
	crtbeginC_u.o				\
	crtendC_u.o

crt_sb_u.o:
	$(CC) $(CFLAGS) -c crt_sb.S -o crt_sb_u.o

crtbeginC_u.o:
	$(CC) $(CFLAGS) -c crtbeginC.c -o crtbeginC_u.o

crtendC_u.o:
	$(CC) $(CFLAGS) -c crtendC.c -o crtendC_u.o

all: $(FILES)
	echo built crt*
clean:
	rm -f $(FILES)
