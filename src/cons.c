#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lisp.h"
static void print_T(cons_t *cons) {
	printf("T");
}

static void free_T(cons_t *cons) {

}

static cons_t *eval_T(cons_t *cons) {
	return cons;
}

static void print_nil(cons_t *cons) {
	printf("nil");
}

static void free_nil(cons_t *cons) {

}

static cons_t *eval_nil(cons_t *cons) {
	return cons;
}

static void print_i(cons_t *cons) {
	printf("%d", cons->ivalue);
}

static void free_i(cons_t *cons) {

}

static cons_t *eval_i(cons_t *cons) {
	return cons;
}

static void print_func(cons_t *cons) {
	printf("%s", cons->str);
}

static void free_func(cons_t *cons) {
	free(cons->str);
}

static cons_t *eval_func(cons_t *cons) {
	func_t *func = search_func(cons->str);
}

static void print_variable(cons_t *cons) {
	printf("%s", cons->str);
}

static void free_variable(cons_t *cons) {
	free(cons->str);
}

static cons_t *eval_variable(cons_t *cons) {

}

static void print_open(cons_t *cons) {
	cons_t *car = cons->car;
	cons_t *cdr = cons->cdr;
	if (car->type == OPEN) {
		printf("(");
	}
	CONS_PRINT(car);
	if (car->type == OPEN) {
		printf(")");
	}
	if (cdr->type == OPEN) {
		if (cdr->type != nil) {
			printf(" ");
			if (cdr->car->type == OPEN) {
				printf("(");
			}
			CONS_PRINT(cons->cdr);
			if (cdr->car->type == OPEN) {
				printf(")");
			}
		}
	} else {
		if (cdr->type != nil) {
			printf(" . ");
			if (cdr->type == OPEN) {
				printf("(");
			}
			CONS_PRINT(cons->cdr);
			if (cdr->type == OPEN) {
				printf(")");
			}
		}
	}
}

static void free_open(cons_t *cons) {

}

static cons_t *eval_open(cons_t *cons) {
	CONS_EVAL(cons->car);
}

static void print_variable_table(cons_t *cons) {
	TODO("print variable table\n");
}

static void free_variable_table(cons_t *cons) {
	int i;
	variable_t *table = cons->variable_data_table;
	variable_t *tempV, *currentV;
	for (i = 0;(unsigned int)i < HASH_SIZE; i++){
		free(table[i].name);
		currentV = table[i].next;
		while (1){
			if (currentV != NULL){
				tempV = currentV->next;
				free(currentV->name);
				free(currentV);
				currentV = tempV;
			} else {
				break;
			}
		}
	}
}

static cons_t *eval_variable_table(cons_t *cons) {
	/* do nothing */
}

struct cons_api_t cons_T_api = {print_T, free_T, eval_T};
struct cons_api_t cons_nil_api = {print_nil, free_nil, eval_nil};
struct cons_api_t cons_int_api = {print_i, free_i, eval_i};
struct cons_api_t cons_func_api = {print_func, free_func, eval_func};
struct cons_api_t cons_variable_api = {print_variable, free_variable, eval_variable};
struct cons_api_t cons_open_api = {print_open, free_open, eval_open};
struct cons_api_t cons_variable_table_api = {print_variable_table, free_variable_table, eval_variable_table};

cons_t *new_int(int n) {
	cons_t *cons = new_cons_cell();
	cons->type = INT;
	cons->ivalue = n;
	cons->api = &cons_int_api;
	return cons;
}

cons_t *new_bool(int n) {
	cons_t *cons = new_cons_cell();
	cons->type = (n) ? T : nil;
	cons->api = (n) ? &cons_T_api : &cons_nil_api;
	return cons;
}

cons_t *new_func(const char *str) {
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
	cons_t *cons = new_cons_cell();
	cons->type = OPEN;
	cons->api = &cons_open_api;
	return cons;
}
