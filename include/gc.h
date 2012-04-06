typedef struct cons_t{
    int type;
	int unused;
	union {
		int ivalue;
		const char* str;
		cons_t *car;
	};
	/* also used as free list */
	struct cons_t *cdr;
}cons_t;

void gc_init();
cons_t *new_int(int n);
cons_t *new_string(const char *str);
cons_t *new_float(float f);

#define PAGESIZE 4096
#define ARENASIZE (PAGESIZE * 16)
#define PAGECONSSIZE ((PAGESIZE/sizeof(cons_t)) - 1)
