#include <stdio.h>
#include "lisp.h"

static cons_t *eval_print(int ARGC, ...) {

}

static cons_t *print(cons_t **VSTACK, int ARGC) {
	cons_t *cons = ARGS(0);
	if (cons->type == OPEN) {
		printf("(");
	}
	CONS_PRINT(cons);
	if (cons->type == OPEN) {
		printf(")");
	}
	printf("\n");
	return cons;
}

static cons_t *eval_car(int ARGC, ...) {

}

static cons_t *car(cons_t** VSTACK, int ARGC) {
	cons_t *cons = ARGS(0);
	if (cons->type != OPEN) {
		fprintf(stderr, "expected list!!\n");
		TODO("exception\n");
	}
	return cons->car;
}

static cons_t *eval_cdr(int ARGC, ...) {

}

static cons_t *cdr(cons_t** VSTACK, int ARGC) {
	cons_t *cons = ARGS(0);
	if (cons->type != OPEN) {
		fprintf(stderr, "expected list!!\n");
		TODO("exception\n");
	}
	return cons->cdr;
}

static cons_t *eval_cons(int ARGC, ...) {

}

static cons_t *cons(cons_t** VSTACK, int ARGC) {
	cons_t *car = ARGS(0);
	cons_t *cdr = ARGS(1);
	cons_t *cons = new_open();
	cons->car = car;
	cons->cdr = cdr;
	return cons;
}

static cons_t *eval_add(int ARGC, ...) {

}

static cons_t *add(cons_t** VSTACK, int ARGC) {
	int i, res = 0;
	for (i = 0; i < ARGC; i++) {
		cons_t *cons = ARGS(i);
		if (cons->type != INT) {
			fprintf(stderr, "type error!!\n");
			TODO("exception\n");
		}
		res += cons->ivalue;
	}
	return new_int(res);
}

static cons_t *eval_sub(int ARGC, ...) {

}

static cons_t *sub(cons_t** VSTACK, int ARGC) {
	int i, res = 0;
	for (i = 0; i < ARGC; i++) {
		cons_t *cons = ARGS(i);
		if (cons->type != INT) {
			fprintf(stderr, "type error!!\n");
			TODO("exception\n");
			asm("int3");
		}
		if (i == 0) {
			res = cons->ivalue;
		} else {
			res -= cons->ivalue;
		}
	}
	return new_int(res);
}

static cons_t *eval_mul(int ARGC, ...) {

}

static cons_t *mul(cons_t** VSTACK, int ARGC) {
	int i, res = 1;
	for (i = 0; i < ARGC; i++) {
		cons_t *cons = ARGS(i);
		if (cons->type != INT) {
			fprintf(stderr, "type error!!\n");
			TODO("exception\n");
		}
		res *= cons->ivalue;
	}
	return new_int(res);
}

static cons_t *eval_div(int ARGC, ...) {

}

static cons_t *div(cons_t** VSTACK, int ARGC) {

}

static cons_t *eval_lt(int ARGC, ...) {

}

static cons_t *lt(cons_t** VSTACK, int ARGC) {
	int i;
	int current_number = ARGS(0)->ivalue;
	for (i = 1; i < ARGC; i++) {
		cons_t *cons = ARGS(i);
		if (cons->type != INT) {
			fprintf(stderr, "type error!\n");
			TODO("exception\n");
		}
		int next_number = cons->ivalue;
		if (current_number >= next_number) {
			return new_bool(0);
		}
		current_number = next_number;
	}
	return new_bool(1);
}

static cons_t *eval_gt(int ARGC, ...) {

}

static cons_t *gt(cons_t** VSTACK, int ARGC) {
	int i;
	int current_number = ARGS(0)->ivalue;
	for (i = 1; i < ARGC; i++) {
		cons_t *cons = ARGS(i);
		if (cons->type != INT) {
			fprintf(stderr, "type error!\n");
			TODO("exception\n");
		}
		int next_number = cons->ivalue;
		if (current_number <= next_number) {
			return new_bool(0);
		}
		current_number = next_number;
	}
	return new_bool(1);
}

static cons_t *eval_eq(int ARGC, ...) {

}

static cons_t *eq(cons_t** VSTACK, int ARGC) {
	int i;
	int current_number = ARGS(0)->ivalue;
	for (i = 1; i < ARGC; i++) {
		cons_t *cons = ARGS(i);
		if (cons->type != INT) {
			fprintf(stderr, "type error!\n");
			TODO("exception\n");
		}
		int next_number = cons->ivalue;
		if (current_number != next_number) {
			return new_bool(0);
		}
		current_number = next_number;
	}
	return new_bool(1);
}

static cons_t *eval_zerop(int ARGC, ...) {

}

static cons_t *zerop(cons_t **VSTACK, int ARGC) {
	cons_t *cons = ARGS(0);
	if (cons->type == INT && cons->ivalue == 0) {
		return new_bool(1);
	}
	return new_bool(0);
}

static cons_t *null(cons_t **VSTACK, int ARGC) {
	return new_bool(ARGS(0)->type == nil);
}

