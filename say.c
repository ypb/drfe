
#include <sys/stat.h>
#include <stdio.h>
/* malloc, free */
#include <stdlib.h>
/* memcpy */
#include <string.h>
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

void say_follow_last(struct db* fiber, char* cdata, struct blob last_id, int times) {
  int i, status;
  struct blob temp;
  char *atomic = db_names[2];

  printf("%s ", cdata); ukey_hprint(blob2ukey(last_id)); putchar('\n');
  last_id = store_restore(fiber, atomic, last_id);
  for (i = 1; i < times; i++) {
	printf("%s ", cdata); ukey_hprint(blob2ukey(last_id)); putchar('\n');
	temp = last_id;
	last_id = store_restore(fiber, atomic, temp);
	blob_free(temp);
  }
}

struct blob say_add_atomic(struct db* fiber, char* cdata, struct ukey mark, struct blob ekey)
{
  int status;
  struct kons data;
  /* dregs */
  char *head = db_names[1];
  char *atomic = db_names[2]; /* "atomic" */
  char *tail = db_names[3];
  struct blob input, uid, current_id, last_id;
  struct blob null_id = { NULL, 0};

  printf("%% aevent: cdata{%s}\n", cdata);
  /* mark = ukey_uniq(mark); */
  current_id = ukey2blob(mark);
  /* doing the input_AEID direction */
  input = blob_static(cdata);
  uid = store_restore(fiber, head, input);

  if (blob_null(uid)) {
	data.key = input;
	data.val = current_id;
	status = store_extend(fiber, head, data);
	if (status == -1) {
	  goto f0;
	} else {
	  data.key = current_id;
	  data.val = input;
	  status = store_extend(fiber, head, data);
	  /* TOFIX: this shouldn't fail... */
	  if (status == -1) {
		goto f1;
	  }
	}
	data.key = current_id;
	data.val = current_id;
	status = store_extend(fiber, tail, data);
	if (status == -1) {
	  goto f2;
	}
	/* here we need to construct something!1!! */
	data.val = blob_concat(ekey, current_id);
	/* TOFIX: using current_id here is too loopy for the say_follow_last... */
	status = store_extend(fiber, atomic, data);
	/* free irrespective of result, as not needed further... */
	blob_free(data.val);
	if (status == -1) {
	  goto f3;
	}
	return current_id;
  f3:	store_remove(fiber, tail, current_id);
  f2:	store_remove(fiber, head, current_id);
  f1:	store_remove(fiber, head, input);
  f0:	blob_free(current_id);
		return null_id;
  } else {
	/* here we should anally make sure AEID_input direction ends up in
	   input_AEID.key == AEID_input.val being the same string */
	/* blob_free(uid); */
	last_id = store_restore(fiber, tail, uid);
	if (blob_null(last_id)) {
	  /* that CAN'T be... */
	  printf("# say_add_atomic: error: no last_id for uid\n");
	  goto guard;
	}
	data.key = current_id;
	/* here, too, blob concoction */
	data.val = blob_concat(ekey, last_id);
	status = store_extend(fiber, atomic, data);
	blob_free(data.val);
	if (status == -1) {
	  printf("# say_add_atomic: error: current_id already preset\n");
	  blob_free(last_id);
	  goto guard;
	}
	/* follow da wabbit */
	/* say_follow_last(fiber, cdata, last_id, 3); */
	/* rabbit chase needs to be modified and moved up */
	data.key = uid;
	data.val = current_id;
	status = store_inside(fiber, tail, data);
	if (status == -1) {
	  printf("# say_add_atomic: error: last_id taileur failure\n");
	  blob_free(last_id);
	  store_remove(fiber, atomic, current_id);
	  goto guard;
	}
	blob_free(last_id);
	blob_free(current_id);
	return uid;
  }

  guard:
	blob_free(current_id);
	blob_free(uid);
	return null_id;
}

struct ukey say_add_event(struct db* fiber, struct blobs elems, struct ukey mark) {
  struct kons data;
  char* event = db_names[0]; /* "event" reg */
  struct blob buf;
  int i, status;
  struct blob eid_key, hid_key;
  struct ukey dakey, temp;

  /* overcompensate */
  buf.dat = malloc(elems.len*sizeof(mark));
  if (buf.dat == NULL)
	return mark;
  /* getting queer */
  buf.len = 0;

  dakey = mark = ukey_uniq(mark);
  eid_key = ukey2blob(mark);

  printf("%% event: "); ukey_print(mark); printf("\n");
  for (i = 0; i < elems.len; i++) {
	dakey = ukey_uniq(dakey);
	printf("; aevent: UID{"); ukey_print(dakey); printf("}\n");
	hid_key = say_add_atomic(fiber, elems.dat[i], dakey, eid_key);
	/* printf("# say_add_event: say_add_atomic(%s) status(%d)\n", elems.dat[i], status); */
	temp = blob2ukey(hid_key);
	memcpy(buf.dat + buf.len, hid_key.dat, hid_key.len);
	buf.len += hid_key.len;
	blob_free(hid_key);
	printf("%% aevent: HID{"); ukey_print(temp); printf("}\n");
  }

  printf("# event_buf: "); blob_rprint(buf); putchar('\n');
  data.key = eid_key;
  data.val = buf;
  status = store_extend(fiber, event, data);
  if (status == -1) {
	printf("# say_add_event: event buf storage failure\n");
	/* oh, GOD, please no... TOFIX: ignoring for now... */
  }
  blob_free(buf);
  blob_free(eid_key);
  return dakey;
}

struct ukey say_events_continue(struct db* fiber) {
  int status;
  struct kons tot;
  char *key = "tip_of_time";
  char *info = db_names[5]; /* "info" register */
  struct ukey ret = { -1, 0, 0 };

  tot.key = blob_static(key);
  tot.val = store_restore(fiber, info, tot.key);

  if (blob_null(tot.val)) {
	ret = ukey_uniq(ret);
	tot.val = ukey2blob(ret);
	status = store_extend(fiber, info, tot);
	/* TOFIX */
  } else {
	ret = blob2ukey(tot.val);
  }
  blob_free(tot.val);

  return ret;
}

int say(int argc, char *argv[])
{
  int status;
  struct db* snipper;
  struct ukey tot; /* tip of tehvents */
  struct ukey now = { -1,0,0 };
  /* it works but gcc warns: initializer element is not computable at load time */
  struct blobs atoms = { argc, argv };
  struct blob test_bkey;
  struct ukey test_ukey;

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

  tot = say_events_continue(snipper);
  printf("; tot: "); ukey_hprint(tot); putchar('\n');
  /* starting off the null end ... recycle gdg? hmmm...*/
  now = ukey_uniq(now);
  printf("; now: "); ukey_hprint(now); putchar('\n');

  /* testing */
  printf("# say: ukey_vs_blob test start\n");
  printf("%% say: now = "); ukey_print(now); putchar('\n');
  test_bkey = ukey2blob(now);
  printf("%% say: now = "); blob_rprint(test_bkey); putchar('\n');
  test_ukey = blob2ukey(test_bkey);
  printf("%% say: now = "); ukey_print(test_ukey); putchar('\n');
  blob_free(test_bkey);
  printf("# say: ukey_vs_blob end\n");

  tot = say_add_event(snipper, atoms, tot);
  printf("%% say: say_add_event status(%s)\n", ukey_null(tot) == 0 ? "!null" : "null");


  status = store_close(snipper);
#ifdef _SDEBUG
  printf("# say: store_close->%i\n", status);
#endif
  if (status == -1)
	return 64;

  return 0;
}

