
#include <tdb.h>

/* SHRUG */

/* a string? */
struct blob {
  uint32_t len ;
  uchar_t *dat ;
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
  struct kons dat[] ;
  struct rack *next ;
} ;

/* a database? key/val store dependent */
struct registry {
  struct blob name ;
  struct registry *next ;
  struct rack entry ;
  TDB_CONTEXT store ;
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

