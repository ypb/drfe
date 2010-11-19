
/*
  an ad hoc garbage collection (! collector)
*/

#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h> /* offsetof */
#include <string.h>

#include "store.h"

#define SMALL_BUF 64

struct db* store_open_fiber(char *dir, struct blobs names)
{
  struct db *fiber ;
  struct registry *temp, *current ;
  TDB_CONTEXT *ctx ;
  char buf[SMALL_BUF] ;
  int n, d = strlen(dir) ;
  int i, j = names.len ;

  if (d > SMALL_BUF - 1)
	return NULL ;
  strncpy(buf, dir, d); buf[d] = '/' ; buf[++d] = '\0' ;

  fiber = (struct db*)malloc(sizeof(struct db)) ;
  if (fiber == NULL)
	return NULL ;
  fiber->first = current = temp = NULL ;
  /* fiber->path.dat = buf ; fiber->path.len = d ; */
  fiber->path = make_blob(buf) ;
#ifdef _SDEBUG
  printf("# store_open_fiber: opening %i regs in %s\n", j, buf) ;
#endif
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
	/* ?make_blob? tho for now names.dat are static */
	temp->name.dat = names.dat[i] ; temp->name.len = n ;

	ctx = tdb_open(buf, 0, 0, O_CREAT| O_RDWR, S_IRUSR | S_IWUSR) ;
	if (ctx == NULL) {
	  perror("# say_open_fiber: failed to open TDB") ;
	  return NULL ;
	}
	temp->store = ctx ;

	if (fiber->first == NULL) {
	  fiber->first = temp ;
	  current = temp ;
	} else {
	  current->next = temp ;
	  current = temp ;
	}
  }

  return fiber ;
}

void store_lsns(struct db *fiber)
{
  uchar_t *path = fiber->path.dat ;
  struct registry *current = fiber->first ;
  printf("# store_lsns: %s\n", path) ;
  while ( current != NULL ) {
	printf("# store_lsns: %s%s (%s)\n", path, current->name.dat, ((char*)current->store + offsetof(TDB_CONTEXT, name))) ;
	current = current->next ;
  }
}

/* just strdup? still need to "destroy_blob" */
struct blob make_blob(char *str)
{
  struct blob test ;
  test.len = strlen(str) ;
  test.dat = (uchar_t*)malloc(test.len) ;
  if (test.dat == NULL) {
	perror("# make_blob: malloc failed") ;
  } else {
	strncpy(test.dat, str, test.len) ;
#ifdef _SDEBUG
	printf("# make_blob: len(%i) str(%s)@(%p) dat(%s)@(%p)\n", test.len, str, str, test.dat, test.dat) ;
#endif
  }
  return test ;
}

