
/*
  an ad hoc garbage collection (! collector)
*/

#include <stdio.h>
#include <string.h>

#include "store.h"

struct db* store_open_fiber(char* dir, struct blobs names)
{
  struct db fiber ;
  char buf[64] ;
  int d = strlen(dir) ;
  int i, j = names.len ;

  strncpy(buf, dir, d); buf[d] = '/' ; buf[++d] = '\0' ;
  fiber.path.dat = buf ; fiber.path.len = d ;
#ifdef _SDEBUG
  printf("# store_open_fiber: opening %i regs in %s\n", j, buf) ;
#endif

  for (i = 0; i < j; i++) {
	printf("# store_open_fiber: reg(%i)=%s\n", i, names.dat[i]) ;
  }

  return &fiber ;
}

