#include <stdio.h>
#include "lisp.h"

static cons_t *car(cons_t** vstack, int ARGC) {
	cons_t *cons = ARGS(vstack, 0);
	if (cons->type != OPEN) {
		fprintf(stderr, "expected list!!\n");
		TODO("exception\n");
	}
	return cons->car;
}

static cons_t *cdr(cons_t** vstack, int ARGC) {
	cons_t *cons = ARGS(vstack, 0);
	if (cons->type != OPEN) {
		fprintf(stderr, "expected list!!\n");
		TODO("exception\n");
	}
	return cons->cdr;
}

static cons_t *cons(cons_t** vstack, int ARGC) {
	cons_t *car = ARGS(vstack, 0);
	cons_t *cdr = ARGS(vstack, 1);
	cons_t *cons = new_open();
	cons->car = car;
	cons->cdr = cdr;
	return cons;
}

static cons_t *add(cons_t** vstack, int ARGC) {
	int i, res = 0;
	for (i = 0; i < ARGC; i++) {
		cons_t *cons = ARGS(vstack, i);
		if (cons->type != INT) {
			fprintf(stderr, "type error!!\n");
			TODO("exception\n");
		}
		res += cons->ivalue;
	}
	return new_int(res);
}

static cons_t *sub(cons_t** vstack, int ARGC) {
	int i, res = 0;
	for (i = 0; i < ARGC; i++) {
		cons_t *cons = ARGS(vstack, i);
		if (cons->type != INT) {
			fprintf(stderr, "type error!!\n");
			TODO("exception\n");
		}
		if (i == 0) {
			res = cons->ivalue;
		} else {
			res -= cons->ivalue;
		}
	}
	return new_int(res);
}

static cons_t *mul(cons_t** vstack, int ARGC) {
	int i, res = 1;
	for (i = 0; i < ARGC; i++) {
		cons_t *cons = ARGS(vstack, i);
		if (cons->type != INT) {
			fprintf(stderr, "type error!!\n");
			TODO("exception\n");
		}
		res *= cons->ivalue;
	}
	return new_int(res);
}

static cons_t *div(cons_t** vstack, int ARGC) {

}

static cons_t *lt(cons_t** vstack, int ARGC) {
	int i;
	int current_number = ARGS(vstack, 0)->ivalue;
	for (i = 1; i < ARGC; i++) {
		cons_t *cons = ARGS(vstack, i);
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

static cons_t *gt(cons_t** vstack, int ARGC) {
	int i;
	int current_number = ARGS(vstack, 0)->ivalue;
	for (i = 1; i < ARGC; i++) {
		cons_t *cons = ARGS(vstack, i);
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

static cons_t *quote(cons_t ** vstack, int ARGC) {
	cons_t *cons = ARGS(vstack, 0);
	return cons;
}

static cons_t *list(cons_t ** vstack, int ARGC) {
	int i;
	if (ARGC == 0) {
		return new_bool(0);
	}
	cons_t *res = new_open();
	cons_t *tmp = res;
	for (i = 0; i < ARGC; i++) {
		tmp->car = ARGS(vstack, i);
		if (i == ARGC - 1) {
			tmp->cdr = new_bool(0);
		} else {
			tmp->cdr = new_open();
			tmp = tmp->cdr;
		}
	}
	return res;
}

static cons_t *length(cons_t **vstack, int ARGC) {
	cons_t *cons = ARGS(vstack, 0);
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

static cons_t *_if(cons_t **vstack, int ARGC, struct array_t *a) {
	fprintf(stderr, "pc[0]: %p\n", array_get(a, 0));
	fprintf(stderr, "pc[1]: %p\n", array_get(a, 1));
	fprintf(stderr, "pc[2]: %p\n", array_get(a, 2));
	cons_t *cons = vm_exec(2, (opline_t*)array_get(a, 0), vstack);
	cons_t *res = NULL;
	fprintf(stderr, "cons_type: %d\n", cons->type);
	if (cons->type != nil) {
		res = vm_exec(2, (opline_t*)array_get(a, 1), vstack);
	} else {
		res = vm_exec(2, (opline_t*)array_get(a, 2), vstack);
	}
	//fprintf(stderr, "res %p\n", res);
	return res;
}

static_mtd_data static_mtds[] = {
	{"car", 1, 0, 0, car, NULL},
	{"cdr", 1, 0, 0, cdr, NULL},
	{"cons", 2, 0, 0, cons, NULL},
	{"+", -1, 0, 0, add, NULL},
	{"-", -1, 0, 0, sub, NULL},
	{"*", -1, 0, 0, mul, NULL},
	{"/", -1, 0, 0, div, NULL},
	{"<", -1, 0, 0, lt, NULL},
	{">", -1, 0, 0, gt, NULL},
	{"quote", 1, 0, 1, quote, NULL},
	{"list", -1, 0, 0, list, NULL},
	{"length", 1, 0, 0, length, NULL},
	{"if", 3, 1, 0, NULL, _if},
	{NULL, 0, 0, 0, NULL, NULL},
};
