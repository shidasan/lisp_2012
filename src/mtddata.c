#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <setjmp.h>
#include "lisp.h"

typedef struct loop_frame_t {
	jmp_buf *buf;
	val_t block_name;
}loop_frame_t;

array_t *loop_frame_list = NULL;
static val_t loop_return_value;

static void loop_frame_push(jmp_buf *buf, val_t block_name) {
	loop_frame_t *frame = (loop_frame_t*)malloc(sizeof(loop_frame_t));
	frame->buf = buf;
	frame->block_name = block_name;
	array_add(loop_frame_list, frame);
}

static loop_frame_t *loop_frame_pop() {
	loop_frame_t *frame = array_pop(loop_frame_list);
	return frame;
}

static val_t print(val_t *VSTACK, int ARGC) {
	val_t cons = ARGS(0);
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

static val_t format(val_t *VSTACK, int ARGC, array_t *a) {
	int length = array_size(a), location = 0, evaluate = 2;
	if (length < 2) {
		EXCEPTION("Too few arguments!!\n");
	}
	val_t destination = vm_exec(2, (opline_t*)array_get(a, 0), VSTACK);
	switch (VAL_TYPE(destination)) {
		case T_OFFSET:
		case nil_OFFSET:
			break;
		default:
			EXCEPTION("Invalid file stream!!\n");
			break;
	}
	val_t format_string = vm_exec(2, (opline_t*)array_get(a, 1), VSTACK);
	if (IS_UNBOX(format_string) || format_string.ptr->type != STRING) {
		EXCEPTION("The control-string must be a string!!\n");
	}
	const char *str = format_string.ptr->str;
	int str_len = strlen(str);
	string_buffer_t *buffer = new_string_buffer();
	val_t val = {0, 0};
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
						val = vm_exec(2, (opline_t*)array_get(a, evaluate), VSTACK);
						VAL_TO_STRING(val, buffer);
						location += 2;
						evaluate++;
						break;

					case 'C':
						if (evaluate >= length) {
							EXCEPTION("There are not enough arguments left for format directive!!\n");
						}
						val = vm_exec(2, (opline_t*)array_get(a, evaluate), VSTACK);
						if (IS_UNBOX(val) && val.ptr->type == STRING) {
							string_buffer_append_s(buffer, "\"");
						}
						VAL_TO_STRING(val, buffer);
						if (IS_UNBOX(val) && val.ptr->type == STRING) {
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
		val = vm_exec(2, (opline_t*)array_get(a, evaluate), VSTACK);
	}
	val_t res = {0, 0};
	char *res_str = string_buffer_to_string(buffer);
	if (IS_nil(destination)) {
		res.ptr = new_string(res_str);
	} else {
		fprintf(stdout, "%s", res_str);
		res = new_bool(0);
	}
	string_buffer_free(buffer);
	FREE(res_str);
	return res;
}

static val_t car(val_t* VSTACK, int ARGC) {
	val_t val = ARGS(0);
	if (!IS_OPEN(val)) {
		fprintf(stderr, "expected list!!\n");
		TODO("exception\n");
	}
	return val.ptr->car;
}

static val_t cdr(val_t* VSTACK, int ARGC) {
	val_t val = ARGS(0);
	if (!IS_OPEN(val)) {
		fprintf(stderr, "expected list!!\n");
		TODO("exception\n");
	}
	return val.ptr->cdr;
}

static val_t cons(val_t* VSTACK, int ARGC) {
	val_t car = ARGS(0);
	val_t cdr = ARGS(1);
	val_t val = {0, 0};
	val.ptr = new_open();
	val.ptr->car = car;
	val.ptr->cdr = cdr;
	return val;
}

static val_t add_ii(val_t v0, val_t v1) {
	v0.ivalue += v1.ivalue;
	return v0;
}

static val_t add_if(val_t v0, val_t v1) {
	v1.fvalue += v0.ivalue;
	return v1;
}

static val_t add_fi(val_t v0, val_t v1) {
	v0.fvalue += v1.ivalue;
	return v0;
}

static val_t add_ff(val_t v0, val_t v1) {
	v0.fvalue += v1.fvalue;
	return v0;
}

static val_t (*add_op[])(val_t, val_t) = {add_ff, add_fi, add_if, add_ii};
static val_t add(val_t* VSTACK, int ARGC) {
	int i;
	val_t res = {0, INT_OFFSET};
	for (i = 0; i < ARGC; i++) {
		val_t val = ARGS(i);
		if (!IS_NUMBER(val)) {
			fprintf(stderr, "type error!!\n");
			TODO("exception\n");
		}
		res = add_op[IS_INT(res)*2+IS_INT(val)](res, val);
	}
	return res;
}

static val_t sub_ii(val_t v0, val_t v1) {
	v0.ivalue -= v1.ivalue;
	return v0;
}

static val_t sub_if(val_t v0, val_t v1) {
	v1.fvalue = v0.ivalue - v1.fvalue;
	return v1;
}

static val_t sub_fi(val_t v0, val_t v1) {
	v0.fvalue -= v1.ivalue;
	return v0;
}

static val_t sub_ff(val_t v0, val_t v1) {
	v0.fvalue -= v1.fvalue;
	return v0;
}

static val_t (*sub_op[])(val_t, val_t) = {sub_ff, sub_fi, sub_if, sub_ii};
static val_t sub(val_t* VSTACK, int ARGC) {
	int i;
	val_t res = ARGS(0);
	if (!IS_NUMBER(res)) {
		EXCEPTION("Excepted Number!!\n");
	}
	for (i = 1; i < ARGC; i++) {
		val_t val = ARGS(i);
		if (!IS_NUMBER(val)) {
			fprintf(stderr, "type error!!\n");
			TODO("exception\n");
		}
		res = sub_op[IS_INT(res)*2+IS_INT(val)](res, val);
	}
	return res;
}

static val_t mul_ii(val_t v0, val_t v1) {
	v0.ivalue *= v1.ivalue;
	return v0;
}

static val_t mul_if(val_t v0, val_t v1) {
	v1.fvalue = v0.ivalue * v1.fvalue;
	return v1;
}

static val_t mul_fi(val_t v0, val_t v1) {
	v0.fvalue *= v1.ivalue;
	return v0;
}

static val_t mul_ff(val_t v0, val_t v1) {
	v0.fvalue *= v1.fvalue;
	return v0;
}

static val_t (*mul_op[])(val_t, val_t) = {mul_ff, mul_fi, mul_if, mul_ii};
static val_t mul(val_t* VSTACK, int ARGC) {
	int i;
	val_t res = {1, INT_OFFSET};
	for (i = 0; i < ARGC; i++) {
		val_t val = ARGS(i);
		if (!IS_NUMBER(val)) {
			fprintf(stderr, "type error!!\n");
			TODO("exception\n");
		}
		res = mul_op[IS_INT(res)*2+IS_INT(val)](res, val);
	}
	return res;
}

static val_t _div(val_t* VSTACK, int ARGC) {

}

static int lt_ii(val_t v0, val_t v1) {
	return v0.ivalue >= v1.ivalue;
}

static int lt_if(val_t v0, val_t v1) {
	return v0.ivalue >= v1.fvalue;
}

static int lt_fi(val_t v0, val_t v1) {
	return v0.fvalue >= v1.ivalue;
}

static int lt_ff(val_t v0, val_t v1) {
	return v0.fvalue >= v1.fvalue;
}

static int (*lt_op[])(val_t, val_t) = {lt_ff, lt_fi, lt_if, lt_ii};

static val_t lt(val_t* VSTACK, int ARGC) {
	int i;
	val_t val = ARGS(0);
	if (!IS_NUMBER(val)) {
		EXCEPTION("Excepted Int!!\n");
	}
	val_t current_number = val;
	for (i = 1; i < ARGC; i++) {
		val = ARGS(i);
		if (!IS_NUMBER(val)) {
			fprintf(stderr, "type error!\n");
			TODO("exception\n");
		}
		val_t next_number = val;
		if (lt_op[IS_INT(current_number)*2+IS_INT(next_number)](current_number, next_number)) {
			return new_bool(0);
		}
		current_number = next_number;
	}
	return new_bool(1);
}

static val_t string_lt(val_t* VSTACK, int ARGC) {
	int i;
	val_t val = ARGS(0);
	if (!IS_STRING(val)) {
		EXCEPTION("Excepted String!!\n");
	}
	const char* current_str = val.ptr->str;
	for (i = 1; i < ARGC; i++) {
		val = ARGS(i);
		if (!IS_STRING(val)) {
			fprintf(stderr, "type error!\n");
			TODO("exception\n");
		}
		const char  *next_str = val.ptr->str;
		if (strcmp(current_str, next_str) >= 0) {
			return new_bool(0);
		}
		current_str = next_str;
	}
	return new_bool(1);
}

static int lte_ii(val_t v0, val_t v1) {
	return v0.ivalue > v1.ivalue;
}

static int lte_if(val_t v0, val_t v1) {
	return v0.ivalue > v1.fvalue;
}

static int lte_fi(val_t v0, val_t v1) {
	return v0.fvalue > v1.ivalue;
}

static int lte_ff(val_t v0, val_t v1) {
	return v0.fvalue > v1.fvalue;
}

static int (*lte_op[])(val_t, val_t) = {lte_ff, lte_fi, lte_if, lte_ii};

static val_t lte(val_t *VSTACK, int ARGC) {
	int i;
	val_t val = ARGS(0);
	if (!IS_NUMBER(val)) {
		EXCEPTION("Excepted Int!!\n");
	}
	val_t current_number = val;
	for (i = 1; i < ARGC; i++) {
		val = ARGS(i);
		if (!IS_NUMBER(val)) {
			fprintf(stderr, "type error!\n");
			TODO("exception\n");
		}
		val_t next_number = val;
		if (lte_op[IS_INT(current_number)*2+IS_INT(next_number)](current_number, next_number)) {
			return new_bool(0);
		}
		current_number = next_number;
	}
	return new_bool(1);
}

static val_t string_lte(val_t* VSTACK, int ARGC) {
	int i;
	val_t val = ARGS(0);
	if (!IS_STRING(val)) {
		EXCEPTION("Excepted String!!\n");
	}
	const char* current_str = val.ptr->str;
	for (i = 1; i < ARGC; i++) {
		val = ARGS(i);
		if (!IS_STRING(val)) {
			fprintf(stderr, "type error!\n");
			TODO("exception\n");
		}
		const char  *next_str = val.ptr->str;
		if (strcmp(current_str, next_str) > 0) {
			return new_bool(0);
		}
		current_str = next_str;
	}
	return new_bool(1);
}

static int gt_ii(val_t v0, val_t v1) {
	return v0.ivalue <= v1.ivalue;
}

static int gt_if(val_t v0, val_t v1) {
	return v0.ivalue <= v1.fvalue;
}

static int gt_fi(val_t v0, val_t v1) {
	return v0.fvalue <= v1.ivalue;
}

static int gt_ff(val_t v0, val_t v1) {
	return v0.fvalue <= v1.fvalue;
}

static int (*gt_op[])(val_t, val_t) = {gt_ff, gt_fi, gt_if, gt_ii};

static val_t gt(val_t* VSTACK, int ARGC) {
	int i;
	val_t val = ARGS(0);
	if (!IS_NUMBER(val)) {
		EXCEPTION("Excepted Int!!\n");
	}
	val_t current_number = val;
	for (i = 1; i < ARGC; i++) {
		val = ARGS(i);
		if (!IS_NUMBER(val)) {
			fprintf(stderr, "type error!\n");
			TODO("exception\n");
		}
		val_t next_number = val;
		if (gt_op[IS_INT(current_number)*2+IS_INT(next_number)](current_number, next_number)) {
			return new_bool(0);
		}
		current_number = next_number;
	}
	return new_bool(1);
}

static val_t string_gt(val_t* VSTACK, int ARGC) {
	int i;
	val_t val = ARGS(0);
	if (!IS_STRING(val)) {
		EXCEPTION("Excepted String!!\n");
	}
	const char* current_str = val.ptr->str;
	for (i = 1; i < ARGC; i++) {
		val = ARGS(i);
		if (!IS_STRING(val)) {
			fprintf(stderr, "type error!\n");
			TODO("exception\n");
		}
		const char  *next_str = val.ptr->str;
		if (strcmp(current_str, next_str) <= 0) {
			return new_bool(0);
		}
		current_str = next_str;
	}
	return new_bool(1);
}

static int gte_ii(val_t v0, val_t v1) {
	return v0.ivalue < v1.ivalue;
}

static int gte_if(val_t v0, val_t v1) {
	return v0.ivalue < v1.fvalue;
}

static int gte_fi(val_t v0, val_t v1) {
	return v0.fvalue < v1.ivalue;
}

static int gte_ff(val_t v0, val_t v1) {
	return v0.fvalue < v1.fvalue;
}

static int (*gte_op[])(val_t, val_t) = {gte_ff, gte_fi, gte_if, gte_ii};

static val_t gte(val_t* VSTACK, int ARGC) {
	int i;
	val_t val = ARGS(0);
	if (!IS_NUMBER(val)) {
		EXCEPTION("Excepted Int!!\n");
	}
	val_t current_number = val;
	for (i = 1; i < ARGC; i++) {
		val = ARGS(i);
		if (!IS_NUMBER(val)) {
			fprintf(stderr, "type error!\n");
			TODO("exception\n");
		}
		val_t next_number = val;
		if (gte_op[IS_INT(current_number)*2+IS_INT(next_number)](current_number, next_number)) {
			return new_bool(0);
		}
		current_number = next_number;
	}
	return new_bool(1);
}

static val_t string_gte(val_t* VSTACK, int ARGC) {
	int i;
	val_t val = ARGS(0);
	if (!IS_STRING(val)) {
		EXCEPTION("Excepted String!!\n");
	}
	const char* current_str = val.ptr->str;
	for (i = 1; i < ARGC; i++) {
		val = ARGS(i);
		if (!IS_STRING(val)) {
			fprintf(stderr, "type error!\n");
			TODO("exception\n");
		}
		const char  *next_str = val.ptr->str;
		if (strcmp(current_str, next_str) < 0) {
			return new_bool(0);
		}
		current_str = next_str;
	}
	return new_bool(1);
}

static int eq_ii(val_t v0, val_t v1) {
	return v0.ivalue != v1.ivalue;
}

static int eq_if(val_t v0, val_t v1) {
	return v0.ivalue != v1.fvalue;
}

static int eq_fi(val_t v0, val_t v1) {
	return v0.fvalue != v1.ivalue;
}

static int eq_ff(val_t v0, val_t v1) {
	return v0.fvalue != v1.fvalue;
}

static int (*eq_op[])(val_t, val_t) = {eq_ff, eq_fi, eq_if, eq_ii};

static val_t eq(val_t* VSTACK, int ARGC) {
	int i;
	val_t val = ARGS(0);
	if (!IS_NUMBER(val)) {
		EXCEPTION("Excepted Int!!\n");
	}
	val_t current_number = val;
	for (i = 1; i < ARGC; i++) {
		val = ARGS(i);
		if (!IS_NUMBER(val)) {
			fprintf(stderr, "type error!\n");
			TODO("exception\n");
		}
		val_t next_number = val;
		if (eq_op[IS_INT(current_number)*2+IS_INT(next_number)](current_number, next_number)) {
			return new_bool(0);
		}
		current_number = next_number;
	}
	return new_bool(1);
}

static val_t string_eq(val_t* VSTACK, int ARGC) {
	int i;
	val_t val = ARGS(0);
	if (!IS_STRING(val)) {
		EXCEPTION("Excepted String!!\n");
	}
	const char* current_str = val.ptr->str;
	for (i = 1; i < ARGC; i++) {
		val = ARGS(i);
		if (!IS_STRING(val)) {
			fprintf(stderr, "type error!\n");
			TODO("exception\n");
		}
		const char  *next_str = val.ptr->str;
		if (strcmp(current_str, next_str) != 0) {
			return new_bool(0);
		}
		current_str = next_str;
	}
	return new_bool(1);
}

static val_t neq(val_t* VSTACK, int ARGC) {
	int i;
	val_t val = ARGS(0);
	if (!IS_NUMBER(val)) {
		EXCEPTION("Excepted Int!!\n");
	}
	if (ARGC == 1) {
		return new_bool(1);
	}
	val_t current_number = val;
	for (i = 1; i < ARGC; i++) {
		val = ARGS(i);
		if (!IS_NUMBER(val)) {
			fprintf(stderr, "type error!\n");
			TODO("exception\n");
		}
		val_t next_number = val;
		if (eq_op[IS_INT(current_number)*2+IS_INT(next_number)](current_number, next_number)) {
			return new_bool(1);
		}
		current_number = next_number;
	}
	return new_bool(0);
}

static val_t string_neq(val_t* VSTACK, int ARGC) {
	int i;
	val_t val = ARGS(0);
	if (!IS_STRING(val)) {
		EXCEPTION("Excepted String!!\n");
	}
	const char* current_str = val.ptr->str;
	for (i = 1; i < ARGC; i++) {
		val = ARGS(i);
		if (!IS_STRING(val)) {
			fprintf(stderr, "type error!\n");
			TODO("exception\n");
		}
		const char  *next_str = val.ptr->str;
		if (strcmp(current_str, next_str) == 0) {
			return new_bool(0);
		}
		current_str = next_str;
	}
	return new_bool(1);
}

static val_t zerop(val_t *VSTACK, int ARGC) {
	val_t val = ARGS(0);
	if (IS_NUMBER(val) && val.ivalue == 0) {
		return new_bool(1);
	}
	return new_bool(0);
}

static val_t null(val_t *VSTACK, int ARGC) {
	return new_bool(IS_nil(ARGS(0)));
}

static val_t not(val_t *VSTACK, int ARGC) {
	return new_bool(IS_nil(ARGS(0)));
}

static val_t atom(val_t *VSTACK, int ARGC) {
	return new_bool(!IS_OPEN(ARGS(0)));
}
static val_t quote(val_t * VSTACK, int ARGC) {
	return ARGS(0);
}

static val_t list(val_t * VSTACK, int ARGC) {
	int i;
	if (ARGC == 0) {
		return new_bool(0);
	}
	val_t res = {0, 0};
	res.ptr = new_open();
	cstack_cons_cell_push(res.ptr);
	val_t tmp = res;
	for (i = 0; i < ARGC; i++) {
		tmp.ptr->car = ARGS(i);
		if (i == ARGC - 1) {
			tmp.ptr->cdr = new_bool(0);
		} else {
			tmp.ptr->cdr.ptr = new_open();
			tmp = tmp.ptr->cdr;
		}
	}
	cstack_cons_cell_pop();
	return res;
}

static val_t length(val_t *VSTACK, int ARGC) {
	val_t val = ARGS(0);
	if (IS_nil(val)) {
		return new_int(0);
	} else if (IS_STRING(val)) {
		return new_int(strlen(val.ptr->str));
	}
	if (!IS_OPEN(val)) {
		EXCEPTION("Not a list!!\n");
	}
	int res = 0;
	while (IS_OPEN(val)) {
		res++;
		val = val.ptr->cdr;
	}
	if (!IS_nil(val)) {
		EXCEPTION("Not a list!!\n");
	}
	return new_int(res);
}

static val_t _if(val_t *VSTACK, int ARGC, struct array_t *a) {
	val_t val = vm_exec(2, (opline_t*)array_get(a, 0), VSTACK);
	val_t res = {0, 0};
	if (!IS_nil(val)) {
		res = vm_exec(2, (opline_t*)array_get(a, 1), VSTACK);
	} else {
		res = vm_exec(2, (opline_t*)array_get(a, 2), VSTACK);
	}
	return res;
}

static val_t _assert(val_t *VSTACK, int ARGC, array_t *a) {
	val_t val = vm_exec(2, (opline_t*)array_get(a, 0), VSTACK);
	if (IS_nil(val)) {
		EXCEPTION("NIL must evalueate to a non-NIL value\n");
		assert(0);
	}
	return val;
}

static val_t cond(val_t *VSTACK, int ARGC, array_t *a) {
	int size = array_size(a), i = 0;
	if (size == 0) {
		return new_bool(0);
	}
	val_t res = {0, 0};
	for (; i < size; i++) {
		val_t val = vm_exec(2, (opline_t*)array_get(a, i), VSTACK);
		VSTACK[1] = val;
		int _length = length(VSTACK+1, 1).ivalue;
		if (_length == 0) {
			EXCEPTION("clause NIL should be a list");
		}
		val_t car = val.ptr->car;
		if (i == size-1 && (IS_SYMBOL(car)) 
				&& strcmp(car.ptr->str, "otherwise") == 0) {
			/* default */
		} else {
			codegen(val.ptr->car);
			val_t res = vm_exec(2, memory+current_index, VSTACK);
			if (IS_nil(res)) {
				continue;
			}
		}
		int j = 1;
		val_t cdr = val.ptr->cdr;
		car = cdr.ptr->car;
		for (; j < _length; j++) {
			codegen(car);
			res = vm_exec(2, memory+current_index, VSTACK);
			cdr = cdr.ptr->cdr;
			car = cdr.ptr->car;
		}
		return res;
	}
	return new_bool(0);
}

static val_t progn(val_t *VSTACK, int ARGC, array_t *a) {
	int size = array_size(a), i = 0;
	val_t res = {0, 0};
	for (; i < size; i++) {
		res = vm_exec(2, (opline_t*)array_get(a, i), VSTACK);
	}
	return res;
}

static val_t loop(val_t *VSTACK, int ARGC, array_t *a) {
	int size = array_size(a), i = 0;
	val_t res = {0, 0};
	jmp_buf buf;
	if (loop_frame_list == NULL) {
		loop_frame_list = new_array();
	}
	VSTACK[1] = new_bool(0);
	loop_frame_push(&buf, VSTACK[1]);
	int jmp = 0;
	if ((jmp = setjmp(buf)) == 0) {
		while (1) {
			i = 0;
			for (; i < size; i++) {
				res = vm_exec(2, (opline_t*)array_get(a, i), VSTACK + 2);
			}
		}
	}
	res = loop_return_value;
	return res;
}

static val_t block(val_t *VSTACK, int ARGC, array_t *a) {
	int size = array_size(a);
	if (size == 0) {
		EXCEPTION("Too few arguments!!\n");
	} else if (size == 1) {
		return new_bool(0);
	}
	val_t res = {0, 0};
	jmp_buf buf;
	if (loop_frame_list == NULL) {
		loop_frame_list = new_array();
	}
	val_t block_name = vm_exec(2, (opline_t*)array_get(a, 0), VSTACK);
	if (!IS_nil(block_name) && !IS_T(block_name) && !IS_SYMBOL(block_name)) {
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
	}
	return res;
}

static val_t _return(val_t *VSTACK, int ARGC) {
	if (ARGC > 1) {
		EXCEPTION("too many arguments!!\n");
	}
	loop_frame_t *frame = NULL;
	while ((frame = loop_frame_pop()) != NULL) {
		jmp_buf *buf = frame->buf;
		val_t block_name = frame->block_name;
		free(frame);
		if (IS_nil(block_name)) {
			loop_return_value = (ARGC == 0) ? new_bool(0) : ARGS(0);
			longjmp(*buf, 1);
		}
	}
	EXCEPTION("No block found!!\n");
}

static val_t _return_from(val_t *VSTACK, int ARGC) {
	if (ARGC != 1 && ARGC != 2) {
		EXCEPTION("Illegal number of arguments!!\n");
	}
	loop_frame_t *frame = NULL;
	val_t args0 = ARGS(0);
	while ((frame = loop_frame_pop()) != NULL) {
		jmp_buf *buf = frame->buf;
		val_t block_name = frame->block_name;
		free(frame);
		if (IS_nil(block_name) && IS_nil(args0) ||
			IS_T(block_name) && IS_T(args0) ||
			strcmp(block_name.ptr->str, args0.ptr->str) == 0) {
			loop_return_value = (ARGC == 1) ? new_bool(0) : ARGS(1);
			longjmp(*buf, 1);
		}
	}
	EXCEPTION("No block found!!\n");
}

static val_t when(val_t *VSTACK, int ARGC, array_t *a) {
	int size = array_size(a);
	if (size == 0) {
		EXCEPTION("argument length does not match!!\n");
	}
	val_t res = vm_exec(2, (opline_t*)array_get(a, 0), VSTACK);
	if (IS_nil(res) || size == 1) {
		return new_bool(0);
	}
	int i = 1;
	for (; i < size; i++) {
		res = vm_exec(2, (opline_t*)array_get(a, i), VSTACK);
	}
	return res;
}

static val_t unless(val_t *VSTACK, int ARGC, array_t *a) {
	int size = array_size(a);
	if (size == 0) {
		EXCEPTION("argument length does not match!!\n");
	}
	val_t res = vm_exec(2, (opline_t*)array_get(a, 0), VSTACK);
	if (IS_T(res) || size == 1) {
		return new_bool(0);
	}
	int i = 1;
	for (; i < size; i++) {
		res = vm_exec(2, (opline_t*)array_get(a, i), VSTACK);
	}
	return res;
}

static val_t defun(val_t *VSTACK, int ARGC, struct array_t *a) {
	val_t fcons = vm_exec(2, (opline_t*)array_get(a, 0), VSTACK);
	if (!IS_SYMBOL(fcons)) {
		EXCEPTION("Excepted Symbol!!\n");
	}
	val_t args = vm_exec(2, (opline_t*)array_get(a, 1), VSTACK);
	int i = 2;
	struct array_t *opline_list = new_array();
	for (; i < array_size(a); i++) {
		array_add(opline_list, memory + next_index);
		val_t fbody = vm_exec(2, (opline_t*)array_get(a, i), VSTACK + i);
		codegen(fbody);
	}
	VSTACK[1] = args;
	int argc = length(VSTACK+1, 1).ivalue;
	set_func(fcons.ptr, opline_list, argc, args, current_environment, 0);
	return fcons;
}

static val_t defmacro(val_t *VSTACK, int ARGC, array_t *a) {
	val_t fcons = vm_exec(2, (opline_t*)array_get(a, 0), VSTACK);
	if (!IS_SYMBOL(fcons)) {
		EXCEPTION("Excepted Symbol!!\n");
	}
	val_t args = vm_exec(2, (opline_t*)array_get(a, 1), VSTACK);
	int i = 2;
	struct array_t *opline_list = new_array();
	for (; i < array_size(a); i++) {
		array_add(opline_list, memory + next_index);
		val_t fbody = vm_exec(2, (opline_t*)array_get(a, i), VSTACK + i);
		codegen(fbody);
	}
	VSTACK[1] = args;
	int argc = length(VSTACK+1, 1).ivalue;
	set_func(fcons.ptr, opline_list, argc, args, current_environment, FLAG_MACRO);
	return fcons;
}

static val_t setq(val_t *VSTACK, int ARGC) {
	val_t variable = ARGS(0);
	val_t value = ARGS(1);
	if (!IS_SYMBOL(variable)) {
		EXCEPTION("Not a atom!!\n");
	}
	set_variable(variable.ptr, value, 0);
	return value;
}

static val_t let_inner(val_t *VSTACK, int ARGC, struct array_t *a, int is_star) {
	val_t value_list = vm_exec(2, (opline_t*)array_get(a, 0), VSTACK);
	val_t variable = {0, 0};
	val_t list = {0, 0};
	val_t value = {0, 0};
	array_t *a1 = new_array();
	array_t *a2 = new_array();
	if (IS_OPEN(value_list)) {
		list = value_list.ptr->car;
		while (list.ptr != NULL && !IS_nil(list)) {
			if (IS_OPEN(list)) {
				VSTACK[1] = list;
				int argc = length(VSTACK+2, 1).ivalue;
				if (argc == 2) {
					variable = list.ptr->car;
					if (!IS_SYMBOL(variable)) {
						EXCEPTION("Excepted Symbol!!\n");
					}
					value = list.ptr->cdr.ptr->car;
					codegen(value);
					val_t res = vm_exec(2, memory + current_index, VSTACK + 1);
					if (is_star) {
						set_variable(variable.ptr, res, 1);
					} else {
						array_add(a1, variable.ptr);
						val_t *tmp = (val_t*)malloc(sizeof(val_t));
						*tmp = res;
						array_add(a2, tmp);
					}
				} else {
					fprintf(stderr, "illegal variable specification!! %d\n", argc);
					assert(0);
				}
			} else {
				//fprintf(stderr, "hi\n");
				if (!IS_SYMBOL(list)) {
					EXCEPTION("Excepted Symbol!!\n");
				}
				if (is_star) {
					val_t p = set_variable(list.ptr, new_bool(0), 1);
				} else {
					array_add(a1, list.ptr);
					array_add(a2, NULL);
				}
			}
			value_list = value_list.ptr->cdr;
			if (IS_nil(value_list)) {
				break;
			}
			list = value_list.ptr->car;
		}
	}
	if (!is_star) {
		int i = 0, size = array_size(a1);
		for (; i < size; i++) {
			cons_t *cons = (cons_t*)array_pop(a1);
			val_t* val = ((val_t*)array_pop(a2));
			if (val == NULL) {
				set_variable(cons, new_bool(0), 1);
			} else {
				set_variable(cons, *val, 1);
				FREE(val);
			}
		}
	}
	array_free(a1);
	array_free(a2);
	val_t res = {0, 0};
	int i;
	for (i = 1; i < array_size(a); i++) {
		res = vm_exec(2, (opline_t*)array_get(a, i), VSTACK);
	}
	return res;
}

static val_t let(val_t *VSTACK, int ARGC, struct array_t *a) {
	return let_inner(VSTACK, ARGC, a, 0);
}

static val_t let_star(val_t *VSTACK, int ARGC, array_t *a) {
	return let_inner(VSTACK, ARGC, a, 1);
}
static val_t eval(val_t *VSTACK, int ARGC) {
	//val_t cons = CONS_EVAL(ARGS(0));
	val_t cons = ARGS(0);
	codegen(cons);
	val_t res = vm_exec(2, memory + current_index, VSTACK + 1);
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
   val_t (*mtd)(val_t*, int);
   val_t (*special_mtd)(val_t*, int, struct array_t*);
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
	{"defmacro", -1,FLAG_SPECIAL_FORM | FLAG_LOCAL_SCOPE, -1, 0, NULL, defmacro},
	{"setq", 2, 0, 1, 0, setq, NULL},
	{"let", -1, FLAG_SPECIAL_FORM | FLAG_LOCAL_SCOPE, 1, 0, NULL, let},
	{"let*", -1, FLAG_SPECIAL_FORM | FLAG_LOCAL_SCOPE, 1, 0, NULL, let_star},
	{"eval", 1, 0, 0, 0, eval, NULL},
	{NULL, 0, 0, 0, 0, NULL, NULL},
};
