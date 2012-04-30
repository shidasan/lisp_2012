#include "lisp.h"

int main() {
	gc_init();
	new_func_data_table();
	new_global_environment();
	array_t *a = new_array();
	int i = 0;
	for (; i < 10; i++) {
		array_add_val(a, new_int(i));
	}
	for (i = 0; i < 10; i++) {
		val_t val = array_pop_val(a);
		fprintf(stderr, "val.ivalue:%d\n", val.ivalue);
	}
	return 0;
}
