#include <stdio.h>

#include "say.h"

int main(int argc, char *argv[])
{
  int ret ;
#ifdef _SDEBUG
  int i ;
  printf("# main: argc=%i\n", argc);
  for (i = 0; i < argc; i++) {
	printf("# main: argv[%i]=%s\n", i, argv[i]);
  }
#endif
  ret = say(argc - 1, argv + 1);
  return ret;
}

