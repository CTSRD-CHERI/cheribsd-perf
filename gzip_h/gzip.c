/*	$NetBSD: gzip.c,v 1.105 2011/08/30 23:06:00 joerg Exp $	*/

/*-
 * Copyright (c) 1997, 1998, 2003, 2004, 2006 Matthew R. Green
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1997, 1998, 2003, 2004, 2006\
 Matthew R. Green.  All rights reserved.");
__FBSDID("$FreeBSD$");
#endif /* not lint */

/*
 * gzip.c -- GPL free gzip using zlib.
 *
 * RFC 1950 covers the zlib format
 * RFC 1951 covers the deflate format
 * RFC 1952 covers the gzip format
 *
 * TODO:
 *	- use mmap where possible
 *	- make bzip2/compress -v/-t/-l support work as well as possible
 */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <inttypes.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <zlib.h>
#include <fts.h>
#include <libgen.h>
#include <stdarg.h>
#include <getopt.h>
#include <time.h>

#include "gzip.h"
#include "gzsandbox-helper.h"

/* what type of file are we dealing with */
enum filetype {
	FT_GZIP,
#ifndef NO_BZIP2_SUPPORT
	FT_BZIP2,
#endif
#ifndef NO_COMPRESS_SUPPORT
	FT_Z,
#endif
#ifndef NO_PACK_SUPPORT
	FT_PACK,
#endif
#ifndef NO_XZ_SUPPORT
	FT_XZ,
#endif
	FT_LAST,
	FT_UNKNOWN
};

#ifndef NO_BZIP2_SUPPORT
#include <bzlib.h>

#define BZ2_SUFFIX	".bz2"
#define BZIP2_MAGIC	"\102\132\150"
#endif

#ifndef NO_COMPRESS_SUPPORT
#define Z_SUFFIX	".Z"
#define Z_MAGIC		"\037\235"
#endif

#ifndef NO_PACK_SUPPORT
#define PACK_MAGIC	"\037\036"
#endif

#ifndef NO_XZ_SUPPORT
#include <lzma.h>
#define XZ_SUFFIX	".xz"
#define XZ_MAGIC	"\3757zXZ"
#endif

#define GZ_SUFFIX	".gz"

#define BUFLEN		(64 * 1024)

#define GZIP_MAGIC0	0x1F
#define GZIP_MAGIC1	0x8B
#define GZIP_OMAGIC1	0x9E

#define GZIP_TIMESTAMP	(off_t)4
#define GZIP_ORIGNAME	(off_t)10

#define HEAD_CRC	0x02
#define EXTRA_FIELD	0x04
#define ORIG_NAME	0x08
#define COMMENT		0x10

#define OS_CODE		3	/* Unix */

typedef struct {
    const char	*zipped;
    int		ziplen;
    const char	*normal;	/* for unzip - must not be longer than zipped */
} suffixes_t;
static suffixes_t suffixes[] = {
#define	SUFFIX(Z, N) {Z, sizeof Z - 1, N}
	SUFFIX(GZ_SUFFIX,	""),	/* Overwritten by -S .xxx */
#ifndef SMALL
	SUFFIX(GZ_SUFFIX,	""),
	SUFFIX(".z",		""),
	SUFFIX("-gz",		""),
	SUFFIX("-z",		""),
	SUFFIX("_z",		""),
	SUFFIX(".taz",		".tar"),
	SUFFIX(".tgz",		".tar"),
#ifndef NO_BZIP2_SUPPORT
	SUFFIX(BZ2_SUFFIX,	""),
	SUFFIX(".tbz",		".tar"),
	SUFFIX(".tbz2",		".tar"),
#endif
#ifndef NO_COMPRESS_SUPPORT
	SUFFIX(Z_SUFFIX,	""),
#endif
#ifndef NO_XZ_SUPPORT
	SUFFIX(XZ_SUFFIX,	""),
#endif
	SUFFIX(GZ_SUFFIX,	""),	/* Overwritten by -S "" */
#endif /* SMALL */
#undef SUFFIX
};
#define NUM_SUFFIXES (sizeof suffixes / sizeof suffixes[0])
#define SUFFIX_MAXLEN	30

#ifndef GIT_COMMIT_STRING
#define GIT_COMMIT_STRING "(unknown revision)"
#endif // GIT_COMMIT_STRING

static	const char	gzip_version[] = "FreeBSD gzip 20111009 (CHERI, gzip_h, " GIT_COMMIT_STRING ")";

#ifndef SMALL
static	const char	gzip_copyright[] = \
"   Copyright (c) 1997, 1998, 2003, 2004, 2006 Matthew R. Green\n"
"   All rights reserved.\n"
"\n"
"   Redistribution and use in source and binary forms, with or without\n"
"   modification, are permitted provided that the following conditions\n"
"   are met:\n"
"   1. Redistributions of source code must retain the above copyright\n"
"      notice, this list of conditions and the following disclaimer.\n"
"   2. Redistributions in binary form must reproduce the above copyright\n"
"      notice, this list of conditions and the following disclaimer in the\n"
"      documentation and/or other materials provided with the distribution.\n"
"\n"
"   THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR\n"
"   IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES\n"
"   OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.\n"
"   IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,\n"
"   INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,\n"
"   BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;\n"
"   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED\n"
"   AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,\n"
"   OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY\n"
"   OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF\n"
"   SUCH DAMAGE.";
#endif

static	int	cflag;			/* stdout mode */
static	int	dflag;			/* decompress mode */
static	int	lflag;			/* list mode */
int	numflag = 6;		/* gzip -1..-9 value */

