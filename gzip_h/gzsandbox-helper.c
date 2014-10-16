#include <sys/types.h>
#include <sys/stat.h>

#include <machine/cheri.h>
#include <machine/cheric.h>

#include <cheri/cheri_enter.h>
#include <cheri/cheri_fd.h>
#include <cheri/cheri_invoke.h>
extern __capability void	*cheri_system_type;
#include <cheri/cheri_system.h>

#include <stdio.h>
#include <string.h>

int	invoke(register_t op, struct cheri_object system_object,
      struct cheri_object fd_object);


int
invoke(register_t op, struct cheri_object system_object,
    struct cheri_object fd_object)
{
  fprintf(stderr, "in invoke()!\n");
  (void) op;
  (void) system_object;
  (void) fd_object;
  return 0x5678;
}

