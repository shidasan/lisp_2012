#include "memory.h"
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <dlfcn.h>
#include <assert.h>
#include <time.h>
#include <ctype.h>
#include <setjmp.h>
#include <math.h>
#include <readline/readline.h>

#ifndef MAIN
#define MAIN
#define LOG_INT_MAX 11
#define HASH_SIZE 16
#define STACKSIZE 50000
#define INSTSIZE 256
#define INIT_FILE_SIZE 50
#define ARGC _argc
#define likely(COND)   (COND)
#define unlikely(COND) (COND)
#define VSTACK _vstack
#define ARGS(N) (VSTACK)[(N) - (ARGC)]
#define TODO(STR) fprintf(stderr, "TODO (%s, %d): ", __FILE__, __LINE__); fprintf(stderr, (STR)); assert(0)
#define EXCEPTION(STR) throw_exception(__FILE__, __LINE__, STR)
#define FMT_EXCEPTION(STR, ...) throw_fmt_exception(__FILE__, __LINE__, STR, __VA_ARGS__)
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

#define TYPE_MASK ((int)0x000F0000)

#define INT_OFFSET                (((int)1) << 16)
#define FLOAT_OFFSET              (((int)2) << 16)
#define T_OFFSET                  (((int)3) << 16)
#define nil_OFFSET                (((int)4) << 16)

#define VAL_TYPE(VAL)             ((VAL).tag & TYPE_MASK)
#define IS_UNBOX(VAL)             (VAL_TYPE(VAL))
#define IS_NULL(VAL)              ((VAL).ptr == NULL)
#define IS_INT(VAL)               (VAL_TYPE(VAL) == INT_OFFSET)
#define IS_FLOAT(VAL)             (VAL_TYPE(VAL) == FLOAT_OFFSET)
#define IS_NUMBER(VAL)            (IS_INT(VAL) || IS_FLOAT(VAL))
#define IS_T(VAL)                 (VAL_TYPE(VAL) == T_OFFSET)
#define IS_nil(VAL)               (VAL_TYPE(VAL) == nil_OFFSET)
#define IS_OPEN(VAL)              (!IS_UNBOX(VAL) && (VAL).ptr->type == OPEN)
#define IS_STRING(VAL)            (!IS_UNBOX(VAL) && (VAL).ptr->type == STRING)
#define IS_ARRAY(VAL)             (!IS_UNBOX(VAL) && (VAL).ptr->type == ARRAY)
#define IS_FUNC(VAL)              (!IS_UNBOX(VAL) && (VAL).ptr->type == FUNC)
#define IS_LAMBDA(VAL)            (!IS_UNBOX(VAL) && (VAL).ptr->type == LAMBDA)
#define IS_VARIABLE(VAL)          (!IS_UNBOX(VAL) && (VAL).ptr->type == VARIABLE)
#define IS_SYMBOL(VAL)            (IS_FUNC(VAL) || IS_VARIABLE(VAL))
#define IS_CALLABLE(VAL)          (IS_SYMBOL(VAL) || IS_LAMBDA(VAL))
#define IS_VARIABLE_TABLE(VAL)    (!IS_UNBOX(VAL) && (VAL).ptr->type == VARIABLE_TABLE)
#define IS_LOCAL_ENVIRONMENT(VAL) (!IS_UNBOX(VAL) && (VAL).ptr->type == LOCAL_ENVIRONMENT)

#define TO_INT(VAL)               ((int)(VAL & ((1 << 32)-1)))

#ifdef USE_DEBUG_MODE
#define DBG_P(STR) fprintf(stderr, "Debug: ");fprintf(stderr, STR);
#else
#define DBG_P(STR)
#endif
/* PUSH, END, JMP, GOTO, RETURN, CALL */
enum eINSTRUCTION { PUSH, MTDCALL, MTDCHECK, SPECIAL_MTD, GET_VARIABLE, GET_ARG, END, JMP};
enum TokType {  tok_int, tok_float, tok_eof, tok_open, tok_close, tok_error, tok_nil, tok_T, tok_symbol, tok_dot, tok_quote, tok_string, tok_array};
enum eTYPE { nil = 0, T = 1, NUM = 2, OPEN = 3, INT = 4, STRING = 5, FUNC = 6, VARIABLE = 7, VARIABLE_TABLE = 8, LOCAL_ENVIRONMENT = 9, LAMBDA = 10, ARRAY = 11};
enum ast_type {ast_atom, ast_list, ast_list_close, ast_static_func, ast_quote, ast_func, ast_variable, ast_special_form};

int shell(int , char**);
void print_return_value(val_t );
void *array_get(struct array_t*, int);
val_t array_get_val(struct array_t*, int);
int array_size(struct array_t *);
void array_set(struct array_t *, int, void *);
void array_add(struct array_t *, void *);
void array_add_val(struct array_t *, val_t);
void *array_pop(struct array_t *);
val_t array_pop_val(struct array_t *);
void string_buffer_append_s(string_buffer_t *buffer, const char *str);
void string_buffer_append_c(string_buffer_t *buffer, char c);
void string_buffer_append_i(string_buffer_t *buffer, int i);
void string_buffer_append_f(string_buffer_t *buffer, float f);
char *string_buffer_to_string(string_buffer_t *buffer);

typedef struct static_mtd_data {
	const char *name;
	int num_args;
	int num_args_minimum;
	int flag;
	int is_quote0;
	int is_quote1;
	val_t (*mtd)(val_t*, int);
	val_t (*special_mtd)(val_t*, struct array_t*);
} static_mtd_data;

opline_t *memory;
int inst_size;

typedef struct loop_frame_t {
	jmp_buf *buf;
	val_t block_name;
	cons_t *environment;
}loop_frame_t;

array_t *loop_frame_list;
void loop_frame_push(jmp_buf *buf, val_t block_name);
loop_frame_t *loop_frame_pop();
void throw_exception(const char *, int, const char *);
void throw_fmt_exception(const char *, int, const char *, va_list);
val_t call_lambda(val_t *, array_t *);
extern int current_index, next_index;
extern void** table;

extern static_mtd_data static_mtds[];
void init_opline();
void unuse_opline(int);
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
int val_length(val_t val);
/*parser.h*/
int parse_program(char *);
/*eval.h*/
val_t vm_exec (int, opline_t *, val_t *);
void set_args(val_t *, int, func_t *);
val_t exec_body(val_t *, int , func_t *);
val_t eval_inner(val_t *, val_t);

/* gc test */
val_t root;

extern const char *bootstrap_functions[];
#endif
