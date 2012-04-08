#include <stdio.h>
#include "lisp.h"

static cons_t *car(cons_t** vstack, int ARGC) {
	cons_t *cons = ARGS(vstack, 0);
	if (cons->type != OPEN) {
		fprintf(stderr, "excepted list!!\n");
		TODO("exception\n");
	}
	return cons->car;
}

static cons_t *cdr(cons_t** vstack, int ARGC) {
	cons_t *cons = ARGS(vstack, 0);
	if (cons->type != OPEN) {
		fprintf(stderr, "excepted list!!\n");
		TODO("exception\n");
	}
	fprintf(stderr, "cdr type: %d, new car: %p\n", cons->type, cons->cdr->car);
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

static cons_t *quote(cons_t ** vstack, int ARGC) {
	fprintf(stderr, "quote\n");
	cons_t *cons = ARGS(vstack, 0);
	fprintf(stderr, "argc, %d, 0: %p\n", ARGC, ARGS(vstack, 0));
	fprintf(stderr, "cons->type: %d\n", cons->type);
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

static_mtd_data static_mtds[] = {
	{"car", 1, 0, 0, car},
	{"cdr", 1, 0, 0, cdr},
	{"cons", 2, 0, 0, cons},
	{"+", -1, 0, 0, add},
	{"-", -1, 0, 0, sub},
	{"*", -1, 0, 0, mul},
	{"/", -1, 0, 0, div},
	{"quote", 1, 1, 1, quote},
	{"list", -1, 0, 0, list},
	{NULL, 0, 0, 0, NULL},
};