#ifndef SMALL
static	int	fflag;			/* force mode */
static	int	kflag;			/* don't delete input files */
int	nflag;			/* don't save name/timestamp */
static	int	Nflag;			/* don't restore name/timestamp */
int	qflag;			/* quiet mode */
static	int	rflag;			/* recursive mode */
int	tflag;			/* test */
static	int	vflag;			/* verbose mode */
static	const char *remove_file = NULL;	/* file to be removed upon SIGINT */
#else
#define		qflag	0
#define		tflag	0
#endif

static	int	exit_value = 0;		/* exit value */

static	char	*infile;		/* name of file coming in */

static	void	maybe_err(const char *fmt, ...) __printflike(1, 2) __dead2;
#if !defined(NO_BZIP2_SUPPORT) || !defined(NO_PACK_SUPPORT) ||	\
    !defined(NO_XZ_SUPPORT)
static	void	maybe_errx(const char *fmt, ...) __printflike(1, 2) __dead2;
#endif
static	void	maybe_warn(const char *fmt, ...) __printflike(1, 2);
static	void	maybe_warnx(const char *fmt, ...) __printflike(1, 2);
static	enum filetype file_gettype(u_char *);
#ifdef SMALL
#define gz_compress(if, of, sz, fn, tm) gz_compress(if, of, sz)
#endif
static	off_t	file_compress(char *, char *, size_t);
static	off_t	file_uncompress(char *, char *, size_t);
static	void	handle_pathname(char *);
static	void	handle_file(char *, struct stat *);
static	void	handle_stdin(void);
static	void	handle_stdout(void);
static	void	print_ratio(off_t, off_t, FILE *);
static	void	print_list(int fd, off_t, const char *, time_t);
static	void	usage(void) __dead2;
static	void	display_version(void) __dead2;
#ifndef SMALL
static	void	display_license(void);
static	void	sigint_handler(int);
#endif
static	const suffixes_t *check_suffix(char *, int);
static	ssize_t	read_retry(int, void *, size_t);

#ifdef SMALL
#define unlink_input(f, sb) unlink(f)
#else
static	off_t	cat_fd(unsigned char *, size_t, off_t *, int fd);
static	void	prepend_gzip(char *, int *, char ***);
static	void	handle_dir(char *);
static	void	print_verbage(const char *, const char *, off_t, off_t);
static	void	print_test(const char *, int);
static	void	copymodes(int fd, const struct stat *, const char *file);
static	int	check_outfile(const char *outfile);
#endif

#ifndef NO_COMPRESS_SUPPORT
static	FILE 	*zdopen(int);
static	off_t	zuncompress(FILE *, FILE *, char *, size_t, off_t *);
#endif

#ifndef NO_PACK_SUPPORT
static	off_t	unpack(int, int, char *, size_t, off_t *);
#endif

#ifndef NO_XZ_SUPPORT
static	off_t	unxz(int, int, char *, size_t, off_t *);
#endif

#ifndef NO_SANDBOX_SUPPORT
int ld_insandbox(void);
int gzsandbox(void *);
#endif

#ifdef SMALL
#define getopt_long(a,b,c,d,e) getopt(a,b,c)
#else
static const struct option longopts[] = {
	{ "stdout",		no_argument,		0,	'c' },
	{ "to-stdout",		no_argument,		0,	'c' },
	{ "decompress",		no_argument,		0,	'd' },
	{ "uncompress",		no_argument,		0,	'd' },
	{ "force",		no_argument,		0,	'f' },
	{ "help",		no_argument,		0,	'h' },
	{ "keep",		no_argument,		0,	'k' },
	{ "list",		no_argument,		0,	'l' },
	{ "no-name",		no_argument,		0,	'n' },
	{ "name",		no_argument,		0,	'N' },
	{ "quiet",		no_argument,		0,	'q' },
	{ "recursive",		no_argument,		0,	'r' },
	{ "suffix",		required_argument,	0,	'S' },
	{ "test",		no_argument,		0,	't' },
	{ "verbose",		no_argument,		0,	'v' },
	{ "version",		no_argument,		0,	'V' },
	{ "fast",		no_argument,		0,	'1' },
	{ "best",		no_argument,		0,	'9' },
	{ "ascii",		no_argument,		0,	'a' },
	{ "license",		no_argument,		0,	'L' },
	{ NULL,			no_argument,		0,	0 },
};
#endif

int
main(int argc, char **argv)
{
	const char *progname = getprogname();
#ifndef SMALL
	char *gzip;
	int len;
#endif
	int ch;

#ifndef SMALL
	if ((gzip = getenv("GZIP")) != NULL)
		prepend_gzip(gzip, &argc, &argv);
	signal(SIGINT, sigint_handler);
#endif

	/*
	 * XXX
	 * handle being called `gunzip', `zcat' and `gzcat'
	 */
	if (strcmp(progname, "gunzip") == 0)
		dflag = 1;
	else if (strcmp(progname, "zcat") == 0 ||
		 strcmp(progname, "gzcat") == 0)
		dflag = cflag = 1;

#ifdef SMALL
#define OPT_LIST "123456789cdhlV"
#else
#define OPT_LIST "123456789acdfhklLNnqrS:tVv"
#endif

	while ((ch = getopt_long(argc, argv, OPT_LIST, longopts, NULL)) != -1) {
		switch (ch) {
		case '1': case '2': case '3':
		case '4': case '5': case '6':
		case '7': case '8': case '9':
			numflag = ch - '0';
			break;
		case 'c':
			cflag = 1;
			break;
		case 'd':
			dflag = 1;
			break;
		case 'l':
			lflag = 1;
			dflag = 1;
			break;
		case 'V':
			display_version();
			/* NOTREACHED */
#ifndef SMALL
		case 'a':
			fprintf(stderr, "%s: option --ascii ignored on this system\n", progname);
			break;
		case 'f':
			fflag = 1;
			break;
		case 'k':
			kflag = 1;
			break;
		case 'L':
			display_license();
			/* NOT REACHED */
		case 'N':
			nflag = 0;
			Nflag = 1;
			break;
		case 'n':
			nflag = 1;
			Nflag = 0;
			break;
		case 'q':
			qflag = 1;
			break;
		case 'r':
			rflag = 1;
			break;
		case 'S':
			len = strlen(optarg);
			if (len != 0) {
				if (len > SUFFIX_MAXLEN)
					errx(1, "incorrect suffix: '%s': too long", optarg);
				suffixes[0].zipped = optarg;
				suffixes[0].ziplen = len;
			} else {
				suffixes[NUM_SUFFIXES - 1].zipped = "";
				suffixes[NUM_SUFFIXES - 1].ziplen = 0;
			}
			break;
		case 't':
			cflag = 1;
			tflag = 1;
			dflag = 1;
			break;
		case 'v':
			vflag = 1;
			break;
#endif
		default:
			usage();
			/* NOTREACHED */
		}
	}
	argv += optind;
	argc -= optind;

	if (argc == 0) {
		if (dflag)	/* stdin mode */
			handle_stdin();
		else		/* stdout mode */
			handle_stdout();
	} else {
		do {
			handle_pathname(argv[0]);
		} while (*++argv);
	}
#ifndef SMALL
	if (qflag == 0 && lflag && argc > 1)
		print_list(-1, 0, "(totals)", 0);
#endif
	exit(exit_value);
}

