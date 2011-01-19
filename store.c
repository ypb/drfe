
/*
  an ad hoc garbage collection (! collector)
*/

#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
/* #include <stddef.h> /* offsetof */
#include <string.h>

#include "store.h"

#define SMALL_BUF 64
#define DEFAULT_REG "info"

struct db* store_open_fiber(char *dir, struct blobs names)
{
  struct db *fiber ;
  struct registry *temp, *current ;
  TDB_CONTEXT *ctx ;
  char buf[SMALL_BUF] ;
  int n, d = strlen(dir) ;
  int i, j = names.len ;

  if (d > SMALL_BUF - 2)
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
	if (SMALL_BUF < d + n + 4) /* d+n+4 AS d "includes" \0 */
	  return NULL ;
	strncpy(buf + d, names.dat[i], n) ;
	strncpy(buf + d + n, ".tdb", 5) ;

	temp = (struct registry*)malloc(sizeof(struct registry)) ;
	if (temp == NULL) {
	  perror("# store_open_fiber: failed allocating struct reg") ;
	  return NULL ; /* or should we already try to clean up here */
	}
	/* ?make_blob? tho for now names.dat are static */
	/* temp->name.dat = names.dat[i] ; temp->name.len = n ; */
	temp->name = make_blob(names.dat[i]);

	ctx = tdb_open(buf, 0, 0, O_CREAT| O_RDWR, S_IRUSR | S_IWUSR) ;
	if (ctx == NULL) {
	  perror("# say_open_fiber: failed to open TDB") ;
	  return NULL ;
	}
	temp->store = ctx ;
	/* ja pierdole... */
	temp->next = NULL ;

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

struct registry* free_register(struct registry *reg)
{
  struct registry* ret = NULL ;
  if (reg == NULL)
	return ret ;
  ret = reg->next ;

  /* TODO: please be conseqential... */
  free_blob(reg->name) ;
  /* can we determine if they are static and can't be deallocated? */
  if (reg->store != NULL)
	tdb_close(reg->store) ;
  /* rack ?!? */
  free(reg) ;

  return ret ;
}

int store_close_fiber(struct db* fiber)
{
  int ret = -1 ;
  struct registry *current ;

  if (fiber == NULL)
	return ret ;
#ifdef _SDEBUG
  printf("# store_close_fiber: %s ::", fiber->path.dat) ;
#endif
  current = fiber->first ;
  while ( current != NULL ) {
#ifdef _SDEBUG
	printf(" %s", current->name.dat) ;
#endif
	current = free_register(current) ;
  }
  free_blob(fiber->path) ;
  free(fiber);
#ifdef _SDEBUG
	printf("\n") ;
#endif
  return 0 ;
}

TDB_CONTEXT* get_reg(struct db* fiber, const char* name)
{
  TDB_CONTEXT *ret = NULL ;
  struct registry *current ;

  if (name == NULL)
	name = DEFAULT_REG;
#ifdef _SDEBUG
  printf("# get_reg: named(%s) from db@(%p)\n", name, (void*)fiber);
#endif
  if (fiber == NULL)
	return ret ;
  current = fiber->first ;
  while ( current != NULL ) {
	if (current->name.dat && (strcmp(current->name.dat, name) == 0))
	  return current->store ;
	current = current->next ;
  }
#ifdef _SDEBUG
  printf("# get_reg: not found...\n");
#endif
  return ret ;
}

int store_operate(struct db* fiber, const char* reg, struct kons data, int opt)
{
  int ret = -1 ;
  TDB_DATA key, val ;
  TDB_CONTEXT *ctx = get_reg(fiber, reg) ;
  if (ctx == NULL)
	return ret ;

  key.dptr = data.key.dat ; key.dsize = data.key.len ;
  val.dptr = data.val.dat ; val.dsize = data.val.len ;
#ifdef _SDEBUG
  printf("# store_opt(%s): %s%s with k(%s)v(%s)\n", opt == TDB_MODIFY ? "inside" : "extend", fiber->path.dat, reg, data.key.dat, data.val.dat) ;
#endif
  return tdb_store(ctx, key, val, opt) ;
}

int store_extend(struct db* fiber, const char* reg, struct kons data)
{
#ifdef _SDEBUG
  printf("# store_extend: k(%s)/v(%s)\n", data.key.dat, data.val.dat);
#endif
  return store_operate(fiber, reg, data, TDB_INSERT) ;
}

int store_exists(struct db* fiber, const char* reg, struct blob key)
{
  int ret = -1 ;
  TDB_DATA data ;
  TDB_CONTEXT *ctx = get_reg(fiber, reg) ;
  if (ctx == NULL)
	return ret ;

  /* data = key ; /* error: conversion to non-scalar type requested */
  data.dptr = key.dat ; data.dsize = key.len ;
  return tdb_exists(ctx, data) ;
}

int store_inside(struct db* fiber, const char* reg, struct kons data)
{
  return store_operate(fiber, reg, data, TDB_MODIFY) ;
}

int store_remove(struct db* fiber, const char* reg, struct blob key)
{
  int ret = -1 ;
  TDB_DATA data ;
  TDB_CONTEXT *ctx = get_reg(fiber, reg) ;
  if (ctx == NULL)
	return ret ;

  data.dptr = key.dat ; data.dsize = key.len ;
#ifdef _SDEBUG
  printf("# store_remove: key=(%s) from %s%s\n", key.dat, fiber->path.dat, reg) ;
#endif
  return tdb_delete(ctx, data) ;
}

struct blob store_restore(struct db* fiber, const char* reg, struct blob key)
{
  struct blob ret = { NULL, 0 } ;
  TDB_DATA data ;
  TDB_CONTEXT *ctx = get_reg(fiber, reg) ;
  if (ctx == NULL)
	return ret ;

