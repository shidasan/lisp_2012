#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <setjmp.h>
#include "lisp.h"

typedef struct loop_frame_t {
	jmp_buf *buf;
	cons_t *block_name;
}loop_frame_t;

array_t *loop_frame_list = NULL;
static cons_t *loop_return_value = NULL;

static void loop_frame_push(jmp_buf *buf, cons_t *block_name) {
	loop_frame_t *frame = (loop_frame_t*)malloc(sizeof(loop_frame_t));
	frame->buf = buf;
	frame->block_name = block_name;
	array_add(loop_frame_list, frame);
}

static loop_frame_t *loop_frame_pop() {
	loop_frame_t *frame = array_pop(loop_frame_list);
	return frame;
}

static cons_t *print(cons_t **VSTACK, int ARGC) {
	cons_t *cons = ARGS(0);
	print_return_value(cons);
	//if (cons->type == OPEN) {
	//	printf("(");
	//}
	//CONS_PRINT(cons, _buffer);
	//if (cons->type == OPEN) {
	//	printf(")");
	//}
	printf("\n");
	return cons;
}

static cons_t *format(cons_t **VSTACK, int ARGC, array_t *a) {
	int length = array_size(a), location = 0, evaluate = 2;
	if (length < 2) {
		EXCEPTION("Too few arguments!!\n");
	}
	cons_t *destination = vm_exec(2, (opline_t*)array_get(a, 0), VSTACK);
	switch (destination->type) {
		case T:
		case nil:
			break;
		default:
			EXCEPTION("Invalid file stream!!\n");
			break;
	}
	cons_t *format_string = vm_exec(2, (opline_t*)array_get(a, 1), VSTACK);
	if (format_string->type != STRING) {
		EXCEPTION("The control-string must be a string!!\n");
	}
	const char *str = format_string->str;
	int str_len = strlen(str);
	string_buffer_t *buffer = new_string_buffer();
	cons_t *cons = NULL;
	while (location <= str_len) {
		switch(str[location]) {
			case '~':
				switch(str[location+1]) {
					case '%':
						string_buffer_append_s(buffer, "\n");
						location += 2;
						break;

					case 'A':
						if (evaluate >= length) {
							EXCEPTION("There are not enough arguments left for format directive!!\n");
						}
						cons = vm_exec(2, (opline_t*)array_get(a, evaluate), VSTACK);
						CONS_TO_STRING(cons, buffer);
						location += 2;
						evaluate++;
						break;

					case 'C':
						if (evaluate >= length) {
							EXCEPTION("There are not enough arguments left for format directive!!\n");
						}
						cons = vm_exec(2, (opline_t*)array_get(a, evaluate), VSTACK);
						if (cons->type == STRING) {
							string_buffer_append_s(buffer, "\"");
						}
						CONS_TO_STRING(cons, buffer);
						if (cons->type == STRING) {
							string_buffer_append_s(buffer, "\"");
						}
						location += 2;
						evaluate++;
						break;

					default:
						string_buffer_append_c(buffer, str[location]);
						location++;
						break;
				}
				break;
			default:
				string_buffer_append_c(buffer, str[location]);
				location++;
		}
	}
	for (; evaluate < length; evaluate++) {
		cons = vm_exec(2, (opline_t*)array_get(a, evaluate), VSTACK);
	}
	cons_t *res = NULL;
	char *res_str = string_buffer_to_string(buffer);
	if (destination->type == nil) {
		res = new_string(res_str);
	} else {
		fprintf(stdout, "%s", res_str);
		res = new_bool(0);
	}
	string_buffer_free(buffer);
	FREE(res_str);
	return res;
}

static cons_t *car(cons_t** VSTACK, int ARGC) {
	cons_t *cons = ARGS(0);
	if (cons->type != OPEN) {
		fprintf(stderr, "expected list!!\n");
		TODO("exception\n");
	}
	return cons->car;
}

