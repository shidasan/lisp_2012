#include "lisp.h"
#define PAGESIZE 4096
#define PAGECONSSIZE ((PAGESIZE/sizeof(cons_t)) - 1)