/* maybe print a warning */
void
maybe_warn(const char *fmt, ...)
{
	va_list ap;

	if (qflag == 0) {
		va_start(ap, fmt);
		vwarn(fmt, ap);
		va_end(ap);
	}
	if (exit_value == 0)
		exit_value = 1;
}

/* ... without an errno. */
void
maybe_warnx(const char *fmt, ...)
{
	va_list ap;

	if (qflag == 0) {
		va_start(ap, fmt);
		vwarnx(fmt, ap);
		va_end(ap);
	}
	if (exit_value == 0)
		exit_value = 1;
}

/* maybe print an error */
void
maybe_err(const char *fmt, ...)
{
	va_list ap;

	if (qflag == 0) {
		va_start(ap, fmt);
		vwarn(fmt, ap);
		va_end(ap);
	}
	exit(2);
}

#if !defined(NO_BZIP2_SUPPORT) || !defined(NO_PACK_SUPPORT) ||	\
    !defined(NO_XZ_SUPPORT)
/* ... without an errno. */
void
maybe_errx(const char *fmt, ...)
{
	va_list ap;

	if (qflag == 0) {
		va_start(ap, fmt);
		vwarnx(fmt, ap);
		va_end(ap);
	}
	exit(2);
}
#endif

#ifndef SMALL
/* split up $GZIP and prepend it to the argument list */
static void
prepend_gzip(char *gzip, int *argc, char ***argv)
{
	char *s, **nargv, **ac;
	int nenvarg = 0, i;

	/* scan how many arguments there are */
	for (s = gzip;;) {
		while (*s == ' ' || *s == '\t')
			s++;
		if (*s == 0)
			goto count_done;
		nenvarg++;
		while (*s != ' ' && *s != '\t')
			if (*s++ == 0)
				goto count_done;
	}
count_done:
	/* punt early */
	if (nenvarg == 0)
		return;

	*argc += nenvarg;
	ac = *argv;

	nargv = (char **)malloc((*argc + 1) * sizeof(char *));
	if (nargv == NULL)
		maybe_err("malloc");

	/* stash this away */
	*argv = nargv;

	/* copy the program name first */
	i = 0;
	nargv[i++] = *(ac++);

	/* take a copy of $GZIP and add it to the array */
	s = strdup(gzip);
	if (s == NULL)
		maybe_err("strdup");
	for (;;) {
		/* Skip whitespaces. */
		while (*s == ' ' || *s == '\t')
			s++;
		if (*s == 0)
			goto copy_done;
		nargv[i++] = s;
		/* Find the end of this argument. */
		while (*s != ' ' && *s != '\t')
			if (*s++ == 0)
				/* Argument followed by NUL. */
				goto copy_done;
		/* Terminate by overwriting ' ' or '\t' with NUL. */
		*s++ = 0;
	}
copy_done:

	/* copy the original arguments and a NULL */
	while (*ac)
		nargv[i++] = *(ac++);
	nargv[i] = NULL;
}
#endif

#ifndef SMALL
/*
 * set the owner, mode, flags & utimes using the given file descriptor.
 * file is only used in possible warning messages.
 */
static void
copymodes(int fd, const struct stat *sbp, const char *file)
{
	struct timeval times[2];
	struct stat sb;

	/*
	 * If we have no info on the input, give this file some
	 * default values and return..
	 */
	if (sbp == NULL) {
		mode_t mask = umask(022);

		(void)fchmod(fd, DEFFILEMODE & ~mask);
		(void)umask(mask);
		return; 
	}
	sb = *sbp;

	/* if the chown fails, remove set-id bits as-per compress(1) */
	if (fchown(fd, sb.st_uid, sb.st_gid) < 0) {
		if (errno != EPERM)
			maybe_warn("couldn't fchown: %s", file);
		sb.st_mode &= ~(S_ISUID|S_ISGID);
	}

	/* we only allow set-id and the 9 normal permission bits */
	sb.st_mode &= S_ISUID | S_ISGID | S_IRWXU | S_IRWXG | S_IRWXO;
	if (fchmod(fd, sb.st_mode) < 0)
		maybe_warn("couldn't fchmod: %s", file);

	TIMESPEC_TO_TIMEVAL(&times[0], &sb.st_atim);
	TIMESPEC_TO_TIMEVAL(&times[1], &sb.st_mtim);
	if (futimes(fd, times) < 0)
		maybe_warn("couldn't utimes: %s", file);

	/* only try flags if they exist already */
        if (sb.st_flags != 0 && fchflags(fd, sb.st_flags) < 0)
		maybe_warn("couldn't fchflags: %s", file);
}
#endif