static cons_t *cdr(cons_t** VSTACK, int ARGC) {
	cons_t *cons = ARGS(0);
	if (cons->type != OPEN) {
		fprintf(stderr, "expected list!!\n");
		TODO("exception\n");
	}
	return cons->cdr;
}

static cons_t *cons(cons_t** VSTACK, int ARGC) {
	cons_t *car = ARGS(0);
	cons_t *cdr = ARGS(1);
	cons_t *cons = new_open();
	cons->car = car;
	cons->cdr = cdr;
	return cons;
}

static cons_t *add(cons_t** VSTACK, int ARGC) {
	int i, res = 0;
	for (i = 0; i < ARGC; i++) {
		cons_t *cons = ARGS(i);
		if (cons->type != INT) {
			fprintf(stderr, "type error!!\n");
			TODO("exception\n");
		}
		res += cons->ivalue;
	}
	return new_int(res);
}

static cons_t *sub(cons_t** VSTACK, int ARGC) {
	int i, res = 0;
	for (i = 0; i < ARGC; i++) {
		cons_t *cons = ARGS(i);
		if (cons->type != INT) {
			fprintf(stderr, "type error!!\n");
			TODO("exception\n");
		}
		if (i == 0) {
			res = cons->ivalue;
		} else {
			res -= cons->ivalue;
		}
	}
	return new_int(res);
}

static cons_t *mul(cons_t** VSTACK, int ARGC) {
	int i, res = 1;
	for (i = 0; i < ARGC; i++) {
		cons_t *cons = ARGS(i);
		if (cons->type != INT) {
			fprintf(stderr, "type error!!\n");
			TODO("exception\n");
		}
		res *= cons->ivalue;
	}
	return new_int(res);
}

static cons_t *_div(cons_t** VSTACK, int ARGC) {

}

static cons_t *lt(cons_t** VSTACK, int ARGC) {
	int i;
	int current_number = ARGS(0)->ivalue;
	for (i = 1; i < ARGC; i++) {
		cons_t *cons = ARGS(i);
		if (cons->type != INT) {
			fprintf(stderr, "type error!\n");
			TODO("exception\n");
		}
		int next_number = cons->ivalue;
		if (current_number >= next_number) {
			return new_bool(0);
		}
		current_number = next_number;
	}
	return new_bool(1);
}

static cons_t *string_lt(cons_t** VSTACK, int ARGC) {
	int i;
	const char* current_str = ARGS(0)->str;
	for (i = 1; i < ARGC; i++) {
		cons_t *cons = ARGS(i);
		if (cons->type != STRING) {
			fprintf(stderr, "type error!\n");
			TODO("exception\n");
		}
		const char  *next_str = cons->str;
		if (strcmp(current_str, next_str) >= 0) {
			return new_bool(0);
		}
		current_str = next_str;
	}
	return new_bool(1);
}

static cons_t *lte(cons_t **VSTACK, int ARGC) {
	int i;
	int current_number = ARGS(0)->ivalue;
	for (i = 1; i < ARGC; i++) {
		cons_t *cons = ARGS(i);
		if (cons->type != INT) {
			fprintf(stderr, "type error!\n");
			TODO("exception\n");
		}
		int next_number = cons->ivalue;
		if (current_number > next_number) {
			return new_bool(0);
		}
		current_number = next_number;
	}
	return new_bool(1);
}

static cons_t *string_lte(cons_t** VSTACK, int ARGC) {
	int i;
	const char* current_str = ARGS(0)->str;
	for (i = 1; i < ARGC; i++) {
		cons_t *cons = ARGS(i);
		if (cons->type != STRING) {
			fprintf(stderr, "type error!\n");
			TODO("exception\n");
		}
		const char  *next_str = cons->str;
		if (strcmp(current_str, next_str) > 0) {
			return new_bool(0);
		}
		current_str = next_str;
	}
	return new_bool(1);
}

