#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lisp.h"
void print_T(cons_t *cons) {
	printf("T\n");
	if (cons->cdr != NULL) {
		CONS_PRINT(cons->cdr);
	}
}

void free_T(cons_t *cons) {

}

cons_t *eval_T(cons_t *cons) {
	return cons;
}

void print_nil(cons_t *cons) {
	printf("nil\n");
	if (cons->cdr != NULL) {
		CONS_PRINT(cons->cdr);
	}
}

void free_nil(cons_t *cons) {

}

cons_t *eval_nil(cons_t *cons) {
	return cons;
}

void print_i(cons_t *cons) {
	printf("%d\n", cons->ivalue);
	if (cons->cdr != NULL) {
		CONS_PRINT(cons->cdr);
	}
}

void free_i(cons_t *cons) {

}

cons_t *eval_i(cons_t *cons) {
	return cons;
}

void print_func(cons_t *cons) {
	TODO("print_func\n");
	if (cons->cdr != NULL) {
		CONS_PRINT(cons->cdr);
	}
}

void free_func(cons_t *cons) {
	free(cons->str);
}

cons_t *eval_func(cons_t *cons) {
	func_t *func = searchF(cons->str);
	//return func->mtd(NULL, NULL);
}

void print_variable(cons_t *cons) {
	TODO("print_variable\n");
	if (cons->cdr != NULL) {
		CONS_PRINT(cons->cdr);
	}
}

void free_variable(cons_t *cons) {
	free(cons->str);
}

cons_t *eval_variable(cons_t *cons) {

}

void print_open(cons_t *cons) {

}

void free_open(cons_t *cons) {

}

cons_t *eval_open(cons_t *cons) {
	CONS_EVAL(cons->car);
}

struct cons_api_t cons_T_api = {print_T, free_T, eval_T};
struct cons_api_t cons_nil_api = {print_nil, free_nil, eval_nil};
struct cons_api_t cons_int_api = {print_i, free_i, eval_i};
struct cons_api_t cons_func_api = {print_func, free_func, eval_func};
struct cons_api_t cons_variable_api = {print_variable, free_variable, eval_variable};
struct cons_api_t cons_open_api = {print_open, free_open, eval_open};

cons_t *new_int(int n) {
	cons_t *cons = new_cons_cell();
	cons->type = INT;
	cons->ivalue = n;
	cons->api = &cons_int_api;
	return cons;
}

cons_t *new_bool(int n) {
	cons_t *cons = new_cons_cell();
	cons->type = nil;
	cons->api = (n) ? &cons_T_api : &cons_nil_api;
	return cons;
}

cons_t *new_func(char *str) {
	cons_t *cons = new_cons_cell();
	cons->type = FUNC;
	cons->api = &cons_func_api;
	char *newstr = (char *)malloc(strlen(str)+1);
	memcpy(newstr, str, strlen(str)+1);
	newstr[strlen(str)] = '\0';
	cons->str = newstr;
	return cons;
}

cons_t *new_variable(char *str) {
	cons_t *cons = new_cons_cell();
	cons->type = VARIABLE;
	cons->api = &cons_variable_api;
	char *newstr = (char *)malloc(strlen(str)+1);
	memcpy(newstr, str, strlen(str)+1);
	newstr[strlen(str)] = '\0';
	cons->str = newstr;
	return cons;
}

cons_t *new_open() {

}
