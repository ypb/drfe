
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
  printf("ukey(%i.%i.%i)", (int)out.epoch, (int)out.seconds, out.count);
}