static cons_t *gt(cons_t** VSTACK, int ARGC) {
	int i;
	int current_number = ARGS(0)->ivalue;
	for (i = 1; i < ARGC; i++) {
		cons_t *cons = ARGS(i);
		if (cons->type != INT) {
			fprintf(stderr, "type error!\n");
			TODO("exception\n");
		}
		int next_number = cons->ivalue;
		if (current_number <= next_number) {
			return new_bool(0);
		}
		current_number = next_number;
	}
	return new_bool(1);
}

static cons_t *string_gt(cons_t** VSTACK, int ARGC) {
	int i;
	const char* current_str = ARGS(0)->str;
	for (i = 1; i < ARGC; i++) {
		cons_t *cons = ARGS(i);
		if (cons->type != STRING) {
			fprintf(stderr, "type error!\n");
			TODO("exception\n");
		}
		const char  *next_str = cons->str;
		if (strcmp(current_str, next_str) <= 0) {
			return new_bool(0);
		}
		current_str = next_str;
	}
	return new_bool(1);
}

static cons_t *gte(cons_t** VSTACK, int ARGC) {
	int i;
	int current_number = ARGS(0)->ivalue;
	for (i = 1; i < ARGC; i++) {
		cons_t *cons = ARGS(i);
		if (cons->type != INT) {
			fprintf(stderr, "type error!\n");
			TODO("exception\n");
		}
		int next_number = cons->ivalue;
		if (current_number < next_number) {
			return new_bool(0);
		}
		current_number = next_number;
	}
	return new_bool(1);
}

static cons_t *string_gte(cons_t** VSTACK, int ARGC) {
	int i;
	const char* current_str = ARGS(0)->str;
	for (i = 1; i < ARGC; i++) {
		cons_t *cons = ARGS(i);
		if (cons->type != STRING) {
			fprintf(stderr, "type error!\n");
			TODO("exception\n");
		}
		const char  *next_str = cons->str;
		if (strcmp(current_str, next_str) < 0) {
			return new_bool(0);
		}
		current_str = next_str;
	}
	return new_bool(1);
}

static cons_t *eq(cons_t** VSTACK, int ARGC) {
	int i;
	int current_number = ARGS(0)->ivalue;
	for (i = 1; i < ARGC; i++) {
		cons_t *cons = ARGS(i);
		if (cons->type != INT) {
			fprintf(stderr, "type error!\n");
			TODO("exception\n");
		}
		int next_number = cons->ivalue;
		if (current_number != next_number) {
			return new_bool(0);
		}
		current_number = next_number;
	}
	return new_bool(1);
}

static cons_t *string_eq(cons_t** VSTACK, int ARGC) {
	int i;
	const char* current_str = ARGS(0)->str;
	for (i = 1; i < ARGC; i++) {
		cons_t *cons = ARGS(i);
		if (cons->type != STRING) {
			fprintf(stderr, "type error!\n");
			TODO("exception\n");
		}
		const char  *next_str = cons->str;
		if (strcmp(current_str, next_str) != 0) {
			return new_bool(0);
		}
		current_str = next_str;
	}
	return new_bool(1);
}

static cons_t *neq(cons_t** VSTACK, int ARGC) {
	int i;
	int current_number = ARGS(0)->ivalue;
	for (i = 1; i < ARGC; i++) {
		cons_t *cons = ARGS(i);
		if (cons->type != INT) {
			fprintf(stderr, "type error!\n");
			TODO("exception\n");
		}
		int next_number = cons->ivalue;
		if (current_number == next_number) {
			return new_bool(0);
		}
		current_number = next_number;
	}
	return new_bool(1);
}

static cons_t *string_neq(cons_t** VSTACK, int ARGC) {
	int i;
	const char* current_str = ARGS(0)->str;
	for (i = 1; i < ARGC; i++) {
		cons_t *cons = ARGS(i);
		if (cons->type != STRING) {
			fprintf(stderr, "type error!\n");
			TODO("exception\n");
		}
		const char  *next_str = cons->str;
		if (strcmp(current_str, next_str) == 0) {
			return new_bool(0);
		}
		current_str = next_str;
	}
	return new_bool(1);
}

