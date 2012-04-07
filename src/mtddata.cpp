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
	TODO("implement add method\n");
}

void sub(cons_t** vstack, cons_t** argstack) {

}

void mul(cons_t** vstack, cons_t** argstack) {

}

void div(cons_t** vstack, cons_t** argstack) {

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
	{NULL, 0, 0, 0, NULL}
};
