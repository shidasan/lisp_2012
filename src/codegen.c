#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include"lisp.h"

void Generate (AST* ast, int i, char* str);
int TempIndex;
char* null = NULL;


void GenerateOperation(AST* ast, int i)
{
    Generate(ast->LHS, i, null);
    if (ast->RHS == NULL){

    } else if (ast->RHS->type == tok_number){
        //printf("push\noperation\n");
        memory[NextIndex].instruction = ast->type + 9;
        memory[NextIndex].op[0].ivalue = ast->RHS->i;
        memory[NextIndex].instruction_ptr = table[memory[NextIndex].instruction];
        NextIndex++;
        free(ast->RHS);
    } else {
        Generate(ast->RHS, i, null);
        //printf("operation\n");
        memory[NextIndex].instruction = ast->type;
        memory[NextIndex].instruction_ptr = table[memory[NextIndex].instruction];
        NextIndex++;
    }
}

void GenerateNumber (AST* ast)
{
    memory[NextIndex].instruction = PUSH;
    memory[NextIndex].op[0].ivalue = ast->i;
    memory[NextIndex].instruction_ptr = table[memory[NextIndex].instruction];
    NextIndex++;
}

void GenerateIf (AST* ast, int i, char* str)
{
    func_t* p;
    Generate(ast->COND, i, str);
    TempIndex = NextIndex;
    //printf("jmp\n");
    memory[NextIndex].instruction = JMP;
    memory[NextIndex].op[0].adr = &memory[NextIndex + 1];
    memory[NextIndex].instruction_ptr = table[memory[NextIndex].instruction];
    NextIndex++;
    Generate(ast->LHS , i, str);
    //printf("return or end\n");
    if (i != 1){
        memory[NextIndex].instruction = END;
    } else {
        p = searchF(str);
        if (p->value <= 1){
            memory[NextIndex].instruction = RETURN;
        } else {
            memory[NextIndex].instruction = NRETURN;
            memory[NextIndex].op[0].ivalue = p->value;
        }
    }
    memory[NextIndex].instruction_ptr = table[memory[NextIndex].instruction];
    NextIndex++;
    memory[TempIndex].op[1].adr = &memory[NextIndex];
    Generate(ast->RHS, i, str);
}

void GenerateSetq (AST* ast,int i)
{
    variable_t* p;
    p = searchV (ast->s);
    free(ast->s);
    Generate (ast->LHS, i, null);
    //printf("setq\n");
    memory[NextIndex].instruction = SETQ;
    memory[NextIndex].instruction_ptr = table[memory[NextIndex].instruction];
    memory[NextIndex].op[0].adr = (opline_t*)p;
    NextIndex++;
}

void GenerateVariable (AST* ast)
{
    //printf("variable\n");
    memory[NextIndex].instruction = PUSH;
    memory[NextIndex].op[0].ivalue = searchV(ast->s)->value;
    free(ast->s);
    memory[NextIndex].instruction_ptr = table[memory[NextIndex].instruction];
    NextIndex++;
}

void GenerateDefun (AST* ast)
{
    memory[NextIndex].instruction = DEFUN;
    memory[NextIndex].instruction_ptr = table[memory[NextIndex].instruction];
    NextIndex++;
    func_t* p = setF (ast->s, ast->LHS->i, &memory[NextIndex], 0, 0, 0, 0, 0);
    //printf("%s/n",p->name);
    memory[NextIndex - 1].op[1].svalue = p->name;
    free(ast->LHS);
    Generate (ast->RHS, 1,ast->s );
    free(ast->s);
    //printf("return\n");
    if (p->value <= 1){
        memory[NextIndex].instruction = RETURN;
        memory[NextIndex].instruction_ptr = table[memory[NextIndex].instruction];
    } else {
        memory[NextIndex].instruction = NRETURN;
        memory[NextIndex].op[0].ivalue = p->value;
        memory[NextIndex].instruction_ptr = table[memory[NextIndex].instruction];
    }
    NextIndex++;
}

void GenerateArg (AST* ast)
{
    if (ast->i == 0){
        memory[NextIndex].instruction = ARG;
        memory[NextIndex].instruction_ptr = table[memory[NextIndex].instruction];
        NextIndex++;
    } else {
        memory[NextIndex].instruction = NARG;
        memory[NextIndex].instruction_ptr = table[memory[NextIndex].instruction];
        memory[NextIndex].op[0].ivalue = ast->i + 1;
        NextIndex++;
    }
}