static cons_t *zerop(cons_t **VSTACK, int ARGC) {
	cons_t *cons = ARGS(0);
	if (cons->type == INT && cons->ivalue == 0) {
		return new_bool(1);
	}
	return new_bool(0);
}

static cons_t *null(cons_t **VSTACK, int ARGC) {
	return new_bool(ARGS(0)->type == nil);
}

static cons_t *not(cons_t **VSTACK, int ARGC) {
	return new_bool(ARGS(0)->type == nil);
}

static cons_t *atom(cons_t **VSTACK, int ARGC) {
	cons_t *cons = ARGS(0);
	return new_bool((cons->type != OPEN));
}
static cons_t *quote(cons_t ** VSTACK, int ARGC) {
	cons_t *cons = ARGS(0);
	return cons;
}

static cons_t *list(cons_t ** VSTACK, int ARGC) {
	int i;
	if (ARGC == 0) {
		return new_bool(0);
	}
	cons_t *res = new_open();
	cstack_cons_cell_push(res);
	cons_t *tmp = res;
	for (i = 0; i < ARGC; i++) {
		tmp->car = ARGS(i);
		if (i == ARGC - 1) {
			tmp->cdr = new_bool(0);
		} else {
			tmp->cdr = new_open();
			tmp = tmp->cdr;
		}
	}
	cstack_cons_cell_pop();
	return res;
}

static cons_t *length(cons_t **VSTACK, int ARGC) {
	cons_t *cons = ARGS(0);
	if (cons->type == nil) {
		return new_int(0);
	} else if (cons->type == STRING) {
		return new_int(strlen(cons->str));
	}
	if (cons->type != OPEN) {
		EXCEPTION("Not a list!!\n");
	}
	int res = 0;
	while (cons->type == OPEN) {
		res++;
		cons = cons->cdr;
	}
	if (cons->type != nil) {
		EXCEPTION("Not a list!!\n");
	}
	return new_int(res);
}

static cons_t *_if(cons_t **VSTACK, int ARGC, struct array_t *a) {
	cons_t *cons = vm_exec(2, (opline_t*)array_get(a, 0), VSTACK);
	cons_t *res = NULL;
	if (cons->type != nil) {
		res = vm_exec(2, (opline_t*)array_get(a, 1), VSTACK);
	} else {
		res = vm_exec(2, (opline_t*)array_get(a, 2), VSTACK);
	}
	return res;
}

static cons_t *_assert(cons_t **VSTACK, int ARGC, array_t *a) {
	cons_t *cons = vm_exec(2, (opline_t*)array_get(a, 0), VSTACK);
	if (cons->type == nil) {
		EXCEPTION("NIL must evalueate to a non-NIL value\n");
		assert(0);
	}
	return cons;
}

static cons_t *cond(cons_t **VSTACK, int ARGC, array_t *a) {
	int size = array_size(a), i = 0;
	if (size == 0) {
		return new_bool(0);
	}
	cons_t *res = NULL;
	for (; i < size; i++) {
		cons_t *cons = vm_exec(2, (opline_t*)array_get(a, i), VSTACK);
		VSTACK[1] = cons;
		int _length = length(VSTACK+1, 1)->ivalue;
		if (_length == 0) {
			EXCEPTION("clause NIL should be a list");
		}
		cons_t *car = cons->car;
		if (i == size-1 && (car->type == FUNC || car->type == VARIABLE) 
				&& strcmp(car->str, "otherwise") == 0) {
			/* default */
		} else {
			codegen(cons->car);
			cons_t *res = vm_exec(2, memory+CurrentIndex, VSTACK);
			if (res->type == nil) {
				continue;
			}
		}
		int j = 1;
		cons_t *cdr = cons->cdr;
		car = cdr->car;
		for (; j < _length; j++) {
			codegen(car);
			res = vm_exec(2, memory+CurrentIndex, VSTACK);
			cdr = cdr->cdr;
			car = cdr->car;
		}
		return res;
	}
	return new_bool(0);
}

