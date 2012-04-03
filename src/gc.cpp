#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lisp.h"
#include "gc.h"

/* allocator */
cons_t *free_list = NULL;

typedef struct cons_page_h_t {
	/* sizeof(cons_page_h_t) must be less than sizeof(cons_t) */
	uintptr_t allocated_ptr;
	uintptr_t *bitmap;
	uintptr_t *tenure;
}cons_page_h_t;	

typedef struct cons_page_t {
	cons_page_h_t h;
	cons_t slots[PAGECONSSIZE];
} cons_page_t;

typedef struct cons_page_tbl_t{
	cons_page_tbl_t *head;
	cons_page_tbl_t *bottom;
	size_t           size;
	uintptr_t       *bitmap;
	uintptr_t        bitmapsize;
	uintptr_t       *tenure;
} cons_page_tbl_t;

static cons_page_tbl_t *cons_page_tbl;

void *valloc(size_t size) {
	void *block = malloc(size + PAGESIZE);
	if (block == NULL/* unlikely */) {
		fprintf(stderr, "OutOfMemory!!\n");
		exit(1);
	}
	if ((uintptr_t)block % PAGESIZE != 0) {
		char *t2 = (char*)((((uintptr_t)block / PAGESIZE) + 1) * PAGESIZE);
		void **p = (void**)(t2/* + size */);
		fprintf(stderr, "%p\n", block);
		p[0] = block;
		block = (void*)t2;
	} else {
		void **p = (void**) ((char*)block);
		p[0] = block;
	}
	return block;
}

cons_t *new_cons() {
	cons_t *o = NULL;
	//FREELIST_POP(o);
}

static void init_page_table() {
	cons_page_tbl = (cons_page_tbl_t *)malloc(sizeof(cons_page_tbl_t));
	bzero(cons_page_tbl, sizeof(cons_page_tbl_t));
}

cons_t *new_cons_cell() {
	cons_t *cons = NULL;
	if (free_list == NULL) {

	}
}

int main() {
	void* ptr = valloc(PAGESIZE * 16);
	fprintf(stderr, "%p\n", ptr);
	fprintf(stderr, "%p\n", (void*)(*(uintptr_t*)ptr));

	malloc(12345);
	ptr = valloc(PAGESIZE * 16);
	fprintf(stderr, "%p\n", ptr);
	fprintf(stderr, "%p\n", (void*)(*(uintptr_t*)ptr));
}
