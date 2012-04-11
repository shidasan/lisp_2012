struct cons_api_t;
struct variable_t;

typedef struct array_t {
	void **list;
	size_t size;
	size_t capacity;
}array_t;

typedef struct cons_t{
    int type;
	//void *unused_ptr;
	union {
		int ivalue;
		char* str;
		struct cons_t *car;
		struct variable_t *variable_data_table;
	};
	union {
		/* also used as free list */
		struct cons_t *cdr;
		/* used for local scope */
		struct cons_t *local_environment;
	};
	struct cons_api_t *api;
}cons_t;

typedef struct cons_api_t {
	void (* print)(cons_t *);
	void (* free)(cons_t *);
	cons_t* (* eval)(cons_t *);
	void (* trace)(cons_t *, struct array_t *);
}cons_api_t;

typedef struct opline_t{
	int instruction;
	void* instruction_ptr;
	union{
		int ivalue; //unused
		char* svalue; //unused
		struct opline_t* adr;
		cons_t *cons;
		struct array_t *a;
	}op[2];
}opline_t;

typedef struct variable_t{
	char* name;
	struct variable_t* next;
	cons_t *cons;
}variable_t;

typedef struct func_t{
	char* name;
	struct func_t* next;
	int value; // size of argument (?)
	int is_static; // cleared by bzero
	int is_special_form;
	int *is_quote;
	int creates_local_scope;
	union {
		struct array_t *opline_list;
		cons_t *(*mtd)(cons_t**, int);
		cons_t *(*special_mtd)(cons_t**, int, struct array_t*);
	};
	cons_t *environment;
	cons_t *args;
}func_t;

typedef struct ast_t {
	int type;
	int sub_type;
	union {
		int ivalue;
		char *str;
		/* array of ast_t */
		struct array_t *a;
		/* atom */
		cons_t *cons;
	};
}ast_t;

#define ADDREF(CONS, A) (array_add((A), (CONS)));\
	fprintf(stderr ,"addref %p\n", CONS)\

#define ADDREF_NULLABLE(CONS, A)\
	if ((CONS) != NULL) {\
		array_add((A), (CONS));\
		fprintf(stderr ,"addref %p\n", CONS);\
	}\

#define CONS_TRACE(CONS, A) (CONS)->api->trace(CONS, A)
#define CONS_PRINT(CONS) (CONS)->api->print(CONS)
#define CONS_FREE(CONS) (CONS)->api->free(CONS)
#define CONS_EVAL(CONS) (CONS)->api->eval(CONS)

void gc_init();

cons_t *new_cons_cell();
cons_t *new_int(int n);
cons_t *new_string(const char *str);
cons_t *new_float(float f);
cons_t *new_bool(int n);
cons_t *new_func(const char *str, cons_t *environment);
cons_t *new_variable(char *str);
cons_t *new_open();
cons_t *new_variable_data_table();
cons_t *new_local_environment();

struct array_t *new_array();
void array_free(struct array_t *a);

ast_t *new_ast(int type, int sub_type);
void ast_free(ast_t *ast);

extern cons_t *stack_value[];
#define PAGESIZE (4096 * 8)
#define ARENASIZE (PAGESIZE * 16)
#define PAGECONSSIZE ((PAGESIZE/sizeof(cons_t)) - 1)
