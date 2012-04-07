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

typedef struct opline_t{
    int instruction;
    void* instruction_ptr;
    union{
        int ivalue; //unused
        char* svalue; //unused
        struct opline_t* adr;
		cons_t *cons;
    }op[2];
}opline_t;

typedef struct variable_t{
    char* name;
    struct variable_t* next;
    int value;
}variable_t;

typedef struct func_t{
    char* name;
    struct func_t* next;
    int value; // size of argument (?)
	int isStatic; // cleared by bzero
    opline_t* adr;
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

#define CONS_PRINT(CONS) (CONS)->api->print(CONS)
#define CONS_FREE(CONS) (CONS)->api->free(CONS)

void gc_init();

cons_t *new_cons_cell();
cons_t *new_int(int n);
cons_t *new_string(const char *str);
cons_t *new_float(float f);
cons_t *new_bool(int n);
cons_t *new_func(char *str);
cons_t *new_variable(char *str);

array_t *new_array();
void array_free(array_t *a);

ast_t *new_ast(int type, int sub_type);
void ast_free(ast_t *ast);
#define PAGESIZE 4096
#define ARENASIZE (PAGESIZE * 16)
#define PAGECONSSIZE ((PAGESIZE/sizeof(cons_t)) - 1)
