
#include <sys/stat.h>
#include <stdio.h>
/* malloc, free */
#include <stdlib.h>
/* memcpy */
#include <string.h>
#include <errno.h>

#include "data.h"
#include "store.h"
#include "version.h"

/* for now it's a sub-directory... relative to cwd of the exec()utor */
#define ROOTPATH ".drfe"

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
	  /* it's a surprising behaviour for now, so inform unsuspecting by-standers */
	  printf("# drfe init: created `%s' directory\n", ROOTPATH);
	  /* TODO: prepend CWD for disconfundum... */
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

void say_print_event(struct db* fiber, struct ukey eid) {
  int i;
  struct blob event_key;
  struct blob event_data;
  struct ablobs aevents;
  char *event = db_names[0];
  char *head = db_names[1];

  event_key = ukey2blob(eid);
  /* check if null? */
  event_data = store_restore(fiber, event, event_key);
  blob_free(event_key);
  if (blob_null(event_data))
	return;

  aevents = ukeys_blob2ablobs(event_data);
  if (aevents.len == 0) {
	blob_free(event_data);
	return;
  }
  printf("; "); ukey_sprint(eid); printf(" ;");
  for (i = 0; i < aevents.len; i++) {
	/* recycle event_key though it's aevent cdata now! */
	event_key = store_restore(fiber, head, aevents.dat[i]);
	if (blob_null(event_key))
	  continue;
	printf(" %s", event_key.dat);
	blob_free(event_key);
  }
  putchar('\n');
  free(aevents.dat);
  blob_free(event_data);
}

