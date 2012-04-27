#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "lisp.h"
static int token_type;
static char* current_char;
static char* token_str;
static int token_int;
static float token_float;

int is_open_space_close(char c) {
	return c == '(' || c == ' ' || c == ')';
}

float tokenize_float(int start) {
	float res = (float)start;
	float f = 0.1;
	while (isdigit(*current_char)) {
		res += f * (*current_char - 48);
		f *= 0.1;
		current_char++;
	}
	return res;
}

int get_next_token_inner (string_buffer_t *buffer)
{
    int ALT = 1;
    token_int = 0;
    while (isspace(*current_char) || *current_char == ','){
        current_char++;
    }
    if (*current_char == '\0' || *current_char == '\n'){
        return tok_eof;
    }
	if (*current_char == '\"') {
		current_char++;
		while (*current_char != '\"') {
			string_buffer_append_c(buffer, *current_char);
			current_char++;
		}
		current_char++;
		token_str = string_buffer_to_string(buffer);
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
		if (*current_char == '.') {
			current_char++;
			if (isdigit(*current_char)) {
				token_float = tokenize_float(token_int);
				return tok_float;
			} else {
				return tok_int;
			}
		}
        if ((*current_char) != '(' && (*current_char) != ')' && (*current_char) != ' ' && (*current_char) != '\n' && *current_char != ',' && *current_char != '\0'){
			return tok_error;
        } else {
            return tok_int;
        }
    }
    if (isalpha(*current_char)){
        while (!is_open_space_close(*current_char)){
			string_buffer_append_c(buffer, *current_char);
            current_char++;
        }

		if (strncmp(buffer->str, "nil", 3) == 0) {
			return tok_nil;
		}
		if (strncmp(buffer->str, "T", 1) == 0) {
			return tok_T;
		}
		token_str = string_buffer_to_string(buffer);
		return tok_symbol;
    }
    if (*current_char == '('){
        current_char++;
        return tok_open;
    } else if (*current_char == ')'){
        current_char++;
        return tok_close;
	} else if (*current_char == '#') {
		current_char++;
		return tok_array;
	} else if (*current_char == '\'') {
		current_char++;
		return tok_quote;
	} else if (*current_char == '.') {
		/* float or dot */
		current_char++;
		if (current_char[0] == ' ') {
			return tok_dot;
		} else if (isdigit(current_char[0])) {
			token_float = tokenize_float(0);
			return tok_float;
		}
	} else if (*current_char == '<') {
		if (current_char[1] == '=' && is_open_space_close(current_char[2])) {
			current_char+=2;
			string_buffer_append_s(buffer, "<=");
			token_str = string_buffer_to_string(buffer);
			return tok_symbol;
		} else if (is_open_space_close(current_char[1])) {
			current_char++;
			string_buffer_append_s(buffer, "<");
			token_str = string_buffer_to_string(buffer);
			return tok_symbol;
		}
	} else if (*current_char == '>') {
		if (current_char[1] == '=' && (is_open_space_close(current_char[2]))) {
			current_char+=2;
			string_buffer_append_s(buffer, ">=");
			token_str = string_buffer_to_string(buffer);
			return tok_symbol;
		} else if (is_open_space_close(current_char[1])) {
			current_char++;
			string_buffer_append_s(buffer, ">");
			token_str = string_buffer_to_string(buffer);
			return tok_symbol;
		}
	} else if (*current_char == '=') {
		if (current_char[1] == '=' && (is_open_space_close(current_char[2]))) {
			current_char+=2;
			string_buffer_append_s(buffer, "==");
			token_str = string_buffer_to_string(buffer);
			return tok_symbol;
		} else if (is_open_space_close(current_char[1])) {
			current_char++;
			string_buffer_append_s(buffer, "=");
			token_str = string_buffer_to_string(buffer);
			return tok_symbol;
		}
	} else if (*current_char == '+') {
		if (is_open_space_close(current_char[1])) {
			current_char++;
			string_buffer_append_s(buffer, "+");
			token_str = string_buffer_to_string(buffer);
			return tok_symbol;
		}
	} else if (*current_char == '-') {
		if (is_open_space_close(current_char[1])) {
			current_char++;
			string_buffer_append_s(buffer, "-");
			token_str = string_buffer_to_string(buffer);
			return tok_symbol;
		}
	} else if (*current_char == '*') {
		if (is_open_space_close(current_char[1])) {
			current_char++;
			string_buffer_append_s(buffer, "*");
			token_str = string_buffer_to_string(buffer);
			return tok_symbol;
		}
	} else if (*current_char == '/') {
		if (current_char[1] == '=' && (is_open_space_close(current_char[2]))) {
			current_char+=2;
			string_buffer_append_s(buffer, "/=");
			token_str = string_buffer_to_string(buffer);
			return tok_symbol;
		} else if (is_open_space_close(current_char[1])) {
			current_char++;
			string_buffer_append_s(buffer, "/");
			token_str = string_buffer_to_string(buffer);
			return tok_symbol;
		}
	}
    if( *current_char == '\0'){
        return tok_eof;
    }
	string_buffer_free(buffer);
	EXCEPTION("Invalid token!!\n");
	return tok_error; //unreach
} 

void get_next_token ()
{
	string_buffer_t *buffer = new_string_buffer();
    token_type = get_next_token_inner(buffer);
	string_buffer_free(buffer);
}

static void tokenizer_init(char *str) {
	/* default maximum token length (mutable) */
	current_char = str;
}

static val_t make_cons_tree2(int is_head_of_list);

static val_t make_cons_single_node(int is_head_of_list) {
	val_t val = null_val();
	switch (token_type) {
	case tok_int:
		return new_int(token_int);
	case tok_float:
		return new_float(token_float);
	case tok_string:
		val.ptr = new_string(token_str);
		break;
	case tok_nil:
		return new_bool(0);
	case tok_T:
		return new_bool(1);
	case tok_array:
		val = make_cons_tree2(0);
		val.ptr = new_cons_array(val);
	case tok_symbol:
		if (is_head_of_list) {
			val.ptr = new_func(token_str, NULL);
		} else {
			val.ptr = new_variable(token_str);
		}
		break;
	case tok_close:
		break;
	default:
		EXCEPTION("Unexpected token!!\n");
		break;
	}
	return val;
}

static val_t make_cons_list() {
	val_t val = null_val();
	val.ptr = new_open();
	val_t tmp = val;
	val_t car = null_val();
	get_next_token();
	tmp.ptr->car = make_cons_tree2(1);
	if (IS_NULL(tmp.ptr->car)) {
	//if (tmp.ptr->car.ivalue == 0) {
		//assert(0);
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
		if (IS_NULL(car)) {
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
		val_t root = null_val();
		root.ptr = new_open();
		cstack_cons_cell_push(root.ptr);
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
	jmp_buf buf;
	val_t null_value = null_val();
	loop_frame_push(&buf, null_value);
	int jmp = 0;
	if ((jmp = setjmp(buf)) == 0) {
		val_t cons = make_cons_tree2(0);
		codegen(cons);
		loop_frame_t *frame = loop_frame_pop();
		FREE(frame);
		return 0;
	} else {
		fprintf(stderr, "Syntax Error\n");
		return 1;
	}
}