static cons_t *not(cons_t **VSTACK, int ARGC) {
	return new_bool(ARGS(0)->type == nil);
}

static cons_t *atom(cons_t **VSTACK, int ARGC) {
	cons_t *cons = ARGS(0);
	return new_bool((cons->type != OPEN));
}
static cons_t *eval_quote(int ARGC, ...) {

}

static cons_t *quote(cons_t ** VSTACK, int ARGC) {
	cons_t *cons = ARGS(0);
	return cons;
}

static cons_t *eval_list(int ARGC, ...) {

}

static cons_t *list(cons_t ** VSTACK, int ARGC) {
	int i;
	if (ARGC == 0) {
		return new_bool(0);
	}
	cons_t *res = new_open();
	cstack_cons_cell_push(res);
	cons_t *tmp = res;
	for (i = 0; i < ARGC; i++) {
		tmp->car = ARGS(i);
		if (i == ARGC - 1) {
			tmp->cdr = new_bool(0);
		} else {
			tmp->cdr = new_open();
			tmp = tmp->cdr;
		}
	}
	cstack_cons_cell_pop();
	return res;
}

static cons_t *eval_length(int ARGC, ...) {

}

static cons_t *length(cons_t **VSTACK, int ARGC) {
	cons_t *cons = ARGS(0);
	if (cons->type == nil) {
		return new_int(0);
	}
	if (cons->type != OPEN) {
		EXCEPTION("Not a list!!\n");
	}
	int res = 0;
	while (cons->type == OPEN) {
		res++;
		cons = cons->cdr;
	}
	if (cons->type != nil) {
		EXCEPTION("Not a list!!\n");
	}
	return new_int(res);
}

static cons_t *eval_if(int ARGC, ...) {

}

static cons_t *_if(cons_t **VSTACK, int ARGC, struct array_t *a) {
	cons_t *cons = vm_exec(2, (opline_t*)array_get(a, 0), VSTACK);
	cons_t *res = NULL;
	if (cons->type != nil) {
		res = vm_exec(2, (opline_t*)array_get(a, 1), VSTACK);
	} else {
		res = vm_exec(2, (opline_t*)array_get(a, 2), VSTACK);
	}
	return res;
}

static cons_t *cond(cons_t **VSTACK, int ARGC, array_t *a) {
	int size = array_size(a), i = 0;
	if (size == 0) {
		return new_bool(0);
	}
	cons_t *res = NULL;
	for (; i < size; i++) {
		cons_t *cons = vm_exec(2, (opline_t*)array_get(a, i), VSTACK);
		VSTACK[1] = cons;
		int _length = length(VSTACK+1, 1)->ivalue;
		if (_length == 0) {
			EXCEPTION("clause NIL should be a list");
		}
		cons_codegen(cons->car);
		cons_t *res = vm_exec(2, memory+CurrentIndex, VSTACK);
		if (res->type == nil) {
			continue;
		}
		int j = 1;
		cons_t *cdr = cons->cdr;
		cons_t *car = cdr->car;
		for (; j < _length; j++) {
			cons_codegen(car);
			res = vm_exec(2, memory+CurrentIndex, VSTACK);
			cdr = cdr->cdr;
			car = cdr->car;
		}
		return res;
	}
	return new_bool(0);
}

static cons_t *progn(cons_t **VSTACK, int ARGC, array_t *a) {
	int size = array_size(a), i = 0;
	cons_t *res = NULL;
	for (; i < size; i++) {
		res = vm_exec(2, (opline_t*)array_get(a, i), VSTACK);
	}
	return res;
}

static cons_t *when(cons_t **VSTACK, int ARGC, array_t *a) {
	int size = array_size(a);
	if (size == 0) {
		EXCEPTION("argument length does not match!!\n");
	}
	cons_t *res = vm_exec(2, (opline_t*)array_get(a, 0), VSTACK);
	if (res->type == nil || size == 1) {
		return new_bool(0);
	}
	int i = 1;
	for (; i < size; i++) {
		res = vm_exec(2, (opline_t*)array_get(a, i), VSTACK);
	}
	return res;
}

static cons_t *unless(cons_t **VSTACK, int ARGC, array_t *a) {
	int size = array_size(a);
	if (size == 0) {
		EXCEPTION("argument length does not match!!\n");
	}
	cons_t *res = vm_exec(2, (opline_t*)array_get(a, 0), VSTACK);
	if (res->type == T || size == 1) {
		return new_bool(0);
	}
	int i = 1;
	for (; i < size; i++) {
		res = vm_exec(2, (opline_t*)array_get(a, i), VSTACK);
	}
	return res;
}

static cons_t *eval_defun(int ARGC, ...) {

}

static cons_t *defun(cons_t **VSTACK, int ARGC, struct array_t *a) {
	cons_t *fcons = vm_exec(2, (opline_t*)array_get(a, 0), VSTACK);
	cons_t *args = vm_exec(2, (opline_t*)array_get(a, 1), VSTACK);
	int i = 2;
	struct array_t *opline_list = new_array();
	for (; i < array_size(a); i++) {
		array_add(opline_list, array_get(a, i));
	}
	VSTACK[1] = args;
	int argc = length(VSTACK+1, 1)->ivalue;
	set_func(fcons, opline_list, argc, args, current_environment);
	return fcons;
}

