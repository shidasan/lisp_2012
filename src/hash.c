#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include"lisp.h"
/* list of variable_data table */
cons_t *current_environment = NULL;
array_t *environment_list = NULL;
/* function_data table */
func_t *func_data_table = NULL;

void new_func_data_table() {
	int i;
	func_data_table = (func_t*)malloc(sizeof(func_t) * HASH_SIZE);
	memset(func_data_table, 0, sizeof(func_t) * HASH_SIZE);
}

void mark_func_data_table(array_t *traced) {
	func_t *table = func_data_table;
	int i = 0;
	for (; i < HASH_SIZE; i++) {
		func_t *func = table + i;
		while (func != NULL) {
			ADDREF_NULLABLE(func->environment, traced);
			ADDREF_NULLABLE(func->args, traced);
			func = func->next;
		}
	}
}

void mark_variable_data_table(variable_t *table, array_t *traced) {
	printf("mark table: %p\n", table);
	int i = 0;
	for (; i < HASH_SIZE; i++) {
		variable_t *variable = table + i;
		while (variable != NULL) {
			ADDREF_NULLABLE(variable->cons, traced);
			variable = variable->next;
		}
	}
	printf("mark table end: %p\n", table);
}

void mark_environment_list(array_t *traced) {
	int i = 0;
	for (; i < array_size(environment_list); i++) {
		ADDREF((cons_t*)array_get(environment_list, i), traced);
	}
}

cons_t *change_local_scope(cons_t *old_environment, cons_t *environment) {
	cons_t *new_environment = new_local_environment();
	new_environment->car = new_variable_data_table();
	new_environment->cdr = environment;
	current_environment = new_environment;
	return old_environment;
}

cons_t *begin_local_scope(func_t *func) {
	cons_t *old_environment = current_environment;
	if (func->creates_local_scope) {
		current_environment = new_local_environment();
		current_environment->car = new_variable_data_table();
		if (current_environment->car == current_environment->car->car) {
			asm("int3");
		}
		current_environment->cdr = old_environment;
	}
	return old_environment;
}

cons_t *end_local_scope(cons_t *old_environment) {
	current_environment = old_environment;
	return NULL;
}

void new_global_environment() {
	current_environment = new_local_environment();
	current_environment->car = new_variable_data_table();
	environment_list = new_array();
}

void environment_list_push(cons_t *environment) {
	array_add(environment_list, environment);
}

cons_t *environment_list_pop(cons_t *environment) {
	return array_pop(environment_list);
}

static void free_variable_data_table(variable_t *table) {
	int i;
	variable_t *tempV, *currentV;
	for (i = 0;(unsigned int)i < HASH_SIZE; i++){
		FREE(table[i].name);
		currentV = table[i].next;
		while (1){
			if (currentV != NULL){
				tempV = currentV->next;
				FREE(currentV->name);
				FREE(currentV);
				currentV = tempV;
			} else {
				break;
			}
		}
	}
}

void free_func_data_table() {
	func_t *tempF, *currentF;
	int i;
	for (i = 0;(unsigned int)i < HASH_SIZE; i++){
		FREE(func_data_table[i].name);
		currentF = func_data_table[i].next;
		while (1){
			if (currentF != NULL){
				tempF = currentF->next;
				FREE(currentF->name);
				FREE(currentF);
				currentF = tempF;
			} else {
				break;
			}
		}
	}
}

struct cons_t* set_variable_inner (cons_t *table, cons_t *cons, cons_t *value, int is_end_of_table_list)
{
	const char *str = cons->str;
	variable_t* p = table->variable_data_table + ((str[0] * str[1]) % HASH_SIZE);
	while (1){
		if (p->name != NULL && strcmp(p->name,str) == 0){
			if (p->name == NULL) {
				p->name = (char*)malloc(strlen(str)+1);
			} else {
				FREE(p->name);
				p->name = (char*)malloc(strlen(str)+1);
			}
			strcpy (p->name, str);
			p->name[strlen(str)] = '\0';
			p->cons = value;
			return p->cons;
		} else if (p->next == NULL){
			if (is_end_of_table_list) {
				p->next = (variable_t*)malloc(sizeof(variable_t));
				memset(p->next, 0, sizeof(variable_t));
				if (p->name == NULL) {
					p->name = (char*)malloc(strlen(str)+1);
				}
				strcpy (p->name, str);
				p->name[strlen(str)] = '\0';
				p->cons = value;
				return p->cons;
			} else {
				return NULL;
			}
		} else {
			p = p->next;
		}
	}
}

