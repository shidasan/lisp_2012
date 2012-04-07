#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "lisp.h"
#define ERROR /*printf("Error In Tokenizer\n");*/ return tok_error;
#define ARGERROR if (ArgCount[ArgIndex] > p->value)printf("Too many "); else printf("Too few "); printf("arguments given to function\n");  return NULL;
#define PERROR FreeAST(ret);return NULL;
#define MSG(STR) fprintf(stderr, (STR))
#define AR 3
#define LR 10
AST* ParseExpression (void);
AST* ParseBlock (void);
void FreeAST (AST*);
static int current_token;
static char* current_char;
static char* TokStr;
static int TokNum;
static int ArgIndex;
static unsigned int LengthRatio;
static int ArgsRatio = AR;
static char** Args;
int GetTok (void)
{
    char* TokTemp = NULL;
    int ALT = 1;
    TokNum = 0;
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
            TokNum = 10 * TokNum + (*current_char - 48);
            current_char++;
        }
        TokNum *= ALT;
        if ((*current_char) != '(' && (*current_char) != ')' && (*current_char) != ' ' && (*current_char) != '\n' && *current_char != ','){
            ERROR
        } else {
            return tok_number;
        }
    }
    if (isalpha(*current_char)){
        while (isalnum(*current_char)){
            TokStr[TokSize] = *current_char;
            TokSize++;
            current_char++;
            if (TokSize >= LengthRatio - 1){
                LengthRatio *= 2;
                TokTemp = (char*)calloc(LengthRatio,sizeof(char));
                strncpy(TokTemp,TokStr,LengthRatio);
                free(TokStr);
                TokStr = TokTemp;
            }
        }

        TokStr[TokSize] = '\0';
		if (strcmp(TokStr, "nil") == 0) {
			return tok_nil;
		}
		if (strcmp(TokStr, "T") == 0) {
			return tok_T;
		}
		return tok_str;
        //if (strcmp(TokStr,"defun") == 0){
        //    if(*current_char != ' '){
        //        ERROR
        //    } else {
        //        return tok_defun;
        //    }
        //} else if (strcmp(TokStr,"if") == 0){
        //    if (*current_char != ' '){
        //        ERROR
        //    } else {
        //        return tok_if;
        //    }
        //} else if (strcmp(TokStr,"setq") == 0){
        //    if (*current_char != ' '){
        //        ERROR
        //    } else {
        //        return tok_setq;
        //    }
		//}else if (strcmp(TokStr,"T") == 0){
        //    return tok_T;
        //} else if (strcmp(TokStr,"nil") == 0){
        //    return tok_nil;
        //}
        //if (*current_char != '(' && *current_char != ')' && *current_char != ' ' && (*current_char) != '\n' && *current_char != ','){
        //    ERROR
        //} else {
        //    return tok_str;
        //}
    }
    if (*current_char == '(' && !isdigit(*(current_char + 1))){
        current_char++;
        return tok_open;
    } else if (*current_char == ')'){
        current_char++;
        return tok_close;
    } else if (*current_char == '<' && (*(current_char + 1) == '(' || *(current_char + 1) == ' ')){
        current_char++;
		return tok_str;
        //return tok_gt;
    } else if (*current_char == '>' && (*(current_char + 1) == '(' || *(current_char + 1) == ' ')){
        current_char++;
		return tok_str;
        //return tok_lt;
    } else if (*current_char == '<' && *(current_char + 1) == '=' && (*(current_char + 2) == '(' || *(current_char + 2) == ' ')){
        current_char += 2;
		return tok_str;
        //return tok_gte;
    } else if (*current_char == '>' && *(current_char + 1) == '=' && (*(current_char + 2) == '(' || *(current_char + 2) == ' ')){
        current_char += 2;
		return tok_str;
        //return tok_lte;
    } else if (*current_char == '=' && (*(current_char + 1) == '(' || *(current_char + 1) == ' ')){
        current_char++;
        return tok_str;
		//return tok_eq;
    } else if (*current_char == '+' && (*(current_char + 1) == '(' || *(current_char + 1) == ' ')){
        current_char++;
        return tok_str;
		//return tok_plus;
    } else if (*current_char == '-' && (*(current_char + 1) == '(' || *(current_char + 1) == ' ')){
        current_char++;
		return tok_str;
        //return tok_minus;
    } else if (*current_char == '*' && (*(current_char + 1) == '(' || *(current_char + 1) == ' ')){
        current_char++;
		return tok_str;
        //return tok_mul;
    } else if (*current_char == '/' && (*(current_char + 1) == '(' || *(current_char + 1) == ' ')){
        current_char++;
		return tok_str;
        //return tok_div;
    }

    if( *current_char == '\0'){
        return tok_eof;
    }
    ERROR
} 

void getNextToken (void)
{
    current_token = GetTok();
}

