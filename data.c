
#include <stdlib.h> /* malloc */
#include <string.h> /* strncpy */
#include <stdio.h> /* printf, perror */

#include "data.h"

/* TODO: add NULL checks on arguments!?!? for realz! */

/* just strdup? still need to "destroy_blob" see free_blob_ */
struct blob blob_make(char *str)
{
  struct blob ret;
  /* counting and storing delimiter \0 TOO */
  ret.len = strlen(str) + 1;
  ret.dat = (char*)malloc(ret.len);
  if (ret.dat == NULL) {
	ret.len = 0;
	perror("# blob_make: malloc failed");
  } else {
	strncpy(ret.dat, str, ret.len);
#ifdef _SDEBUG
	printf("# blob_make: str(%s)@(%p) -> dat(%s)@(%p) len(%d)\n", str, str, ret.dat, ret.dat, ret.len);
#endif
  }
  return ret;
}

/* a macro?!? */
void blob_free(struct blob str)
{
  if (str.dat != NULL)
	free(str.dat);
}

/* pointeless passing around */
struct blob blob_static(char* data) {
  struct blob temp;
  temp.dat = data;
  temp.len = strlen(data) + 1;
  return temp;
}

void blob_rprint(struct blob bob) {
  int i, j=bob.len;
  printf("blob(");
  for (i = 0; i < j; i++) {
	printf(" 0x%2.2x", (unsigned char)bob.dat[i]);
  }
  if (j > 0) {
	printf(" )");
  } else {
	printf(")");
  }
}

/* "UNIVERSAL" KEY */

struct ukey ukey_make() {
  /* "non-null" init */
  struct ukey temp = { 0, 0, 0 };
  time(&temp.seconds);
  return temp;
}

int ukey_null(struct ukey check) {
  return (check.seconds == ((time_t) -1));
}

struct ukey ukey_uniq(struct ukey last) {
  struct ukey new;
  new = ukey_make();
  /* start pessimistic */
  if (ukey_null(last)) {
	printf("%% ukey_uniq: last ukey encountered null\n");
	return new;
  }
  /* caller should check if new is null...
	 nevertheless continue with optimism */
  if (new.seconds > last.seconds) {
	return new;
  } else if (new.seconds == last.seconds) {
	last.count++;
	return last;
  } else {
	/* this could be covered by second arm,
	   but it's a serious error requiring a HOLLER */
	printf("%% ukey_uniq: clock skew of (%d) seconds\n", \
		   (int)(last.seconds - new.seconds));
	last.count++;
	return last;
  }
}

void ukey_print(struct ukey out) {
  printf("ukey(%i.%i(0x%x).%i)", (int)out.epoch,
		 (int)out.seconds, (unsigned int)out.seconds,
		 out.count);
}

/* for now, this is all not so bravely bit twiddly ;-o */
struct blob ukey2blob(struct ukey key) {
  /* to extract or not to extract, unelegant ;( */
  time_t s; /* seconds */
  unsigned int c; /* count */
  unsigned char e; /* epoch */
  size_t slen, clen, elen, Tlen;
  char *blob, *temp;
  struct blob ret = { NULL, 0 };

  if (ukey_null(key))
	return ret;

  Tlen = 1;
  s = key.seconds; slen = sizeof(s); Tlen += slen;
  c = key.count; clen = sizeof(c); Tlen += clen;
  /* because of char "mis-alignment"...
	 perhaps agin this struct thing is not a good IDEA */
  e = key.epoch; elen = sizeof(e); Tlen += elen;

  blob = (char*)malloc(Tlen);
  if (blob == NULL)
	return ret;

  /* printf("# ukey2blob: doing...\n"); */
  /* warning: overflow in implicit constant conversion */
  *blob = (char)MAGICBYTE; temp=blob+1;
  memcpy(temp, &s, slen); temp+=slen;
  memcpy(temp, &c, clen); temp+=clen;
  memcpy(temp, &e, elen);
  /* blob[10] = '\0'; */
  ret.dat = blob;
  ret.len = Tlen;

  /* printf("# ukey2blob: done\n"); */
  return ret;
}
/* jeez, that's so ugly... AND loooong... */
struct ukey blob2ukey(struct blob bob) {
  time_t s; /* seconds */
  unsigned int c; /* count */
  unsigned char e; /* epoch */
  size_t slen, clen, elen, Tlen;
  char* temp = bob.dat;
  struct ukey ret = { -1, 0, 0 };

  if (temp == NULL)
	return ret;
  if (*temp != (char)MAGICBYTE) {
#ifdef _SDEBUG
	printf("# blob2ukey: no MAGICBYTE found\n");
#endif
	return ret;
  }

  Tlen = 1;
  slen = sizeof(s); Tlen += slen;
  clen = sizeof(c); Tlen += clen;
  elen = sizeof(e); Tlen += elen;

  if (Tlen != bob.len)
	return ret;

  temp += 1;
  memcpy(&s, temp, slen); temp+=slen;
  memcpy(&c, temp, clen); temp+=clen;
  memcpy(&e, temp, elen);
  /* same "shit" with alignment... */
  ret.seconds = s;
  ret.count = c;
  ret.epoch = e;

  return ret;
}

