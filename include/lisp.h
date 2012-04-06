#ifndef MAIN
#define MAIN
#include "gc.h"
#define STACKSIZE 1000
#define INSTSIZE 100000
enum eINSTRUCTION { PUSH, PLUS, MINUS, MUL, DIV, GT, GTE, LT, LTE, EQ, PLUS2, MUNUS2, MUL2, DIV2, GT2, GTE2, LT2, LTE2, EQ2, END, JMP, GOTO, NGOTO, RETURN, NRETURN,  ARG, NARG, DEFUN, SETQ };
enum TokType {  tok_number, tok_plus, tok_minus, tok_mul, tok_div, tok_gt, tok_gte, tok_lt, tok_lte, tok_eq, tok_if, tok_defun, tok_str, tok_eof, tok_setq, tok_valiable, tok_func, tok_arg, tok_open, tok_close, tok_error, tok_nil, tok_T};
enum eTYPE { T = 0, nil = 1, NUM = 2, LIST = 3};
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

typedef struct Variable_Data_t{
    char* name;
    struct Variable_Data_t* next;
    int value;
}Variable_Data_t;

typedef struct Function_Data_t{
    char* name;
    struct Function_Data_t* next;
    int value; // size of argument (?)
	int isStatic; // cleared by bzero
    opline_t* adr;
}Function_Data_t;

extern Function_Data_t Function_Data[1024];
extern Variable_Data_t Variable_Data[1024];
extern opline_t memory[INSTSIZE];
extern int CurrentIndex, NextIndex;
extern char* str;
extern void** table;

extern static_mtd_data static_mtds[];
/*hash.h*/
struct Function_Data_t* setF (const char* str, int i , void* adr, int LengthRatio, int isStatic);
struct Variable_Data_t* setV (const char* str, int LengthRatio);
struct Variable_Data_t* searchV (char* str);
struct Function_Data_t* searchF (char* str);
/*generator.h*/
void GenerateProgram (AST*);
/*parser.h*/
int ParseProgram();
/*eval.h*/
void** eval (int);

/* gc */
#endif /*MAIN*/
