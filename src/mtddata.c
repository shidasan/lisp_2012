#include "lisp.h"

static val_t loop_return_value;

void loop_frame_push(jmp_buf *buf, val_t block_name) {
	loop_frame_t *frame = (loop_frame_t*)malloc(sizeof(loop_frame_t));
	frame->buf = buf;
	frame->block_name = block_name;
	frame->environment = current_environment;
	array_add(loop_frame_list, frame);
}

loop_frame_t *loop_frame_pop() {
	loop_frame_t *frame = array_pop(loop_frame_list);
	return frame;
}
void throw_inner() {
	loop_frame_t *frame;
	while ((frame = loop_frame_pop()) != NULL) {
		jmp_buf *buf = frame->buf;
		val_t block_name = frame->block_name;
		cons_t *environment = frame->environment;
		free(frame);
		if (IS_NULL(block_name)) {
			environment = current_environment;
			longjmp(*buf, 1);
		}
	}
}
void throw_exception(const char *_file, int _line, const char *format){
	(void)_file;(void)_line;
	fprintf(stderr, "From %s, %d): \n", _file, _line);
	fprintf(stderr, "%s", format);
	throw_inner();
}

void throw_fmt_exception(const char *_file, int _line, const char *format, va_list ap ){
	(void)_file;(void)_line;
	fprintf(stderr, "From %s, %d: \n", _file, _line);
	fprintf(stderr, format, ap);
	throw_inner();
}

void throw_type_exception(const char *_file, int _line, const char *type, val_t val) {
	fprintf(stderr, "From %s, %d: \n", _file, _line);
	string_buffer_t *buffer = new_string_buffer();
	VAL_TO_STRING(val, buffer, 1);
	char *str = string_buffer_to_string(buffer);
	fprintf(stderr, "%s is not a %s\n", str, type);
	free(str);
	string_buffer_free(buffer);
	throw_inner();
}

static val_t print(val_t *VSTACK, int ARGC) {
	val_t cons = ARGS(0);
	print_return_value(cons);
	printf("\n");
	return cons;
}

static val_t format(val_t *VSTACK, array_t *a) {
	int length = array_size(a), location = 0, evaluate = 2;
	if (length < 2) {
		EXCEPTION("Too few arguments!!\n");
	}
	val_t destination = vm_exec(memory + (uintptr_t)array_get(a, 0), VSTACK);
	switch (VAL_TYPE(destination)) {
		case T_OFFSET:
		case nil_OFFSET:
			break;
		default:
			EXCEPTION("Invalid file stream!!\n");
			break;
	}
	val_t format_string = vm_exec(memory + (uintptr_t)array_get(a, 1), VSTACK);
	if (IS_UNBOX(format_string) || format_string.ptr->type != STRING) {
		EXCEPTION("The control-string must be a string!!\n");
	}
	const char *str = format_string.ptr->str;
	int str_len = strlen(str);
	string_buffer_t *buffer = new_string_buffer();
	val_t val;
	val.ptr = NULL;
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
				val = vm_exec(memory + (uintptr_t)array_get(a, evaluate), VSTACK);
				VAL_TO_STRING(val, buffer, 0);
				location += 2;
				evaluate++;
				break;

			case 'C':
				if (evaluate >= length) {
					EXCEPTION("There are not enough arguments left for format directive!!\n");
				}
				val = vm_exec(memory + (uintptr_t)array_get(a, evaluate), VSTACK);
				VAL_TO_STRING(val, buffer, 1);
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
		val = vm_exec(memory + (uintptr_t)array_get(a, evaluate), VSTACK);
	}
	val_t res;
	res.ptr = NULL;
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
		EXPECTED("list", val);
	}
	return val.ptr->car;
}

static val_t cdr(val_t* VSTACK, int ARGC) {
	val_t val = ARGS(0);
	if (!IS_OPEN(val)) {
		EXPECTED("list", val);
	}
	return val.ptr->cdr;
}

static val_t eq(val_t* VSTACK, int ARGC) {
	val_t val0 = ARGS(0);
	val_t val1 = ARGS(1);
	return new_bool(val0.ptr == val1.ptr);
}

