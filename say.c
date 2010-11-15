
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>

#include <tdb.h>

#define ROOTPATH "store"
#define DATABASE "events.tdb"

#define init_td_data(x) { (unsigned char*)x, sizeof(x) }

/* BAAAD GLOBALs */

char *db_names[] = {
  "event.tdb",
  "head.tdb",
  "atomic.tdb",
  "tail.tdb",
  "meta.tdb",
  "info.tdb"
} ;

/*
  above should be of the same length as below on account of *pitering
   in say_fini but especially @ the end of say_init
 */

struct db_fiber {
  TDB_CONTEXT *event ;
  TDB_CONTEXT *head ;
  TDB_CONTEXT *atomic ;
  TDB_CONTEXT *tail ;
  TDB_CONTEXT *meta ;
  TDB_CONTEXT *info ;
} db_tables = { NULL, NULL, NULL, NULL, NULL, NULL } ;

int say_open_fiber() ;
int say_test(int) ;

int say_init(char *dir)
{
  struct stat buffer ;
  int status, err ;
  int ret = -1 ;

  if (dir == NULL) {
	dir = ROOTPATH ;
  }

  status = stat(dir, &buffer) ;

  if (status == -1) {
	err = errno ;
	switch (err) {
	case ENOENT:
	  status = mkdir(dir, S_IRWXU | S_IRGRP | S_IXGRP ) ;
	  if (status == -1) {
		perror("# say_init: mkdir ROOTPATH failed") ;
		return ret ;
	  }
	  status = stat(dir, &buffer) ;
	  break;
	default:
	  perror("# say_init: stat ROOTPATH failed") ;
	  return ret ;
	}
  }

  if (! S_ISDIR(buffer.st_mode)) {
	printf("# say_init: ROOTPATH=%s not a directory\n", dir) ;
	return ret ;
  }

  return say_open_fiber() ;
}

int say_open_fiber()
{
  int err, ret = -1 ;
  char *fullpath ;
  TDB_CONTEXT *ctx ;
  TDB_CONTEXT **piter = &db_tables.event ;
  int i, j = sizeof(struct db_fiber) / sizeof(TDB_CONTEXT*) ;
  int k, l = sizeof(ROOTPATH) ;

  for (i = 0; i < j; i++) {
	k = l + strlen(db_names[i]) + 1 ;
	fullpath = (char*)malloc(k) ;
	if (fullpath == NULL) {
	  perror("# say_open_fiber: failed creating full paths") ;
	  return ret ;
	}
	/* TODO: check for err here? perhaps it's lazy using snprintf? */
	snprintf(fullpath, k, "%s/%s", ROOTPATH, db_names[i]) ;
	/* ah!a, I see: snprintf is _BSD_SOURCE || _XOPEN_SOURCE >= 500 || _ISOC99_SOURCE || _POSIX_C_SOURCE >= 200112L; or cc -std=c99 */
#ifdef _SDEBUG
	printf("# say_open_fiber: opening %s\n", fullpath) ;
#endif
	ctx = tdb_open(fullpath, 0, 0, O_CREAT| O_RDWR, S_IRUSR | S_IWUSR) ;

	if (ctx == NULL) {
	  err = errno ;
	  perror("# say_open_fiber: failed to open DB") ;
	  printf("# say_open_fiber: errno=%i\n", err) ;
	  return ret ;
	}

	*piter = ctx ;
	/* can/should we really? we could "re-store" them in db_names and clean in say_fini... */
	free(fullpath) ;
	piter++ ;
  }
  return 0 ;
}

/* those two are 'almost' about the same thingies */

int say_fini()
{
  int ret = -1 ;
  TDB_CONTEXT *current ;
  TDB_CONTEXT **piter = &db_tables.event ;
  int i, j = sizeof(struct db_fiber) / sizeof(TDB_CONTEXT*) ;

  for (i = 0; i < j; i++) {
	current = *piter ;
	if (current != NULL) {
#ifdef _SDEBUG
	  printf("# say_fini: closing (%i) *ctx=%p\n", i, (void*) current) ;
#endif
	  ret = tdb_close(current) ;
	}
	piter++;
  }

  return ret ;
}

