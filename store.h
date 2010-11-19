
#include <inttypes.h>

/* that's silly that tdb.h requires this too explicitly... */
#include <sys/types.h>
#include <tdb.h>

/* to typedef the rest or not to typedef */
typedef unsigned char uchar_t ;

/* SHRUG */

/* a strings? */
struct blob {
  uchar_t *dat ;
  size_t len ;
} ;

struct blobs {
  uint32_t len ;
  uchar_t **dat ;
} ;

/* a record? */
struct kons {
  struct blob key ;
  struct blob val ;
} ;

/* a list? in a lists */
struct rack {
  uint32_t index ;
  uint32_t len ;
  struct kons *dat ;
  struct rack *next ;
} ;

/* a database? key/val store dependent */
struct registry {
  struct blob name ;
  struct registry *next ;
  struct rack entry ;
  TDB_CONTEXT *store ;
} ;

/* a (volatile?) contents... */

struct data {
  uint32_t size ;
  void *start ;
  void *current ;
  struct data *next ;
} ;

/* a database */

struct db {
  struct blob path ;
  struct registry *first ;
} ;

struct db* store_open_fiber(char*, struct blobs) ;
int store_close_fiber(struct db*) ;
void store_lsns(struct db*) ;
struct blob make_blob(char*) ;

int store_extend(struct db*, const char*, struct kons) ;
int store_exists(struct db*, const char*, struct blob) ;
int store_inside(struct db*, const char*, struct kons) ;
int store_remove(struct db*, const char*, struct blob) ;

struct blob store_restore(struct db*, const char*, struct blob) ;

