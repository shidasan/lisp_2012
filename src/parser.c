#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "lisp.h"
#define ERROR /*printf("Error In Tokenizer\n");*/ return tok_error;
#define ARGERROR if (ArgCount[ArgIndex] > p->value)printf("Too many "); else printf("Too few "); printf("arguments given to function\n");  return NULL;
#define PERROR FreeAST(ret);return NULL;
#define MSG(STR) fprintf(stderr, "Parse Error !! : ");fprintf(stderr, (STR))
#define AR 3
#define LR 10
AST* ParseExpression (void);
AST* ParseBlock (void);
void FreeAST (AST*);
static int token_type;
static char* current_char;
static char* token_str;
static int token_int;
static int ArgIndex;
static unsigned int LengthRatio;
static int ArgsRatio = AR;
static char** Args;

int is_open_space_close(char c) {
	return c == '(' || c == ' ' || c == ')';
}

int GetTok (void)
{
    char* TokTemp = NULL;
    int ALT = 1;
    token_int = 0;
    unsigned int TokSize = 0;
    while (isspace(*current_char) || *current_char == ','){
        current_char++;
    }
    if (*current_char == '\0' || *current_char == '\n'){
        return tok_eof;
    }
	if (*current_char == '\"') {
		current_char++;
		int len = 0;
		while (*current_char != '\"') {
			token_str[TokSize] = *current_char;
			TokSize++;
			current_char++;
			if (TokSize >= LengthRatio - 1) {
                LengthRatio *= 2;
                TokTemp = (char*)calloc(LengthRatio,sizeof(char));
                strncpy(TokTemp,token_str,LengthRatio);
                free(token_str);
                token_str = TokTemp;
			}
		}
		token_str[TokSize] = '\0';
		current_char++;
		return tok_string;
	}
    if (isdigit(*current_char) || (*current_char == '-' && isdigit(*(current_char + 1)))){
        if (*current_char == '-'){
            ALT = -1;
            current_char++;
        }
        while (isdigit(*current_char)){
            token_int = 10 * token_int + (*current_char - 48);
            current_char++;
        }
        token_int *= ALT;
        if ((*current_char) != '(' && (*current_char) != ')' && (*current_char) != ' ' && (*current_char) != '\n' && *current_char != ',' && *current_char != '\0'){
            ERROR
        } else {
            return tok_number;
        }
    }
    if (isalpha(*current_char)){
        while (!is_open_space_close(*current_char)){
            token_str[TokSize] = *current_char;
            TokSize++;
            current_char++;
            if (TokSize >= LengthRatio - 1){
                LengthRatio *= 2;
                TokTemp = (char*)calloc(LengthRatio,sizeof(char));
                strncpy(TokTemp,token_str,LengthRatio);
                free(token_str);
                token_str = TokTemp;
            }
        }

        token_str[TokSize] = '\0';
		if (strcmp(token_str, "nil") == 0) {
			return tok_nil;
		}
		if (strcmp(token_str, "T") == 0) {
			return tok_T;
		}
		return tok_symbol;
    }
    if (*current_char == '('){
        current_char++;
        return tok_open;
    } else if (*current_char == ')'){
        current_char++;
        return tok_close;
	} else if (*current_char == '\'') {
		current_char++;
		return tok_quote;
	} else if (*current_char == '.') {
		/* float or dot */
		if (current_char[1] == ' ') {
			current_char++;
			return tok_dot;
		} else if (isnumber(current_char[1])) {
			TODO("float tokinizing\n");
		}
	} else if (*current_char == '<') {
		if (current_char[1] == '=' && is_open_space_close(current_char[2])) {
			current_char+=2;
			token_str[0] = '<';
			token_str[1] = '=';
			token_str[2] = '\0';
			return tok_symbol;
		} else if (is_open_space_close(current_char[1])) {
			current_char++;
			token_str[0] = '<';
			token_str[1] = '\0';
			return tok_symbol;
		}
	} else if (*current_char == '>') {
		if (current_char[1] == '=' && (is_open_space_close(current_char[2]))) {
			current_char+=2;
			token_str[0] = '>';
			token_str[1] = '=';
			token_str[2] = '\0';
			return tok_symbol;
		} else if (is_open_space_close(current_char[1])) {
			current_char++;
			token_str[0] = '>';
			token_str[1] = '\0';
			return tok_symbol;
		}
	} else if (*current_char == '=') {
		if (current_char[1] == '=' && (is_open_space_close(current_char[2]))) {
			current_char+=2;
			token_str[0] = '=';
			token_str[1] = '=';
			token_str[2] = '\0';
			return tok_symbol;
		} else if (is_open_space_close(current_char[1])) {
			current_char++;
			token_str[0] = '=';
			token_str[1] = '\0';
			return tok_symbol;
		}
	} else if (*current_char == '+') {
		if (is_open_space_close(current_char[1])) {
			current_char++;
			token_str[0] = '+';
			token_str[1] = '\0';
			return tok_symbol;
		}
	} else if (*current_char == '-') {
		if (is_open_space_close(current_char[1])) {
			current_char++;
			token_str[0] = '-';
			token_str[1] = '\0';
			return tok_symbol;
		}
	} else if (*current_char == '*') {
		if (is_open_space_close(current_char[1])) {
			current_char++;
			token_str[0] = '*';
			token_str[1] = '\0';
			return tok_symbol;
		}
	} else if (*current_char == '/') {
		if (current_char[1] == '=' && (is_open_space_close(current_char[2]))) {
			current_char+=2;
			token_str[0] = '/';
			token_str[1] = '=';
			token_str[2] = '\0';
			return tok_symbol;
		} else if (is_open_space_close(current_char[1])) {
			current_char++;
			token_str[0] = '/';
			token_str[1] = '\0';
			return tok_symbol;
		}
	}
    if( *current_char == '\0'){
        return tok_eof;
    }
    ERROR
} 

