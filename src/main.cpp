#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>
#include"lisp.h"
#include"config.h"
#define STRLEN 50
Function_Data_t Function_Data[1024];
Variable_Data_t Variable_Data[1024];
opline_t memory[INSTSIZE];
int CurrentIndex, NextIndex;
char* strtmp;
char* str;
void** table;
void Clean (void);
static char *(*myreadline)(const char *);
static int (*myadd_history)(const char *);

static int add_history(const char *line) {
	return 0;
}

static char* readline(const char* prompt)
{
	static int checkCTL = 0;
	int ch, pos = 0;
	static char linebuf[1024]; // THREAD-UNSAFE
	fputs(prompt, stdout);
	while((ch = fgetc(stdin)) != EOF) {
		if(ch == '\r') continue;
		if(ch == 27) {
			/* ^[[A */;
			fgetc(stdin); fgetc(stdin);
			if(checkCTL == 0) {
				fprintf(stdout, " - use readline, it provides better shell experience.\n");
				checkCTL = 1;
			}
			continue;
		}
		if(ch == '\n' || pos == sizeof(linebuf) - 1) {
			linebuf[pos] = 0;
			break;
		}
		linebuf[pos] = ch;
		pos++;
	}
	if(ch == EOF) return NULL;
	char *p = (char*)malloc(pos+1);
	memcpy(p, linebuf, pos+1);
	return p;
}

void set_static_mtds() {

}

int main (int argc, char* args[])
{
    FILE* file = NULL;
    int StrSize = STRLEN;
    int StrIndex = 0;
    table = eval(1);
   if (argc > 1){
        file = fopen(args[1],"r");
    }
    CurrentIndex = NextIndex = 0;
    int i;
    for (i = 0; i < (signed int)(sizeof(Function_Data)/sizeof(Function_Data[0])); i++) {
        Function_Data[i].name = NULL;
        Function_Data[i].next = NULL;
        Variable_Data[i].name = NULL;
        Variable_Data[i].next = NULL;
    }
	set_static_mtds();
	void *handler = dlopen("libreadline" K_OSDLLEXT, RTLD_LAZY);
	void *f = (handler != NULL) ? dlsym(handler, "readline") : NULL;
	myreadline = (f != NULL) ? (char* (*)(const char*))f : readline;
	f = (handler != NULL) ? dlsym(handler, "add_history") : NULL;
	myadd_history = (f != NULL) ? (int (*)(const char*))f : add_history;
	fprintf(stderr, "%p\n", handler);
    while (1){
        StrSize = STRLEN;
        StrIndex = 0;
        CurrentIndex = NextIndex;
        if (argc > 1){
        str = (char*)malloc(StrSize);
            while (1) {
                if ((str[StrIndex] = fgetc(file)) == EOF){
                    fclose(file);
                    free(str);
                    Clean();
                    exit(0);
                }
                if (StrIndex == StrSize - 1){
                    StrSize *= 2;
                    strtmp = (char*)malloc(StrSize);
                    strncpy(strtmp, str, StrSize);
                    free(str);
                    str = strtmp;
                }
                if (str[StrIndex] == '\n' || str[StrIndex] == '\0'){
                    str[StrIndex] = '\n';
                    str[StrIndex + 1] = '\0';
                    break;
                }
                StrIndex++;
            }

        } else {
			str = myreadline(">>> ");
        }
        if (strcmp(str,"bye\n") == 0){
            printf("bye\n");
            free(str);
            Clean();
            exit(0);
        }
        if (ParseProgram() == 0){
			myadd_history(str);
            eval(argc + 1);
        } else if (strcmp(str, "\n") == 0 || strcmp(str, "\0") == 0) {
			/* ignore */
		}else {
			if (argc > 2 && strcmp(args[2], "--testing") == 0) {
				/* test failing */
				exit(1);
			}
			/* exit when error occers while reading FILE* */
            //argc = 1;
        }
        free(str);
    }

}


void Clean (void)
{
    Function_Data_t *tempF, *currentF;
    Variable_Data_t *tempV, *currentV;
    int i;
    for (i = 0;(unsigned int)i < (sizeof(Function_Data) / sizeof(Function_Data[0])); i++){
        free(Function_Data[i].name);
        currentF = Function_Data[i].next;
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
    for (i = 0;(unsigned int)i < (sizeof(Variable_Data) / sizeof(Variable_Data[0])); i++){
        free(Variable_Data[i].name);
        currentV = Variable_Data[i].next;
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