static cons_t *progn(cons_t **VSTACK, int ARGC, array_t *a) {
	int size = array_size(a), i = 0;
	cons_t *res = NULL;
	for (; i < size; i++) {
		res = vm_exec(2, (opline_t*)array_get(a, i), VSTACK);
	}
	return res;
}

static cons_t *loop(cons_t **VSTACK, int ARGC, array_t *a) {
	int size = array_size(a), i = 0;
	cons_t *res = NULL;
	jmp_buf buf;
	if (loop_frame_list == NULL) {
		loop_frame_list = new_array();
	}
	VSTACK[0] = new_bool(0);
	loop_frame_push(&buf, VSTACK[0]);
	int jmp = 0;
	if ((jmp = setjmp(buf)) == 0) {
		while (1) {
			i = 0;
			for (; i < size; i++) {
				res = vm_exec(2, (opline_t*)array_get(a, i), VSTACK + 1);
			}
		}
	}
	res = loop_return_value;
	loop_return_value = NULL;
	return res;
}

static cons_t *block(cons_t **VSTACK, int ARGC, array_t *a) {
	int size = array_size(a);
	if (size == 0) {
		EXCEPTION("Too few arguments!!\n");
	} else if (size == 1) {
		return new_bool(0);
	}
	cons_t *res = NULL;
	jmp_buf buf;
	if (loop_frame_list == NULL) {
		loop_frame_list = new_array();
	}
	cons_t *block_name = vm_exec(2, (opline_t*)array_get(a, 0), VSTACK);
	if (block_name->type != nil && block_name->type != T && block_name->type != FUNC && block_name->type != VARIABLE) {
		EXCEPTION("Excepted symbol!!\n");
	}
	loop_frame_push(&buf, block_name);
	int jmp = 0;
	if ((jmp = setjmp(buf)) == 0) {
		int i = 1; 
		for (; i < size; i++) {
			res = vm_exec(2, (opline_t*)array_get(a, i), VSTACK + 1);
		}
		loop_frame_t *frame = loop_frame_pop();
		FREE(frame);
	} else {
		res = loop_return_value;
		loop_return_value = NULL;
	}
	return res;
}

static cons_t *_return(cons_t **VSTACK, int ARGC) {
	if (ARGC > 1) {
		EXCEPTION("too many arguments!!\n");
	}
	loop_frame_t *frame = NULL;
	while ((frame = loop_frame_pop()) != NULL) {
		jmp_buf *buf = frame->buf;
		cons_t *block_name = frame->block_name;
		free(frame);
		if (block_name->type == nil) {
			loop_return_value = (ARGC == 0) ? new_bool(0) : ARGS(0);
			longjmp(*buf, 1);
		}
	}
	EXCEPTION("No block found!!\n");
}

static cons_t *_return_from(cons_t **VSTACK, int ARGC) {
	if (ARGC != 1 && ARGC != 2) {
		EXCEPTION("Illegal number of arguments!!\n");
	}
	loop_frame_t *frame = NULL;
	cons_t *args0 = ARGS(0);
	while ((frame = loop_frame_pop()) != NULL) {
		jmp_buf *buf = frame->buf;
		cons_t *block_name = frame->block_name;
		free(frame);
		if (block_name->type == nil && args0->type == nil ||
			block_name->type == T && args0->type == T ||
			strcmp(block_name->str, args0->str) == 0) {
			loop_return_value = (ARGC == 1) ? new_bool(0) : ARGS(1);
			longjmp(*buf, 1);
		}
	}
	EXCEPTION("No block found!!\n");
}

static cons_t *when(cons_t **VSTACK, int ARGC, array_t *a) {
	int size = array_size(a);
	if (size == 0) {
		EXCEPTION("argument length does not match!!\n");
	}
	cons_t *res = vm_exec(2, (opline_t*)array_get(a, 0), VSTACK);
	if (res->type == nil || size == 1) {
		return new_bool(0);
	}
	int i = 1;
	for (; i < size; i++) {
		res = vm_exec(2, (opline_t*)array_get(a, i), VSTACK);
	}
	return res;
}

