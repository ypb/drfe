
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
  ret.dat = (char*)malloc(ret.len*sizeof(char));
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

int blob_null(struct blob str) {
  return (str.dat == NULL);
}

/* pointeless passing around */
struct blob blob_static(char* data) {
  struct blob temp;
  temp.dat = data;
  temp.len = strlen(data) + 1;
  return temp;
}

/* ah, damn, this will include '\0'... blobs are not strings
   after ALL!1!1! blob_join for the "string" subcategory?!? */
struct blob blob_concat(struct blob a, struct blob b) {
  struct blob ret = { NULL, 0 };

  if (blob_null(a) && blob_null(b))
	return ret;
  if (blob_null(a))
	return blob_make(b.dat);
  if (blob_null(b))
	return blob_make(a.dat);

  ret.dat = (char*)malloc((a.len + b.len)*sizeof(char));
  if (ret.dat == NULL)
	return ret;

  memcpy(ret.dat, a.dat, a.len);
  memcpy(ret.dat + a.len, b.dat, b.len);
  ret.len = a.len + b.len;

  return ret;
}

void blob_rprint(struct blob bob) {
  int i, j=bob.len;
  printf("blob(");
  for (i = 0; i < j; i++) {
	printf(" 0x%2.2x", (unsigned char)bob.dat[i]);
  }
  if (j > 0) {
	printf(" ) length=%i", bob.len);
  } else {
	printf(")");
  }
}

/* "UNIVERSAL" KEY */

struct ukey null_ukey() {
  struct ukey before_unix = { -1, 0, 0 };
  return before_unix;
}

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
#ifdef _SDEBUG
	printf("%% ukey_uniq: last ukey encountered null\n");
#endif
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
  /* hmmm... how do you do to once upon it do... TOFIX */
#ifdef _SDEBUG
  printf(" sizeof{%i+%i+%i=%i}", sizeof(out.seconds),
		 sizeof(out.count), sizeof(out.epoch), sizeof(out));
#endif
}

void ukey_hprint(struct ukey out) {
  char stime[64];
  struct tm* time;

  time = gmtime(&out.seconds);
  strftime(stime, 64, "%Y%m%d %H%M%S  %Z", time);
  printf("%s", stime);
}

void ukey_sprint(struct ukey out) {
  char stime[64];
  struct tm* time;

  time = localtime(&out.seconds);
  strftime(stime, 64, "%y%m%d %H%M", time);
  printf("%s", stime);
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

  /* allow null_ukey blob after all
  if (ukey_null(key))
	return ret;
  */

  Tlen = 1;
  s = key.seconds; slen = sizeof(s); Tlen += slen;
  c = key.count; clen = sizeof(c); Tlen += clen;
  /* because of char "mis-alignment"...
	 perhaps agin this struct thing is not a good IDEA */
  e = key.epoch; elen = sizeof(e); Tlen += elen;

  blob = (char*)malloc(Tlen*sizeof(char));
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

  /* after all we return null_ukey on NULL blob... */
  if (temp == NULL) {
#ifdef _SDEBUG
	printf("# blob2ukey: NULL blob detected\n");
#endif
	return ret;
  }
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

  if (Tlen != bob.len) {
#ifdef _SDEBUG
	printf("# blob2ukey: blob/ukey length mismatch\n");
#endif
	return ret;
  }

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

/* well, we do need individuals' lengths */
struct ablobs __ukeys_blob2ablobs(struct blob ukeys) {
  int i, len, j;
  char* stream, *end;
  struct blob temp;
  struct ablobs ret = { 0, NULL};

  if (ukeys.dat == NULL || (*ukeys.dat != (char)MAGICBYTE))
	return ret;

  /* overassuming all ukeys are of minimal length */
  len = (ukeys.len / MINUKEYLEN) + 1;
  ret.dat = (struct blob*)malloc(len*(sizeof(struct blob)));
  if (ret.dat == NULL)
	return ret;

  temp.dat = ukeys.dat;
  /* that's all wery, wery riskee */
  end = ukeys.dat + ukeys.len;
  for (i = 0, stream = ukeys.dat + 1, j = 1;
	   stream < end && i < len;
	   stream++, j++) {
	if (*stream == (char)MAGICBYTE) {
	  temp.len = j;
	  ret.dat[i] = temp;
	  i++;
	  temp.dat = stream;
	  j = 0;
	}
  }
  temp.len = j;
  ret.dat[i] = temp;
  ret.len = i+1;
  return ret;
}

/*
#define member_size(type, member) sizeof(((type *)0)->member)
sizeof(((parent_t){0}).text)
#define member_sizeof(T,F) sizeof(((T *)0)->F)
*/

/* the latter seems nicer but: "warning: ISO C90 forbids compound literals" */
#define sizeof_member(type, member) sizeof(((type *)0)->member)
/* #define sizeof_member(type, member) sizeof(((type){0}).member) */

/*
  TODO: above function is wrong because ukey can contain MAGICBYTE...
  but with variable length ukey this will get over-complicated i.e.
  invent better data structure?
*/
struct ablobs ukeys_blob2ablobs(struct blob ukeys) {
  int i, len, j, ukeylen;
  char* stream, *end;
  struct blob temp;
  struct ablobs ret = { 0, NULL};

  if (ukeys.dat == NULL || (*ukeys.dat != (char)MAGICBYTE))
	return ret;

  ukeylen = 1 + sizeof_member(struct ukey, seconds);
  ukeylen += sizeof_member(struct ukey, count);
  ukeylen += sizeof_member(struct ukey, epoch);
#ifdef _SDEBUG
  printf("# ukeys_blob2ablobs: dynamic ukey length = %d\n", ukeylen);
#endif
  /* TODO: static size, will have to redo on variable length ukeyblob */
  len = (ukeys.len / ukeylen);
  ret.dat = (struct blob*)malloc(len*(sizeof(struct blob)));
  if (ret.dat == NULL)
	return ret;

  /* that's all wery, wery riskee */
  end = ukeys.dat + ukeys.len;
  for (i = 0, stream = ukeys.dat;
	   stream < end && i < len;
	   stream += ukeylen, i++) {
	if (*stream == (char)MAGICBYTE) {
	  temp.dat = stream;
	  temp.len = ukeylen;
	  ret.dat[i] = temp;
	}
  }

  ret.len = i;
  return ret;
}
