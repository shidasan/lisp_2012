#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lisp.h"

void val_to_string(val_t val, string_buffer_t *buffer) {
	switch(VAL_TYPE(val)) {
	case INT_OFFSET:
		string_buffer_append_i(buffer, val.ivalue);
		break;
	case T_OFFSET:
		string_buffer_append_s(buffer, "T");
		break;
	case nil_OFFSET:
		string_buffer_append_s(buffer, "nil");
		break;
	default:
		CONS_TO_STRING(val.ptr->api->print(val.ptr));
		break;
	}
}
static void print_T(cons_t *cons, string_buffer_t *buffer) {
	string_buffer_append_s(buffer, "T");
}

static void free_T(cons_t *cons) {

}

static cons_t *eval_T(cons_t *cons) {
	return cons;
}

static void trace_T(cons_t *cons, struct array_t *traced) {
	//ADDREF(cons, traced);
}

static void print_nil(cons_t *cons, string_buffer_t *buffer) {
	string_buffer_append_s(buffer, "nil");
}

static void free_nil(cons_t *cons) {

}

static cons_t *eval_nil(cons_t *cons) {
	return cons;
}

static void trace_nil(cons_t *cons, struct array_t *traced) {
	//ADDREF(cons, traced);
}

static void print_i(cons_t *cons, string_buffer_t *buffer) {
	string_buffer_append_i(buffer, cons->ivalue);
}

static void free_i(cons_t *cons) {
}

static cons_t *eval_i(cons_t *cons) {
	return cons;
}

static void trace_i(cons_t *cons, struct array_t *traced) {
	//ADDREF(cons, traced);
}

static void print_func(cons_t *cons, string_buffer_t *buffer) {
	string_buffer_append_s(buffer, cons->str);
}

static void free_func(cons_t *cons) {
	FREE(cons->str);
}

static cons_t *eval_func(cons_t *cons) {
	return cons;
}

static void trace_func(cons_t *cons, struct array_t *traced) {
	//ADDREF(cons, traced);
	//ADDREF(cons->car, traced);
	ADDREF_NULLABLE(cons->cdr, traced);
}

static void print_variable(cons_t *cons, string_buffer_t *buffer) {
	string_buffer_append_s(buffer, cons->str);
}

static void free_variable(cons_t *cons) {
	FREE(cons->str);
}

static cons_t *eval_variable(cons_t *cons) {
	return search_variable(cons->str);
}

static void trace_variable(cons_t *cons, struct array_t *traced) {
	//ADDREF(cons, traced);
}

static void print_open(cons_t *cons, string_buffer_t *buffer) {
	cons_t *car = cons->car;
	cons_t *cdr = cons->cdr;
	if (car->type == OPEN) {
		string_buffer_append_s(buffer, "(");
	}
	CONS_TO_STRING(car, buffer);
	if (car->type == OPEN) {
		string_buffer_append_s(buffer, ")");
	}
	if (cdr->type == OPEN) {
		if (cdr->cdr->type != nil && cdr->cdr->type != OPEN) {
			string_buffer_append_s(buffer, " . ");
		} else {
			string_buffer_append_s(buffer, " ");
		}
		if (cdr->cdr->type != nil && cdr->cdr->type != OPEN) {
			string_buffer_append_s(buffer, "(");
		}
		CONS_TO_STRING(cons->cdr, buffer);
		if (cdr->cdr->type != nil && cdr->cdr->type != OPEN) {
			string_buffer_append_s(buffer, ")");
		}
	} else {
		if (cdr->type != nil) {
			string_buffer_append_s(buffer, " . ");
			if (cdr->type == OPEN) {
				string_buffer_append_s(buffer, "(");
			}
			CONS_TO_STRING(cons->cdr, buffer);
			if (cdr->type == OPEN) {
				string_buffer_append_s(buffer, ")");
			}
		}
	}
}

static void free_open(cons_t *cons) {

}

static cons_t *eval_open(cons_t *cons) {
	TODO("eval list\n");
	cons_t *car = CONS_EVAL(cons->car);
	func_t *func = search_func(car->str);
}

static void trace_open(cons_t *cons, struct array_t *traced) {
	ADDREF(cons->car, traced);
	ADDREF_NULLABLE(cons->cdr, traced);
}

static void print_variable_table(cons_t *cons, string_buffer_t *buffer) {
	TODO("print variable table\n");
}

