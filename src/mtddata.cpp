#include <stdio.h>
#include "lisp.h"
void car(cons_t** vstack, cons_t** argstack) {

}

void cdr(cons_t** vstack, cons_t** argstack) {

}

void cons(cons_t** vstack, cons_t** argstack) {

}

void list(cons_t** vstack, cons_t** argstack) {

}

void add(cons_t** vstack, cons_t** argstack) {

}

void sub(cons_t** vstack, cons_t** argstack) {

}

void mul(cons_t** vstack, cons_t** argstack) {

}

void div(cons_t** vstack, cons_t** argstack) {

}

static_mtd_data static_mtds[] = {
	{"car", 1, car},
	{"cdr", 1, cdr},
	{"cons", -1, cons},
	{"list", -1, list},
	{"+", -1, add},
	{"-", -1, sub},
	{"*", -1, mul},
	{"/", -1, div},
	{NULL, 0, NULL}
};
