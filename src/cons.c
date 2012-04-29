#include "lisp.h"

void val_to_string(val_t val, string_buffer_t *buffer, int is_root) {
	if (!IS_UNBOX(val) && is_root) {
		switch(val.ptr->type) {
		case OPEN:
			string_buffer_append_c(buffer, '(');
			break;
		case STRING:
			string_buffer_append_c(buffer, '\"');
			break;
		default:
			break;
		}
	}
	switch(VAL_TYPE(val)) {
	case INT_OFFSET:
		string_buffer_append_i(buffer, val.ivalue);
		break;
	case FLOAT_OFFSET:
		string_buffer_append_f(buffer, val.fvalue);
		break;
	case T_OFFSET:
		string_buffer_append_s(buffer, "T");
		break;
	case nil_OFFSET:
		string_buffer_append_s(buffer, "NIL");
		break;
	default:
		val.ptr->api->to_string(val.ptr, buffer);
		break;
	}
	if (!IS_UNBOX(val) && is_root) {
		switch(val.ptr->type) {
		case OPEN:
			string_buffer_append_c(buffer, ')');
			break;
		case STRING:
			string_buffer_append_c(buffer, '\"');
			break;
		default:
			break;
		}
	}
}
static void print_func(cons_t *cons, string_buffer_t *buffer) {
	string_buffer_append_s(buffer, cons->str);
}

static void free_func(cons_t *cons) {
	FREE(cons->str);
}

static void print_variable(cons_t *cons, string_buffer_t *buffer) {
	string_buffer_append_s(buffer, cons->str);
}

static void free_variable(cons_t *cons) {
	FREE(cons->str);
}

static void print_open(cons_t *cons, string_buffer_t *buffer) {
	val_t car = cons->car;
	val_t cdr = cons->cdr;
	if (IS_OPEN(car)) {
		string_buffer_append_s(buffer, "(");
	}
	VAL_TO_STRING(car, buffer, 0);
	if (IS_OPEN(car)) {
		string_buffer_append_s(buffer, ")");
	}
	if (IS_OPEN(cdr)) {
		if (!IS_nil(cdr.ptr->cdr) && !IS_OPEN(cdr.ptr->cdr)) {
			string_buffer_append_s(buffer, " . ");
		} else {
			string_buffer_append_s(buffer, " ");
		}
		if (!IS_nil(cdr.ptr->cdr) && !IS_OPEN(cdr.ptr->cdr)) {
			string_buffer_append_s(buffer, "(");
		}
		VAL_TO_STRING(cons->cdr, buffer, 0);
		if (!IS_nil(cdr.ptr->cdr) && !IS_OPEN(cdr.ptr->cdr)) {
			string_buffer_append_s(buffer, ")");
		}
	} else {
		if (!IS_nil(cdr)) {
			string_buffer_append_s(buffer, " . ");
			if (IS_OPEN(cdr)) {
				string_buffer_append_s(buffer, "(");
			}
			VAL_TO_STRING(cons->cdr, buffer, 0);
			if (IS_OPEN(cdr)) {
				string_buffer_append_s(buffer, ")");
			}
		}
	}
}

static void trace_open(cons_t *cons, struct array_t *traced) {
	ADDREF_VAL(cons->car, traced);
	ADDREF_VAL_NULLABLE(cons->cdr, traced);
}

static void free_variable_table(cons_t *cons) {
	int i;
	variable_t *table = cons->variable_data_table;
	variable_t *tmp, *cur;
	for (i = 0;i < HASH_SIZE; i++){
		cur = table + i;
		while (cur != NULL){
			if (cur->name != NULL) {
				tmp = cur->next;
				FREE(cur->name);
				if (cur > table + HASH_SIZE) {
					FREE(cur);
				}
				cur = tmp;
			} else {
				break;
			}
		}
	}
	FREE(cons->variable_data_table);
}

static void trace_variable_table(cons_t *cons, struct array_t *traced) {
	mark_variable_data_table(cons->variable_data_table, traced);
}

static void trace_local_environment(cons_t *cons, struct array_t *traced) {
	ADDREF(cons, traced);
	ADDREF_VAL_NULLABLE(cons->cdr, traced);
	ADDREF_VAL(cons->car, traced);
}

static void print_string(cons_t *cons, string_buffer_t *buffer) {
	string_buffer_append_s(buffer, cons->str);
}

static void free_string(cons_t *cons) {
	FREE(cons->str);
}

static void print_lambda(cons_t *cons, string_buffer_t *buffer) {
	string_buffer_append_s(buffer, "<function :lambda ");
	VAL_TO_STRING(cons->env->args, buffer, 1);
	string_buffer_append_c(buffer, ' ');
	int i = 0;
	array_t *a = cons->cdr.a;
	for (; i < (int)array_size(a); i++) {
		lambda_data_t *data = (lambda_data_t*)array_get(a, i);
		VAL_TO_STRING(data->body, buffer, 1);

		if (i != (int)array_size(a)-1) {
			string_buffer_append_c(buffer, ' ');
		}
	}
	string_buffer_append_c(buffer, '>');
}

static void free_lambda(cons_t *cons) {
	array_t *a = cons->cdr.a;
	int i = 0;
	for (; i < (int)array_size(a); i++) {
		lambda_data_t *data = (lambda_data_t*)array_get(a, i);
		FREE(data);
	}
	array_free(a);
}

