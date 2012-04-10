#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include"lisp.h"
/* list of variable_data table */
cons_t *variable_data_table = NULL;
/* function_data table */
func_t *func_data_table = NULL;

void new_func_data_table() {
	int i;
	func_data_table = (func_t*)malloc(sizeof(func_t) * HASH_SIZE);
	bzero(func_data_table, sizeof(func_t) * HASH_SIZE);
}

void variable_data_table_push() {

}

cons_t *variable_data_table_pop() {

}
void free_func_data_table() {
	func_t *tempF, *currentF;
	int i;
	for (i = 0;(unsigned int)i < HASH_SIZE; i++){
		free(func_data_table[i].name);
		currentF = func_data_table[i].next;
		while (1){
			if (currentF != NULL){
				tempF = currentF->next;
				free(currentF->name);
				free(currentF);
				currentF = tempF;
			} else {
				break;
			}
		}
	}
}
void free_variable_data_table(variable_t *table) {

}

struct cons_t* set_variable (cons_t *cons, cons_t *value/*const char* str,int LengthRatio*/)
{
	const char *str = cons->str;
	variable_t* p = variable_data_table->variable_data_table + ((str[0] * str[1]) % HASH_SIZE);
	while (1){
		if (p->name == NULL || strcmp(p->name,str) == 0){
			if (p->name == NULL) {
				p->name = (char*)malloc(strlen(str)+1);
			}
			strcpy (p->name, str);
			p->name[strlen(str)] = '\0';
			p->cons = value;
			return p->cons;
		} else if (p->next == NULL){
			p->next = (variable_t*)malloc(sizeof(variable_t));
			bzero(p->next, sizeof(variable_t));
			p = p->next;
		} else {
			p = p->next;
		}
	}
}

struct cons_t* search_variable (char* str)
{
	variable_t* p = variable_data_table->variable_data_table + ((str[0] * str[1]) % HASH_SIZE);
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

struct func_t* set_static_func (const char* str,int i, void* adr, void *special_mtd, int is_static, int is_special_form, int *is_quote)
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
			p->is_special_form = is_special_form;
			if (is_special_form) {
				p->special_mtd = special_mtd;
			} else {
				p->mtd = adr;
			}
			return p;
		} else if (p->next == NULL){
			p->next = (func_t*)malloc(sizeof(func_t));
			bzero(p->next, sizeof(variable_t));
			p = p->next;
		} else {
			p = p->next;
		}
	}
}

struct func_t* set_func (cons_t *cons, struct array_t *opline_list, int argc, cons_t *args) {
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
			return p;
		} else if (p->next == NULL){
			p->next = (func_t*)malloc(sizeof(func_t));
			bzero(p->next, sizeof(variable_t));
			p = p->next;
		} else {
			p = p->next;
		}
	}
}
