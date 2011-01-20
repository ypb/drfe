
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>

#include "data.h"
#include "store.h"

/* for now it's a sub-directory... relative to cwd of the exec()utor */
#define ROOTPATH ".say_store"

#define init_td_data(x) { (unsigned char*)x, sizeof(x) }

/* BAAAD GLOBALs */

char *db_names[] = {
  "event",
  "head",
  "atomic",
  "tail",
  "meta",
  "info"
} ;

struct blobs reg_names = { 6, db_names } ;


struct db* say_init(char *dir)
{
  struct stat buffer ;
  int status, err ;
  struct db* ret = NULL;

  if (dir == NULL) {
	dir = ROOTPATH ;
  }

  status = stat(dir, &buffer) ;

#ifdef _SDEBUG
  printf("# say_init: stat(ROOTPATH, &buffer)=%i\n", status) ;
#endif

  if (status == -1) {
	err = errno ;
	switch (err) {
	case ENOENT:
	  status = mkdir(dir, S_IRWXU) ; /* | S_IRGRP | S_IXGRP | S_IRWXO ) ; very restrictive */
	  if (status == -1) {
		perror("# say_init: mkdir ROOTPATH failed") ;
		return ret ;
	  }
	  /* oh, and for a moment i thought struct stat st_mode updates itself dynamically, LOL */
	  status = stat(dir, &buffer) ;
	  break;
	default:
	  perror("# say_init: stat ROOTPATH failed") ;
	  return ret ;
	}
  }

#ifdef _SDEBUG
  printf("# say_init: ROOTPATH st_mode is %o\n", buffer.st_mode) ;
#endif

  if (! (S_ISDIR(buffer.st_mode))) {
	printf("# say_init: ROOTPATH(%s) not a directory\n", dir) ;
	return ret ;
  }

  ret = store_open_fibers(dir, reg_names);
  if (ret == NULL) {
	perror("# say_init: failed to open database");
	return ret;
  }
  /* LOL, shouldn't that be a SEGFAULT? too little stack use? HMMMMM....?*/
#ifdef _SDEBUG 
  printf("# say_init: db->path.dat=%s db->path.len=%i\n", ret->path.dat, ret->path.len);
#endif

  return ret;
}

int say_add_atomic(struct db* fiber, char* cdata)
{
  /* here key is textual cdata we are storing */
  struct kons input_AEID;
  /* and here value is that cdata if it's already present with an above AEID */
  struct kons AEID_input;
  /* it's not only slightly confusing me alone ;*/
  char *reg = db_names[2]; /* "atomic" */
  int ret = -1;

  /* doing the input_AEID direction */
  struct blob input = blob_static(cdata);
  struct blob AEID = store_restore(fiber, reg, input);

  /* TODO: encapsulate in a fun? like blob_null or smth? */
  if (AEID.dat == NULL) {
	/* for now val = key... i.e. cdata */
	input_AEID.key = input;
	/* but will need to generate "timed" key here as input's value */
	input_AEID.val = input;
	ret = store_extend(fiber, reg, input_AEID);
  } else {
	/* here we should anally make sure AEID_input direction ends up in
	   input_AEID.key == AEID_input.val being the same string */
	blob_free(AEID);
	ret = 0;
  }

  return ret;
}

struct ukey say_add_event(struct db* fiber, struct blobs elems, struct ukey mark) {
  int i, status;
  struct ukey dakey;

  dakey = mark;

  printf("%% event: "); ukey_print(mark); printf("\n");
  for (i = 0; i < elems.len; i++) {
	dakey = ukey_uniq(dakey);
	printf("%% aevent: "); ukey_print(dakey); printf("\n");
	status = say_add_atomic(fiber, elems.dat[i]);
	printf("# say_add_event: say_add_atomic(%s) status(%d)\n", elems.dat[i], status);
  }
  return dakey;
}

int say(int argc, char *argv[])
{
  int status;
  struct db* snipper;
  struct ukey event = { -1,0,0 };
  /* it works but gcc warns: initializer element is not computable at load time */
  struct blobs atoms = { argc, argv };

#ifdef _SDEBUG
  int i;
  for (i = 0; i < argc; i++) {
	printf("# say: argv[%i]=%s\n", i, argv[i]);
  }
#endif

  snipper = say_init(NULL);
  if (snipper == NULL)
	return 128;
#ifdef _SDEBUG
  store_test(snipper);
#endif

  /* starting off the null end ... recycle gdg? hmmm...*/
  event = ukey_uniq(event);
  printf("%% say: dakey = "); ukey_print(event);
  printf(" sizeof is %i+%i+%i=%i\n", sizeof(event.seconds),
		 sizeof(event.count), sizeof(event.epoch), sizeof(event));

  event = say_add_event(snipper, atoms, event);
  printf("%% say: say_add_event status(%s)\n", ukey_null(event) == 0 ? "!null" : "null");


  status = store_close(snipper);
#ifdef _SDEBUG
  printf("# say: store_close->%i\n", status);
#endif
  if (status == -1)
	return 64;

  return 0;
}

