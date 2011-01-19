
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

/* malloccy stuff */
struct blob blob_make(char*);
void blob_free(struct blob);

