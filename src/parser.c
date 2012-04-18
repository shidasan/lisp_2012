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

static cons_t *make_cons_single_node(int is_head_of_list) {
	cons_t *cons = NULL;
	if (token_type == tok_number) {
		cons = new_int(token_int);
	} else if (token_type == tok_string) {
		cons = new_string(token_str);
	} else if (token_type == tok_nil) {
		cons = new_bool(0);
	} else if (token_type == tok_T) {
		cons = new_bool(1);
	} else if (token_type == tok_symbol) {
		if (is_head_of_list) {
			cons = new_func(token_str, NULL);
		} else {
			cons = new_variable(token_str);
		}
	}
	return cons;
}

static cons_t *make_cons_tree2(int is_head_of_list);

static cons_t *make_cons_list() {
	cons_t *cons = new_open();
	cons_t *tmp = cons;
	cons_t *car = NULL;
	int flag = 1;
	get_next_token();
	tmp->car = make_cons_tree2(1);
	if (tmp->car == NULL) {
		tmp = new_bool(0);
		return tmp;
	}
	while (1) {
		get_next_token();
		if (token_type == tok_dot) {
			get_next_token();
			tmp->cdr = make_cons_tree2(0);
			/* eat ')' */
			get_next_token();
			if (token_type != tok_close) {
				EXCEPTION("Excepted \')\'!!\n");
			}
			break;
		}
		car = make_cons_tree2(0);
		if (car == NULL) {
			tmp->cdr = new_bool(0);
			break;
		}
		tmp->cdr = new_open();
		tmp->cdr->car = car;
		tmp = tmp->cdr;
	}
	return cons;
}

static cons_t *make_cons_tree2(int is_head_of_list) {
	if (token_type == tok_open) {
		return make_cons_list();
	}else if (token_type == tok_quote) {	
		cons_t *root = new_open();
		cstack_cons_cell_push(root);
		root->car = new_func("quote", NULL);
		root->cdr = new_open();
		root->cdr->cdr = new_bool(0);
		get_next_token();
		root->cdr->car = make_cons_tree2(0);
		cstack_cons_cell_pop();
		return root;
	} else {
		return make_cons_single_node(is_head_of_list);
	}
}

static ast_t *parse_expression(int is_head_of_list, int is_quote_unused);

static ast_t *parse_list() {
	ast_t *ast = new_ast(ast_list, -1);
	ast_t *childast = parse_expression(1, 0);
	if (childast == NULL) {
		ast_free(ast);
		ast = new_ast(ast_atom, nil);
		ast->cons = new_bool(0);
		return ast;
	}
	array_add(ast->a, childast);
	func_t *func;
	int* quote_position = NULL;
	if (childast->type == ast_static_func || childast->type == ast_special_form) {
		func = search_func(childast->cons->str);
		if (func != NULL && func->is_quote[0]) {
			quote_position = func->is_quote;
		}
	}
	if (childast->type == ast_special_form) {

	}
	int args_count = 1;
	while (1) {
		if (quote_position != NULL && ((args_count == quote_position[0] || args_count == quote_position[1]) || quote_position[0] == -1)) {
			/* make_cons_tree2 must not eat any token */
			get_next_token();
			if (token_type == tok_close) {
				break;
			}
			cons_t *cons = make_cons_tree2(0);
			childast = new_ast(ast_atom, cons->type);
			childast->cons = cons;
		} else {
			childast = parse_expression(0, 0);
		}
		if (childast == NULL) {
			break;
		}
		array_add(ast->a, childast);
		args_count++;
	}
	return ast;
}

static ast_t *parse_expression(int is_head_of_list, int is_quote_unused) {
	get_next_token();
	ast_t *ast = NULL;
	if (token_type == tok_eof) {
		return NULL;
	}
	if (token_type == tok_number) {
		ast = new_ast(ast_atom, INT);
		ast->cons = new_int(token_int);
	} else if (token_type == tok_string) {
		ast = new_ast(ast_atom, STRING);
		ast->cons = new_string(token_str);
	} else if (token_type == tok_nil) {
		ast = new_ast(ast_atom, nil);
		ast->cons = new_bool(0);
	} else if (token_type == tok_T) {
		ast = new_ast(ast_atom, T);
		ast->cons = new_bool(1);
	} else if (token_type == tok_open) {
		ast =  parse_list();
	} else if (token_type == tok_quote) {
		/* quote macro */
		ast = new_ast(ast_list, -1);
		get_next_token();
		ast_t *funcast = new_ast(ast_static_func, -1);
		funcast->cons = new_func("quote", NULL);
		array_add(ast->a, funcast);
		cons_t *cons = make_cons_tree2(0);
		ast_t *childast = new_ast(ast_atom, cons->type);
		childast->cons = cons;
		array_add(ast->a, childast);
	} else if (token_type == tok_symbol) {
		ast = NULL;
		if (is_head_of_list) {
			func_t *func = search_func(token_str);
			if (func != NULL && func->is_static) {
				if (func->is_special_form) {
					ast = new_ast(ast_special_form, -1);
					ast->cons = new_func((const char*)token_str, NULL);
				} else if (func->is_static) {
					ast = new_ast(ast_static_func, -1);
					ast->cons = new_func((const char*)token_str, NULL);
				}
			} else {
				ast = new_ast(ast_func, -1);
				ast->cons = new_func((const char*)token_str, NULL);
			}
		} else {
			ast = new_ast(ast_variable, -1);
			ast->cons = new_variable(token_str);
		}
	} else if (token_type == tok_close) {
		//ast_t *ast = new_ast(ast_list_close, -1);
		ast = NULL;
	} else {
		TODO("parsing error or does not implemented\n");
		fprintf(stderr, "token_type: %d\n", token_type);
	}
	return ast;
}

int parse_program (char *str) {
	tokenizer_init(str);
	ast_t *ast = parse_expression(0, 0);
	if (ast != NULL) {
		codegen(ast);
		return 0;
	}
	printf("Syntax Error\n");
}
