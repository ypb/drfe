
/*
  an ad hoc garbage collection (! collector)
*/

#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "store.h"

#define SMALL_BUF 64

struct db* store_open_fiber(char *dir, struct blobs names)
{
  struct db fiber ;
  struct registry *temp, *current ;
  TDB_CONTEXT *ctx ;
  char buf[SMALL_BUF] ;
  int n, d = strlen(dir) ;
  int i, j = names.len ;

  if (d > SMALL_BUF - 1)
	return NULL ;
  strncpy(buf, dir, d); buf[d] = '/' ; buf[++d] = '\0' ;
  fiber.path.dat = buf ; fiber.path.len = d ;
#ifdef _SDEBUG
  printf("# store_open_fiber: opening %i regs in %s\n", j, buf) ;
#endif

  fiber.first = current = temp = NULL ;

  for (i = 0; i < j; i++) {
#ifdef _SDEBUG
	printf("# store_open_fiber: opening reg(%i)=%s\n", i, names.dat[i]) ;
#endif
	n = strlen(names.dat[i]) ;
	if (SMALL_BUF < d + n + 3) /* d+(n-1)+4 AS d "includes" \0 */
	  return NULL ;
	strncpy(buf + d, names.dat[i], n) ;
	strncpy(buf + d + n, ".tdb", 5) ;

	temp = (struct registry*)malloc(sizeof(struct registry)) ;
	if (temp == NULL) {
	  perror("# store_open_fiber: failed allocating struct reg") ;
	  return NULL ; /* or should we already try to clean up here */
	}
	temp->name.dat = names.dat[i] ; temp->name.len = n ;

	ctx = tdb_open(buf, 0, 0, O_CREAT| O_RDWR, S_IRUSR | S_IWUSR) ;
	if (ctx == NULL) {
	  perror("# say_open_fiber: failed to open TDB") ;
	  return NULL ;
	}
	temp->store = ctx ;

	if (fiber.first == NULL) {
	  fiber.first = temp ;
	  current = temp ;
	} else {
	  current->next = temp ;
	  current = temp ;
	}
  }

  return &fiber ;
}

void store_lsns(struct db *fiber)
{
  uchar_t *path = fiber->path.dat ;
  struct registry *current = fiber->first ;
  printf("# store_lsns: %s\n", path) ;
  while ( current != NULL ) {
	printf("# store_lsns: %s%s\n", path, current->name.dat) ;
	current = current->next ;
  }
}