void get_next_token (void)
{
    token_type = GetTok();
}

static void tokenizer_init(char *str) {
	/* default maximum token length (mutable) */
	LengthRatio = LR;
	current_char = str;
    token_str = (char*)calloc(LengthRatio,sizeof(char*));
}

static val_t make_cons_single_node(int is_head_of_list) {
	val_t val = {0};
	if (token_type == tok_number) {
		val = new_int(token_int);
	} else if (token_type == tok_string) {
		val.ptr = new_string(token_str);
	} else if (token_type == tok_nil) {
		val = new_bool(0);
	} else if (token_type == tok_T) {
		val = new_bool(1);
	} else if (token_type == tok_symbol) {
		if (is_head_of_list) {
			val.ptr = new_func(token_str, NULL);
		} else {
			val.ptr = new_variable(token_str);
		}
	}
	return val;
}

static val_t make_cons_tree2(int is_head_of_list);

static val_t make_cons_list() {
	val_t val;
	val.ptr = new_open();
	val_t tmp = val;
	val_t car = {0};
	int flag = 1;
	get_next_token();
	tmp.ptr->car = make_cons_tree2(1);
	if (tmp.ptr->car.ivalue == 0) {
		tmp = new_bool(0);
		return tmp;
	}
	while (1) {
		get_next_token();
		if (token_type == tok_dot) {
			get_next_token();
			tmp.ptr->cdr = make_cons_tree2(0);
			/* eat ')' */
			get_next_token();
			if (token_type != tok_close) {
				EXCEPTION("Excepted \')\'!!\n");
			}
			break;
		}
		car = make_cons_tree2(0);
		if (car.ivalue == 0) {
			tmp.ptr->cdr = new_bool(0);
			break;
		}
		tmp.ptr->cdr.ptr = new_open();
		tmp.ptr->cdr.ptr->car = car;
		tmp = tmp.ptr->cdr;
	}
	return val;
}

static val_t make_cons_tree2(int is_head_of_list) {
	if (token_type == tok_open) {
		return make_cons_list();
	}else if (token_type == tok_quote) {	
		val_t root = {0};
		root.ptr = new_open();
		cstack_cons_cell_push(root);
		root.ptr->car.ptr = new_func("quote", NULL);
		root.ptr->cdr.ptr = new_open();
		root.ptr->cdr.ptr->cdr = new_bool(0);
		get_next_token();
		root.ptr->cdr.ptr->car = make_cons_tree2(0);
		cstack_cons_cell_pop();
		return root;
	} else {
		return make_cons_single_node(is_head_of_list);
	}
}

int parse_program (char *str) {
	tokenizer_init(str);
	get_next_token();
	val_t cons = make_cons_tree2(0);
	if (cons.ivalue = 0) {
		codegen(cons);
		return 0;
	}
	printf("Syntax Error\n");
}
