#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>
#include"lisp.h"
#include"config.h"
#define STRLEN 50
opline_t memory[INSTSIZE];
int CurrentIndex, NextIndex;
char* strtmp;
char* str;
void** table;
static char *(*myreadline)(const char *);
static int (*myadd_history)(const char *);
static cons_t *stack_value[STACKSIZE];

static int add_history(const char *line) {
	return 0;
}

static void init_opline() {
	CurrentIndex = NextIndex;
}

static void init_opline_first() {
	CurrentIndex = NextIndex = 0;
}

static char *str_join(char *tmptmpstr, char *leftover) {
	char *tmpstr = (char*)malloc(sizeof(char) * (strlen(tmptmpstr) + strlen(leftover) + 1));
	memcpy(tmpstr, leftover, strlen(leftover));
	memcpy(tmpstr + strlen(leftover), tmptmpstr, sizeof(tmpstr));
	tmpstr[strlen(tmptmpstr) + strlen(leftover)] = '\0';
	return tmpstr;
}

void set_static_mtds() {
	int i = 0;
	while (static_mtds[i].mtd != NULL || static_mtds[i].special_mtd != NULL) {
		static_mtd_data *data = static_mtds + i;
		set_static_func(data->name, data->num_args, (void*)data->mtd, (void*)data->special_mtd, 1, data->is_special_form, &(data->is_quote0));
		i++;
	}
}

int get_split_point(char *src, int start) {
	int res = start, level = 0;
	while (src[res] != '\0') {
		if (src[res] == '(') {
			level++;
		}
		if (src[res] == ')') {
			level--;
			if (level == 0) {
				return res;
			}
		}
		res++;
	}
	if (level > 0) {
		return -1;
	}
	return strlen(src);
}
int is_unexpected_input(char *str) {
	return (strcmp(str, " ") != 0 && strcmp(str, "\n") != 0 && strcmp(str, "\0") != 0);
}
char *split_and_eval(int argc, char **args, char *tmpstr) {
	int prev_point = 0;
	int next_point = 0;
	char *leftover = NULL;
	while ((next_point = get_split_point(tmpstr, prev_point)) != -1) {
		init_opline();
		str = (char*)malloc(next_point - prev_point + 2);
		memcpy(str, tmpstr + prev_point, next_point - prev_point + 1);
		str[next_point - prev_point + 1] = '\0';
		prev_point = next_point + 1;
		//str = tmpstr;
		if (strncmp(str,"exit", 3) == 0){
			printf("bye\n");
			free(str);
			free(tmpstr);
			exit(0);
		}
		int status;
		if (strlen(str) > 0) {
			status = -1;
			if (strlen(str) > 0 && is_unexpected_input(str)) {
				status = parse_program(str);
			}
			if (status == 0){
				myadd_history(str);
				cons_t *cons = (cons_t*)vm_exec(argc + 1, memory + CurrentIndex, stack_value);
				if (cons != NULL) {
					if (cons->type == OPEN) {
						printf("(");
					}
					CONS_PRINT(cons);
					if (cons->type == OPEN) {
						printf(")");
					}
					printf("\n");
				}
			} else if (strcmp(str, "\n") == 0 || strcmp(str, "\0") == 0) {
				/* ignore */
			} else if (status == 1) {
				if (argc > 2 && strcmp(args[2], "--testing") == 0) {
					/* test failing */
					exit(1);
				}
				/* exit when error occers while reading FILE* */
				//argc = 1;
			}
		}
		free(str);
		if (next_point == strlen(tmpstr)) {
			break;
		}
	}
	/* copy leftover */
	if (next_point == -1) {
		leftover = (char*)malloc(strlen(tmpstr) - prev_point + 2);
		memcpy(leftover, tmpstr + prev_point ,strlen(tmpstr) - prev_point + 1);
		leftover[strlen(tmpstr) - prev_point] = ' ';
		leftover[strlen(tmpstr) - prev_point + 1] = '\0';
	}
	return leftover;
}

int main (int argc, char* args[])
{
	FILE* file = NULL;
	int StrSize = STRLEN;
	int StrIndex = 0;
	char *tmpstr = NULL, *leftover = NULL;
	table = (void**)vm_exec(1, NULL, NULL);
	new_func_data_table();
	variable_data_table_push();
	gc_init();
	if (argc > 1){
		file = fopen(args[1],"r");
	}
	init_opline_first();
	int i;
	set_static_mtds();
	void *handler = dlopen("libreadline" K_OSDLLEXT, RTLD_LAZY);
	void *f = (handler != NULL) ? dlsym(handler, "readline") : NULL;
	myreadline = (f != NULL) ? (char* (*)(const char*))f : NULL;
	f = (handler != NULL) ? dlsym(handler, "add_history") : NULL;
	myadd_history = (f != NULL) ? (int (*)(const char*))f : add_history;
	StrSize = STRLEN;
	StrIndex = 0;
	if (argc > 1){
		init_opline();
		tmpstr = (char*)malloc(StrSize);
		while (1) {
			if (StrIndex == StrSize - 1){
				StrSize *= 2;
				strtmp = (char*)malloc(StrSize);
				strncpy(strtmp, tmpstr, StrSize);
				free(tmpstr);
				tmpstr = strtmp;
			}
			if ((tmpstr[StrIndex] = fgetc(file)) == EOF){
				tmpstr[StrIndex] = '\0';
				fclose(file);
				break;
				//free(tmpstr);
				//exit(0);
			}
			if (tmpstr[StrIndex] == '\n') {
				tmpstr[StrIndex] = ' ';
			}
			//if (tmpstr[StrIndex] == '\n' || tmpstr[StrIndex] == '\0'){
			//	tmpstr[StrIndex] = '\n';
			//	tmpstr[StrIndex + 1] = '\0';
			//	break;
			//}
			StrIndex++;
		}
		split_and_eval(argc, args, tmpstr);
	} else {
		while (1) {
			init_opline();
			if (leftover != NULL) {
				char *tmptmpstr = myreadline("    ");
				tmpstr = str_join(tmptmpstr, leftover);
				free(leftover);
				leftover = NULL;
				free(tmptmpstr);
				tmptmpstr = NULL;
			} else {
				tmpstr = myreadline(">>> ");
			}
			leftover = split_and_eval(argc, args, tmpstr);
		}
	}
	free(tmpstr);
	return 0;
}

/*
void Clean (void)
{
}
*/