/* what sort of file is this? */
static enum filetype
file_gettype(u_char *buf)
{

	if (buf[0] == GZIP_MAGIC0 &&
	    (buf[1] == GZIP_MAGIC1 || buf[1] == GZIP_OMAGIC1))
		return FT_GZIP;
	else
#ifndef NO_BZIP2_SUPPORT
	if (memcmp(buf, BZIP2_MAGIC, 3) == 0 &&
	    buf[3] >= '0' && buf[3] <= '9')
		return FT_BZIP2;
	else
#endif
#ifndef NO_COMPRESS_SUPPORT
	if (memcmp(buf, Z_MAGIC, 2) == 0)
		return FT_Z;
	else
#endif
#ifndef NO_PACK_SUPPORT
	if (memcmp(buf, PACK_MAGIC, 2) == 0)
		return FT_PACK;
	else
#endif
#ifndef NO_XZ_SUPPORT
	if (memcmp(buf, XZ_MAGIC, 4) == 0)	/* XXX: We only have 4 bytes */
		return FT_XZ;
	else
#endif
		return FT_UNKNOWN;
}

#ifndef SMALL
/* check the outfile is OK. */
static int
check_outfile(const char *outfile)
{
	struct stat sb;
	int ok = 1;

	if (lflag == 0 && stat(outfile, &sb) == 0) {
		if (fflag)
			unlink(outfile);
		else if (isatty(STDIN_FILENO)) {
			char ans[10] = { 'n', '\0' };	/* default */

			fprintf(stderr, "%s already exists -- do you wish to "
					"overwrite (y or n)? " , outfile);
			(void)fgets(ans, sizeof(ans) - 1, stdin);
			if (ans[0] != 'y' && ans[0] != 'Y') {
				fprintf(stderr, "\tnot overwriting\n");
				ok = 0;
			} else
				unlink(outfile);
		} else {
			maybe_warnx("%s already exists -- skipping", outfile);
			ok = 0;
		}
	}
	return ok;
}

static void
unlink_input(const char *file, const struct stat *sb)
{
	struct stat nsb;

	if (kflag)
		return;
	if (stat(file, &nsb) != 0)
		/* Must be gone already */
		return;
	if (nsb.st_dev != sb->st_dev || nsb.st_ino != sb->st_ino)
		/* Definitely a different file */
		return;
	unlink(file);
}

static void
sigint_handler(int signo __unused)
{

	if (remove_file != NULL)
		unlink(remove_file);
	_exit(2);
}
#endif

static const suffixes_t *
check_suffix(char *file, int xlate)
{
	const suffixes_t *s;
	int len = strlen(file);
	char *sp;

	for (s = suffixes; s != suffixes + NUM_SUFFIXES; s++) {
		/* if it doesn't fit in "a.suf", don't bother */
		if (s->ziplen >= len)
			continue;
		sp = file + len - s->ziplen;
		if (strcmp(s->zipped, sp) != 0)
			continue;
		if (xlate)
			strcpy(sp, s->normal);
		return s;
	}
	return NULL;
}

/*
 * compress the given file: create a corresponding .gz file and remove the
 * original.
 */
static off_t
file_compress(char *file, char *outfile, size_t outsize)
{
	int in;
	int out;
	off_t size, insize;
#ifndef SMALL
	struct stat isb, osb;
	const suffixes_t *suff;
#endif

	in = open(file, O_RDONLY);
	if (in == -1) {
		maybe_warn("can't open %s", file);
		return (-1);
	}

#ifndef SMALL
	if (fstat(in, &isb) != 0) {
		maybe_warn("couldn't stat: %s", file);
		close(in);
		return (-1);
	}
#endif

	if (cflag == 0) {
#ifndef SMALL
		if (isb.st_nlink > 1 && fflag == 0) {
			maybe_warnx("%s has %d other link%s -- skipping",
			    file, isb.st_nlink - 1,
			    (isb.st_nlink - 1) == 1 ? "" : "s");
			close(in);
			return (-1);
		}

		if (fflag == 0 && (suff = check_suffix(file, 0)) &&
		    suff->zipped[0] != 0) {
			maybe_warnx("%s already has %s suffix -- unchanged",
			    file, suff->zipped);
			close(in);
			return (-1);
		}
#endif

		/* Add (usually) .gz to filename */
		if ((size_t)snprintf(outfile, outsize, "%s%s",
		    file, suffixes[0].zipped) >= outsize)
			memcpy(outfile + outsize - suffixes[0].ziplen - 1,
			    suffixes[0].zipped, suffixes[0].ziplen + 1);

#ifndef SMALL
		if (check_outfile(outfile) == 0) {
			close(in);
			return (-1);
		}
#endif
	}

	if (cflag == 0) {
		out = open(outfile, O_WRONLY | O_CREAT | O_EXCL, 0600);
		if (out == -1) {
			maybe_warn("could not create output: %s", outfile);
			fclose(stdin);
			return (-1);
		}
#ifndef SMALL
		remove_file = outfile;
#endif
	} else
		out = STDOUT_FILENO;

	insize = gz_compress_wrapper(in, out, &size, basename(file), (uint32_t)isb.st_mtime);

	(void)close(in);

	/*
	 * If there was an error, insize will be -1.
	 * If we compressed to stdout, just return the size.
	 * Otherwise stat the file and check it is the correct size.
	 * We only blow away the file if we can stat the output and it
	 * has the expected size.
	 */
	if (cflag != 0)
		return (insize == -1 ? -1 : size);

#ifndef SMALL
	if (fstat(out, &osb) != 0) {
		maybe_warn("couldn't stat: %s", outfile);
		goto bad_outfile;
	}

	if (osb.st_size != size) {
		maybe_warnx("output file: %s wrong size (%ju != %ju), deleting",
		    outfile, (uintmax_t)osb.st_size, (uintmax_t)size);
		goto bad_outfile;
	}

	copymodes(out, &isb, outfile);
	remove_file = NULL;
#endif
	if (close(out) == -1)
		maybe_warn("couldn't close output");

	/* output is good, ok to delete input */
	unlink_input(file, &isb);
	return (size);

#ifndef SMALL
    bad_outfile:
	if (close(out) == -1)
		maybe_warn("couldn't close output");

	maybe_warnx("leaving original %s", file);
	unlink(outfile);
	return (size);
#endif
}

