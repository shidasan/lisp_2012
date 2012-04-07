#ifndef MAIN
#define MAIN
#include "memory.h"
#define STACKSIZE 1000
#define INSTSIZE 100000
#define TODO(STR) fprintf(stderr, "TODO: "); fprintf(stderr, (STR));
enum eINSTRUCTION { PUSH, PLUS, MINUS, MUL, DIV, GT, GTE, LT, LTE, EQ, PLUS2, MUNUS2, MUL2, DIV2, GT2, GTE2, LT2, LTE2, EQ2, END, JMP, GOTO, NGOTO, RETURN, NRETURN,  ARG, NARG, DEFUN, SETQ };
enum TokType {  tok_number, tok_plus, tok_minus, tok_mul, tok_div, tok_gt, tok_gte, tok_lt, tok_lte, tok_eq, tok_if, tok_defun, tok_str, tok_eof, tok_setq, tok_valiable, tok_func, tok_arg, tok_open, tok_close, tok_error, tok_nil, tok_T, tok_symbol};
enum eTYPE { nil = 0, T = 1, NUM = 2, LIST = 3, FUNC = 4, VARIABLE = 5};
enum ast_type {ast_atom, ast_list, ast_list_close, ast_symbol};

void *array_get(struct array_t, size_t);
size_t array_size(struct array_t *);
void array_set(struct array_t *, size_t, void *);
void array_add(struct array_t *, void *);
void *array_pop(struct array_t *a);

typedef struct opline_t{
    int instruction;
    void* instruction_ptr;
    union{
        int ivalue;
        char* svalue;
        struct opline_t* adr;
    }op[2];
}opline_t;

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
	void (*mtd)(cons_t*, cons_t*);
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
/*eval.h*/
void** eval (int);

/* gc */
#endif /*MAIN*/
