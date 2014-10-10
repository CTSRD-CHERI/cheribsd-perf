#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct
{
  uint8_t gzm_id1[1];
  uint8_t gzm_id2[1];
  uint8_t gzm_cm[1];
  uint8_t gzm_flg[1];
  uint8_t gzm_mtime[4];
  uint8_t gzm_xfl[1];
  uint8_t gzm_os[1];
} gzip_member;

typedef struct
{
  uint8_t gze_si1[1];
  uint8_t gze_si2[1];
  uint8_t gze_len[2];
} gzip_extra;

#define GZM_FLG_FTEXT     1
#define GZM_FLG_FHCRC     2
#define GZM_FLG_FEXTRA    4
#define GZM_FLG_FNAME     8
#define GZM_FLG_FCOMMENT 16

#define SZPREFIX(n) \
( \
  (n) >= 1000000000 ? 'G' : \
  (n) >=    1000000 ? 'M' : \
  (n) >=       1000 ? 'k' : \
  ' ' \
)
#define SZDIV(n) \
( \
  (n) >= 1000000000 ? (n)/1000000000 : \
  (n) >=    1000000 ? (n)/   1000000 : \
  (n) >=       1000 ? (n)/      1000 : \
  (n) \
)

void gzip_show (FILE * fp);
void gzip_read_member (FILE * fp, gzip_member * gzm);
void gzip_print_member (gzip_member gzm);
void gzip_read_extra (FILE * fp, gzip_extra * gzm);
void gzip_print_extra (gzip_extra gzm);
uint32_t gzm_get (uint8_t * cptr, int sz);
#define GZM_GET(gzmbr) gzm_get(gzmbr, sizeof gzmbr)
const char * gzm_flgs (uint32_t gzm_flg);
const char * gzm_oss (uint32_t gzm_os);
const char * gzm_mtimes (uint32_t gzm_mtime);

int main (int argc, char ** argv)
{
  FILE * fp;

  if (argc < 2)
  {
    printf("usage: %s <filename>\n", argv[0]);
    return 0;
  }

  fp = fopen(argv[1], "rb");
  if (!fp)
  {
    perror("gzipi: fopen");
    return 1;
  }

  gzip_show(fp);

  fclose(fp);

  return 0;
}

uint32_t gzm_get (uint8_t * cptr, int sz)
{
  uint32_t n;
  n = 0;
  if (sz >= 1) n  = cptr[0];
  if (sz >= 2) n |= cptr[1] <<  8;
  if (sz >= 3) n |= cptr[2] << 16;
  if (sz >= 4) n |= cptr[3] << 24;
  return n;
}

void gzip_read_member (FILE * fp, gzip_member * gzm)
{
  int n;
  n = fread(gzm, 1, sizeof *gzm, fp);
  if (n != sizeof *gzm)
  {
    fputs("gzipi: read error\n", stderr);
    exit(1);
  }
}

void gzip_print_member (gzip_member gzm)
{
  uint32_t gzm_flg, gzm_mtime, gzm_os;
  gzm_flg   = GZM_GET(gzm.gzm_flg);
  gzm_mtime = GZM_GET(gzm.gzm_mtime);
  gzm_os    = GZM_GET(gzm.gzm_os);
  printf(
    "ID1:       0x%x\n"
    "ID2:       0x%x\n"
    "CM:        0x%x\n"
    "FLG:       0x%x (%s)\n"
    "MTIME:     %u (%s)\n"
    "XFL:       %u\n"
    "OS:        %u (%s)\n",
    GZM_GET(gzm.gzm_id1),
    GZM_GET(gzm.gzm_id2),
    GZM_GET(gzm.gzm_cm),
    gzm_flg, gzm_flgs(gzm_flg),
    gzm_mtime, gzm_mtimes(gzm_mtime),
    GZM_GET(gzm.gzm_xfl),
    gzm_os, gzm_oss(gzm_os));
}


void gzip_read_extra (FILE * fp, gzip_extra * gze)
{
  int n;
  n = fread(gze, 1, sizeof *gze, fp);
  if (n != sizeof *gze)
  {
    fputs("gzipi: read error\n", stderr);
    exit(1);
  }
}

void gzip_print_extra (gzip_extra gze)
{
  uint32_t gze_len;
  gze_len = GZM_GET(gze.gze_len);
  printf(
    "SI1:       0x%x\n"
    "SI2:       0x%x\n"
    "LEN:       0x%x (%u%c)\n",
    GZM_GET(gze.gze_si1),
    GZM_GET(gze.gze_si2),
    gze_len, SZDIV(gze_len), SZPREFIX(gze_len));
}

void gzip_show (FILE * fp)
{
  gzip_member gzm;
  gzip_extra gze;
  uint32_t gzm_flg;
  int c;

  gzip_read_member(fp, &gzm);
  gzip_print_member(gzm);

  gzm_flg = GZM_GET(gzm.gzm_flg);

  if (gzm_flg & GZM_FLG_FEXTRA)
  {
    gzip_read_extra(fp, &gze);
    gzip_print_extra(gze);
    if (fseek(fp, GZM_GET(gze.gze_len), SEEK_CUR))
    {
      perror("gzipi: fseek");
      exit(1);
    }
  }

  if (gzm_flg & GZM_FLG_FNAME)
  {
    fputs("FILENAME:  ", stdout);
    while (1)
    {
      c = fgetc(fp);
      if (!c) break;
      if (c == EOF)
      {
        if (feof(fp))
          fputs("gzipi: fgetc: unexpected EOF\n", stderr);
        else
          perror("gzipi: fgetc");
        exit(1);
      }
      putchar(c);
    }
    putchar('\n');
  }
}

const char * gzm_flgs (uint32_t gzm_flg)
{
  static char s[512];
  int len;
  strcpy(s, "");
  if (gzm_flg & GZM_FLG_FTEXT)    strcat(s, "FTEXT, ");
  if (gzm_flg & GZM_FLG_FHCRC)    strcat(s, "FHCRC, ");
  if (gzm_flg & GZM_FLG_FEXTRA)   strcat(s, "FEXTRA, ");
  if (gzm_flg & GZM_FLG_FNAME)    strcat(s, "FNAME, ");
  if (gzm_flg & GZM_FLG_FCOMMENT) strcat(s, "FCOMMENT, ");
  len = strlen(s);
  if (len) s[len-2] = '\0';
  return s;
}

const char * gzm_oss (uint32_t gzm_os)
{
  static char s[512];
  switch (gzm_os)
  {
    case   0: return strcpy(s, "FAT");
    case   1: return strcpy(s, "Amiga");
    case   2: return strcpy(s, "VMS");
    case   3: return strcpy(s, "Unix");
    case   4: return strcpy(s, "VM/CMS");
    case   5: return strcpy(s, "Atari TOS");
    case   6: return strcpy(s, "HPFS");
    case   7: return strcpy(s, "Macintosh");
    case   8: return strcpy(s, "Z-System");
    case   9: return strcpy(s, "CP/M");
    case  10: return strcpy(s, "TOPS-20");
    case  11: return strcpy(s, "NTFS");
    case  12: return strcpy(s, "QDOS");
    case  13: return strcpy(s, "Acorn RISCOS");
    default : return strcpy(s, "Unknown");
  }
}

const char * gzm_mtimes (uint32_t gzm_mtime)
{
  static char s[512];
  time_t gzm_mtimet;
  gzm_mtimet = gzm_mtime;
  strcpy(s, ctime(&gzm_mtimet));
  s[24] ^= '\n';
  return s;
}