/* uncompress the given file and remove the original */
static off_t
file_uncompress(char *file, char *outfile, size_t outsize)
{
	struct stat isb, osb;
	off_t size;
	ssize_t rbytes;
	unsigned char header1[4];
	enum filetype method;
	int fd, ofd, zfd = -1;
#ifndef SMALL
	ssize_t rv;
	time_t timestamp = 0;
	unsigned char name[PATH_MAX + 1];
#endif

	/* gather the old name info */

	fd = open(file, O_RDONLY);
	if (fd < 0) {
		maybe_warn("can't open %s", file);
		goto lose;
	}

	strlcpy(outfile, file, outsize);
	if (check_suffix(outfile, 1) == NULL && !(cflag || lflag)) {
		maybe_warnx("%s: unknown suffix -- ignored", file);
		goto lose;
	}

	rbytes = read(fd, header1, sizeof header1);
	if (rbytes != sizeof header1) {
		/* we don't want to fail here. */
#ifndef SMALL
		if (fflag)
			goto lose;
#endif
		if (rbytes == -1)
			maybe_warn("can't read %s", file);
		else
			goto unexpected_EOF;
		goto lose;
	}

	method = file_gettype(header1);
#ifndef SMALL
	if (fflag == 0 && method == FT_UNKNOWN) {
		maybe_warnx("%s: not in gzip format", file);
		goto lose;
	}

#endif

#ifndef SMALL
	if (method == FT_GZIP && Nflag) {
		unsigned char ts[4];	/* timestamp */

		rv = pread(fd, ts, sizeof ts, GZIP_TIMESTAMP);
		if (rv >= 0 && rv < (ssize_t)(sizeof ts))
			goto unexpected_EOF;
		if (rv == -1) {
			if (!fflag)
				maybe_warn("can't read %s", file);
			goto lose;
		}
		timestamp = ts[3] << 24 | ts[2] << 16 | ts[1] << 8 | ts[0];

		if (header1[3] & ORIG_NAME) {
			rbytes = pread(fd, name, sizeof name, GZIP_ORIGNAME);
			if (rbytes < 0) {
				maybe_warn("can't read %s", file);
				goto lose;
			}
			if (name[0] != 0) {
				/* preserve original directory name */
				char *dp = strrchr(file, '/');
				if (dp == NULL)
					dp = file;
				else
					dp++;
				snprintf(outfile, outsize, "%.*s%.*s",
						(int) (dp - file), 
						file, (int) rbytes, name);
			}
		}
	}
#endif
	lseek(fd, 0, SEEK_SET);

	if (cflag == 0 || lflag) {
		if (fstat(fd, &isb) != 0)
			goto lose;
#ifndef SMALL
		if (isb.st_nlink > 1 && lflag == 0 && fflag == 0) {
			maybe_warnx("%s has %d other links -- skipping",
			    file, isb.st_nlink - 1);
			goto lose;
		}
		if (nflag == 0 && timestamp)
			isb.st_mtime = timestamp;
		if (check_outfile(outfile) == 0)
			goto lose;
#endif
	}

	if (cflag == 0 && lflag == 0) {
		zfd = open(outfile, O_WRONLY|O_CREAT|O_EXCL, 0600);
		if (zfd == STDOUT_FILENO) {
			/* We won't close STDOUT_FILENO later... */
			zfd = dup(zfd);
			close(STDOUT_FILENO);
		}
		if (zfd == -1) {
			maybe_warn("can't open %s", outfile);
			goto lose;
		}
#ifndef SMALL
		remove_file = outfile;
#endif
	} else
		zfd = STDOUT_FILENO;

	switch (method) {
#ifndef NO_BZIP2_SUPPORT
	case FT_BZIP2:
		/* XXX */
		if (lflag) {
			maybe_warnx("no -l with bzip2 files");
			goto lose;
		}

		size = unbzip2_wrapper(fd, zfd, NULL, 0, NULL);
		break;
#endif

#ifndef NO_COMPRESS_SUPPORT
	case FT_Z: {
		FILE *in, *out;

		/* XXX */
		if (lflag) {
			maybe_warnx("no -l with Lempel-Ziv files");
			goto lose;
		}

		if ((in = zdopen(fd)) == NULL) {
			maybe_warn("zdopen for read: %s", file);
			goto lose;
		}

		out = fdopen(dup(zfd), "w");
		if (out == NULL) {
			maybe_warn("fdopen for write: %s", outfile);
			fclose(in);
			goto lose;
		}

		size = zuncompress(in, out, NULL, 0, NULL);
		/* need to fclose() if ferror() is true... */
		if (ferror(in) | fclose(in)) {
			maybe_warn("failed infile fclose");
			unlink(outfile);
			(void)fclose(out);
		}
		if (fclose(out) != 0) {
			maybe_warn("failed outfile fclose");
			unlink(outfile);
			goto lose;
		}
		break;
	}
#endif

#ifndef NO_PACK_SUPPORT
	case FT_PACK:
		if (lflag) {
			maybe_warnx("no -l with packed files");
			goto lose;
		}

		size = unpack(fd, zfd, NULL, 0, NULL);
		break;
#endif

#ifndef NO_XZ_SUPPORT
	case FT_XZ:
		if (lflag) {
			maybe_warnx("no -l with xz files");
			goto lose;
		}

		size = unxz(fd, zfd, NULL, 0, NULL);
		break;
#endif

#ifndef SMALL
	case FT_UNKNOWN:
		if (lflag) {
			maybe_warnx("no -l for unknown filetypes");
			goto lose;
		}
		size = cat_fd(NULL, 0, NULL, fd);
		break;
#endif
	default:
		if (lflag) {
			print_list(fd, isb.st_size, outfile, isb.st_mtime);
			close(fd);
			return -1;	/* XXX */
		}

		size = gz_uncompress_wrapper(fd, zfd, NULL, 0, NULL, file);
		break;
	}

	if (close(fd) != 0)
		maybe_warn("couldn't close input");
	if (zfd != STDOUT_FILENO && close(zfd) != 0)
		maybe_warn("couldn't close output");

	if (size == -1) {
		if (cflag == 0)
			unlink(outfile);
		maybe_warnx("%s: uncompress failed", file);
		return -1;
	}

	/* if testing, or we uncompressed to stdout, this is all we need */
#ifndef SMALL
	if (tflag)
		return size;
#endif
	/* if we are uncompressing to stdin, don't remove the file. */
	if (cflag)
		return size;

	/*
	 * if we create a file...
	 */
	/*
	 * if we can't stat the file don't remove the file.
	 */

	ofd = open(outfile, O_RDWR, 0);
	if (ofd == -1) {
		maybe_warn("couldn't open (leaving original): %s",
			   outfile);
		return -1;
	}
	if (fstat(ofd, &osb) != 0) {
		maybe_warn("couldn't stat (leaving original): %s",
			   outfile);
		close(ofd);
		return -1;
	}
	if (osb.st_size != size) {
		maybe_warnx("stat gave different size: %ju != %ju (leaving original)",
		    (uintmax_t)size, (uintmax_t)osb.st_size);
		close(ofd);
		unlink(outfile);
		return -1;
	}
#ifndef SMALL
	copymodes(ofd, &isb, outfile);
	remove_file = NULL;
#endif
	close(ofd);
	unlink_input(file, &isb);
	return size;

    unexpected_EOF:
	maybe_warnx("%s: unexpected end of file", file);
    lose:
	if (fd != -1)
		close(fd);
	if (zfd != -1 && zfd != STDOUT_FILENO)
		close(fd);
	return -1;
}

