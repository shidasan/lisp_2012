#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include"lisp.h"

struct cons_t* setV (cons_t *cons, cons_t *value/*const char* str,int LengthRatio*/)
{
	const char *str = cons->str;
    variable_t* p = &Variable_Data[(str[0] * str[1]) % (sizeof(Variable_Data) / sizeof(Variable_Data[0]))];
    while (1){
        if (p->name == NULL || strcmp(p->name,str) == 0){
			if (p->name == NULL) {
				p->name = (char*)malloc(strlen(str));
			}
            strcpy (p->name, str);
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

struct cons_t* searchV (char* str)
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

struct func_t* searchF (char* str)
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

struct func_t* setF (const char* str,int i, void* adr, void *special_mtd, int LengthRatio, int is_static, int is_special_form, int *is_quote)
{

    func_t* p = &Function_Data[(str[0] * str[1]) % (sizeof(Function_Data) / sizeof(Function_Data[0]))];
    while (1){
        if (p->name == NULL || strcmp(p->name,str) == 0){
			if (p->name != NULL && strcmp(p->name, str) == 0 && p->is_static && !is_static) {
				return NULL;
			}
            if (p->name == NULL){
                p->name = (char*)malloc(LengthRatio);
                strncpy (p->name, str, LengthRatio);
            }
            p->value = i;
			p->is_static = is_static;
			p->is_quote = is_quote;
			p->is_special_form = is_special_form;
			if (is_special_form) {
				p->adr = (opline_t*)special_mtd;
			} else {
				p->adr = (opline_t*)adr;
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
