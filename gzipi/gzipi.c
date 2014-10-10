#include <stdio.h>

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

void gzip_show (FILE * fp);
void gzip_read_member (FILE * fp, gzip_member * gzm);
void gzip_print_member (gzip_member gzm);
uint32_t gzm_get (uint8_t * cptr, int sz);
#define GZM_GET(gzmbr) gzm_get(&gzmbr, sizeof gzmbr)
const char * gzm_flgs (uint32_t gzm_flg)
const char * gzm_oss (uint32_t gzm_os)
const char * gzm_mtimes (uint32_t gzm_mtime)

int main (int argc, char ** argv)
{
  FILE * fp;

  if (argc < 2)
  {
    print_usage();
    return 0;
  }

  fp = fopen(argv[1], "rb");
  if (!fp)
  {
    perror("gzipi: fopen");
    return 1;
  }

  gzipi_show(fp);

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
  n = fread(&gzm, 1, sizeof *gzm, fp);
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
    gzm_mtime, gzm_mtimes(gzm_flg),
    GZM_GET(gzm.gzm_xfl),
    gzm_os, gzm_oss(gzm_os));
}

void gzip_show (FILE * fp)
{
  gzip_member gzm;
  gzip_read_member(fp, &gzm);
  gzip_print_member(gzm);
}

const char * gzm_flgs (uint32_t gzm_flg)
{
  static char s[512];
  int len;
  strcpy(s, "");
  if (gzm_flg &  1) strcat(s, "FTEXT, ");
  if (gzm_flg &  2) strcat(s, "FHCRC, ");
  if (gzm_flg &  4) strcat(s, "FEXTRA, ");
  if (gzm_flg &  8) strcat(s, "FNAME, ");
  if (gzm_flg & 16) strcat(s, "FCOMMENT, ");
  len = strlen(s);
  if (len) s[len-2] = '\0';
  return s;
}

const char * gzm_oss (uint32_t gzm_os)
{
  static char s[512];
  switch (gzm_os)
  {
    case  0: return strcpy(s, "FAT");
    case  1: return strcpy(s, "Amiga");
    case  2: return strcpy(s, "VMS");
    case  3: return strcpy(s, "Unix");
    case  4: return strcpy(s, "VM/CMS");
    case  5: return strcpy(s, "Atari TOS");
    case  6: return strcpy(s, "HPFS");
    case  7: return strcpy(s, "Macintosh");
    case  8: return strcpy(s, "Z-System");
    case  9: return strcpy(s, "CP/M");
    case 10: return strcpy(s, "TOPS-20");
    case 11: return strcpy(s, "NTFS");
    case 12: return strcpy(s, "QDOS");
    case 13: return strcpy(s, "Acorn RISCOS");
    default: return strcpy(s, "Unknown");
  }
}

const char * gzm_mtimes (uint32_t gzm_mtime)
{
  static char s[512];
  return strcpy(s, ctime(gzm_mtime));
}

