struct cons_api_t;
typedef struct cons_t{
    int type;
	union {
		int ivalue;
		char* str;
		cons_t *car;
	};
	/* also used as free list */
	struct cons_t *cdr;
	struct cons_api_t *api;
}cons_t;

typedef struct cons_api_t {
	void (* print)(cons_t *);
	void (* free)(cons_t *);
}cons_api_t;

typedef struct ast_t {
	int type;
	union {
		int ivalue;
		char *svalue;
		/* array of cons_t */
		struct array_t *a;
	};
}ast_t;

#define CONS_PRINT(CONS) (CONS)->api->print(CONS)
#define CONS_FREE(CONS) (CONS)->api->free(CONS)

void gc_init();

cons_t *new_cons_cell();
cons_t *new_int(int n);
cons_t *new_string(const char *str);
cons_t *new_float(float f);
cons_t *new_bool(int n);

array_t *new_array();
void array_free(array_t *a);

ast_t *new_ast(int type);
void ast_free(ast_t *ast);
#define PAGESIZE 4096
#define ARENASIZE (PAGESIZE * 16)
#define PAGECONSSIZE ((PAGESIZE/sizeof(cons_t)) - 1)