#ifndef SMALL
static off_t
cat_fd(unsigned char * prepend, size_t count, off_t *gsizep, int fd)
{
	char buf[BUFLEN];
	off_t in_tot;
	ssize_t w;

	in_tot = count;
	w = write(STDOUT_FILENO, prepend, count);
	if (w == -1 || (size_t)w != count) {
		maybe_warn("write to stdout");
		return -1;
	}
	for (;;) {
		ssize_t rv;

		rv = read(fd, buf, sizeof buf);
		if (rv == 0)
			break;
		if (rv < 0) {
			maybe_warn("read from fd %d", fd);
			break;
		}

		if (write(STDOUT_FILENO, buf, rv) != rv) {
			maybe_warn("write to stdout");
			break;
		}
		in_tot += rv;
	}

	if (gsizep)
		*gsizep = in_tot;
	return (in_tot);
}
#endif

static void
handle_stdin(void)
{
	unsigned char header1[4];
	off_t usize, gsize;
	enum filetype method;
	ssize_t bytes_read;
#ifndef NO_COMPRESS_SUPPORT
	FILE *in;
#endif

#ifndef SMALL
	if (fflag == 0 && lflag == 0 && isatty(STDIN_FILENO)) {
		maybe_warnx("standard input is a terminal -- ignoring");
		return;
	}
#endif

	if (lflag) {
		struct stat isb;

		/* XXX could read the whole file, etc. */
		if (fstat(STDIN_FILENO, &isb) < 0) {
			maybe_warn("fstat");
			return;
		}
		print_list(STDIN_FILENO, isb.st_size, "stdout", isb.st_mtime);
		return;
	}

	bytes_read = read_retry(STDIN_FILENO, header1, sizeof header1);
	if (bytes_read == -1) {
		maybe_warn("can't read stdin");
		return;
	} else if (bytes_read != sizeof(header1)) {
		maybe_warnx("(stdin): unexpected end of file");
		return;
	}

	method = file_gettype(header1);
	switch (method) {
	default:
#ifndef SMALL
		if (fflag == 0) {
			maybe_warnx("unknown compression format");
			return;
		}
		usize = cat_fd(header1, sizeof header1, &gsize, STDIN_FILENO);
		break;
#endif
	case FT_GZIP:
		usize = gz_uncompress_wrapper(STDIN_FILENO, STDOUT_FILENO, 
			      (char *)header1, sizeof header1, &gsize, "(stdin)");
		break;
#ifndef NO_BZIP2_SUPPORT
	case FT_BZIP2:
		usize = unbzip2_wrapper(STDIN_FILENO, STDOUT_FILENO,
				(char *)header1, sizeof header1, &gsize);
		break;
#endif
#ifndef NO_COMPRESS_SUPPORT
	case FT_Z:
		if ((in = zdopen(STDIN_FILENO)) == NULL) {
			maybe_warnx("zopen of stdin");
			return;
		}

		usize = zuncompress(in, stdout, (char *)header1,
		    sizeof header1, &gsize);
		fclose(in);
		break;
#endif
#ifndef NO_PACK_SUPPORT
	case FT_PACK:
		usize = unpack(STDIN_FILENO, STDOUT_FILENO,
			       (char *)header1, sizeof header1, &gsize);
		break;
#endif
#ifndef NO_XZ_SUPPORT
	case FT_XZ:
		usize = unxz(STDIN_FILENO, STDOUT_FILENO,
			     (char *)header1, sizeof header1, &gsize);
		break;
#endif
	}

#ifndef SMALL
        if (vflag && !tflag && usize != -1 && gsize != -1)
		print_verbage(NULL, NULL, usize, gsize);
	if (vflag && tflag)
		print_test("(stdin)", usize != -1);
#endif 

}

