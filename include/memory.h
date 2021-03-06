#include <stdlib.h>
struct cons_api_t;
struct variable_t;

#define STRING_BUFFER_INIT_SIZE 256
typedef struct string_buffer_t {
	char *str;
	size_t size;
	size_t capacity;
}string_buffer_t;

struct val_t;

typedef struct array_t {
	union {
		void **list;
		struct val_t *val;
	};
	int size;
	int capacity;
}array_t;

struct cons_t;

typedef struct val_t {
	union {
		struct cons_t *ptr;
		struct array_t *a;
		struct val_t *list;
		struct {
			union {
				int ivalue;
				float fvalue;
			};
			int tag;
		};
	};
}val_t;

typedef struct lambda_env_t {
	val_t args;
	struct cons_t *environment;
}lambda_env_t;

typedef struct lambda_data_t {
	int opline_idx;
	val_t body;
}lambda_data_t;

typedef struct cons_t{
	int type;
	union {
		void* ptr;
		const char* str;
		struct val_t car;
		struct variable_t *variable_data_table;
		struct lambda_env_t *env;
		/* array size */
		int size;
	};
	union {
		/* also used as free list */
		struct val_t cdr;
		/* used for local scope */
		struct cons_t *local_environment;
		/* used for array */
		val_t *list;
	};
	struct cons_api_t *api;
}cons_t;

typedef struct cons_api_t {
	void (* to_string)(cons_t *, string_buffer_t*);
	void (* free)(cons_t *);
	void (* trace)(cons_t *, struct array_t *);
}cons_api_t;

typedef struct opline_t{
	int instruction;
	void* instruction_ptr;
	union{
		int ivalue; //unused
		char* svalue; //unused
		struct opline_t* adr;
		val_t val;
		struct array_t *a;
	}op[2];
}opline_t;

void opline_free();

typedef struct variable_t{
	char* name;
	struct variable_t* next;
	val_t cons;
}variable_t;

typedef struct func_t{
	char* name;
	struct func_t* next;
	int value;
	int value_minimum;
	int flag;
	int *is_quote;
	union {
		struct array_t *opline_list;
		val_t (*mtd)(val_t*, int);
		val_t (*special_mtd)(val_t*, struct array_t*);
	};
	cons_t *environment;
	val_t args;
}func_t;

void func_data_table_free();

#define ADDREF(CONS, A) \
	(array_add((A), (CONS)));\

#define ADDREF_VAL(VAL, A) \
	if (!IS_UNBOX(VAL)) {\
		(array_add((A), (VAL).ptr));\
	}\

#define ADDREF_NULLABLE(CONS, A)\
	if ((CONS) != NULL) {\
		array_add((A), (CONS));\
	}\

#define ADDREF_VAL_NULLABLE(VAL, A) \
	if (!IS_UNBOX(VAL) && (VAL).ptr != NULL) {\
		(array_add((A), (VAL).ptr));\
	}\

#define FREE(PTR)\
	free(PTR);\
	(PTR) = NULL\

#define CONS_TRACE(CONS, A) (CONS)->api->trace(CONS, A)
void val_to_string(val_t , string_buffer_t*, int);
#define VAL_TO_STRING(VAL, BUFFER, IS_ROOT) val_to_string(VAL, BUFFER, IS_ROOT)
#define VAL_PRINT(VAL, BUFFER)\
	string_buffer_t* BUFFER = new_string_buffer();\
	VAL_TO_STRING(VAL, BUFFER, 1);\
	char *BUFFER##_str = string_buffer_to_string(BUFFER);\
	fprintf(stdout, "%s", BUFFER##_str);\
	FREE(BUFFER##_str);\
	FREE(BUFFER);\


#define CONS_FREE(CONS) (CONS)->api->free(CONS)
#define CONS_EVAL(CONS) (CONS)->api->eval(CONS)

void gc_init();
void gc_end();
void cstack_cons_cell_push(cons_t *);
cons_t *cstack_cons_cell_pop();
void cstack_cons_cell_clear();

val_t null_val();
val_t new_int(int n);
val_t new_float(float f);
val_t new_bool(int n);
cons_t* new_cons_cell();
cons_t* new_string(const char *str);
cons_t* new_cons_array(val_t);
cons_t* new_cons_array_list(array_t*);
cons_t *new_lambda(lambda_env_t *, array_t *);
cons_t* new_func(char *str, cons_t *environment);
cons_t* new_variable(char *str);
cons_t* new_open();
cons_t* new_variable_data_table();
cons_t* new_local_environment();
void environment_clear();

array_t *new_array();
void array_free(array_t *a);

string_buffer_t *new_string_buffer();
void string_buffer_free(string_buffer_t *buffer);

extern val_t stack_value[];
extern val_t *esp;
#define PAGESIZE (4096*8)
#define ARENASIZE (PAGESIZE * 16)
#define PAGECONSSIZE ((PAGESIZE/sizeof(cons_t)) - 1)
