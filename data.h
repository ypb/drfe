
/* this header madness must be sorted out once and for good */
#include <time.h>

/* to typedef the rest or not to typedef */
typedef unsigned char uchar_t ;
/* not needed anymore */

/* SHRUG */

/* a string? dat or len first?!?*/
struct blob {
  char* dat;
  size_t len;
};

/* an array of "strings" */
struct blobs {
  int len;
  char* *dat;
};

struct ablobs {
  int len;
  struct blob* dat;
};

/* malloccy stuff */
struct blob blob_make(char*);
void blob_free(struct blob);

/* this could probably be just a macro see init_td_data in say.c */
struct blob blob_static(char*);
struct blob blob_concat(struct blob, struct blob);

int blob_null(struct blob);

void blob_rprint(struct blob);

/* "UNIVERSAL" KEY */

struct ukey {
  time_t seconds;
  unsigned int count;
  unsigned char epoch;
};

/*
  this would be unpacked representation; perhaps do a repeating every 4 bytes
  1+2+3+2 epoch bits so they fit "nicely" into one byte, then four bits will
  be wasted on "continuation" marks and event count will be 6+5+4+5 bits per
  byte for 2^11 (~mili-) and upto 2^20 (micro-second) "resolution"...
*/

struct ukey null_ukey();
/* start ukeying now! */
struct ukey ukey_make();
/* make sure no key is identical AND all is well ordered...*/
struct ukey ukey_uniq(struct ukey);
/* i.e. passing last known good key should return "the next good one" */

/* "The bytes 0xfe and 0xff are never used in the UTF-8 encoding." */
#define MAGICBYTE 0xfe
#define MINUKEYLEN 6

struct blob ukey2blob(struct ukey);
struct ukey blob2ukey(struct blob);
/* construct an array of pointers to individual ukeys in a continuous blob... */
struct ablobs ukeys_blob2ablobs(struct blob);

int ukey_null(struct ukey);
void ukey_print(struct ukey);
void ukey_hprint(struct ukey);
