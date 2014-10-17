#include <sys/types.h>
#include <sys/stat.h>

#include <machine/cheri.h>
#include <machine/cheric.h>

#include <cheri/cheri_enter.h>
#include <cheri/cheri_fd.h>
#include <cheri/cheri_invoke.h>
extern __capability void	*cheri_system_type;
#include <cheri/cheri_system.h>

int invoke(register_t op, __capability void * co_codecap, __capability void * co_datacap);

#define CHERI_STR_PTR(str)  cheri_ptr((void*)(str), 1+lenstr((str)))

size_t lenstr (const char * str);
size_t lenstr (const char * str)
{
  size_t len = 0;
  while (*str)
    str++, len++;
  return len;
}

int
invoke(register_t op, __capability void * co_codecap, __capability void * co_datacap)
{
  /* reconstruct the cheri_object */
  struct cheri_object fd_object;
  fd_object.co_codecap = co_codecap;
  fd_object.co_datacap = co_datacap;

  static const char * msg = "in invoke()!\n";

  /* write to stderr */
  cheri_fd_write_c(fd_object, CHERI_STR_PTR(msg));

  /* no stdlib */ /*fprintf(stderr, "in invoke()!\n");*/
  /*cheri_system_puts(CHERI_STR_PTR("in invoke()!\n"));*/
  (void) op;
  (void) fd_object;
  return 0x5678;
}

