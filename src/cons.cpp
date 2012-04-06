#include <stdio.h>
#include "lisp.h"
void print_T(cons_t *cons) {
	printf("T\n");
}

void free_T(cons_t *cons) {

}

void print_nil(cons_t *cons) {
	printf("nil\n");
}

void free_nil(cons_t *cons) {

}

void print_i(cons_t *cons) {
	printf("%d\n", cons->ivalue);
}

void free_i(cons_t *cons) {

}

struct cons_api_t cons_T_api = {print_T, free_T};
struct cons_api_t cons_nil_api = {print_nil, free_nil};
struct cons_api_t cons_int_api = {print_i, free_i};

cons_t *new_int(int n) {
	cons_t *cons = new_cons_cell();
	cons->type = NUM;
	cons->ivalue = n;
	cons->api = &cons_int_api;
}