  data.dptr = key.dat ; data.dsize = key.len ;
  data = tdb_fetch(ctx, data) ;
  ret.dat = data.dptr ; ret.len = data.dsize ;
  return ret ;
}

void store_lsns(struct db *fiber)
{
  uchar_t *path = fiber->path.dat ;
  struct registry *current = fiber->first ;
  printf("# store_lsns: %s\n", path) ;
  while ( current != NULL ) {
	printf("# store_lsns: %s%s (%s)\n", path, current->name.dat, tdb_name(current->store)) ;
	current = current->next ;
  }
}

int store_test(struct db *fiber)
{
  int stat, ret = -1 ;
  char *reg = DEFAULT_REG;
  struct kons tdat = { {"0", 2}, {"ala ma kota", 12} } ;
  struct blob tres = {NULL, 0} ;
  struct kons cunt = { {"eventcount", 11}, {"0", 2}} ;
  if (fiber == NULL)
	return ret ;

  printf("# store_test: db->path.dat=%s db->path.len=%i\n", fiber->path.dat, fiber->path.len) ;
  store_lsns(fiber) ;
  /* printf("EXTENDING\n"); */
  stat = store_extend(fiber, reg, cunt);
  printf("# store_test: store_extend->%i\n", stat) ;

  stat = store_extend(fiber, reg, tdat) ;
  /* oh man... like for REALZ
	 stat = store_inside(fiber, reg, tdat) ; */
  if (stat == -1) {
	printf("# store_test: store_inside tdat failed\n") ;
  } else {
	tres = store_restore(fiber, reg, tdat.key) ;

	if (tres.dat == NULL) {
	  printf("# store_test: store_restore tdat.key failed\n") ;
	} else {
	  printf("# store_test: comparing stored vs. fetched data: %s\n", \
			 strcmp((char*)tdat.val.dat, (char*)tres.dat) == 0 ? "OK" : "fail") ;
	  ret = 0 ;
	  free(tres.dat) ;
	}
	store_remove(fiber, reg, tdat.key) ;
  }
  return ret ;
}

/* just strdup? still need to "destroy_blob" */
struct blob make_blob(char *str)
{
  struct blob test ;
  test.len = strlen(str) ;
  test.dat = (uchar_t*)malloc(test.len + 1) ;
  if (test.dat == NULL) {
	perror("# make_blob: malloc failed") ;
  } else {
	strncpy(test.dat, str, test.len + 1) ;
#ifdef _SDEBUG
	printf("# make_blob: len(%i) str(%s)@(%p) dat(%s)@(%p)\n", test.len, str, str, test.dat, test.dat) ;
#endif
  }
  return test ;
}

/* a macro?!? */
void free_blob(struct blob str)
{
  if (str.dat != NULL)
	free(str.dat) ;
}
