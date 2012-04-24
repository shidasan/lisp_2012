#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include"lisp.h"

static int TempIndex;
static char* null = NULL;

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
	switch (ast->type) {
		case nil:
		case VARIABLE:
		case T:
		case INT:
		case OPEN:
			new_opline(PUSH, ast->cons);
			break;
		default:
			TODO("atom codegen\n");
	}
}

static void gen_variable(ast_t *ast) {
	new_opline(GET_VARIABLE, ast->cons);
}

static void gen_static_func(ast_t *ast, int list_length) {
	if (ast->type == ast_atom) {
		EXCEPTION("Not a function!!\n");
	}
	if (ast->type == ast_static_func) {
		new_opline(MTDCALL, ast->cons);
		memory[NextIndex-1].op[1].ivalue = list_length-1;
	} else if (ast->type == ast_func) {
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
	if (ast->type == ast_variable) {
		gen_variable(ast);
	}
	if (ast->type == ast_static_func || ast->type == ast_func) {
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

void init_opline() {
	CurrentIndex = NextIndex;
}

void codegen(ast_t *ast) {
	init_opline();
	gen_expression(ast, 0);
	new_opline(END, NULL);
}

static int cons_length(cons_t *cons) {
	if (cons->type == nil) {
		return 0;
	}
	if (cons->type != OPEN) {
		return -1;
	}
	int res = 0;
	while (cons->type == OPEN) {
		res++;
		cons = cons->cdr;
	}
	if (cons->type != nil) {
		return -1;
	}
	return res;
}

static void cons_atom(cons_t *cons) {
	new_opline(PUSH, cons);
}

static void cons_variable(cons_t *cons) {
	new_opline(GET_VARIABLE, cons);
}

static void cons_mtd_check(cons_t *cons, int list_length) {
	new_opline(MTDCHECK, cons);
	memory[NextIndex-1].op[1].ivalue = list_length-1;
}

static void cons_expression(cons_t *cons);

static void cons_func(cons_t *cons) {
	cons_t *car = cons->car;
	func_t *func = search_func(car->str);
	int i = 1, size = cons_length(cons);
	int *quote_position = NULL;
	if (func != NULL && FLAG_IS_STATIC(func->flag) && func->is_quote[0]) {
		quote_position = func->is_quote;
	}
	cons_mtd_check(car, size);
	cons_t *cdr = cons->cdr;
	for (; i < size; i++) {
		if (quote_position != NULL && (i == quote_position[0] || i == quote_position[1] || quote_position[0] == -1)) {
			new_opline(PUSH, cdr->car);
		} else {
			cons_expression(cdr->car);
		}
		cdr = cdr->cdr;
	}
	new_opline(MTDCALL, car);
	memory[NextIndex-1].op[1].ivalue = size-1;
}

void cons_codegen(cons_t *cons);

static void cons_special_form(cons_t *cons) {
	int i = 1;
	array_t *a = new_array();
	opline_t *pc = memory + NextIndex;
	new_opline_special_method(SPECIAL_MTD, cons->car, a);
	new_opline(END, NULL);
	func_t *func = search_func(cons->car->str);
	int *quote_position = NULL;
	if (func != NULL && FLAG_IS_STATIC(func->flag) && func->is_quote[0]) {
		quote_position = func->is_quote;
	}
	int length = cons_length(cons);
	cons_t *cdr = cons->cdr;
	for (; i < length; i++) {
		array_add(a, memory + NextIndex);
		if (quote_position != NULL && (i == quote_position[0] || i == quote_position[1] || quote_position[0] == -1)) {
			new_opline(PUSH, cdr->car);
		} else {
			cons_expression(cdr->car);
		}
		if (i != length-1) {
			new_opline(END, NULL);
		}
		cdr = cdr->cdr;
	}
}

static void cons_list (cons_t *cons) {
	func_t *func = search_func(cons->car->str);
	if (func != NULL && FLAG_IS_SPECIAL_FORM(func->flag)) {
		cons_special_form(cons);
	} else {
		cons_func(cons);
	}
}

static void cons_expression(cons_t *cons) {
	switch(cons->type) {
		case OPEN:
			cons_list(cons);
			break;
		case VARIABLE:
			cons_variable(cons);
			break;
		default:
			cons_atom(cons);
			break;
	}
}

void cons_codegen(cons_t *cons) {
	init_opline();
	cons_expression(cons);
	new_opline(END, NULL);
}
