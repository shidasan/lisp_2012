#ifndef MAIN
#define MAIN
#include <assert.h>
#include "memory.h"
#include "config.h"
#define LOG_INT_MAX 11
#define HASH_SIZE 16
#define STACKSIZE 50000
#define INSTSIZE 100000
#define ARGC _argc
#define VSTACK _vstack
#define ARGS(N) (VSTACK)[(N) - (ARGC)]
#define TODO(STR) fprintf(stderr, "TODO (%s, %d): ", __FILE__, __LINE__); fprintf(stderr, (STR)); assert(0)
#define EXCEPTION(STR) fprintf(stderr, "Exception!! (%s, %d): ", __FILE__, __LINE__);fprintf(stderr, (STR));assert(0)
#define MTD_EVAL(MTD, ...)\
	MTD(__VA_ARGS__)

#ifdef USE_DEBUG_MODE
#define DBG_P(STR) fprintf(stderr, "Debug: ");fprintf(stderr, STR);
#else
#define DBG_P(STR)
#endif
/* PUSH, END, JMP, GOTO, RETURN, CALL */
enum eINSTRUCTION { PUSH, MTDCALL, MTDCHECK, SPECIAL_MTD, GET_VARIABLE, GET_ARG, END};
enum TokType {  tok_number, tok_plus, tok_minus, tok_mul, tok_div, tok_gt, tok_gte, tok_lt, tok_lte, tok_eq, tok_if, tok_defun, tok_str, tok_eof, tok_setq, tok_valiable, tok_func, tok_arg, tok_open, tok_close, tok_error, tok_nil, tok_T, tok_symbol, tok_dot, tok_quote, tok_string};
enum eTYPE { nil = 0, T = 1, NUM = 2, OPEN = 3, INT = 4, STRING = 5, FUNC = 6, VARIABLE = 7, VARIABLE_TABLE = 8, LOCAL_ENVIRONMENT = 9};
enum ast_type {ast_atom, ast_list, ast_list_close, ast_static_func, ast_quote, ast_func, ast_variable, ast_special_form};

int shell(int , char**);
void print_return_value(cons_t *);
void *array_get(struct array_t*, size_t);
size_t array_size(struct array_t *);
void array_set(struct array_t *, size_t, void *);
void array_add(struct array_t *, void *);
void *array_pop(struct array_t *);
void string_buffer_append_s(string_buffer_t *buffer, const char *str);
void string_buffer_append_c(string_buffer_t *buffer, char c);
void string_buffer_append_i(string_buffer_t *buffer, int i);
char *string_buffer_to_string(string_buffer_t *buffer);

typedef struct static_mtd_data {
	const char *name;
	int num_args;
	int creates_local_scope;
	int is_special_form;
	int is_quote0;
	int is_quote1;
	cons_t *(*mtd)(cons_t**, int);
	cons_t *(*special_mtd)(cons_t**, int, struct array_t*);
} static_mtd_data;

typedef struct AST{
	int type;
	union {
		int i;
		char* s;
		cons_t *cons;
	};
	struct AST *LHS,*RHS,*COND;
}AST;

extern opline_t memory[INSTSIZE];
extern int CurrentIndex, NextIndex;
extern void** table;

extern static_mtd_data static_mtds[];
void init_opline();
/*hash.h*/
void mark_func_data_table(array_t *);
void mark_variable_data_table(variable_t *, array_t *);
void new_func_data_table();
cons_t* begin_local_scope();
cons_t* change_local_scope(cons_t *, cons_t *);
cons_t *end_local_scope(cons_t *old_environment);
//struct func_t* set_static_func (const char* str, int i , void* adr, void* special_mtd, int isStatic, int is_special_form, int *is_quote, int creates_local_scope);
struct func_t* set_static_func (static_mtd_data *data);
struct func_t* set_func(cons_t *cons, struct array_t *opline_list, int argc, cons_t *args, cons_t *current_environment);
void new_global_environment();
extern cons_t *current_environment;
void mark_environment_list(array_t*);
void environment_list_push(cons_t *);
cons_t *environment_list_pop();

struct cons_t* set_variable (cons_t *cons, cons_t *value, int set_local_scope);
struct cons_t* search_variable (char* str);

struct func_t* search_func (char* str);
/*generator.h*/
void GenerateProgram (AST*);
void codegen(ast_t *);
void cons_codegen(cons_t *);
/*parser.h*/
int ParseProgram(char *);
int parse_program(char *);
/*eval.h*/
cons_t* vm_exec (int, opline_t *, cons_t **);

/* gc test */
cons_t *root;
#endif