struct cons_t *set_variable(cons_t *cons, cons_t *value, int set_local_scope) {
	cons_t *environment = current_environment;
	cons_t *table = environment->car;
	cons_t *res = NULL;
	fprintf(stderr, "set_variable, car->type: %d\n", table->type);
	while ((res = set_variable_inner(table, cons, value, set_local_scope || environment->cdr == NULL)) == NULL) {
	fprintf(stderr, "set_variable, car->type: %d\n", table->type);
		environment = environment->cdr;
		table = environment->car;
	}
	return res;
}

struct cons_t* search_variable_inner (cons_t *table, char* str)
{
	variable_t* p = table->variable_data_table + ((str[0] * str[1]) % HASH_SIZE);
	while (1){
		if (p->name != NULL && strcmp (p->name, str) == 0){
			return p->cons;
		} else if (p->next != NULL){
			p = p->next;
		} else {
			return NULL;
		}
	}
}

struct cons_t *search_variable(char *str) {
	cons_t *environment = current_environment;
	cons_t *table = environment->car;
	cons_t *res = NULL;
	while ((res = search_variable_inner(table, str)) == NULL) {
		if (environment->cdr == NULL) {
			return NULL;
		} else {
			environment = environment->cdr;
			table = environment->car;
		}
	}
	fprintf(stderr, "search_variable: %p type: %d\n", res, res->type);
	return res;
}

struct func_t* search_func (char* str)
{
	struct func_t* p = func_data_table + ((str[0] * str[1]) % HASH_SIZE);
	while (1){
		if (p->name != NULL && strcmp (p->name, str) == 0){
			return p;
		} else if (p->next != NULL){
			p = p->next;
		} else {
			return NULL;
		}
	}
}

struct func_t* set_static_func (const char* str,int i, void* adr, void *special_mtd, int is_static, int is_special_form, int *is_quote, int creates_local_scope)
{

	func_t* p = func_data_table + ((str[0] * str[1]) % HASH_SIZE);
	while (1){
		if (p->name == NULL || strcmp(p->name,str) == 0){
			if (p->name != NULL && strcmp(p->name, str) == 0 && p->is_static && !is_static) {
				return NULL;
			}
			if (p->name == NULL){
				p->name = (char*)malloc(strlen(str));
				strncpy (p->name, str, strlen(str));
				p->name[strlen(str)] = '\0';
			}
			p->value = i;
			p->is_static = is_static;
			p->is_quote = is_quote;
			p->creates_local_scope = creates_local_scope;
			p->is_special_form = is_special_form;
			if (is_special_form) {
				p->special_mtd = special_mtd;
			} else {
				p->mtd = adr;
			}
			return p;
		} else if (p->next == NULL){
			p->next = (func_t*)malloc(sizeof(func_t));
			memset(p->next, 0, sizeof(variable_t));
			p = p->next;
		} else {
			p = p->next;
		}
	}
}

struct func_t* set_func (cons_t *cons, struct array_t *opline_list, int argc, cons_t *args, cons_t *current_environment) {
	char *str = cons->str;
	func_t* p = func_data_table + ((str[0] * str[1]) % HASH_SIZE);
	while (1){
		if (p->name == NULL || strcmp(p->name,str) == 0){
			if (p->name != NULL && strcmp(p->name, str) == 0 && p->is_static) {
				return NULL;
			}
			if (p->name == NULL){
				p->name = (char*)malloc(strlen(str)+1);
				strncpy (p->name, str, strlen(str));
				p->name[strlen(str)] = '\0';
			}
			p->value = argc;
			p->is_static = 0;
			p->is_quote = 0;
			p->is_special_form = 0;
			p->opline_list = opline_list;
			p->args = args;
			p->environment = current_environment;
			return p;
		} else if (p->next == NULL){
			p->next = (func_t*)malloc(sizeof(func_t));
			memset(p->next, 0, sizeof(variable_t));
			p = p->next;
		} else {
			p = p->next;
		}
	}
}
