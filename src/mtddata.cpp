#include <stdio.h>
#include "lisp.h"
cons_t *car(cons_t** vstack, int ARGC) {

}

cons_t *cdr(cons_t** vstack, int ARGC) {

}

cons_t *cons(cons_t** vstack, int ARGC) {

}

cons_t *list(cons_t** vstack, int ARGC) {

}

cons_t *add(cons_t** vstack, int ARGC) {
	int i, res = 0;
	for (i = 0; i < ARGC; i++) {
		cons_t *cons = ARGS(vstack, i);
		if (cons->type != INT) {
			fprintf(stderr, "type error!!\n");
			asm("int3");
		}
		res += cons->ivalue;
	}
	return new_int(res);
}

cons_t *sub(cons_t** vstack, int ARGC) {
	int i, res = 0;
	for (i = 0; i < ARGC; i++) {
		cons_t *cons = ARGS(vstack, i);
		if (cons->type != INT) {
			fprintf(stderr, "type error!!\n");
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

cons_t *mul(cons_t** vstack, int ARGC) {
	int i, res = 1;
	for (i = 0; i < ARGC; i++) {
		cons_t *cons = ARGS(vstack, i);
		if (cons->type != INT) {
			fprintf(stderr, "type error!!\n");
			asm("int3");
		}
		res *= cons->ivalue;
	}
	return new_int(res);
}

cons_t *div(cons_t** vstack, int ARGC) {

}

cons_t *quote(cons_t ** vstack, int ARGC) {
	fprintf(stderr, "quote\n");
	cons_t *cons = ARGS(vstack, 0);
	fprintf(stderr, "argc, %d, 0: %p\n", ARGC, ARGS(vstack, 0));
	fprintf(stderr, "cons->type: %d\n", cons->type);
	return cons;
}

static_mtd_data static_mtds[] = {
	{"car", 1, 0, 0, car},
	{"cdr", 1, 0, 0, cdr},
	{"cons", -1, 0, 0, cons},
	{"list", -1, 0, 0, list},
	{"+", -1, 0, 0, add},
	{"-", -1, 0, 0, sub},
	{"*", -1, 0, 0, mul},
	{"/", -1, 0, 0, div},
	{"quote", 1, 1, 1, quote},
	{NULL, 0, 0, 0, NULL},
};
