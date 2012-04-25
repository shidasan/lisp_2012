#ifndef MAIN
#define MAIN
#include <assert.h>
#include <ctype.h>
#include "memory.h"
#include "config.h"
#define LOG_INT_MAX 11
#define HASH_SIZE 16
#define STACKSIZE 50000
#define INSTSIZE 100000
#define ARGC _argc
#define likely(COND)   (COND)
#define unlikely(COND) (COND)
#define VSTACK _vstack
#define ARGS(N) (VSTACK)[(N) - (ARGC)]
#define TODO(STR) fprintf(stderr, "TODO (%s, %d): ", __FILE__, __LINE__); fprintf(stderr, (STR)); assert(0)
#define EXCEPTION(STR) fprintf(stderr, "Exception!! (%s, %d): ", __FILE__, __LINE__);fprintf(stderr, (STR));assert(0)
#define MTD_EVAL(MTD, ...)\
	MTD(__VA_ARGS__)

#define FLAG_MACRO                     (1 << 0)
#define FLAG_STATIC                    (1 << 1)
#define FLAG_SPECIAL_FORM              (1 << 2)
#define FLAG_LOCAL_SCOPE               (1 << 3)

#define FLAG_IS_MACRO(FLAG)            (FLAG & FLAG_MACRO)
#define FLAG_IS_STATIC(FLAG)           (FLAG & FLAG_STATIC)
#define FLAG_IS_SPECIAL_FORM(FLAG)     (FLAG & FLAG_SPECIAL_FORM)
#define FLAG_CREATES_LOCAL_SCOPE(FLAG) (FLAG & FLAG_LOCAL_SCOPE)

#define TYPE_MASK ((uintptr_t)0x000F000000000000)

#define INT_OFFSET                (((uintptr_t)1) << 48)
#define FLOAT_OFFSET              (((uintptr_t)2) << 48)
#define T_OFFSET              (((uintptr_t)3) << 48)
#define nil_OFFSET              (((uintptr_t)4) << 48)

#define IS_UNBOX(VAL)            ((VAL).ivalue & TYPE_MASK)
#define IS_INT(VAL)               (((VAL).ivalue & TYPE_MASK) == INT_OFFSET)
#define IS_FLOAT(VAL)             (((VAL).ivalue & TYPE_MASK) == FLOAT_OFFSET)
#define IS_T(VAL)             (((VAL).ivalue & TYPE_MASK) == T_OFFSET)
#define IS_nil(VAL)             (((VAL).ivalue & TYPE_MASK) == nil_OFFSET)

#define TO_INT(VAL)               ((int)(VAL & ((1 << 32)-1)))

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
void print_return_value(val_t );
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
	int flag;
	int is_quote0;
	int is_quote1;
	val_t (*mtd)(cons_t**, int);
	val_t (*special_mtd)(cons_t**, int, struct array_t*);
} static_mtd_data;

typedef struct AST{
	int type;
	union {
		int i;
		char* s;
		val_t cons;
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
cons_t* change_local_scope(cons_t* , cons_t*);
cons_t* end_local_scope(cons_t *old_environment);
//struct func_t* set_static_func (const char* str, int i , void* adr, void* special_mtd, int isStatic, int is_special_form, int *is_quote, int creates_local_scope);
struct func_t* set_static_func (static_mtd_data *data);
struct func_t* set_func(cons_t *cons, struct array_t *opline_list, int argc, val_t args, cons_t *current_environment, int flag);
void new_global_environment();
extern cons_t* current_environment;
void mark_environment_list(array_t*);
void environment_list_push(cons_t*);
cons_t *environment_list_pop();

struct val_t set_variable (cons_t *cons, val_t value, int set_local_scope);
struct val_t search_variable (char* str);

struct func_t* search_func (char* str);
/*generator.h*/
void codegen(val_t );
/*parser.h*/
int parse_program(char *);
/*eval.h*/
val_t vm_exec (int, opline_t *, val_t *);

/* gc test */
val_t root;
#endif
