#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include"lisp.h"

static void new_opline(enum eINSTRUCTION e, val_t val) {
	if (next_index == inst_size) {
		fprintf(stderr, "expand opline inst_size%d\n", inst_size);
		int newsize = inst_size * 2;
		opline_t *newmemory = (opline_t*)malloc(sizeof(opline_t) * newsize);
		memset(newmemory, 0, sizeof(opline_t) * newsize);
		memcpy(newmemory, memory, sizeof(opline_t) * inst_size);
		FREE(memory);
		memory = newmemory;
		inst_size = newsize;
	}
	memory[next_index].instruction = e;
	memory[next_index].instruction_ptr = table[memory[next_index].instruction];
	memory[next_index].op[0].val = val;
	next_index++;
}

static void new_opline_special_method(enum eINSTRUCTION e, val_t cons, struct array_t *a) {
	new_opline(e, cons);
	memory[next_index-1].op[1].a = a;
}

void init_opline() {
	current_index = next_index;
}

static int cons_length(val_t val) {
	if (IS_nil(val)) {
		return 0;
	}
	if (!IS_OPEN(val)) {
		return -1;
	}
	int res = 0;
	while (IS_OPEN(val)) {
		res++;
		val = val.ptr->cdr;
	}
	if (!IS_nil(val)) {
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

static void gen_expression(val_t);

static void gen_mtd_check(val_t val, int list_length) {
	new_opline(MTDCHECK, val);
	memory[next_index-1].op[1].ivalue = list_length-1;
}

static void gen_func(val_t val) {
	assert(!IS_UNBOX(val));
	val_t car = val.ptr->car;
	func_t *func = search_func(car.ptr->str);
	int i = 1, size = cons_length(val);
	int *quote_position = NULL;
	if (func != NULL && FLAG_IS_STATIC(func->flag) && func->is_quote[0]) {
		quote_position = func->is_quote;
	}
	val_t cdr = val.ptr->cdr;
	//if (func == NULL || func->value != -1 && func->value != size) {
	if (func == NULL) {
		gen_mtd_check(car, size);
	}
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
	memory[next_index-1].op[1].ivalue = size-1;
}

void codegen(val_t);

static void gen_special_form(val_t val) {
	int i = 1;
	assert(!IS_UNBOX(val));
	array_t *a = new_array();
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
		uintptr_t cast = (uintptr_t)next_index;
		array_add(a, (void*)cast);
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
	if (IS_UNBOX(car)) {
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
	if (IS_UNBOX(val)) {
		gen_atom(val);
	} else {
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
}

void codegen(val_t cons) {
	init_opline();
	gen_expression(cons);
	val_t tmp = {0};
	new_opline(END, tmp);
}