static val_t cons(val_t* VSTACK, int ARGC) {
	val_t car = ARGS(0);
	val_t cdr = ARGS(1);
	val_t val = null_val();
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
	val_t res = new_int(0);
	for (i = 0; i < ARGC; i++) {
		val_t val = ARGS(i);
		if (!IS_NUMBER(val)) {
			EXPECTED("number", val);
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
		EXPECTED("number", res);
	}
	if (ARGC == 1) {
		if (IS_INT(res)) {
			res.ivalue *= -1;
		} else {
			res.fvalue *= -1;
		}
		return res;
	}
	for (i = 1; i < ARGC; i++) {
		val_t val = ARGS(i);
		if (!IS_NUMBER(val)) {
			EXPECTED("number", res);
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
	val_t res = new_int(1);
	for (i = 0; i < ARGC; i++) {
		val_t val = ARGS(i);
		if (!IS_NUMBER(val)) {
			EXPECTED("number", res);
		}
		res = mul_op[IS_INT(res)*2+IS_INT(val)](res, val);
	}
	return res;
}

static val_t div_ii(val_t v0, val_t v1) {
	if (v1.ivalue == 0) {
		EXCEPTION("0 div\n");
	}
	if (v0.ivalue % v1.ivalue == 0) {
		v0.ivalue /= v1.ivalue;
		return v0;
	} else {
		return new_float(((float)v0.ivalue / (float)v1.ivalue));
	}
}

static val_t div_if(val_t v0, val_t v1) {
	if (v1.fvalue == 0) {
		EXCEPTION("0 div\n");
	}
	return new_float((float)v0.ivalue / v1.fvalue);
}

static val_t div_fi(val_t v0, val_t v1) {
	if (v1.ivalue == 0) {
		EXCEPTION("0 div\n");
	}
	v0.fvalue /= v1.ivalue;
	return v0;
}

static val_t div_ff(val_t v0, val_t v1) {
	if (v1.fvalue == 0) {
		EXCEPTION("0 div\n");
	}
	v0.fvalue /= v1.fvalue;
	return v0;
}

static val_t (*div_op[])(val_t, val_t) = {div_ff, div_fi, div_if, div_ii};
static val_t _div(val_t* VSTACK, int ARGC) {
	int i;
	val_t res = ARGS(0);
	for (i = 1; i < ARGC; i++) {
		val_t val = ARGS(i);
		if (!IS_NUMBER(val)) {
			EXPECTED("number", res);
		}
		res = div_op[IS_INT(res)*2+IS_INT(val)](res, val);
	}
	return res;
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
		EXPECTED("number", val);
	}
	val_t current_number = val;
	for (i = 1; i < ARGC; i++) {
		val = ARGS(i);
		if (!IS_NUMBER(val)) {
			EXPECTED("number", val);
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
		EXPECTED("string", val);
	}
	const char* current_str = val.ptr->str;
	for (i = 1; i < ARGC; i++) {
		val = ARGS(i);
		if (!IS_STRING(val)) {
			EXCEPTION("type error!!\n");
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
		EXPECTED("number", val);
	}
	val_t current_number = val;
	for (i = 1; i < ARGC; i++) {
		val = ARGS(i);
		if (!IS_NUMBER(val)) {
			EXPECTED("number", val);
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
		EXPECTED("string", val);
	}
	const char* current_str = val.ptr->str;
	for (i = 1; i < ARGC; i++) {
		val = ARGS(i);
		if (!IS_STRING(val)) {
			EXPECTED("string", val);
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
		EXPECTED("number", val);
	}
	val_t current_number = val;
	for (i = 1; i < ARGC; i++) {
		val = ARGS(i);
		if (!IS_NUMBER(val)) {
			EXPECTED("number", val);
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
		EXPECTED("string", val);
	}
	const char* current_str = val.ptr->str;
	for (i = 1; i < ARGC; i++) {
		val = ARGS(i);
		if (!IS_STRING(val)) {
			EXPECTED("string", val);
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
		EXPECTED("number", val);
	}
	val_t current_number = val;
	for (i = 1; i < ARGC; i++) {
		val = ARGS(i);
		if (!IS_NUMBER(val)) {
			EXPECTED("number", val);
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
		EXPECTED("string", val);
	}
	const char* current_str = val.ptr->str;
	for (i = 1; i < ARGC; i++) {
		val = ARGS(i);
		if (!IS_STRING(val)) {
			EXPECTED("string", val);
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

static val_t opeq(val_t* VSTACK, int ARGC) {
	int i;
	val_t val = ARGS(0);
	if (!IS_NUMBER(val)) {
		EXPECTED("number", val);
	}
	val_t current_number = val;
	for (i = 1; i < ARGC; i++) {
		val = ARGS(i);
		if (!IS_NUMBER(val)) {
			EXPECTED("number", val);
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
		EXPECTED("string", val);
	}
	const char* current_str = val.ptr->str;
	for (i = 1; i < ARGC; i++) {
		val = ARGS(i);
		if (!IS_STRING(val)) {
			EXPECTED("string", val);
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
		EXPECTED("number", val);
	}
	if (ARGC == 1) {
		return new_bool(1);
	}
	val_t current_number = val;
	for (i = 1; i < ARGC; i++) {
		val = ARGS(i);
		if (!IS_NUMBER(val)) {
			EXPECTED("number", val);
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
		EXPECTED("string", val);
	}
	const char* current_str = val.ptr->str;
	for (i = 1; i < ARGC; i++) {
		val = ARGS(i);
		if (!IS_STRING(val)) {
			EXPECTED("string", val);
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

static val_t _sqrt (val_t *VSTACK, int ARGC) {
	val_t val = ARGS(0);
	val_t res = null_val();
	if (IS_FLOAT(val)) {
		res =  new_float((float)sqrt(val.fvalue));
	} else if (IS_INT(val)) {
		res = new_float((float)sqrt((double)val.ivalue));
	} else {
		EXPECTED("number", val);
	}
	return res;
}

static val_t _random(val_t *VSTACK, int ARGC) {
	val_t val = ARGS(0);
	if (IS_INT(val)) {
		return new_int(rand() % val.ivalue);
	} else if (IS_FLOAT(val)) {
		return new_float(rand() / (RAND_MAX / val.fvalue));
	} else {
		EXPECTED("number", val);
		return null_val(); //unreachable
	}
}

static val_t _sin(val_t *VSTACK, int ARGC) {
	val_t val = ARGS(0);
	if (!IS_NUMBER(val)) {
		EXPECTED("number", val);
	}
	if (IS_INT(val)) {
		return new_float(sin((float)val.ivalue));
	} else {
		return new_float(sin(val.fvalue));
	}
}

static val_t _cos(val_t *VSTACK, int ARGC) {
	val_t val = ARGS(0);
	if (!IS_NUMBER(val)) {
		EXPECTED("number", val);
	}
	if (IS_INT(val)) {
		return new_float(cos((float)val.ivalue));
	} else {
		return new_float(cos(val.fvalue));
	}
}

static val_t _tan(val_t *VSTACK, int ARGC) {
	val_t val = ARGS(0);
	if (!IS_NUMBER(val)) {
		EXPECTED("number", val);
	}
	if (IS_INT(val)) {
		return new_float(tan((float)val.ivalue));
	} else {
		return new_float(tan(val.fvalue));
	}
}

static val_t _floor(val_t *VSTACK, int ARGC) {
	val_t val = ARGS(0);
	if (!IS_NUMBER(val)) {
		EXPECTED("number", val);
	}
	if (IS_FLOAT(val)) {
		return new_int(floor(val.fvalue));
	} else {
		return new_int(floor((float)val.ivalue));
	}
}

static val_t atom(val_t *VSTACK, int ARGC) {
	return new_bool(!IS_OPEN(ARGS(0)));
}

static val_t consp(val_t *VSTACK, int ARGC) {
	return new_bool(IS_OPEN(ARGS(0)));
}

static val_t listp(val_t *VSTACK, int ARGC) {
	val_t val = ARGS(0);
	return new_bool(IS_OPEN(val) || IS_nil(val));
}

static val_t symbolp(val_t *VSTACK, int ARGC) {
	val_t val = ARGS(0);
	return new_bool(IS_SYMBOL(val));
}

static val_t numberp(val_t *VSTACK, int ARGC) {
	return new_bool(IS_NUMBER(ARGS(0)));
}

static val_t integerp(val_t *VSTACK, int ARGC) {
	return new_bool(IS_INT(ARGS(0)));
}


static val_t quote(val_t * VSTACK, int ARGC) {
	return ARGS(0);
}

static val_t funcall(val_t * VSTACK, int ARGC) {
	val_t val = ARGS(0);
	if (!IS_CALLABLE(val)) {
		EXPECTED("symbol", val);
	}
	func_t *func = search_func(val.ptr->str);
	if (func == NULL && !IS_LAMBDA(val)) {
		FMT_EXCEPTION("function %s not found!!\n", (void*)val.ptr->str);
	}
	val_t res = null_val();
	cons_t *old_environment = NULL;
	if (func != NULL && FLAG_IS_STATIC(func->flag)) {
		old_environment = begin_local_scope(func);
		res = func->mtd(VSTACK, ARGC - 1);
		end_local_scope(old_environment);
	} else if (IS_LAMBDA(val)) { /* call lambda function */
		lambda_env_t *env = val.ptr->env;
		val_t args = env->args;
		array_t *lambda_data_list = val.ptr->cdr.a;
		cons_t *old_environment = change_local_scope(current_environment, env->environment);
		environment_list_push(old_environment);
		int i = 1;
		for (; i < ARGC; i++) {
			val_t car = args.ptr->car;
			if (!IS_SYMBOL(car)) {
				EXPECTED("symbol", car);
			}
			val_t value = ARGS(i);
			set_variable(car.ptr, value, 1);
			args = args.ptr->cdr;
		}
		for (i = 0; i < array_size(lambda_data_list); i++) {
			lambda_data_t *data = (lambda_data_t*)array_get(lambda_data_list, i);
			res = vm_exec(memory + data->opline_idx, VSTACK+1);
		}
		environment_list_pop();
		end_local_scope(old_environment);
	} else {
		res = call_mtd(VSTACK + ARGC-3, ARGC-1, func);
	}
	return res;
}

static val_t list(val_t * VSTACK, int ARGC) {
	int i;
	if (ARGC == 0) {
		return new_bool(0);
	}
	val_t res = null_val();
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
	} else if (IS_ARRAY(val)) {
		return new_int(val.ptr->size);
	} else if (!IS_OPEN(val)) {
		EXPECTED("list", val);
	}
	int res = 0;
	while (IS_OPEN(val)) {
		res++;
		val = val.ptr->cdr;
	}
	if (!IS_nil(val)) {
		EXPECTED("list", val);
	}
	return new_int(res);
}

static val_t svref(val_t *VSTACK, int ARGC) {
	val_t val = ARGS(0);
	val_t i = ARGS(1);
	if (!IS_ARRAY(val)) {
		EXPECTED("array", val);
	}
	if (!IS_INT(i)) {
		EXPECTED("integer", i);
	}
	if (i.ivalue < 0 || i.ivalue >= val.ptr->size) {
		fprintf(stderr, "ivalue: %d, size: %d\n", i.ivalue, val.ptr->size);
		EXCEPTION("Array out of bounds!!\n");
	}
	return val.ptr->list[i.ivalue];
}

static val_t svstore(val_t *VSTACK, int ARGC) {
	val_t val = ARGS(0);
	val_t i = ARGS(1);
	val_t j = ARGS(2);
	if (!IS_ARRAY(val)) {
		EXPECTED("array", val);
	}
	if (!IS_INT(i)) {
		EXPECTED("integer", i);
	}
	if (i.ivalue < 0 || i.ivalue >= val.ptr->size) {
		fprintf(stderr, "%d\n", i.ivalue);
		EXCEPTION("Array out of bounds!!\n");
	}
	val.ptr->list[i.ivalue] = j;
	return j;
}

static val_t _vector(val_t *VSTACK, int ARGC) {
	array_t *a = new_array();
	int i = 0;
	for (; i < ARGC; i++) {
		array_add_val(a, ARGS(i));
	}
	val_t res = null_val();
	res.ptr = new_cons_array_list(a);
	FREE(a);
	return res;
}

static val_t _make_array(val_t *VSTACK, int ARGC) {
	val_t val = ARGS(0);
	if (!IS_OPEN(val)) {
		EXPECTED("list", val);
	}
	VSTACK[1] = val;
	int _length = length(VSTACK+2, 1).ivalue;
	if (_length != 1) {
		EXCEPTION("TODO\n");
	}
	if (!IS_INT(val.ptr->car)) {
		EXPECTED("integer", val.ptr->car);
	}
	int i = 0, size = val.ptr->car.ivalue;
	array_t *a = new_array();
	for (; i < size; i++) {
		array_add_val(a, new_int(0));
	}
	val_t res = null_val();
	res.ptr = new_cons_array_list(a);
	FREE(a);
	return res;
}

static val_t _and(val_t *VSTACK, array_t *a) {
	int i = 0, size = array_size(a);
	for (; i < size; i++) {
		val_t val = vm_exec(memory + (uintptr_t)array_get(a, i), VSTACK);
		if (IS_nil(val)) {
			return new_bool(0);
		}
	}
	return new_bool(1);
}

static val_t _or(val_t *VSTACK, array_t *a) {
	int i = 0, size = array_size(a);
	for (; i < size; i++) {
		val_t val = vm_exec(memory + (uintptr_t)array_get(a, i), VSTACK);
		if (!IS_nil(val)) {
			return new_bool(1);
		}
	}
	return new_bool(0);
}

static val_t _if(val_t *VSTACK, struct array_t *a) {
	val_t val = vm_exec(memory + (uintptr_t)array_get(a, 0), VSTACK);
	val_t res = null_val();
	if (!IS_nil(val)) {
		res = vm_exec(memory + (uintptr_t)array_get(a, 1), VSTACK);
	} else {
		res = vm_exec(memory + (uintptr_t)array_get(a, 2), VSTACK);
	}
	return res;
}

static val_t _assert(val_t *VSTACK, array_t *a) {
	val_t val = vm_exec(memory + (uintptr_t)array_get(a, 0), VSTACK);
	if (IS_nil(val)) {
		EXCEPTION("NIL must evalueate to a non-NIL value\n");
		assert(0);
	}
	return val;
}

val_t eval_inner(val_t *VSTACK, val_t val);

static val_t cond(val_t *VSTACK, array_t *a) {
	int size = array_size(a), i = 0;
	if (size == 0) {
		return new_bool(0);
	}
	val_t res = null_val();
	for (; i < size; i++) {
		val_t val = vm_exec(memory + (uintptr_t)array_get(a, i), VSTACK);
		VSTACK[1] = val;
		int _length = length(VSTACK+2, 1).ivalue;
		if (_length == 0) {
			EXCEPTION("clause NIL should be a list");
		}
		val_t car = val.ptr->car;
		if (i == size-1 && (IS_SYMBOL(car)) 
				&& strcmp(car.ptr->str, "otherwise") == 0) {
			/* default */
		} else {
			res = eval_inner(VSTACK, val.ptr->car);
			//codegen(val.ptr->car);
			//res = vm_exec(memory+current_index, VSTACK);
			if (IS_nil(res)) {
				continue;
			}

			int j = 1;
			val_t cdr = val;
			car = cdr.ptr->car;
			for (; j < _length; j++) {
				cdr = cdr.ptr->cdr;
				car = cdr.ptr->car;
				res = eval_inner(VSTACK, car);
				//codegen(car);
				//res = vm_exec(memory+current_index, VSTACK);
			}
			return res;
		}
	}
	return new_bool(0);
}

static val_t progn(val_t *VSTACK, array_t *a) {
	int size = array_size(a), i = 0;
	val_t res = null_val();
	for (; i < size; i++) {
		res = vm_exec(memory + (uintptr_t)array_get(a, i), VSTACK);
	}
	return res;
}

static val_t loop(val_t *VSTACK, array_t *a) {
	int size = array_size(a), i = 0;
	val_t res = null_val();
	jmp_buf *buf = (jmp_buf*)malloc(sizeof(jmp_buf));
	VSTACK[1] = new_bool(0);
	loop_frame_push(buf, VSTACK[1]);
	if (setjmp(*buf) == 0) {
		while (1) {
			i = 0;
			for (; i < size; i++) {
				res = vm_exec(memory + (uintptr_t)array_get(a, i), VSTACK+i+1);
			}
		}
	} else {
		FREE(buf);
		res = loop_return_value;
	}
	FREE(buf);
	res = loop_return_value;
	return res;
}

static val_t dolist(val_t *VSTACK, array_t *a) {
	val_t args0 = vm_exec(memory + (uintptr_t)array_get(a, 0), VSTACK);
	if (!IS_OPEN(args0)) {
		EXPECTED("list", args0);
	}
	VSTACK[1] = args0;
	int list_length = length(VSTACK+2, 1).ivalue;
	if (list_length != 2 && list_length != 3) {
		EXCEPTION("Invalid list length!!\n");
	}
	val_t symbol = args0.ptr->car;
	if (!IS_SYMBOL(symbol)) {
		EXPECTED("symbol", symbol);
	}
	set_variable(symbol.ptr, new_bool(0), 1);
	args0 = args0.ptr->cdr;
	val_t list = eval_inner(VSTACK+1, args0.ptr->car);
	if (!IS_OPEN(list)) {
		EXPECTED("list", list);
	}
	args0 = args0.ptr->cdr;
	val_t expr = null_val();
	if (IS_OPEN(args0)) {
		expr = args0.ptr->car;
	}
	val_t car;
	while (!IS_nil(list)) {
		car = list.ptr->car;
		set_variable(symbol.ptr, car, 1);
		int i = 0;
		for (; i < array_size(a); i++) {
			vm_exec(memory + (uintptr_t)array_get(a, i), VSTACK);
		}
		list = list.ptr->cdr;
	}
	set_variable(symbol.ptr, car, 1);
	val_t res = new_bool(0);
	if (IS_OPEN(expr)) {
		res = eval_inner(VSTACK+1, expr);
	}
	return res;
}

static val_t dotimes(val_t *VSTACK, array_t *a) {
	val_t args0 = vm_exec(memory + (uintptr_t)array_get(a, 0), VSTACK);
	if (!IS_OPEN(args0)) {
		EXPECTED("list", args0);
	}
	VSTACK[1] = args0;
	int list_length = length(VSTACK+2, 1).ivalue;
	if (list_length != 2 && list_length != 3) {
		EXCEPTION("Invalid list length!!\n");
	}
	val_t symbol = args0.ptr->car;
	if (!IS_SYMBOL(symbol)) {
		EXPECTED("symbol", symbol);
	}
	set_variable(symbol.ptr, new_bool(0), 1);
	args0 = args0.ptr->cdr;
	val_t limit = eval_inner(VSTACK+1, args0.ptr->car);
	if (!IS_INT(limit)) {
		EXPECTED("integer", limit);
	}
	args0 = args0.ptr->cdr;
	val_t expr = null_val();
	if (IS_OPEN(args0)) {
		expr = args0.ptr->car;
	}
	int count = 0;
	while (count < limit.ivalue) {
		set_variable(symbol.ptr, new_int(count), 1);
		int i = 0;
		for (; i < array_size(a); i++) {
			vm_exec(memory + (uintptr_t)array_get(a, i), VSTACK);
		}
		count++;
	}
	set_variable(symbol.ptr, new_int(count), 1);
	val_t res = new_bool(0);
	if (!IS_NULL(expr)) {
		res = eval_inner(VSTACK+1, expr);
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
		cons_t *environment = frame->environment;
		FREE(frame);
		if (IS_nil(block_name)) {
			loop_return_value = (ARGC == 0) ? new_bool(0) : ARGS(0);
			current_environment = environment;
			longjmp(*buf, 1);
		}
		if (!IS_NULL(block_name)) {
			FREE(buf);
		}
		fprintf(stderr, "frame not match!!\n");
	}
	EXCEPTION("No block found!!\n");
	return null_val(); //unreachable
}

static val_t block(val_t *VSTACK, array_t *a) {
	int size = array_size(a);
	if (size == 0) {
		EXCEPTION("Too few arguments!!\n");
	} else if (size == 1) {
		return new_bool(0);
	}
	val_t res = null_val();
	jmp_buf *buf = (jmp_buf*)malloc(sizeof(jmp_buf));
	val_t block_name = vm_exec(memory + (uintptr_t)array_get(a, 0), VSTACK);
	if (!IS_nil(block_name) && !IS_T(block_name) && !IS_SYMBOL(block_name)) {
		EXPECTED("symbol", block_name);
	}
	loop_frame_push(buf, block_name);
	if (setjmp(*buf) == 0) {
		int i = 1; 
		for (; i < size; i++) {
			res = vm_exec(memory + (uintptr_t)array_get(a, i), VSTACK + 1);
		}
		loop_frame_t *frame = loop_frame_pop();
		FREE(frame);
	} else {
		FREE(buf);
		res = loop_return_value;
	}
	return res;
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
		if ((IS_nil(block_name) && IS_nil(args0)) ||
			(IS_T(block_name) && IS_T(args0)) ||
			strcmp(block_name.ptr->str, args0.ptr->str) == 0) {
			loop_return_value = (ARGC == 1) ? new_bool(0) : ARGS(1);
			longjmp(*buf, 1);
		}
	}
	EXCEPTION("No block found!!\n");
	return null_val();//unreachable
}

static val_t when(val_t *VSTACK, array_t *a) {
	int size = array_size(a);
	if (size == 0) {
		EXCEPTION("argument length does not match!!\n");
	}
	val_t res = vm_exec(memory + (uintptr_t)array_get(a, 0), VSTACK);
	if (IS_nil(res) || size == 1) {
		return new_bool(0);
	}
	int i = 1;
	for (; i < size; i++) {
		res = vm_exec(memory + (uintptr_t)array_get(a, i), VSTACK);
	}
	return res;
}

static val_t unless(val_t *VSTACK, array_t *a) {
	int size = array_size(a);
	if (size == 0) {
		EXCEPTION("argument length does not match!!\n");
	}
	val_t res = vm_exec(memory + (uintptr_t)array_get(a, 0), VSTACK);
	if (IS_T(res) || size == 1) {
		return new_bool(0);
	}
	int i = 1;
	for (; i < size; i++) {
		res = vm_exec(memory + (uintptr_t)array_get(a, i), VSTACK);
	}
	return res;
}

static val_t defun(val_t *VSTACK, array_t *a) {
	val_t fcons = vm_exec(memory + (uintptr_t)array_get(a, 0), VSTACK);
	if (!IS_SYMBOL(fcons)) {
		EXPECTED("symbol", fcons);
	}
	val_t args = vm_exec(memory + (uintptr_t)array_get(a, 1), VSTACK);
	int i = 2;
	struct array_t *opline_list = new_array();
	for (; i < array_size(a); i++) {
		array_add(opline_list, ((void*)((uintptr_t)next_index)));
		val_t fbody = vm_exec(memory + (uintptr_t)array_get(a, i), VSTACK + i);
		codegen(fbody);
	}
	VSTACK[1] = args;
	int argc = length(VSTACK+2, 1).ivalue;
	set_func(fcons.ptr, opline_list, argc, args, current_environment, 0);
	return fcons;
}

static lambda_data_t *new_lambda_data(int opline_idx, val_t body) {
	lambda_data_t *data = (lambda_data_t*)malloc(sizeof(lambda_data_t));
	data->opline_idx = opline_idx;
	data->body = body;
	return data;
}

static lambda_env_t *new_lambda_env(val_t args, cons_t *environment) {
	lambda_env_t *env = (lambda_env_t*)malloc(sizeof(lambda_env_t));
	env->environment = environment;
	env->args = args;
	return env;
}

static val_t lambda(val_t *VSTACK, array_t *a) {
	val_t args = vm_exec(memory + (uintptr_t)array_get(a, 0), VSTACK);
	int i = 1;
	struct array_t *fbody_list = new_array();
	for (; i < array_size(a); i++) {
		int idx = next_index;
		val_t fbody = vm_exec(memory + (uintptr_t)array_get(a, i), VSTACK + i);
		array_add(fbody_list, new_lambda_data(idx, fbody));
		codegen(fbody);
	}
	VSTACK[1] = args;
	//set_func(fcons.ptr, opline_list, argc, args, current_environment, 0);
	val_t lambda_func = null_val();
	lambda_env_t *env = new_lambda_env(args, current_environment);
	lambda_func.ptr = new_lambda(env, fbody_list);
	return lambda_func;
}

static val_t defmacro(val_t *VSTACK, array_t *a) {
	val_t fcons = vm_exec(memory + (uintptr_t)array_get(a, 0), VSTACK);
	if (!IS_SYMBOL(fcons)) {
		EXPECTED("symbol", fcons);
	}
	val_t args = vm_exec(memory + (uintptr_t)array_get(a, 1), VSTACK);
	int i = 2;
	struct array_t *opline_list = new_array();
	for (; i < array_size(a); i++) {
		array_add(opline_list, ((void*)((uintptr_t)next_index)));
		val_t fbody = vm_exec(memory + (uintptr_t)array_get(a, i), VSTACK + i);
		codegen(fbody);
	}
	VSTACK[1] = args;
	int argc = length(VSTACK+2, 1).ivalue;
	set_func(fcons.ptr, opline_list, argc, args, current_environment, FLAG_MACRO | FLAG_LOCAL_SCOPE);
	return fcons;
}

static val_t setq(val_t *VSTACK, int ARGC) {
	val_t variable = ARGS(0);
	val_t value = ARGS(1);
	if (!IS_SYMBOL(variable)) {
		EXPECTED("symbol", variable);
	}
	set_variable(variable.ptr, value, 0);
	return value;
}

static val_t let_inner(val_t *VSTACK, struct array_t *a, int is_star) {
	val_t value_list = vm_exec(memory + (uintptr_t)array_get(a, 0), VSTACK);
	val_t variable = null_val();
	val_t list = null_val();
	val_t value = null_val();
	array_t *a1 = new_array();
	array_t *a2 = new_array();
	if (IS_OPEN(value_list)) {
		while (!IS_nil(value_list)) {
			list = value_list.ptr->car;
			if (IS_OPEN(list)) {
				VSTACK[1] = list;
				//VAL_PRINT(list, _buffer);
				//fprintf(stdout, "\n");
				int argc = length(VSTACK+2, 1).ivalue;
				if (argc == 2) {
					variable = list.ptr->car;
					if (!IS_SYMBOL(variable)) {
						EXPECTED("symbol", variable);
					}
					value = list.ptr->cdr.ptr->car;
					val_t res = eval_inner(VSTACK, value);
					//codegen(value);
					//val_t res = vm_exec(memory + current_index, VSTACK + 1);
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
					EXPECTED("symbol", list);
				}
				if (is_star) {
					set_variable(list.ptr, new_bool(0), 1);
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
	val_t res = null_val();
	int i;
	for (i = 1; i < array_size(a); i++) {
		res = vm_exec(memory + (uintptr_t)array_get(a, i), VSTACK);
	}
	return res;
}

static val_t let(val_t *VSTACK, struct array_t *a) {
	return let_inner(VSTACK, a, 0);
}

static val_t let_star(val_t *VSTACK, array_t *a) {
	return let_inner(VSTACK, a, 1);
}
val_t eval_inner(val_t *VSTACK, val_t val) {
	int idx = next_index;
	codegen(val);
	val_t res = vm_exec(memory + current_index, VSTACK + 1);
	unuse_opline(idx);
	return res;
}
static val_t eval(val_t *VSTACK, int ARGC) {
	//val_t cons = CONS_EVAL(ARGS(0));
	val_t val = ARGS(0);
	return eval_inner(VSTACK, val);
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
	{"CONS", 2, 0, 0, 0, 0, cons, NULL},
	{"CAR", 1, 0, 0, 0, 0, car, NULL},
	{"CDR", 1, 0, 0, 0, 0, cdr, NULL},
	{"EQ", 2, 0, 0, 0, 0, eq, NULL},
	{"PRINT", 1, 0, 0, 0, 0, print, NULL},
	{"FORMAT", -1, 2, FLAG_SPECIAL_FORM, 0, 0, NULL, format},
	{"+", -1, 0, 0, 0, 0, add, NULL},
	{"-", -1, 1, 0, 0, 0, sub, NULL},
	{"*", -1, 0, 0, 0, 0, mul, NULL},
	{"/", -1, 1, 0, 0, 0, _div, NULL},
	{"<", -1, 0, 0, 0, 0, lt, NULL},
	{"STRING<", -1, 1, 0, 0, 0, string_lt, NULL},
	{"<=", -1, 1, 0, 0, 0, lte, NULL},
	{"STRING<=", -1, 1, 0, 0, 0, string_lte, NULL},
	{">", -1, 1, 0, 0, 0, gt, NULL},
	{"STRING>", -1, 1, 0, 0, 0, string_gt, NULL},
	{">=", -1, 1, 0, 0, 0, gte, NULL},
	{"STRING>=", -1, 1, 0, 0, 0, string_gte, NULL},
	{"=", -1, 1, 0, 0, 0, opeq, NULL},
	{"STRING=", -1, 1, 0, 0, 0, string_eq, NULL},
	{"/=", -1, 1, 0, 0, 0, neq, NULL},
	{"STRING/=", -1, 1, 0, 0, 0, string_neq, NULL},
	{"NULL", 1, 0, 0, 0, 0, null, NULL},
	{"NOT", 1, 0, 0, 0, 0, not, NULL},
	{"SQRT", 1, 0, 0, 0, 0, _sqrt, NULL},
	{"RANDOM", 1, 0, 0, 0, 0, _random, NULL},
	{"SIN", 1, 0, 0, 0, 0, _sin, NULL},
	{"COS", 1, 0, 0, 0, 0, _cos, NULL},
	{"TAN", 1, 0, 0, 0, 0, _tan, NULL},
	{"FLOOR", 1, 0, 0, 0, 0, _floor, NULL},
	{"ATOM", 1, 0, 0, 0, 0, atom, NULL},
	{"CONSP", 1, 0, 0, 0, 0, consp, NULL},
	{"LISTP", 1, 0, 0, 0, 0, listp, NULL},
	{"SYMBOLP", 1, 0, 0, 0, 0, symbolp, NULL},
	{"NUMBERP", 1, 0, 0, 0, 0, numberp, NULL},
	{"INTEGERP", 1, 0, 0, 0, 0, integerp, NULL},
	{"ZEROP", 1, 0, 0, 0, 0, zerop, NULL},
	{"QUOTE", 1, 0, 0, 1, 1, quote, NULL},
	{"LIST", -1, 0, 0, 0, 0, list, NULL},
	{"LENGTH", 1, 0, 0, 0, 0, length, NULL},
	{"SVREF", 2, 0, 0, 0, 0, svref, NULL},
	{"SYSTEM::SVSTORE", 3, 0, 0, 0, 0, svstore, NULL},
	{"VECTOR", -1, 0, 0, 0, 0, _vector, NULL},
	{"MAKE-ARRAY", 1, 0, 0, 0, 0, _make_array, NULL},
	{"AND", -1, 0, FLAG_SPECIAL_FORM, 0, 0, NULL, _and},
	{"OR", -1, 0, FLAG_SPECIAL_FORM, 0, 0, NULL, _or},
	{"IF", 3, 0, FLAG_SPECIAL_FORM, 0, 0, NULL, _if},
	{"ASSERT", 1, 0, FLAG_SPECIAL_FORM, 0, 0, NULL, _assert},
	{"COND", -1, 0, FLAG_SPECIAL_FORM, -1, 0, NULL, cond},
	{"PROGN", -1, 0, FLAG_SPECIAL_FORM, 0, 0, NULL, progn},
	{"LOOP", -1, 0, FLAG_SPECIAL_FORM, 0, 0, NULL, loop},
	{"DOLIST", -1, 1, FLAG_SPECIAL_FORM | FLAG_LOCAL_SCOPE, 1, 0, NULL, dolist},
	{"DOTIMES", -1, 1, FLAG_SPECIAL_FORM | FLAG_LOCAL_SCOPE, 1, 0, NULL, dotimes},
	{"BLOCK", -1, 1, FLAG_SPECIAL_FORM, 1, 0, NULL, block},
	{"RETURN", -1, 0, 0, 0, 0, _return, NULL},
	{"RETURN-FROM", -1, 1, 0, 1, 0, _return_from, NULL},
	{"WHEN", -1, 1, FLAG_SPECIAL_FORM, 0, 0, NULL, when},
	{"UNLESS", -1, 1, FLAG_SPECIAL_FORM, 0, 0, NULL, unless},
	{"DEFUN", -1, 2, FLAG_SPECIAL_FORM | FLAG_LOCAL_SCOPE, -1, 0, NULL, defun},
	{"LAMBDA", -1, 1, FLAG_SPECIAL_FORM | FLAG_LOCAL_SCOPE, -1, 0, NULL, lambda},
	{"DEFMACRO", -1, 2,FLAG_SPECIAL_FORM | FLAG_LOCAL_SCOPE, -1, 0, NULL, defmacro},
	{"SETQ", 2, 0, 0, 1, 0, setq, NULL},
	{"FUNCALL", -1, 1, 0, 0, 0, funcall, NULL},
	{"LET", -1, 1, FLAG_SPECIAL_FORM | FLAG_LOCAL_SCOPE, 1, 0, NULL, let},
	{"LET*", -1, 1, FLAG_SPECIAL_FORM | FLAG_LOCAL_SCOPE, 1, 0, NULL, let_star},
	{"EVAL", 1, 0, 0, 0, 0, eval, NULL},
	{NULL, 0, 0, 0, 0, 0, NULL, NULL},
};
