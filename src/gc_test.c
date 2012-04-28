#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lisp.h"

int main() {
	gc_init();
	new_func_data_table();
	new_global_environment();
	//root = NULL;
	//while (1) {
	//	root = new_open();
	//}
	return 0;
}
