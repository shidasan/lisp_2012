#ifndef MAIN
#define MAIN
#include "memory.h"
#include "config.h"
#define STACKSIZE 1000
#define INSTSIZE 100000
#define TODO(STR) fprintf(stderr, "TODO: "); fprintf(stderr, (STR));

#ifdef USE_DEBUG_MODE
#define DBG_P(STR) fprintf(stderr, "Debug: ");fprintf(stderr, STR);
#else
#define DBG_P(STR)
#endif

enum eINSTRUCTION { PUSH, PLUS, MINUS, MUL, DIV, GT, GTE, LT, LTE, EQ, PLUS2, MUNUS2, MUL2, DIV2, GT2, GTE2, LT2, LTE2, EQ2, END, JMP, GOTO, NGOTO, RETURN, NRETURN,  ARG, NARG, DEFUN, SETQ };
enum TokType {  tok_number, tok_plus, tok_minus, tok_mul, tok_div, tok_gt, tok_gte, tok_lt, tok_lte, tok_eq, tok_if, tok_defun, tok_str, tok_eof, tok_setq, tok_valiable, tok_func, tok_arg, tok_open, tok_close, tok_error, tok_nil, tok_T, tok_symbol};
enum eTYPE { nil = 0, T = 1, NUM = 2, LIST = 3, INT = 4, STRING = 5, FUNC = 6, VARIABLE = 7};
enum ast_type {ast_atom, ast_list, ast_list_close, ast_func, ast_variable};

void *array_get(struct array_t, size_t);
size_t array_size(struct array_t *);
void array_set(struct array_t *, size_t, void *);
void array_add(struct array_t *, void *);
void *array_pop(struct array_t *a);

typedef struct rbp_t {
	union {
		cons_t *dummy;
		int ivalue;
	};
}rbp_t;

typedef struct sfp_t {
	cons_t *cons;
	union {
		int ivalue;
	};
}sfp_t;

typedef struct static_mtd_data {
	const char *name;
	int num_args;
	void (*mtd)(cons_t**, cons_t**);
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

extern func_t Function_Data[1024];
extern variable_t Variable_Data[1024];
extern opline_t memory[INSTSIZE];
extern int CurrentIndex, NextIndex;
extern void** table;

extern static_mtd_data static_mtds[];
/*hash.h*/
struct func_t* setF (const char* str, int i , void* adr, int LengthRatio, int isStatic);
struct variable_t* setV (const char* str, int LengthRatio);
struct variable_t* searchV (char* str);
struct func_t* searchF (char* str);
/*generator.h*/
void GenerateProgram (AST*);
void codegen(ast_t *);
/*parser.h*/
int ParseProgram(char *);
int parse_program(char *);
/*eval.h*/
void** eval (int);

/* gc */
#endif /*MAIN*/