static void
handle_stdout(void)
{
	off_t gsize, usize;
	struct stat sb;
	time_t systime;
	uint32_t mtime;
	int ret;

#ifndef SMALL
	if (fflag == 0 && isatty(STDOUT_FILENO)) {
		maybe_warnx("standard output is a terminal -- ignoring");
		return;
	}
#endif
	/* If stdin is a file use it's mtime, otherwise use current time */
	ret = fstat(STDIN_FILENO, &sb);

#ifndef SMALL
	if (ret < 0) {
		maybe_warn("Can't stat stdin");
		return;
	}
#endif

	if (S_ISREG(sb.st_mode))
		mtime = (uint32_t)sb.st_mtime;
	else {
		systime = time(NULL);
#ifndef SMALL
		if (systime == -1) {
			maybe_warn("time");
			return;
		} 
#endif
		mtime = (uint32_t)systime;
	}
	 		
	usize = gz_compress_wrapper(STDIN_FILENO, STDOUT_FILENO, &gsize, "", mtime);
#ifndef SMALL
        if (vflag && !tflag && usize != -1 && gsize != -1)
		print_verbage(NULL, NULL, usize, gsize);
#endif 
}

/* do what is asked for, for the path name */
static void
handle_pathname(char *path)
{
	char *opath = path, *s = NULL;
	ssize_t len;
	int slen;
	struct stat sb;

	/* check for stdout/stdin */
	if (path[0] == '-' && path[1] == '\0') {
		if (dflag)
			handle_stdin();
		else
			handle_stdout();
		return;
	}

retry:
	if (stat(path, &sb) != 0 || (fflag == 0 && cflag == 0 &&
	    lstat(path, &sb) != 0)) {
		/* lets try <path>.gz if we're decompressing */
		if (dflag && s == NULL && errno == ENOENT) {
			len = strlen(path);
			slen = suffixes[0].ziplen;
			s = malloc(len + slen + 1);
			if (s == NULL)
				maybe_err("malloc");
			memcpy(s, path, len);
			memcpy(s + len, suffixes[0].zipped, slen + 1);
			path = s;
			goto retry;
		}
		maybe_warn("can't stat: %s", opath);
		goto out;
	}

	if (S_ISDIR(sb.st_mode)) {
#ifndef SMALL
		if (rflag)
			handle_dir(path);
		else
#endif
			maybe_warnx("%s is a directory", path);
		goto out;
	}

	if (S_ISREG(sb.st_mode))
		handle_file(path, &sb);
	else
		maybe_warnx("%s is not a regular file", path);

out:
	if (s)
		free(s);
}

/* compress/decompress a file */
static void
handle_file(char *file, struct stat *sbp)
{
	off_t usize, gsize;
	char	outfile[PATH_MAX];

	infile = file;
	if (dflag) {
		usize = file_uncompress(file, outfile, sizeof(outfile));
#ifndef SMALL
		if (vflag && tflag)
			print_test(file, usize != -1);
#endif
		if (usize == -1)
			return;
		gsize = sbp->st_size;
	} else {
		gsize = file_compress(file, outfile, sizeof(outfile));
		if (gsize == -1)
			return;
		usize = sbp->st_size;
	}


#ifndef SMALL
	if (vflag && !tflag)
		print_verbage(file, (cflag) ? NULL : outfile, usize, gsize);
#endif
}

#ifndef SMALL
/* this is used with -r to recursively descend directories */
static void
handle_dir(char *dir)
{
	char *path_argv[2];
	FTS *fts;
	FTSENT *entry;

	path_argv[0] = dir;
	path_argv[1] = 0;
	fts = fts_open(path_argv, FTS_PHYSICAL | FTS_NOCHDIR, NULL);
	if (fts == NULL) {
		warn("couldn't fts_open %s", dir);
		return;
	}

	while ((entry = fts_read(fts))) {
		switch(entry->fts_info) {
		case FTS_D:
		case FTS_DP:
			continue;

		case FTS_DNR:
		case FTS_ERR:
		case FTS_NS:
			maybe_warn("%s", entry->fts_path);
			continue;
		case FTS_F:
			handle_file(entry->fts_path, entry->fts_statp);
		}
	}
	(void)fts_close(fts);
}
#endif

/* print a ratio - size reduction as a fraction of uncompressed size */
static void
print_ratio(off_t in, off_t out, FILE *where)
{
	int percent10;	/* 10 * percent */
	off_t diff;
	char buff[8];
	int len;

	diff = in - out/2;
	if (diff <= 0)
		/*
		 * Output is more than double size of input! print -99.9%
		 * Quite possibly we've failed to get the original size.
		 */
		percent10 = -999;
	else {
		/*
		 * We only need 12 bits of result from the final division,
		 * so reduce the values until a 32bit division will suffice.
		 */
		while (in > 0x100000) {
			diff >>= 1;
			in >>= 1;
		}
		if (in != 0)
			percent10 = ((u_int)diff * 2000) / (u_int)in - 1000;
		else
			percent10 = 0;
	}

	len = snprintf(buff, sizeof buff, "%2.2d.", percent10);
	/* Move the '.' to before the last digit */
	buff[len - 1] = buff[len - 2];
	buff[len - 2] = '.';
	fprintf(where, "%5s%%", buff);
}