static cons_t *unless(cons_t **VSTACK, int ARGC, array_t *a) {
	int size = array_size(a);
	if (size == 0) {
		EXCEPTION("argument length does not match!!\n");
	}
	cons_t *res = vm_exec(2, (opline_t*)array_get(a, 0), VSTACK);
	if (res->type == T || size == 1) {
		return new_bool(0);
	}
	int i = 1;
	for (; i < size; i++) {
		res = vm_exec(2, (opline_t*)array_get(a, i), VSTACK);
	}
	return res;
}

static cons_t *defun(cons_t **VSTACK, int ARGC, struct array_t *a) {
	cons_t *fcons = vm_exec(2, (opline_t*)array_get(a, 0), VSTACK);
	cons_t *args = vm_exec(2, (opline_t*)array_get(a, 1), VSTACK);
	int i = 2;
	struct array_t *opline_list = new_array();
	for (; i < array_size(a); i++) {
		array_add(opline_list, memory + NextIndex);
		cons_t *fbody = vm_exec(2, (opline_t*)array_get(a, i), VSTACK + i);
		codegen(fbody);
	}
	VSTACK[1] = args;
	int argc = length(VSTACK+1, 1)->ivalue;
	set_func(fcons, opline_list, argc, args, current_environment, 0);
	return fcons;
}

static cons_t *defmacro(cons_t **VSTACK, int ARGC, array_t *a) {
	cons_t *fcons = vm_exec(2, (opline_t*)array_get(a, 0), VSTACK);
	cons_t *args = vm_exec(2, (opline_t*)array_get(a, 1), VSTACK);
	int i = 2;
	struct array_t *opline_list = new_array();
	for (; i < array_size(a); i++) {
		array_add(opline_list, memory + NextIndex);
		cons_t *fbody = vm_exec(2, (opline_t*)array_get(a, i), VSTACK + i);
		codegen(fbody);
	}
	VSTACK[1] = args;
	int argc = length(VSTACK+1, 1)->ivalue;
	set_func(fcons, opline_list, argc, args, current_environment, FLAG_MACRO | FLAG_SPECIAL_FORM);
	return fcons;
}

static cons_t *setq(cons_t **VSTACK, int ARGC) {
	cons_t *variable = ARGS(0);
	cons_t *value = ARGS(1);
	if (variable->type != VARIABLE || value->type == VARIABLE || value->type == FUNC) {
		EXCEPTION("Not a atom!!\n");
	}
	set_variable(variable, value, 0);
	return value;
}

static cons_t *let(cons_t **VSTACK, int ARGC, struct array_t *a) {
	cons_t *value_list = vm_exec(2, (opline_t*)array_get(a, 0), VSTACK);
	cons_t *variable = NULL;
	cons_t *list = NULL;
	cons_t *value = NULL;
	if (value_list->type == OPEN) {
		list = value_list->car;
		while (list != NULL && list->type != nil) {
			if (list->type == OPEN) {
				VSTACK[1] = list;
				int argc = length(VSTACK+2, 1)->ivalue;
				if (argc == 2) {
					variable = list->car;
					value = list->cdr->car;
					codegen(value);
					cons_t *res = vm_exec(2, memory + CurrentIndex, VSTACK + 1);
					set_variable(variable, res, 1);
				} else {
					fprintf(stderr, "illegal variable specification!! %d\n", argc);
					assert(0);
				}
			} else {
				cons_t *p = set_variable(list, new_bool(0), 1);
			}
			value_list = value_list->cdr;
			list = value_list->car;
		}
	}
	cons_t *res = NULL;
	int i;
	for (i = 1; i < array_size(a); i++) {
		res = vm_exec(2, (opline_t*)array_get(a, i), VSTACK + i);
	}
	return res;
}