AST* ParseArgument (void)
{
    char** ArgsTemp;
    int count = 0, count1 = 0;
    AST* ret = (AST*)malloc(sizeof(AST));
    ret->LHS = ret->RHS = ret->COND = NULL;
    ret->s = NULL;
    getNextToken(); //eat '('
    if (current_token != tok_open){
        printf("wrong prototype\n");
        FreeAST(ret);
        return NULL;
    }
    getNextToken();
    while (current_token != tok_close){
        if (current_token != tok_str){
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
        strncpy(Args[count],TokStr,LengthRatio);
        count++;
        getNextToken();
    }
    ret->type = tok_arg;
    ret->i = count;
    return ret;

}

AST* ParseIf (void)
{
    getNextToken(); //eat '(' or number
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
    getNextToken();
    if (current_token != tok_close){
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
    getNextToken();
    if (current_token == tok_str){
        ret->s = (char*)malloc(sizeof(char) * LengthRatio);
        strncpy(ret->s,TokStr,LengthRatio);
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
    p = setF(ret->s, ret->LHS->i, NULL, LengthRatio, 0);
	/* overwriting static method */
	if (p == NULL) {
		PERROR
	}
    ret->RHS = ParseBlock();
    if (ret->RHS == NULL){
        FreeAST(ret);
        return NULL;
    }
    getNextToken();
    if (current_token == tok_close){
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
    getNextToken();
    if (current_token == tok_str){
        ret->s = (char*)malloc(LengthRatio);
        strncpy(ret->s, TokStr, LengthRatio);
    } else {
        printf("error in setq\n");
        FreeAST(ret);
        return NULL;
    }
    getNextToken();
    ret->LHS = ParseExpression();
    if (ret->LHS == NULL){
        printf("error in setq\n");
        FreeAST(ret);
        return NULL;
    }
    setV(ret->s,LengthRatio);
    return ret;
}

AST* ParseVariable (void)
{
    AST* ret = NULL;
    int i;
    for(i = 0; i < ArgsRatio; i++){
        if (Args[i] != NULL && strcmp(Args[i],TokStr) == 0){
            ret = (AST*)malloc(sizeof(AST));
            ret->LHS = ret->RHS = ret->COND = NULL;
            ret->s = NULL;
            ret->type = tok_arg;
            ret->i = i;
            return ret;
        }
    }
    if (searchV(TokStr) != NULL){
        ret = (AST*)malloc(sizeof(AST));
        ret->LHS = ret->RHS = ret->COND = NULL;
        ret->s = NULL;
        ret->type = tok_valiable;
        ret->s = (char*)malloc(LengthRatio);
        strncpy(ret->s, TokStr, LengthRatio);
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
    ret->i = TokNum;
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
        getNextToken(); // eat operator
        OpType = current_token;
        if (current_token == tok_str && (*current_char == ' ' || *current_char == ')' || *current_char == ',')){
            p = searchF(TokStr);
            if (p != NULL){
                ArgCount[ArgIndex] = 0;
                fname = (char*)malloc(LengthRatio);
                strncpy(fname, TokStr,LengthRatio);
                ret->type = tok_func;
                if ( p->value == 0){
                    ret->s = fname;
                    ret->LHS = NULL;
                    ret->RHS = NULL;
                    getNextToken(); // eat ')'
                    if (current_token == tok_close){
                        return ret;
                    } else {
                        free(fname);
                        ARGERROR
                    }
                } else if (p->value == 1){
                    getNextToken();
                    ret->s = fname;
                    ret->RHS = ParseExpression();
                    ret->LHS = NULL;
                    getNextToken(); //eat ')'
                    if (current_token == tok_close && ret->RHS != NULL){
                        return ret;
                    } else {
                        free(fname);
                        ARGERROR
                    }
                } else {
                    getNextToken();
                    free(ret);
                    ret = NULL;
                    ArgCount[ArgIndex]++;
                    LHS = ParseExpression();
                    if (LHS != NULL){
                        getNextToken();
                        if (current_token == tok_open || current_token == tok_number || current_token == tok_str){
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
        getNextToken();
        LHS = ParseExpression();
        getNextToken();
        if (current_token == tok_close){
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
            getNextToken();
            if (current_token == tok_number || current_token == tok_open || current_token == tok_str){
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
            } else if (current_token == tok_close){
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
    if (current_token == tok_number){
        ret = ParseNumber();
        if (ret == NULL){ PERROR }
        return ret;
    } else if (current_token == tok_T){
        ret = ParseT();
        return ret;
    } else if (current_token == tok_nil){
        ret = ParseNil();
        return ret;
    }else if (current_token == tok_str){
        ret = ParseVariable();
        if (ret == NULL){ PERROR }
        return ret;
    } else if (current_token == tok_open){
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
    getNextToken();
    if (current_token == tok_open){
        getNextToken();
        if (current_token == tok_defun){
            ret = ParseDefun();
            if (ret == NULL){ PERROR }
            return ret;
        } else if (current_token == tok_setq){
            ret = ParseSetq();
            getNextToken();
            if (ret == NULL){ PERROR }
            return ret;
        } else if (current_token == tok_if){
            ret = ParseIf();
            if (ret == NULL){
                PERROR
            }
            return ret;
        } else {
            current_char = p;
            getNextToken();
            ret = ParseExpression();
            if (ret == NULL){ PERROR }
            return ret;
        }
    } else if (current_token == tok_number){
        ret = ParseNumber();
        return ret;
    } else if (current_token == tok_T){
        ret = ParseT();
        return ret;
    } else if (current_token == tok_nil){
        ret = ParseNil();
        return ret;
    }else if (current_token == tok_str){
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
    TokStr = (char*)calloc(LengthRatio,sizeof(char*));
    AST* ret = NULL;
    current_char = str;

    getNextToken();
    if (current_token == tok_eof){
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
    getNextToken();
    free(TokStr);
    if (ret != NULL && current_token == tok_eof ){
        GenerateProgram(ret);
        return 0;
    }
	printf("Syntax Error\n");
    return 1;
}

void tokenizer_init(char *str) {
	/* default maximum token length (mutable) */
	LengthRatio = LR;
	current_char = str;
}

ast_t *parse_expression() {
	getNextToken();
	if (current_token == tok_eof) {
		return NULL;
	}
}

int parse_program (char *str) {
	tokenizer_init(str);
	ast_t *ast = parse_expression();
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