static void free_variable_table(cons_t *cons) {
	int i;
	variable_t *table = cons->variable_data_table;
	variable_t *tempV, *currentV;
	//fprintf(stderr, "free_variable_table %p\n", table);
	for (i = 0;i < HASH_SIZE; i++){
		currentV = table + i;
		FREE(table[i].name);
		//currentV = table[i].next;
		while (currentV != NULL){
			if (currentV->name != NULL) {
				tempV = currentV->next;
				FREE(currentV->name);
				if (currentV > table + HASH_SIZE) {
					FREE(currentV);
				}
				currentV = tempV;
				//fprintf(stderr, "next %p\n", tempV);
			} else {
				break;
			}
		}
	}
	//memset(cons->variable_data_table, 0, sizeof(variable_t) * HASH_SIZE);
	FREE(cons->variable_data_table);
}

static cons_t *eval_variable_table(cons_t *cons) {
	/* do nothing */
}

static void trace_variable_table(cons_t *cons, struct array_t *traced) {
	//fprintf(stderr, "mark variable table%p\n", cons);
	//ADDREF(cons, traced);
	mark_variable_data_table(cons->variable_data_table, traced);
}

static void print_local_environment(cons_t *cons, string_buffer_t *buffer) {
	TODO("print local environment\n");
}

static void free_local_environment(cons_t *cons) {
	
}

static cons_t *eval_local_environment(cons_t *cons) {

}

static void trace_local_environment(cons_t *cons, struct array_t *traced) {
	//fprintf(stderr, "mark local_environment %p\n", cons);
	//fprintf(stderr, "mark cons->car %p\n", cons->car);
	ADDREF(cons, traced);
	ADDREF_NULLABLE(cons->cdr, traced);
	ADDREF(cons->car, traced);
}

static void print_string(cons_t *cons, string_buffer_t *buffer) {
	printf("%s", cons->str);
}

static void free_string(cons_t *cons) {
	FREE(cons->str);
}

static cons_t *eval_string(cons_t *cons) {

}

static void trace_string(cons_t *cons, array_t *traced) {

}

struct cons_api_t cons_T_api = {print_T, free_T, eval_T, trace_T};
struct cons_api_t cons_nil_api = {print_nil, free_nil, eval_nil, trace_nil};
struct cons_api_t cons_int_api = {print_i, free_i, eval_i, trace_i};
struct cons_api_t cons_func_api = {print_func, free_func, eval_func, trace_func};
struct cons_api_t cons_variable_api = {print_variable, free_variable, eval_variable, trace_variable};
struct cons_api_t cons_open_api = {print_open, free_open, eval_open, trace_open};
struct cons_api_t cons_variable_table_api = {print_variable_table, free_variable_table, eval_variable_table, trace_variable_table};
struct cons_api_t cons_local_environment_api = {print_local_environment, free_local_environment, eval_local_environment, trace_local_environment};
struct cons_api_t cons_string_api = {print_string, free_string, eval_string, trace_string};

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

cons_t *new_func(const char *str, cons_t *environment) {
	cons_t *cons = new_cons_cell();
	cons->type = FUNC;
	cons->api = &cons_func_api;
	char *newstr = (char *)malloc(strlen(str)+1);
	memcpy(newstr, str, strlen(str)+1);
	newstr[strlen(str)] = '\0';
	cons->str = newstr;
	cons->local_environment = environment;
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

cons_t *new_variable_data_table() {
	cons_t *cons = new_cons_cell();
	cons->type = VARIABLE_TABLE;
	cons->api = &cons_variable_table_api;
	cons->variable_data_table = (variable_t*)malloc(sizeof(variable_t) * HASH_SIZE);
	memset(cons->variable_data_table, 0, sizeof(variable_t) * HASH_SIZE);
	if (cons->car == cons->car->car) {
		asm("int3");
	}
	return cons;
}

/* its car type must variable_data_table */
cons_t *new_local_environment() {
	cons_t *cons = new_cons_cell();
	cons->type = LOCAL_ENVIRONMENT;
	cons->api = &cons_local_environment_api;
	return cons;
}

cons_t *new_string(const char *str) {
	cons_t *cons = new_cons_cell();
	cons->type = STRING;
	cons->api = &cons_string_api;
	int len = strlen(str);
	cons->str = (char*)malloc(len+1);
	memcpy(cons->str, str, len);
	cons->str[len] = '\0';
	return cons;
}
