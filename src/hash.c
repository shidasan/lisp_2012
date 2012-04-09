#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include"lisp.h"

struct cons_t* set_variable (cons_t *cons, cons_t *value/*const char* str,int LengthRatio*/)
{
	const char *str = cons->str;
	variable_t* p = &Variable_Data[(str[0] * str[1]) % (sizeof(Variable_Data) / sizeof(Variable_Data[0]))];
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
	struct variable_t* p = &Variable_Data[(str[0] * str[1]) % (sizeof(Variable_Data) / sizeof(Variable_Data[0]))];
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
	struct func_t* p = &Function_Data[(str[0] * str[1]) % (sizeof(Function_Data) / sizeof(Function_Data[0]))];
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

	func_t* p = &Function_Data[(str[0] * str[1]) % (sizeof(Function_Data) / sizeof(Function_Data[0]))];
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
	func_t* p = &Function_Data[(str[0] * str[1]) % (sizeof(Function_Data) / sizeof(Function_Data[0]))];
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