#ifndef SMALL
/* print compression statistics, and the new name (if there is one!) */
static void
print_verbage(const char *file, const char *nfile, off_t usize, off_t gsize)
{
	if (file)
		fprintf(stderr, "%s:%s  ", file,
		    strlen(file) < 7 ? "\t\t" : "\t");
	print_ratio(usize, gsize, stderr);
	if (nfile)
		fprintf(stderr, " -- replaced with %s", nfile);
	fprintf(stderr, "\n");
	fflush(stderr);
}

/* print test results */
static void
print_test(const char *file, int ok)
{

	if (exit_value == 0 && ok == 0)
		exit_value = 1;
	fprintf(stderr, "%s:%s  %s\n", file,
	    strlen(file) < 7 ? "\t\t" : "\t", ok ? "OK" : "NOT OK");
	fflush(stderr);
}
#endif

/* print a file's info ala --list */
/* eg:
  compressed uncompressed  ratio uncompressed_name
      354841      1679360  78.8% /usr/pkgsrc/distfiles/libglade-2.0.1.tar
*/
static void
print_list(int fd, off_t out, const char *outfile, time_t ts)
{
	static int first = 1;
#ifndef SMALL
	static off_t in_tot, out_tot;
	uint32_t crc = 0;
#endif
	off_t in = 0, rv;

	if (first) {
#ifndef SMALL
		if (vflag)
			printf("method  crc     date  time  ");
#endif
		if (qflag == 0)
			printf("  compressed uncompressed  "
			       "ratio uncompressed_name\n");
	}
	first = 0;

	/* print totals? */
#ifndef SMALL
	if (fd == -1) {
		in = in_tot;
		out = out_tot;
	} else
#endif
	{
		/* read the last 4 bytes - this is the uncompressed size */
		rv = lseek(fd, (off_t)(-8), SEEK_END);
		if (rv != -1) {
			unsigned char buf[8];
			uint32_t usize;

			rv = read(fd, (char *)buf, sizeof(buf));
			if (rv == -1)
				maybe_warn("read of uncompressed size");
			else if (rv != sizeof(buf))
				maybe_warnx("read of uncompressed size");

			else {
				usize = buf[4] | buf[5] << 8 |
					buf[6] << 16 | buf[7] << 24;
				in = (off_t)usize;
#ifndef SMALL
				crc = buf[0] | buf[1] << 8 |
				      buf[2] << 16 | buf[3] << 24;
#endif
			}
		}
	}

#ifndef SMALL
	if (vflag && fd == -1)
		printf("                            ");
	else if (vflag) {
		char *date = ctime(&ts);

		/* skip the day, 1/100th second, and year */
		date += 4;
		date[12] = 0;
		printf("%5s %08x %11s ", "defla"/*XXX*/, crc, date);
	}
	in_tot += in;
	out_tot += out;
#else
	(void)&ts;	/* XXX */
#endif
	printf("%12llu %12llu ", (unsigned long long)out, (unsigned long long)in);
	print_ratio(in, out, stdout);
	printf(" %s\n", outfile);
}

/* display the usage of NetBSD gzip */
static void
usage(void)
{

	fprintf(stderr, "%s\n", gzip_version);
	fprintf(stderr,
#ifdef SMALL
    "usage: %s [-" OPT_LIST "] [<file> [<file> ...]]\n",
#else
    "usage: %s [-123456789acdfhklLNnqrtVv] [-S .suffix] [<file> [<file> ...]]\n"
    " -1 --fast            fastest (worst) compression\n"
    " -2 .. -8             set compression level\n"
    " -9 --best            best (slowest) compression\n"
    " -c --stdout          write to stdout, keep original files\n"
    "    --to-stdout\n"
    " -d --decompress      uncompress files\n"
    "    --uncompress\n"
    " -f --force           force overwriting & compress links\n"
    " -h --help            display this help\n"
    " -k --keep            don't delete input files during operation\n"
    " -l --list            list compressed file contents\n"
    " -N --name            save or restore original file name and time stamp\n"
    " -n --no-name         don't save original file name or time stamp\n"
    " -q --quiet           output no warnings\n"
    " -r --recursive       recursively compress files in directories\n"
    " -S .suf              use suffix .suf instead of .gz\n"
    "    --suffix .suf\n"
    " -t --test            test compressed file\n"
    " -V --version         display program version\n"
    " -v --verbose         print extra statistics\n",
#endif
	    getprogname());
	exit(0);
}

#ifndef SMALL
/* display the license information of FreeBSD gzip */
static void
display_license(void)
{

	fprintf(stderr, "%s (based on NetBSD gzip 20111009)\n", gzip_version);
	fprintf(stderr, "%s\n", gzip_copyright);
	exit(0);
}
#endif

/* display the version of NetBSD gzip */
static void
display_version(void)
{

	fprintf(stderr, "%s\n", gzip_version);
	exit(0);
}

#ifndef NO_BZIP2_SUPPORT
#include "unbzip2.c"
#endif
#ifndef NO_COMPRESS_SUPPORT
#include "zuncompress.c"
#endif
#ifndef NO_PACK_SUPPORT
#include "unpack.c"
#endif
#ifndef NO_XZ_SUPPORT
#include "unxz.c"
#endif

static ssize_t
read_retry(int fd, void *buf, size_t sz)
{
	char *cp = buf;
	size_t left = MIN(sz, (size_t) SSIZE_MAX);

	while (left > 0) {
		ssize_t ret;

		ret = read(fd, cp, left);
		if (ret == -1) {
			return ret;
		} else if (ret == 0) {
			break; /* EOF */
		}
		cp += ret;
		left -= ret;
	}

	return sz - left;
}
