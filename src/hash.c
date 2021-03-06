#include "lisp.h"
/* list of variable_data table */
cons_t *current_environment = NULL;
cons_t *root_environment = NULL;
array_t *environment_list = NULL;
/* function_data table */
func_t *func_data_table = NULL;

void new_func_data_table() {
	func_data_table = (func_t*)malloc(sizeof(func_t) * HASH_SIZE);
	memset(func_data_table, 0, sizeof(func_t) * HASH_SIZE);
}

void mark_func_data_table(array_t *traced) {
	int i = 0;
	for (; i < HASH_SIZE; i++) {
		func_t *func = func_data_table + i;
		while (func != NULL) {
			if (func->name) {
				ADDREF_NULLABLE(func->environment, traced);
				ADDREF_VAL_NULLABLE(func->args, traced);
			}
			func = func->next;
		}
	}
}

void mark_variable_data_table(variable_t *table, array_t *traced) {
	//printf("mark table: %p\n", table);
	int i = 0;
	for (; i < HASH_SIZE; i++) {
		variable_t *variable = table + i;
		while (variable != NULL) {
			if (variable->name) {
				ADDREF_VAL_NULLABLE(variable->cons, traced);
			}
			variable = variable->next;
		}
	}
	//printf("mark table end: %p\n", table);
}

void mark_environment_list(array_t *traced) {
	int i = 0;
	for (; i < array_size(environment_list); i++) {
		ADDREF((cons_t*)array_get(environment_list, i), traced);
	}
}

cons_t *change_local_scope(cons_t *old_environment, cons_t *environment) {
	cons_t *new_environment = new_local_environment();
	cstack_cons_cell_push(new_environment);
	cons_t *table = new_variable_data_table();
	new_environment->car.ptr = table;
	new_environment->cdr.ptr = environment;
	current_environment = new_environment;
	cstack_cons_cell_pop();
	return old_environment;
}

cons_t *begin_local_scope(func_t *func) {
	cons_t *old_environment = current_environment;
	cstack_cons_cell_push(old_environment);
	if (FLAG_CREATES_LOCAL_SCOPE(func->flag)) {
		current_environment = new_local_environment();
		current_environment->car.ptr = new_variable_data_table();
		current_environment->cdr.ptr = old_environment;
	}
	cstack_cons_cell_pop();
	return old_environment;
}

cons_t *end_local_scope(cons_t *old_environment) {
	current_environment = old_environment;
	return NULL;
}

void new_global_environment() {
	current_environment = new_local_environment();
	root_environment = current_environment;
	current_environment->car.ptr = new_variable_data_table();
	environment_list = new_array();
}

void environment_clear() {
	array_free(environment_list);
	environment_list = new_array();
	current_environment = root_environment;
}

void environment_list_push(cons_t *environment) {
	array_add(environment_list, environment);
}

cons_t *environment_list_pop() {
	return array_pop(environment_list);
}

void func_data_table_free() {
	func_t *cur = NULL;
	int i;
	for (i = 0;(unsigned int)i < HASH_SIZE; i++){
		cur = func_data_table + i;
		while (cur){
			if (cur->name) {
				func_t *tmp = cur->next;
				if (!FLAG_IS_STATIC(cur->flag)) {
					free(cur->name);
					array_free(cur->opline_list);
				}
				if (cur != func_data_table + i) {
					free(cur);
				}
				cur = tmp;
			} else {
				break;
			}
		}
	}
	free(func_data_table);
}

struct val_t set_variable_inner (cons_t *table, cons_t *cons, val_t value, int is_end_of_table_list)
{
	const char *str = cons->str;
	variable_t* p = table->variable_data_table + ((str[0] * str[1]) % HASH_SIZE);
	while (1){
		if (p->name == NULL) {
			if (is_end_of_table_list) {
				p->name = (char*)malloc(strlen(str)+1);
				strcpy (p->name, str);
				p->name[strlen(str)] = '\0';
				p->cons = value;
				return p->cons;
			} else {
				val_t res = null_val();
				return res;
			}
		} else if (strcmp(p->name, str) == 0) {
			p->cons = value;
			return p->cons;
		} else if (p->next == NULL) {
			if (is_end_of_table_list) {
				p->next = (variable_t*)malloc(sizeof(variable_t));
				memset(p->next, 0, sizeof(variable_t));
				p = p->next;
			} else {
				return null_val();
			}
		} else {
			p = p->next;
		}
	}
}

