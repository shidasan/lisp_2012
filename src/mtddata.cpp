#include <stdio.h>
#include "lisp.h"
void lisp_car(cons_t* vstack, cons_t* argstack) {

}

void lisp_cdr(cons_t* vstack, cons_t* argstack) {

}

void lisp_cons(cons_t* vstack, cons_t* argstack) {

}

void lisp_list(cons_t* vstack, cons_t* argstack) {

}

static_mtd_data static_mtds[] = {
	{"car", 1, lisp_car},
	{"cdr", 1, lisp_cdr},
	{"cons", -1, lisp_cons},
	{"list", -1, lisp_list},
	{NULL, 0, NULL}
};