void say_follow_last(struct db* fiber, char* cdata, struct blob last_id, int times) {
  int i, j;
  struct ablobs ukeys;
  struct blob temp;
  struct ukey event, previous;
  struct ukey events[times];
  char *atomic = db_names[2];

  j = -1; /* found nothing so far */
  for (i = 0; i < times; i++) {
	temp = store_restore(fiber, atomic, last_id);
	blob_free(last_id);
	if (blob_null(temp))
	  break;
	ukeys = ukeys_blob2ablobs(temp);
	/* perhaps make sure we got what we were asking for */
#ifdef _SDEBUG
	printf("# say_follow_last: ablobs.len = %i\n", ukeys.len);
#endif
	if (ukeys.len < 2) {
	  blob_free(temp);
	  if (ukeys.dat != NULL)
		free(ukeys.dat);
	  break;
	}
	event = blob2ukey(ukeys.dat[0]);
	/* need blob_clone... hmm...*/
	previous = blob2ukey(ukeys.dat[1]);
#ifdef _SDEBUG
	printf("%s event: ", cdata); ukey_hprint(event);
	printf(" previous: "); ukey_hprint(previous); putchar('\n');
#endif
	/* say_print_event(fiber, event); */
	events[i] = event;
	j++;

	blob_free(temp);
	if (ukeys.dat != NULL)
	  free(ukeys.dat);
	/* break the cycle */
	if (ukey_null(previous)) {
	  break;
	} else {
	  last_id = ukey2blob(previous);
	}
  }
  /* now, print anything found in REVERSE order */
  for (i = j; i >= 0; i--) {
	say_print_event(fiber, events[i]);
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

#ifdef _SDEBUG
  printf("%% aevent: cdata{%s}\n", cdata);
#endif
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
	last_id = ukey2blob(null_ukey());
	/* here we need to construct something!1!! */
	data.val = blob_concat(ekey, last_id);
	/* TOFIX: using current_id here is too loopy for the say_follow_last... */
	status = store_extend(fiber, atomic, data);
	/* free irrespective of result, as not needed further... */
	blob_free(last_id);
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
	data.key = uid;
	data.val = current_id;
	status = store_inside(fiber, tail, data);
	if (status == -1) {
	  printf("# say_add_atomic: error: last_id taileur failure\n");
	  blob_free(last_id);
	  store_remove(fiber, atomic, current_id);
	  goto guard;
	}
	/* follow da wabbit, awkward for now... */
	say_follow_last(fiber, cdata, last_id, 7);
	/* TODO: return smarter structure to pass it onto _add_event...*/
	/* blob_free(last_id); */
	/* says_follows_lasts frees thems lasts as its goes... */
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

#ifdef _SDEBUG
  printf("%% event: "); ukey_print(mark); printf("\n");
#endif
  for (i = 0; i < elems.len; i++) {
	dakey = ukey_uniq(dakey);
#ifdef _SDEBUG
	printf("; aevent: UID{"); ukey_print(dakey); printf("}\n");
#endif
	hid_key = say_add_atomic(fiber, elems.dat[i], dakey, eid_key);
	/* printf("# say_add_event: say_add_atomic(%s) status(%d)\n", elems.dat[i], status); */
	temp = blob2ukey(hid_key);
	memcpy(buf.dat + buf.len, hid_key.dat, hid_key.len);
	buf.len += hid_key.len;
	blob_free(hid_key);
#ifdef _SDEBUG
	printf("%% aevent: HID{"); ukey_print(temp); printf("}\n");
#endif
  }

#ifdef _SDEBUG
  printf("# event_buf: "); blob_rprint(buf); putchar('\n');
#endif
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

void ask(struct db* fiber, struct blobs args)
{
  int i, depth;
  char* head = db_names[1];
  char* tail = db_names[3];
  struct blob input;
  struct blob uid;
  struct blob last_id;

  if (args.len <= 0)
	return;
  /* terminal height... TODO: try to determine dynamically */
  depth = 36 / args.len;
  /* about twice the "absolut" minimum of the 80x20 "standard" */
  for (i = 0; i < args.len; i++) {
#ifdef _SDEBUG
	printf("# ask: argv[%i]=\"%s\"\n", i, args.dat[i]);
#endif
	input = blob_static(args.dat[i]);
	uid = store_restore(fiber, head, input);
	if (blob_null(uid))
	  continue;
	last_id = store_restore(fiber, tail, uid);
	blob_free(uid);
	if (blob_null(last_id))
	  continue;
	say_follow_last(fiber, args.dat[i], last_id, depth);
  }
}

void say_ukeyblob_test(const char*, struct ukey);

int say(int argc, char *argv[])
{
#ifdef _SDEBUG
  int i;
#endif
  int status;
  struct db* snipper;
  struct ukey tot; /* tip of tehvents */
  struct ukey now = { -1,0,0 };
  /* it works but gcc warns: initializer element is not computable at load time */
  struct blobs atoms = { argc, argv };

  /* a feeble fix */
  if (argc <= 0) {
	drfe_banner(); drfe_help(); return 0;
  }

#ifdef _SDEBUG
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
  printf("; "); ukey_hprint(tot); printf(" ; tot\n");
  /* starting off the null end ... recycle gdg? hmmm...*/
  now = ukey_uniq(now);

#ifdef _SDEBUG
  say_ukeyblob_test("now", now);
#endif

  if (argc > 0) {
	if (strcmp(argv[0], "ask") == 0) {
	  atoms.len -= 1; atoms.dat += 1;
	  ask(snipper, atoms);
	} else if (strcmp(argv[0], "say") == 0) {
	  atoms.len -= 1; atoms.dat += 1;

  tot = say_add_event(snipper, atoms, tot);
#ifdef _SDEBUG
  printf("%% say: say_add_event status(%s)\n", ukey_null(tot) == 0 ? "!null" : "null");
#endif
	} else {
	  /* TOFIX */
	  drfe_help();
	}
  }

  printf("; "); ukey_hprint(now); printf(" ; now\n");
  status = store_close(snipper);
#ifdef _SDEBUG
  printf("# say: store_close->%i\n", status);
#endif
  if (status == -1)
	return 64;

  return 0;
}

void say_ukeyblob_test(const char* name, struct ukey test) {
  struct blob test_bkey;
  /* testing */
  printf("# say_ukeyblob_test: start\n");
  printf("%%  original: %s = ", name); ukey_print(test); putchar('\n');
  test_bkey = ukey2blob(test);
  printf("%% converted: %s = ", name); blob_rprint(test_bkey); putchar('\n');
  test = blob2ukey(test_bkey);
  printf("%%  reverted: %s = ", name); ukey_print(test); putchar('\n');
  blob_free(test_bkey);
  printf("# say_ukeyblob_test: end\n");
}

