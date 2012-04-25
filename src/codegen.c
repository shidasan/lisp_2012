#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include"lisp.h"

static int TempIndex;
static char* null = NULL;

static void new_opline(enum eINSTRUCTION e, val_t val) {
	memory[NextIndex].instruction = e;
	memory[NextIndex].instruction_ptr = table[memory[NextIndex].instruction];
	memory[NextIndex].op[0].val = val;
	NextIndex++;
}

static void new_opline_special_method(enum eINSTRUCTION e, val_t cons, struct array_t *a) {
	new_opline(e, cons);
	memory[NextIndex-1].op[1].a = a;
}

void init_opline() {
	CurrentIndex = NextIndex;
}

static int cons_length(val_t val) {
	if (unlikely(IS_NUMBER(val)) || val.ptr->type != OPEN) {
		return -1;
	}
	if (val.ptr->type == nil) {
		return 0;
	}
	int res = 0;
	while (IS_NUMBER(val) && val.ptr->type == OPEN) {
		res++;
		val = val.ptr->cdr;
	}
	if (IS_NUMBER(val) || val.ptr->type != nil) {
		return -1;
	}
	return res;
}

static void gen_atom(val_t val) {
	new_opline(PUSH, val);
}

static void gen_variable(val_t val) {
	new_opline(GET_VARIABLE, val);
}

static void gen_mtd_check(val_t val, int list_length) {
	assert(!IS_NUMBER(val));
	new_opline(MTDCHECK, val);
	memory[NextIndex-1].op[1].ivalue = list_length-1;
}

static void gen_expression(val_t);

static void gen_func(val_t val) {
	assert(!IS_NUMBER(val));
	val_t car = val.ptr->car;
	func_t *func = search_func(car.ptr->str);
	int i = 1, size = cons_length(val);
	int *quote_position = NULL;
	if (func != NULL && FLAG_IS_STATIC(func->flag) && func->is_quote[0]) {
		quote_position = func->is_quote;
	}
	gen_mtd_check(car, size);
	val_t cdr = val.ptr->cdr;
	for (; i < size; i++) {
		if (quote_position != NULL && (i == quote_position[0] || i == quote_position[1] || quote_position[0] == -1)) {
			new_opline(PUSH, cdr.ptr->car);
		} else if (func != NULL && FLAG_IS_MACRO(func->flag)) {
			new_opline(PUSH, cdr.ptr->car);
		} else {
			gen_expression(cdr.ptr->car);
		}
		cdr = cdr.ptr->cdr;
	}
	new_opline(MTDCALL, car);
	memory[NextIndex-1].op[1].ivalue = size-1;
}

void codegen(val_t);

static void gen_special_form(val_t val) {
	int i = 1;
	assert(!IS_NUMBER(val));
	array_t *a = new_array();
	opline_t *pc = memory + NextIndex;
	new_opline_special_method(SPECIAL_MTD, val.ptr->car, a);
	val_t tmp = {0};
	new_opline(END, tmp);
	func_t *func = search_func(val.ptr->car.ptr->str);
	int *quote_position = NULL;
	if (func != NULL && FLAG_IS_STATIC(func->flag) && func->is_quote[0]) {
		quote_position = func->is_quote;
	}
	int length = cons_length(val);
	val_t cdr = val.ptr->cdr;
	for (; i < length; i++) {
		array_add(a, memory + NextIndex);
		if (quote_position != NULL && (i == quote_position[0] || i == quote_position[1] || quote_position[0] == -1)) {
			new_opline(PUSH, cdr.ptr->car);
		} else {
			gen_expression(cdr.ptr->car);
		}
		if (i != length-1) {
			new_opline(END, tmp);
		}
		cdr = cdr.ptr->cdr;
	}
}

static void gen_list (val_t cons) {
	val_t car = cons.ptr->car;
	if (IS_NUMBER(car)) {
		EXCEPTION("Excepted symbol!!\n");
	}
	func_t *func = search_func(car.ptr->str);
	if (func != NULL && FLAG_IS_SPECIAL_FORM(func->flag)) {
		gen_special_form(cons);
	} else {
		gen_func(cons);
	}
}

static void gen_expression(val_t val) {
	if (IS_NUMBER(val)) {
		gen_atom(val);
	}
	switch(val.ptr->type) {
		case OPEN:
			gen_list(val);
			break;
		case VARIABLE:
			gen_variable(val);
			break;
		default:
			gen_atom(val);
			break;
	}
}

void codegen(val_t cons) {
	init_opline();
	gen_expression(cons);
	val_t tmp = {0};
	new_opline(END, tmp);
}