static cons_t *eval_setq(int ARGC, ...) {

}

static cons_t *setq(cons_t **VSTACK, int ARGC) {
	cons_t *variable = ARGS(0);
	cons_t *value = ARGS(1);
	if (variable->type != VARIABLE || value->type == VARIABLE || value->type == FUNC) {
		EXCEPTION("Not a atom!!\n");
	}
	set_variable(variable, value, 0);
	return value;
}

static cons_t *eval_let(int ARGC, ...) {

}

static cons_t *let(cons_t **VSTACK, int ARGC, struct array_t *a) {
	cons_t *value_list = vm_exec(2, (opline_t*)array_get(a, 0), VSTACK);
	cons_t *variable = NULL;
	cons_t *list = NULL;
	cons_t *value = NULL;
	if (value_list->type == OPEN) {
		list = value_list->car;
		while (list != NULL && list->type != nil) {
			if (list->type == OPEN) {
				VSTACK[1] = list;
				int argc = length(VSTACK+2, 1)->ivalue;
				if (argc == 2) {
					variable = list->car;
					value = list->cdr->car;
					set_variable(variable, value, 1);
				} else {
					fprintf(stderr, "illegal variable specification!! %d\n", argc);
					asm("int3");
				}
			} else {
				cons_t *p = set_variable(list, new_bool(0), 1);
			}
			value_list = value_list->cdr;
			list = value_list->car;
		}
	}
	cons_t *res = NULL;
	int i;
	for (i = 1; i < array_size(a); i++) {
		res = vm_exec(2, (opline_t*)array_get(a, i), VSTACK + i);
	}
	return res;
}

static cons_t *eval_eval(int ARGC, ...) {

}

static cons_t *eval(cons_t **VSTACK, int ARGC) {
	//cons_t *cons = CONS_EVAL(ARGS(0));
	cons_t *cons = ARGS(0);
	cons_codegen(cons);
	cons_t *res = vm_exec(2, memory + CurrentIndex, VSTACK + 1);
	return res;
}

/*
typedef struct static_mtd_data {
	const char *name;
	int num_args;
	int creates_local_scope;
	int is_special_form;
	int is_quote0;
	int is_quote1;
	cons_t *(*mtd)(cons_t**, int);
	cons_t *(*special_mtd)(cons_t**, int, struct array_t*);
} static_mtd_data;
*/

static_mtd_data static_mtds[] = {
	{"print", 1, 0, 0, 0, 0, print, NULL, eval_print},
	{"car", 1, 0, 0, 0, 0, car, NULL, eval_car},
	{"cdr", 1, 0, 0, 0, 0, cdr, NULL, eval_cdr},
	{"cons", 2, 0, 0, 0, 0, cons, NULL, eval_cons},
	{"+", -1, 0, 0, 0, 0, add, NULL, eval_add},
	{"-", -1, 0, 0, 0, 0, sub, NULL, eval_sub},
	{"*", -1, 0, 0, 0, 0, mul, NULL, eval_mul},
	{"/", -1, 0, 0, 0, 0, div, NULL, eval_div},
	{"<", -1, 0, 0, 0, 0, lt, NULL, eval_lt},
	{">", -1, 0, 0, 0, 0, gt, NULL, eval_gt},
	{"=", -1, 0, 0, 0, 0, eq, NULL, eval_eq},
	{"zerop", 1, 0, 0, 0, 0, zerop, NULL, eval_zerop},
	{"null", 1, 0, 0, 0, 0, null, NULL, NULL},
	{"not", 1, 0, 0, 0, 0, not, NULL, NULL},
	{"atom", 1, 0, 0, 0, 0, atom, NULL, NULL},
	{"quote", 1, 0, 0, 1, 1, quote, NULL, eval_quote},
	{"list", -1, 0, 0, 0, 0, list, NULL, eval_list},
	{"length", 1, 0, 0, 0, 0, length, NULL, eval_length},
	{"if", 3, 0, 1, 0, 0, NULL, _if, eval_if},
	{"cond", -1, 0, 1, -1, 0, NULL, cond, NULL},
	{"progn", -1, 0, 1, 0, 0, NULL, progn, NULL},
	{"when", -1, 0, 1, 0, 0, NULL, when, NULL},
	{"unless", -1, 0, 1, 0, 0, NULL, unless, NULL},
	{"defun", -1, 1, 1, 1, 2, NULL, defun, eval_defun},
	{"setq", 2, 0, 0, 1, 0, setq, NULL, eval_setq},
	{"let", -1, 1, 1, 1, 0, NULL, let, eval_let},
	{"eval", 1, 0, 0, 0, 0, eval, NULL, eval_eval},
	{NULL, 0, 0, 0, 0, 0, NULL, NULL},
};
