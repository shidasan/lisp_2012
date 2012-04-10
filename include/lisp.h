#ifndef MAIN
#define MAIN
#include "memory.h"
#include "config.h"
#define HASH_SIZE 64
#define STACKSIZE 1000
#define INSTSIZE 100000
#define ARGC _argc
#define VSTACK _vstack
#define ARGS(N) (VSTACK)[(N) - (ARGC)]
#define TODO(STR) fprintf(stderr, "TODO (%s, %d): ", __FILE__, __LINE__); fprintf(stderr, (STR));
#define EXCEPTION(STR) fprintf(stderr, "Exception!! (%s, %d): ", __FILE__, __LINE__);fprintf(stderr, (STR));

#ifdef USE_DEBUG_MODE
#define DBG_P(STR) fprintf(stderr, "Debug: ");fprintf(stderr, STR);
#else
#define DBG_P(STR)
#endif
/* PUSH, END, JMP, GOTO, RETURN, CALL */
enum eINSTRUCTION { PUSH, PLUS, MINUS, MUL, DIV, GT, GTE, LT, LTE, EQ, PLUS2, MUNUS2, MUL2, DIV2, GT2, GTE2, LT2, LTE2, EQ2, END, JMP, GOTO, NGOTO, RETURN, NRETURN,  ARG, NARG, DEFUN, SETQ, MTDCALL, MTDCHECK, SPECIAL_MTD, VARIABLE_PUSH};
enum TokType {  tok_number, tok_plus, tok_minus, tok_mul, tok_div, tok_gt, tok_gte, tok_lt, tok_lte, tok_eq, tok_if, tok_defun, tok_str, tok_eof, tok_setq, tok_valiable, tok_func, tok_arg, tok_open, tok_close, tok_error, tok_nil, tok_T, tok_symbol, tok_dot, tok_quote};
enum eTYPE { nil = 0, T = 1, NUM = 2, OPEN = 3, INT = 4, STRING = 5, FUNC = 6, VARIABLE = 7, VARIABLE_TABLE};
enum ast_type {ast_atom, ast_list, ast_list_close, ast_static_func, ast_quote, ast_func, ast_variable, ast_special_form};

void *array_get(struct array_t*, size_t);
size_t array_size(struct array_t *);
void array_set(struct array_t *, size_t, void *);
void array_add(struct array_t *, void *);
void *array_pop(struct array_t *);

typedef struct static_mtd_data {
	const char *name;
	int num_args;
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
/*hash.h*/
void new_func_data_table();
void variable_data_table_push();
cons_t *variable_data_table_pop();
void free_variable_data_table(variable_t *);
struct func_t* set_static_func (const char* str, int i , void* adr, void* special_mtd, int isStatic, int is_special_form, int *is_quote);
struct func_t* set_func(cons_t *cons, struct array_t *opline_list, int argc, cons_t *args);
struct cons_t* set_variable (cons_t *cons, cons_t *value, int set_local_scope);
struct cons_t* search_variable (char* str);
struct func_t* search_func (char* str);
/*generator.h*/
void GenerateProgram (AST*);
void codegen(ast_t *);
/*parser.h*/
int ParseProgram(char *);
int parse_program(char *);
/*eval.h*/
cons_t* vm_exec (int, opline_t *, cons_t **);

/* gc */
#endif /*MAIN*/
