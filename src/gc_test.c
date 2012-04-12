#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lisp.h"

int main() {
	gc_init();
	while (1) {
		cons_t *cons = new_cons_cell();
	}
}
