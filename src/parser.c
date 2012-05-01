#include "lisp.h"
static int token_type;
static char* current_char;
static char* token_str;
static int token_int;
static float token_float;

int is_open_space_close(char c) {
	return c == '(' || c == ' ' || c == ')' || c == '\n' || c == '\t' || c == '\0';
}

float tokenize_float(int start, int alt) {
	float res = (float)start;
	float f = 0.1;
	while (isdigit(*current_char)) {
		res += f * (*current_char - 48);
		f *= 0.1;
		current_char++;
	}
	return res * alt;
}

char *string_toupper(char *c, int size) {
	int i = 0;
	for (; i < size; i++) {
		if (c[i] >= 'a' && c[i] <= 'z') {
			c[i] = toupper(c[i]);
		}
	}
	return c;
}
int skip_line() {
	int len = strlen(current_char);
	int i = 0;
	while ((*current_char != '\n') && i < len ) {
		current_char++;
		i++;
	}
	return i == len;
}
int get_next_token_inner (string_buffer_t *buffer)
{
    int alt = 1;
    token_int = 0;
L_start:
    while (isspace(*current_char) || *current_char == '\n' || *current_char == '\t'){
        current_char++;
    }
    if (*current_char == '\0'){
        return tok_eof;
    }
	if (*current_char == ';') {
		if (skip_line()) { //end of string
			return tok_eof;
		}
		current_char++;
		goto L_start;
	}
	if (*current_char == '\"') {
		current_char++;
		while (*current_char != '\"') {
			if (*current_char == '\0') {
				return tok_error;
			}
			string_buffer_append_c(buffer, *current_char);
			current_char++;
		}
		current_char++;
		token_str = string_buffer_to_string(buffer);
		return tok_string;
	}
    if (isdigit(*current_char) || (*current_char == '-' && isdigit(*(current_char + 1)))){
        if (*current_char == '-'){
            alt = -1;
            current_char++;
        }
        while (isdigit(*current_char)){
            token_int = 10 * token_int + (*current_char - 48);
            current_char++;
        }
		if (*current_char == '.') {
			current_char++;
			if (isdigit(*current_char)) {
				token_float = tokenize_float(token_int, alt);
				return tok_float;
			} else {
				return tok_int;
			}
		}
        token_int *= alt;
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
		string_toupper(buffer->str, buffer->size);
		if (buffer->size == 3 && strncmp(buffer->str, "NIL", 3) == 0) {
			return tok_nil;
		}
		if (buffer->size == 1 && strncmp(buffer->str, "T", 1) == 0) {
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
		if (current_char[0] == ' ' || current_char[0] == '\n' || current_char[0] == '\t') {
			return tok_dot;
		} else if (isdigit(current_char[0])) {
			token_float = tokenize_float(0, 1);
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
	return tok_error; //unreachable
} 

void get_next_token ()
{
	string_buffer_t *buffer = new_string_buffer();
    token_type = get_next_token_inner(buffer);
	string_buffer_free(buffer);
	if (token_type == tok_symbol) {
		token_str = string_toupper(token_str, strlen(token_str));
	} else if (token_type == tok_error) {
		EXCEPTION("Invalid token!!\n");
	}
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
		get_next_token();
		val = make_cons_tree2(0);
		if (!IS_OPEN(val) && !IS_nil(val)) {
			EXCEPTION("Expected '(' or nil after '#' !!\n");
		}
		val.ptr = new_cons_array(val);
		break;
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
	cstack_cons_cell_push(val.ptr);
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
				EXCEPTION("Expected \')\'!!\n");
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
	cstack_cons_cell_pop();
	return val;
}

static val_t make_cons_tree2(int is_head_of_list) {
	if (token_type == tok_open) {
		return make_cons_list();
	}else if (token_type == tok_quote) {	
		val_t root = null_val();
		root.ptr = new_open();
		cstack_cons_cell_push(root.ptr);
		char *qstr = (char*)malloc(6);
		qstr[0] = 'Q';qstr[1] = 'U';qstr[2] = 'O';
		qstr[3] = 'T';qstr[4] = 'E';qstr[5] = '\0';
		root.ptr->car.ptr = new_func(qstr, NULL);
		//root.ptr->car.ptr = new_func("QUOTE", NULL);
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
	if (str) {
		tokenizer_init(str);
	}
	jmp_buf buf;
	val_t null_value = null_val();
	loop_frame_push(&buf, null_value);
	if (setjmp(buf) == 0) {
		get_next_token();
		if (token_type == tok_eof) {
			return 1;
		}
		val_t cons = make_cons_tree2(0);
		if (IS_NULL(cons)) {
			EXCEPTION("Unexpected ')' !!!!\n");
		}
		codegen(cons);
		loop_frame_t *frame = loop_frame_pop();
		FREE(frame);
		return 0;
	} else {
		return 1;
	}
}