static cons_t *eval(cons_t **VSTACK, int ARGC) {
	//cons_t *cons = CONS_EVAL(ARGS(0));
	cons_t *cons = ARGS(0);
	codegen(cons);
	cons_t *res = vm_exec(2, memory + CurrentIndex, VSTACK + 1);
	return res;
}

/*
   typedef struct static_mtd_data {
   const char *name;
   int num_args;
   int creates_local_scope;
   int is_special_form;
   int is_quote0;
   int is_quote1;
   cons_t *(*mtd)(cons_t**, int);
   cons_t *(*special_mtd)(cons_t**, int, struct array_t*);
   } static_mtd_data;
   */

static_mtd_data static_mtds[] = {
	{"print", 1, 0, 0, 0, print, NULL},
	{"format", -1, FLAG_SPECIAL_FORM, 0, 0, NULL, format},
	{"car", 1, 0, 0, 0, car, NULL},
	{"cdr", 1, 0, 0, 0, cdr, NULL},
	{"cons", 2, 0, 0, 0, cons, NULL},
	{"+", -1, 0, 0, 0, add, NULL},
	{"-", -1, 0, 0, 0, sub, NULL},
	{"*", -1, 0, 0, 0, mul, NULL},
	{"/", -1, 0, 0, 0, _div, NULL},
	{"<", -1, 0, 0, 0, lt, NULL},
	{"string<", -1, 0, 0, 0, string_lt, NULL},
	{"<=", -1, 0, 0, 0, lte, NULL},
	{"string<=", -1, 0, 0, 0, string_lte, NULL},
	{">", -1, 0, 0, 0, gt, NULL},
	{"string>", -1, 0, 0, 0, string_gt, NULL},
	{">=", -1, 0, 0, 0, gte, NULL},
	{"string>=", -1, 0, 0, 0, string_gte, NULL},
	{"=", -1, 0, 0, 0, eq, NULL},
	{"string=", -1, 0, 0, 0, string_eq, NULL},
	{"/=", -1, 0, 0, 0, neq, NULL},
	{"string/=", -1, 0, 0, 0, string_neq, NULL},
	{"zerop", 1, 0, 0, 0, zerop, NULL},
	{"null", 1, 0, 0, 0, null, NULL},
	{"not", 1, 0, 0, 0, not, NULL},
	{"atom", 1, 0, 0, 0, atom, NULL},
	{"quote", 1, 0, 1, 1, quote, NULL},
	{"list", -1, 0, 0, 0, list, NULL},
	{"length", 1, 0, 0, 0, length, NULL},
	{"if", 3, FLAG_SPECIAL_FORM, 0, 0, NULL, _if},
	{"assert", 1, FLAG_SPECIAL_FORM, 0, 0, NULL, _assert},
	{"cond", -1, FLAG_SPECIAL_FORM, -1, 0, NULL, cond},
	{"progn", -1, FLAG_SPECIAL_FORM, 0, 0, NULL, progn},
	{"loop", -1, FLAG_SPECIAL_FORM, 0, 0, NULL, loop},
	{"block", -1, FLAG_SPECIAL_FORM, 1, 0, NULL, block},
	{"return", -1, 0, 0, 0, _return, NULL},
	{"return-from", -1, 0, 1, 0, _return_from, NULL},
	{"when", -1, FLAG_SPECIAL_FORM, 0, 0, NULL, when},
	{"unless", -1, FLAG_SPECIAL_FORM, 0, 0, NULL, unless},
	{"defun", -1, FLAG_SPECIAL_FORM | FLAG_LOCAL_SCOPE, -1, 2, NULL, defun},
	{"defmacro", -1,FLAG_SPECIAL_FORM | FLAG_LOCAL_SCOPE, -1, 0, NULL, defun},
	{"setq", 2, 0, 1, 0, setq, NULL},
	{"let", -1, FLAG_SPECIAL_FORM | FLAG_LOCAL_SCOPE, 1, 0, NULL, let},
	{"eval", 1, 0, 0, 0, eval, NULL},
	{NULL, 0, 0, 0, 0, NULL, NULL},
};
