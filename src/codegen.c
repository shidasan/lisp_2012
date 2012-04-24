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

void init_opline() {
	CurrentIndex = NextIndex;
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

static void gen_atom(cons_t *cons) {
	new_opline(PUSH, cons);
}

static void gen_variable(cons_t *cons) {
	new_opline(GET_VARIABLE, cons);
}

static void gen_mtd_check(cons_t *cons, int list_length) {
	new_opline(MTDCHECK, cons);
	memory[NextIndex-1].op[1].ivalue = list_length-1;
}

static void gen_expression(cons_t *cons);

static void gen_func(cons_t *cons) {
	cons_t *car = cons->car;
	func_t *func = search_func(car->str);
	int i = 1, size = cons_length(cons);
	int *quote_position = NULL;
	if (func != NULL && FLAG_IS_STATIC(func->flag) && func->is_quote[0]) {
		quote_position = func->is_quote;
	}
	gen_mtd_check(car, size);
	cons_t *cdr = cons->cdr;
	for (; i < size; i++) {
		if (quote_position != NULL && (i == quote_position[0] || i == quote_position[1] || quote_position[0] == -1)) {
			new_opline(PUSH, cdr->car);
		} else {
			gen_expression(cdr->car);
		}
		cdr = cdr->cdr;
	}
	new_opline(MTDCALL, car);
	memory[NextIndex-1].op[1].ivalue = size-1;
}

void codegen(cons_t *cons);

static void gen_special_form(cons_t *cons) {
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
			gen_expression(cdr->car);
		}
		if (i != length-1) {
			new_opline(END, NULL);
		}
		cdr = cdr->cdr;
	}
}

static void gen_list (cons_t *cons) {
	func_t *func = search_func(cons->car->str);
	if (func != NULL && FLAG_IS_SPECIAL_FORM(func->flag)) {
		gen_special_form(cons);
	} else {
		gen_func(cons);
	}
}

static void gen_expression(cons_t *cons) {
	switch(cons->type) {
		case OPEN:
			gen_list(cons);
			break;
		case VARIABLE:
			gen_variable(cons);
			break;
		default:
			gen_atom(cons);
			break;
	}
}

void codegen(cons_t *cons) {
	init_opline();
	gen_expression(cons);
	new_opline(END, NULL);
}
