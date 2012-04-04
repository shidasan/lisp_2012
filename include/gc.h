#include "lisp.h"
#define PAGESIZE 4096
#define ARENASIZE (PAGESIZE * 16)
#define PAGECONSSIZE ((PAGESIZE/sizeof(cons_t)) - 1)