void GenerateFunc (AST* ast, int i)
{
    AST* temp = ast;
    AST* temp1 = NULL;
    func_t* p = searchF(ast->s);
    free(ast->s);
    int count = p->value;
    while (1){
        if (count == 0){
            memory[NextIndex].instruction = PUSH;
            memory[NextIndex].op[0].ivalue = 0;
            memory[NextIndex].instruction_ptr = table[memory[NextIndex].instruction];
            NextIndex++;
            //printf("goto\n");
            memory[NextIndex].instruction = GOTO;
            memory[NextIndex].op[0].adr = p->adr;
            memory[NextIndex].instruction_ptr = table[memory[NextIndex].instruction];
            NextIndex++;
            break;
        } else if (count == 1){
            Generate(temp->RHS, i, null);
            //printf("goto\n");
            memory[NextIndex].instruction = GOTO;
            memory[NextIndex].op[0].adr = p->adr;
            memory[NextIndex].op[1].ivalue = p->value;
            memory[NextIndex].instruction_ptr = table[memory[NextIndex].instruction];
            NextIndex++;
            break;
        } else if (count == 2){
            Generate(temp->LHS, i, null);
            Generate(temp->RHS, i, null);
            memory[NextIndex].instruction = NGOTO;
            memory[NextIndex].op[0].adr = p->adr;
            memory[NextIndex].op[1].ivalue = p->value;
            memory[NextIndex].instruction_ptr = table[memory[NextIndex].instruction];
            NextIndex++;
            break;
        } else {
            Generate(temp->LHS, i, null);
            temp1 = temp;
            temp = temp->RHS;
            if (temp != ast)
                free(temp);
            count--;
        }
    }

}

void Generate (AST* ast, int i,char* str)
{
    switch(ast->type){
        case tok_plus:case tok_mul:case tok_minus:case tok_div:
        case tok_gt:case tok_gte:case tok_lt:case tok_lte:
        case tok_eq:
            GenerateOperation(ast, i);
            break;

        case tok_number:
            GenerateNumber(ast);
            break;

        case tok_if:
            GenerateIf (ast, i, str);
            break;

        case tok_setq:
            GenerateSetq (ast, i);
            break;

        case tok_valiable:
            GenerateVariable (ast);
            break;

        case tok_defun:
            GenerateDefun (ast);
            break;

        case tok_arg:
            GenerateArg (ast);
            break;
        case tok_func:
            GenerateFunc (ast, i);
            break;

        default:
            printf("default\n");
            break;
    }
    free(ast);
}

void GenerateProgram (AST* ast)
{
    Generate(ast, 0, null);
    //printf("return or end\n");
    memory[NextIndex].instruction = END;
    memory[NextIndex].instruction_ptr = table[memory[NextIndex].instruction];
    NextIndex++;
}

static void new_opline(enum eINSTRUCTION e, cons_t *cons) {
	memory[NextIndex].instruction = e;
	memory[NextIndex].instruction_ptr = table[memory[NextIndex].instruction];
	memory[NextIndex].op[0].cons = cons;
	NextIndex++;
}

static void new_opline_special_method(enum eINSTRUCTION e, cons_t *cons, struct array_t *a) {
	new_opline(e, cons);
	memory[NextIndex-1].op[1].a = a;
}

static void gen_atom(ast_t *ast) {
	if (ast->sub_type == nil) {
		new_opline(PUSH, ast->cons);
	}
	if (ast->sub_type == T) {
		new_opline(PUSH, ast->cons);
	}
	if (ast->sub_type == INT) {
		new_opline(PUSH, ast->cons);
	}
	if (ast->sub_type == OPEN) {
		new_opline(PUSH, ast->cons);
	}
}

static void gen_static_func(ast_t *ast, int list_length) {
	if (ast->type == ast_atom) {
		EXCEPTION("Not a function!!\n");
	}
	if (ast->type == ast_static_func) {
		new_opline(MTDCALL, ast->cons);
		memory[NextIndex-1].op[1].ivalue = list_length-1;
	}
}

static void gen_mtd_check(ast_t *ast, int list_length) {
	new_opline(MTDCHECK, ast->cons);
	memory[NextIndex-1].op[1].ivalue = list_length-1;
}

static void gen_expression(ast_t *ast, int list_length);

static void gen_list(ast_t *ast) {
	int i = 0;
	for (; i < array_size(ast->a); i++) {
		ast_t *child_ast = (ast_t *)array_get(ast->a, i);
		gen_expression(child_ast, array_size(ast->a));
	}
	gen_static_func((ast_t *)array_get(ast->a, 0), array_size(ast->a));
}

static void gen_special_form(ast_t *ast) {
	int i = 1;
	struct array_t *a = new_array();
	opline_t *pc = memory + NextIndex;
	new_opline_special_method(SPECIAL_MTD, ((ast_t *)array_get(ast->a, 0))->cons, a);
	new_opline(END, NULL);
	for (; i < array_size(ast->a); i++) {
		array_add(a, memory + NextIndex);
		ast_t *child_ast = (ast_t *)array_get(ast->a, i);
		gen_expression(child_ast, array_size(ast->a));
		if (i != array_size(ast->a)-1) {
			new_opline(END, NULL);
		}
	}
}

static void gen_expression(ast_t *ast, int list_length) {
	if (ast->type == ast_atom) {
		gen_atom(ast);
	}
	if (ast->type == ast_static_func) {
		gen_mtd_check(ast, list_length);
	}
	if (ast->type == ast_list) {
		ast_t *ast0 = (ast_t *)array_get(ast->a, 0);
		if (ast0->type == ast_special_form) {
			gen_special_form(ast);
		} else {
			gen_list(ast);
		}
	}
}

void codegen(ast_t *ast) {
	gen_expression(ast, 0);
	new_opline(END, NULL);
}