int say_sync_eventcount(char opt, uint32_t *count)
{
  TDB_CONTEXT *ctx = db_tables.info ;
  TDB_DATA key = init_td_data("eventcount") ;
  int ret = -1 ;
  int status ;

  TDB_DATA current, val ;
  char *buf ;

  switch (opt) {
	/* FETCH */
  case 'f':
	current = tdb_fetch(ctx, key) ;
	if (current.dptr == NULL) {
	  /* default value */
	  TDB_DATA defval = init_td_data("0") ;
	  status = tdb_store(ctx, key, defval, TDB_INSERT) ;
	  /* HA! and how will you communicate failure? */
	  if (status == -1) {
		perror("# say_sync_eventcount: failed tdb_store TDB_INSERT 0") ;
		return ret ;
	  }
	  *count = 0 ;
	  return 0 ;
	} else {
	  /* damn signedness */
	  *count = (uint32_t)strtoul(current.dptr, NULL, 10) ;
	  /* TODO: check it out, man! errno und such... */
	  free(current.dptr) ;
	  return 0 ;
	}
	break ;
	/* STORE */
  case 's':
	buf = (char*)malloc(24) ;
	if (buf == NULL) {
	  perror("# say_sync_eventcount: failed creating buf") ;
	  return ret ;
	}
	snprintf(buf, 24, "%i", *count) ;
	val.dptr = buf ;
	val.dsize = strlen(buf) ;
	status = tdb_store(ctx, key, val, 0) ;
	if (status == -1) {
	  perror("# say_sync_eventcount: failed tdb_store 0 some_value") ;
	} else {
	  ret = 0 ;
	}
	free(buf);
	return ret ;
	break ;
  default:
	return 0 ;
  }
}

char* say_add_atomic(char* cdata)
{
  char *ret = NULL ;

  return ret ;
}

int say_add_event(int argc, char *argv[])
{
  int i, ret = -1 ;
  char buf[1024] ;
  char *bpos = 0, *atomnum ;
  uint32_t count = 0 ;

  if (say_sync_eventcount('f', &count) == -1)
	return ret ;
  /* NOTICE lack of zeroth event... */
  count++ ;
  /* BAH... sync_eventcount should really also check if event actually exists... a solution to EH below?!? */

  for (i = 0; i < argc; i++) {
	atomnum = say_add_atomic(argv[i]);
	if (atomnum == NULL) {
	  perror("# say_add_event: say_add_atomic failure") ;
	  return ret ;
	}
	/* aggregate atomicids HERE */
  }

  /* EH, that's not so simple since this ALSO must work if we modified DB otherwise... */
  if (say_sync_eventcount('s', &count) == -1)
	return ret ;

  printf("# say_add_event: eventcount=%i eventdata=%s\n", count, buf) ;
  ret = 0 ;

  return ret ;
}

int say(int argc, char *argv[])
{
  int status ;
  TDB_CONTEXT *ctx ;

#ifdef _SDEBUG
  int i ;
  for (i = 0; i < argc; i++) {
	printf("# say: argv[%i]=%s\n", i, argv[i]) ;
  }
#endif

  status = say_init(NULL) ;
#ifdef _SDEBUG
  say_test(status) ;
#endif

  say_add_event(argc, argv) ;

  /* TOFIX? it may be wiser to only close when say_init not fails? nah... close all non-NULL! */
  return say_fini();
  /* return tdb_close(ctx) ; */
}

int say_test(int init_status)
{
  int ret = -1 ;
  TDB_CONTEXT *ctx ;
  TDB_DATA test_key = init_td_data("0") ;
  TDB_DATA test_val = init_td_data("ala ma kota") ;

  /* printf("# say: testing(%i)\n", status) ; */
  if (! (init_status == -1)) {
	ctx = db_tables.info ;

	init_status = tdb_store(ctx, test_key, test_val, TDB_INSERT) ;
	if (init_status == -1) {
	  printf("# say_test: tdb_store test failed\n") ;
	} else {
	  TDB_DATA test_res = tdb_fetch(ctx, test_key) ;

	  if (test_res.dptr == NULL) {
		printf("# say_test: tdb_fetch test failed\n") ;
	  } else {
		printf("# say_test: comparing stored vs. fetched data: %s\n", \
			   strcmp((char*)test_val.dptr, (char*)test_res.dptr) == 0 ? "OK" : "fail") ;
		ret = 0 ;
		free(test_res.dptr) ;
	  }

	  tdb_delete(ctx, test_key) ;
	  return ret ;
	}
	return ret ;
  }
  return ret ;
}