struct val_t set_variable(cons_t *cons, val_t value, int set_local_scope) {
	cons_t *environment = current_environment;
	cons_t *table = environment->car.ptr;
	//fprintf(stderr, "set_variable, car->type: %d\n", table->type);
	val_t res = set_variable_inner(table, cons, value, set_local_scope | (environment->cdr.ptr == NULL));
	while (!IS_UNBOX(res) && res.ptr == NULL) {
	//fprintf(stderr, "set_variable, car->type: %d\n", table->type);
		environment = environment->cdr.ptr;
		table = environment->car.ptr;
		res = set_variable_inner(table, cons, value, set_local_scope | (environment->cdr.ptr == NULL));
	}
	return res;
}

struct val_t search_variable_inner (cons_t *table, const char* str)
{
	variable_t* p = table->variable_data_table + ((str[0] * str[1]) % HASH_SIZE);
	val_t res = null_val();
	while (1){
		if (p->name == NULL) {
			res.ivalue = 0;
			return res;
		} else if (strcmp(p->name, str) == 0) {
			return p->cons;
		} else if (p->next != NULL) {
			p = p->next;
		} else {
			res.ivalue = 0;
			return res;
		}
	}
}

struct val_t search_variable(const char *str) {
	cons_t *environment = current_environment;
	cons_t *table = environment->car.ptr;
	val_t res = search_variable_inner(table, str);
	while (IS_NULL(res)) {
		if (environment->cdr.ptr == NULL) {
			return res;
		} else {
			environment = environment->cdr.ptr;
			table = environment->car.ptr;
		}
		res = search_variable_inner(table, str);
	}
	//fprintf(stderr, "search_variable: %p type: %d\n", res, res->type);
	return res;
}

struct func_t* search_func (const char* str)
{
	struct func_t* p = func_data_table + ((str[0] * str[1]) % HASH_SIZE);
	while (1){
		if (p->name != NULL && strcmp (p->name, str) == 0){
			return p;
		} else if (p->next != NULL){
			p = p->next;
		} else {
			return NULL;
		}
	}
}

struct func_t* set_static_func (static_mtd_data *data) {
	const char *str = data->name;
	int flag = data->flag;
	void *adr = (void*)data->mtd;
	void *special_mtd = (void*)data->special_mtd;
	int *is_quote = &(data->is_quote0);
	func_t* p = func_data_table + ((str[0] * str[1]) % HASH_SIZE);
	while (1){
		if (p->name == NULL || strcmp(p->name,str) == 0){
			if (p->name != NULL && strcmp(p->name, str) == 0) {
				return NULL;
			}

			if (p->name == NULL){
				p->name = (char*)malloc(strlen(str)+1);
				strncpy (p->name, str, strlen(str));
				p->name[strlen(str)] = '\0';
			}
			p->value = data->num_args;
			p->value_minimum = data->num_args_minimum;
			p->flag = flag | FLAG_STATIC;
			p->is_quote = is_quote;
			if (FLAG_IS_SPECIAL_FORM(flag)) {
				p->special_mtd = special_mtd;
			} else {
				p->mtd = adr;
			}
			return p;
		} else if (p->next == NULL){
			p->next = (func_t*)malloc(sizeof(func_t));
			memset(p->next, 0, sizeof(func_t));
			p = p->next;
		} else {
			p = p->next;
		}
	}
}

struct func_t* set_func (cons_t *cons, struct array_t *opline_list, int argc, val_t args, cons_t *current_environment, int flag) {
	const char *str = cons->str;
	func_t* p = func_data_table + ((str[0] * str[1]) % HASH_SIZE);
	while (1){
		if (p->name == NULL || strcmp(p->name,str) == 0){
			if (p->name != NULL && strcmp(p->name, str) == 0 && FLAG_IS_STATIC(p->flag)) {
				EXCEPTION("Cannot overwrite static function!!\n");
				return NULL;
			}
			if (p->name == NULL){
				p->name = (char*)malloc(strlen(str)+1);
				strncpy (p->name, str, strlen(str));
				p->name[strlen(str)] = '\0';
			}
			p->value = argc;
			p->flag = flag;
			p->is_quote = 0;
			p->opline_list = opline_list;
			p->args = args;
			p->environment = current_environment;
			return p;
		} else if (p->next == NULL){
			p->next = (func_t*)malloc(sizeof(func_t));
			memset(p->next, 0, sizeof(variable_t));
			p = p->next;
		} else {
			p = p->next;
		}
	}
}