static void trace_lambda(cons_t *cons, array_t *traced) {
	ADDREF_VAL(cons->car, traced);
}


static void print_cons_array(cons_t *cons, string_buffer_t *buffer) {
	string_buffer_append_s(buffer, "#(");
	int size = cons->car.ivalue, i = 0;
	for (; i < size; i++) {
		VAL_TO_STRING(cons->list[i], buffer, 1);
		if (i != size-1) {
			string_buffer_append_c(buffer, ' ');
		}
	}
	string_buffer_append_c(buffer, ')');
}

static void free_cons_array(cons_t *cons) {
	free(cons->list);
}

static void trace_cons_array(cons_t *cons, array_t *traced) {
	int i = 0;
	for (; i < cons->size; i++) {
		ADDREF_VAL(cons->list[i], traced);
	}
}

static void default_print(cons_t *cons, string_buffer_t *buffer) {
	(void)cons;(void)buffer;
}

static void default_free(cons_t *cons) {
	(void)cons;
}

static void default_trace(cons_t *cons, array_t *a) {
	(void)cons;(void)a;
}

struct cons_api_t cons_func_api = {print_func, free_func, default_trace};
struct cons_api_t cons_variable_api = {print_variable, free_variable, default_trace};
struct cons_api_t cons_open_api = {print_open, default_free, trace_open};
struct cons_api_t cons_string_api = {print_string, free_string, default_trace};
struct cons_api_t cons_lambda_api = {print_lambda, free_lambda, trace_lambda};
struct cons_api_t cons_array_api = {print_cons_array, free_cons_array, trace_cons_array};
struct cons_api_t cons_variable_table_api = {default_print, free_variable_table, trace_variable_table};
struct cons_api_t cons_local_environment_api = {default_print, default_free, trace_local_environment};

val_t null_val() {
	val_t res;
	res.ptr = NULL;
	return res;
}
val_t new_float(float f) {
	val_t res = null_val();
	res.tag = FLOAT_OFFSET;
	res.fvalue = f;
	return res;
}
val_t new_int(int n) {
	val_t res = null_val();
	res.tag = INT_OFFSET;
	res.ivalue = n;
	return res;
}

val_t new_bool(int n) {
	val_t res = null_val();
	res.tag = (n) ? T_OFFSET : nil_OFFSET;
	return res;
}

cons_t *new_func(char *str, cons_t *environment) {
	cons_t *cons = new_cons_cell();
	cons->type = FUNC;
	cons->api = &cons_func_api;
	cons->str = str;
	//char *newstr = (char *)malloc(strlen(str)+1);
	//memcpy(newstr, str, strlen(str)+1);
	//newstr[strlen(str)] = '\0';
	//cons->str = newstr;
	cons->local_environment = environment;
	return cons;
}

cons_t *new_variable(char *str) {
	cons_t *cons = new_cons_cell();
	cons->type = VARIABLE;
	cons->api = &cons_variable_api;
	cons->str = str;
	//char *newstr = (char *)malloc(strlen(str)+1);
	//memcpy(newstr, str, strlen(str)+1);
	//newstr[strlen(str)] = '\0';
	//cons->str = newstr;
	return cons;
}

cons_t *new_open() {
	cons_t *cons = new_cons_cell();
	cons->type = OPEN;
	cons->api = &cons_open_api;
	return cons;
}

cons_t *new_variable_data_table() {
	cons_t *cons = new_cons_cell();
	cons->type = VARIABLE_TABLE;
	cons->api = &cons_variable_table_api;
	cons->variable_data_table = (variable_t*)malloc(sizeof(variable_t) * HASH_SIZE);
	memset(cons->variable_data_table, 0, sizeof(variable_t) * HASH_SIZE);
	return cons;
}

/* its car type must variable_data_table */
cons_t *new_local_environment() {
	cons_t *cons = new_cons_cell();
	cons->type = LOCAL_ENVIRONMENT;
	cons->api = &cons_local_environment_api;
	return cons;
}

cons_t *new_string(char *str) {
	cons_t *cons = new_cons_cell();
	cons->type = STRING;
	cons->api = &cons_string_api;
	cons->str = str;
	//int len = strlen(str);
	//cons->str = (char*)malloc(len+1);
	//memcpy(cons->str, str, len);
	//cons->str[len] = '\0';
	return cons;
}

cons_t *new_lambda(lambda_env_t *env, array_t *a) {
	cons_t *cons = new_cons_cell();
	cons->type = LAMBDA;
	cons->api = &cons_lambda_api;
	cons->env = env;
	cons->cdr.a = a;
	return cons;
}

cons_t *new_cons_array(val_t val) {
	cons_t *cons = new_cons_cell();
	cons->type = ARRAY;
	cons->api = &cons_array_api;
	val_t car = null_val();
	array_t *a = new_array();
	if (!IS_nil(val)) {
		while (!IS_NULL(val) && !IS_nil(val)) {
			car = val.ptr->car;
			array_add_val(a, car);
			val = val.ptr->cdr;
		}
	}
	cons->size = array_size(a);
	cons->list = (val_t*)a->list;
	FREE(a);
	return cons;
}
cons_t *new_cons_array_list(array_t *a) {
	cons_t *cons = new_cons_cell();
	cons->type = ARRAY;
	cons->api = &cons_array_api;
	cons->size = array_size(a);
	cons->list = (val_t*)a->list;
	return cons;
}
