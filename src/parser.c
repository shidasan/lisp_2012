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

int GetTok (void)
{
    char* TokTemp = NULL;
    int ALT = 1;
    token_int = 0;
    unsigned int TokSize = 0;
    if (*current_char == '\0' || *current_char == '\n'){
        return tok_eof;
    }
    while (isspace(*current_char) || *current_char == ','){
        current_char++;
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
        while (isalnum(*current_char)){
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
		if (current_char[1] == '=' && (current_char[2] == '(' || current_char[2] == ' ')) {
			current_char+=2;
			token_str[0] = '<';
			token_str[1] = '=';
			token_str[2] = '\0';
			return tok_symbol;
		} else if (current_char[1] == '(' || current_char[1] == ' ') {
			current_char++;
			token_str[0] = '<';
			token_str[1] = '\0';
			return tok_symbol;
		}
	} else if (*current_char == '>') {
		if (current_char[1] == '=' && (current_char[2] == '(' || current_char[2] == ' ')) {
			current_char+=2;
			token_str[0] = '>';
			token_str[1] = '=';
			token_str[2] = '\0';
			return tok_symbol;
		} else if (current_char[1] == '(' || current_char[1] == ' ') {
			current_char++;
			token_str[0] = '>';
			token_str[1] = '\0';
			return tok_symbol;
		}
	} else if (*current_char == '=') {
		if (current_char[1] == '=' && (current_char[2] == '(' || current_char[2] == ' ')) {
			current_char+=2;
			token_str[0] = '=';
			token_str[1] = '=';
			token_str[2] = '\0';
			return tok_symbol;
		} else if (current_char[1] == '(' || current_char[1] == ' ') {
			current_char++;
			token_str[0] = '=';
			token_str[1] = '\0';
			return tok_symbol;
		}
	} else if (*current_char == '+') {
		if (current_char[1] == '(' || current_char[1] == ' ') {
			current_char++;
			token_str[0] = '+';
			token_str[1] = '\0';
			return tok_symbol;
		}
	} else if (*current_char == '-') {
		if (current_char[1] == '(' || current_char[1] == ' ') {
			current_char++;
			token_str[0] = '-';
			token_str[1] = '\0';
			return tok_symbol;
		}
	} else if (*current_char == '*') {
		if (current_char[1] == '(' || current_char[1] == ' ') {
			current_char++;
			token_str[0] = '*';
			token_str[1] = '\0';
			return tok_symbol;
		}
	} else if (*current_char == '/') {
		if (current_char[1] == '(' || current_char[1] == ' ') {
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

AST* ParseArgument (void)
{
    char** ArgsTemp;
    int count = 0, count1 = 0;
    AST* ret = (AST*)malloc(sizeof(AST));
    ret->LHS = ret->RHS = ret->COND = NULL;
    ret->s = NULL;
    get_next_token(); //eat '('
    if (token_type != tok_open){
        printf("wrong prototype\n");
        FreeAST(ret);
        return NULL;
    }
    get_next_token();
    while (token_type != tok_close){
        if (token_type != tok_symbol){
            FreeAST(ret);
            return NULL;
        }
        if (count == ArgsRatio){
            ArgsRatio *= 2;
            ArgsTemp = (char**)calloc(ArgsRatio,sizeof(char*));
            for (count1 = 0; count1 < ArgsRatio / 2; count1++){
                ArgsTemp[count1] = Args[count1];
            }
            free(Args);
            Args = ArgsTemp;
        }
        Args[count] = (char*)malloc(LengthRatio);
        strncpy(Args[count],token_str,LengthRatio);
        count++;
        get_next_token();
    }
    ret->type = tok_arg;
    ret->i = count;
    return ret;

}

AST* ParseIf (void)
{
    get_next_token(); //eat '(' or number
    AST* ret = (AST*)malloc(sizeof(AST));
    ret->LHS = ret->RHS = ret->COND = NULL;
    ret->s = NULL;
    ret->type = tok_if;
    ret->COND = ParseExpression();
    ret->LHS = ParseBlock();
    ret->RHS = ParseBlock();
    if (ret->LHS == NULL || ret->RHS == NULL || ret->COND == NULL ){
        PERROR
    }
    get_next_token();
    if (token_type != tok_close){
        PERROR
    }
    return ret;
}

AST* ParseDefun (void)
{
    func_t* p = NULL;
    int i = 0;
    AST* ret = (AST*)malloc(sizeof(AST));
    ret->LHS = ret->RHS = ret->COND = NULL;
    ret->s = NULL;
    ret->type = tok_defun;
    get_next_token();
    if (token_type == tok_symbol){
        ret->s = (char*)malloc(sizeof(char) * LengthRatio);
        strncpy(ret->s,token_str,LengthRatio);
    } else {
        printf("Error in defun\n");
        FreeAST(ret);
        return NULL;
    }
    ret->LHS = ParseArgument();
    if (ret->LHS == NULL){
        FreeAST(ret);
        return NULL;
    }
    p = set_static_func(ret->s, ret->LHS->i, NULL, NULL, 0, 0, 0, 0);
	/* overwriting static method */
	if (p == NULL) {
		PERROR
	}
    ret->RHS = ParseBlock();
    if (ret->RHS == NULL){
        FreeAST(ret);
        return NULL;
    }
    get_next_token();
    if (token_type == tok_close){
        for (i = 0; i < ArgsRatio; i++){
            free(Args[i]);
            Args[i] = NULL;
        }
        return ret;
    }
    PERROR
}

AST* ParseSetq (void)
{
    AST* ret = (AST*)malloc(sizeof(AST));
    ret->LHS = ret->RHS = ret->COND = NULL;
    ret->s = NULL;
    ret->type = tok_setq;
    get_next_token();
    if (token_type == tok_symbol){
        ret->s = (char*)malloc(LengthRatio);
        strncpy(ret->s, token_str, LengthRatio);
    } else {
        printf("error in setq\n");
        FreeAST(ret);
        return NULL;
    }
    get_next_token();
    ret->LHS = ParseExpression();
    if (ret->LHS == NULL){
        printf("error in setq\n");
        FreeAST(ret);
        return NULL;
    }
    //setV(ret->s,LengthRatio);
    return ret;
}

AST* ParseVariable (void)
{
    AST* ret = NULL;
    int i;
    for(i = 0; i < ArgsRatio; i++){
        if (Args[i] != NULL && strcmp(Args[i],token_str) == 0){
            ret = (AST*)malloc(sizeof(AST));
            ret->LHS = ret->RHS = ret->COND = NULL;
            ret->s = NULL;
            ret->type = tok_arg;
            ret->i = i;
            return ret;
        }
    }
    if (search_variable(token_str) != NULL){
        ret = (AST*)malloc(sizeof(AST));
        ret->LHS = ret->RHS = ret->COND = NULL;
        ret->s = NULL;
        ret->type = tok_valiable;
        ret->s = (char*)malloc(LengthRatio);
        strncpy(ret->s, token_str, LengthRatio);
        return ret;
    } else {
        printf("valiable not found\n");
        FreeAST(ret);
        return NULL;
    }
}

AST* ParseNumber (void)
{
    AST* ret = NULL;
    ret = (AST*)malloc(sizeof(AST));
    ret->LHS = ret->RHS = ret->COND = NULL;
    ret->s = NULL;
    ret->type = tok_number;
    ret->i = token_int;
    ret->LHS = ret->RHS = NULL;
    return ret;
}

AST* ParseT (void)
{
    AST* ret = NULL;
    ret = (AST*)malloc(sizeof(AST));
    ret->LHS = ret->RHS = ret->COND = NULL;
    ret->s = NULL;
    ret->type = tok_T;
    ret->LHS = ret->RHS = NULL;
    return ret;
}

AST* ParseNil (void)
{
    AST* ret = NULL;
    ret = (AST*)malloc(sizeof(AST));
    ret->RHS = ret->LHS = ret->COND = NULL;
    ret->s = NULL;
    ret->type = tok_nil;
    ret->LHS = ret->RHS = NULL;
    return ret;
}

AST* ParseOperation (int Tok,AST* pRHS)
{
    static int ArgCount[20];
    char* fname;
    AST* ret = (AST*)malloc(sizeof(AST));
    ret->LHS = ret->RHS = ret->COND = NULL;
    ret->s = NULL;
    AST *LHS,*RHS;
    func_t* p;
    int OpType;
    if (pRHS == NULL){
        //printf("plus ");
        get_next_token(); // eat operator
        OpType = token_type;
        if (token_type == tok_symbol && (*current_char == ' ' || *current_char == ')' || *current_char == ',')){
            p = search_func(token_str);
            if (p != NULL){
                ArgCount[ArgIndex] = 0;
                fname = (char*)malloc(LengthRatio);
                strncpy(fname, token_str,LengthRatio);
                ret->type = tok_func;
                if ( p->value == 0){
                    ret->s = fname;
                    ret->LHS = NULL;
                    ret->RHS = NULL;
                    get_next_token(); // eat ')'
                    if (token_type == tok_close){
                        return ret;
                    } else {
                        free(fname);
                        ARGERROR
                    }
                } else if (p->value == 1){
                    get_next_token();
                    ret->s = fname;
                    ret->RHS = ParseExpression();
                    ret->LHS = NULL;
                    get_next_token(); //eat ')'
                    if (token_type == tok_close && ret->RHS != NULL){
                        return ret;
                    } else {
                        free(fname);
                        ARGERROR
                    }
                } else {
                    get_next_token();
                    free(ret);
                    ret = NULL;
                    ArgCount[ArgIndex]++;
                    LHS = ParseExpression();
                    if (LHS != NULL){
                        get_next_token();
                        if (token_type == tok_open || token_type == tok_number || token_type == tok_symbol){
                            RHS = ParseOperation(tok_func, LHS);
                        } else {
                            free(fname);
                            ARGERROR
                        }
                        RHS->type = tok_func;
                        if (RHS != NULL){
                            if (ArgCount[ArgIndex] == p->value){
                                RHS->s = fname;
                                return RHS;
                            } else {
                                free(fname);
                                ARGERROR
                            }
                        } else {
                            free(fname);
                            PERROR
                        }
                    } else {
                        free(fname);
                        PERROR
                    }
                }
            } else {
                PERROR
            }
        }
        get_next_token();
        LHS = ParseExpression();
        get_next_token();
        if (token_type == tok_close){
            ret->type = OpType;
            ret->LHS = LHS;
            ret->RHS = NULL;
            return ret;
        }
        RHS = ParseExpression();

    } else {
        OpType = Tok;
        LHS = pRHS;
        ArgCount[ArgIndex]++;
        RHS = ParseExpression();
    }
    switch(OpType){
        case tok_plus:case tok_minus:case tok_mul:case tok_div:
        case tok_lt:case tok_lte:case tok_gt:case tok_gte:
        case tok_eq:case tok_func:
            if (LHS == NULL || RHS == NULL){
                return NULL;
                FreeAST(ret);
            }
            if (LHS->type == tok_nil || RHS->type == tok_nil){
                printf("nil is not a real number\n");
                FreeAST(ret);
                return NULL;
            }
            if (LHS->type == tok_T || RHS->type == tok_T){
                printf("T is not a real number\n");
                FreeAST(ret);
                return NULL;
            }
            get_next_token();
            if (token_type == tok_number || token_type == tok_open || token_type == tok_symbol){
                ret->type = OpType;
                ret->LHS = LHS;
                if (pRHS == NULL && (OpType == tok_minus || OpType == tok_div)){
                    if (OpType == tok_minus){
                        ret->RHS = ParseOperation(tok_plus, RHS);
                    } else {
                        ret->RHS = ParseOperation(tok_mul, RHS);
                    }
                } else {
                    ret->RHS = ParseOperation(OpType, RHS);
                }
                if (ret->RHS == NULL) {PERROR}
            } else if (token_type == tok_close){
                ret->LHS = LHS;
                ret->RHS = RHS;
                ret->type = OpType;
            } else {
                PERROR
            }
            return ret;
            break;

        default:
            PERROR
    }
}

AST* ParseExpression (void)
{
    AST* ret = NULL;
    if (token_type == tok_number){
        ret = ParseNumber();
        if (ret == NULL){ PERROR }
        return ret;
    } else if (token_type == tok_T){
        ret = ParseT();
        return ret;
    } else if (token_type == tok_nil){
        ret = ParseNil();
        return ret;
    }else if (token_type == tok_symbol){
        ret = ParseVariable();
        if (ret == NULL){ PERROR }
        return ret;
    } else if (token_type == tok_open){
        ArgIndex++;
        ret = ParseOperation(-1, NULL);
        ArgIndex--;
        if (ret == NULL){ PERROR }
        return ret;
    } else {
        PERROR
    }
}

AST* ParseBlock (void)
{
    AST* ret = NULL;
    char *p = current_char;
    get_next_token();
    if (token_type == tok_open){
        get_next_token();
        if (token_type == tok_defun){
            ret = ParseDefun();
            if (ret == NULL){ PERROR }
            return ret;
        } else if (token_type == tok_setq){
            ret = ParseSetq();
            get_next_token();
            if (ret == NULL){ PERROR }
            return ret;
        } else if (token_type == tok_if){
            ret = ParseIf();
            if (ret == NULL){
                PERROR
            }
            return ret;
        } else {
            current_char = p;
            get_next_token();
            ret = ParseExpression();
            if (ret == NULL){ PERROR }
            return ret;
        }
    } else if (token_type == tok_number){
        ret = ParseNumber();
        return ret;
    } else if (token_type == tok_T){
        ret = ParseT();
        return ret;
    } else if (token_type == tok_nil){
        ret = ParseNil();
        return ret;
    }else if (token_type == tok_symbol){
        ret = ParseVariable();
        return ret;
    }else {
        return NULL;
    }
    return NULL;
}

int ParseProgram (char *str)
{
    ArgsRatio = AR;
    LengthRatio = LR;
    Args = (char**)calloc(ArgsRatio,sizeof(char*));
    token_str = (char*)calloc(LengthRatio,sizeof(char*));
    AST* ret = NULL;
    current_char = str;

    get_next_token();
    if (token_type == tok_eof){
        return 2;
    }

    current_char = str;
    ret = ParseBlock();

    int i = 0;
    for (; i < ArgsRatio; i++) {
        free(Args[i]);
        Args[i] = NULL;
    }
    free(Args);
    get_next_token();
    free(token_str);
    if (ret != NULL && token_type == tok_eof ){
        GenerateProgram(ret);
        return 0;
    }
	printf("Syntax Error\n");
    return 1;
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
		if (quote_position != NULL && (args_count == quote_position[0] || args_count == quote_position[1])) {
			/* make_cons_tree2 must not eat any token */
			get_next_token();
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
				//func_t *func = search_func(token_str);
				//cons_t *environment = NULL;
				//if (func == NULL) {
				//	environment = new_local_environment();
				//} else {
				//	environment = func->environment;
				//}
				//environment->local_environment = compile_time_environment;
				//compile_time_environment = environment;
				ast = new_ast(ast_func, -1);
				//ast->cons = new_func((const char*)token_str, environment);
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

static void environment_init() {
	//compile_time_environment = current_environment;
}

int parse_program (char *str) {
	environment_init();
	tokenizer_init(str);
	ast_t *ast = parse_expression(0, 0);
	if (ast != NULL) {
		codegen(ast);
		return 0;
	}
	printf("Syntax Error\n");
}

void FreeAST (AST* ast)
{
    if (ast == NULL){
        return;
    }
    if (ast->LHS != NULL){
        FreeAST(ast->LHS);
        ast->LHS = NULL;
    }
    if (ast->COND != NULL){
        FreeAST(ast->COND);
        ast->COND = NULL;
    }
    if (ast->RHS != NULL){
        FreeAST(ast->RHS);
        ast->RHS = NULL;
    }
    free(ast->s);
    ast->s = NULL;
    free(ast);
}
