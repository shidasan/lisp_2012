#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lisp.h"

/* Array */

void *array_get(array_t *a, size_t n) {
	if (n < 0 || n >= a->size) {
		fprintf(stderr, "out of bounds\n");
		asm("int3");
	}
	return a->list[n];
}

size_t array_size(array_t *a) {
	return a->size;
}

void array_set(array_t *a, size_t n, void *v) {
	if (n < 0 || n >= a->size) {
		fprintf(stderr, "out if bounds\n");
	}
	a->list[n] = v;
}

void array_add(array_t *a, void *v) {
	if (a->size < a->capacity) {
		a->list[a->size] = v;
		a->size++;
	} else {
		size_t newcapacity = a->capacity * 2;
		void **newlist = (void**)malloc(sizeof(void**) * newcapacity);
		memcpy(newlist, a->list, sizeof(void**) * newcapacity);
		free(a->list);
		a->list = newlist;
		a->capacity = newcapacity;
		a->list[a->size] = v;
		a->size++;
	}
}

void *array_pop(array_t *a) {
	if (a->size <= 0) {
		return NULL;
	}
	a->size--;
	return a->list[a->size];
}

void array_trace(array_t *a, array_t *traced) {
	int i = 0;
	for (; i < array_size(a); i++) {
		ADDREF(((cons_t*)array_get(a, i)), traced);
	}
}

array_t *new_array() {
	array_t *a = (array_t *)malloc(sizeof(array_t));
	a->capacity = 8;
	a->size = 0;
	a->list = (void**)malloc(sizeof(void*) * a->capacity);
	return a;
}

void array_free(array_t *a) {
	free(a->list);
	free(a);
}

/* AST */

ast_t *new_ast(int type, int sub_type) {
	ast_t *ast = (ast_t *)malloc(sizeof(ast_t));
	memset(ast, 0, sizeof(ast_t));
	ast->type = type;
	ast->sub_type = sub_type;
	ast->a = new_array();
	return ast;
}

void ast_free(ast_t *ast) {
	if (ast->type == ast_list) {
		array_free(ast->a);
	}
	if (ast->type == ast_func) {
		free(ast->str);
	}
	if (ast->type == ast_variable) {
		free(ast->str);
	}
	free(ast);
}

/* allocator */

cons_t *free_list = NULL;
static size_t unused_object;
static size_t object_capacity;

typedef struct cons_page_h_t {
	/* sizeof(cons_page_h_t) must be less than sizeof(cons_t) */
	uintptr_t allocated_ptr;
	uintptr_t *bitmap;
	uintptr_t *tenure;
	uintptr_t *unused;
}cons_page_h_t;	

typedef struct cons_page_t {
	cons_page_h_t h;
	cons_t slots[PAGECONSSIZE];
} cons_page_t;

typedef struct cons_tbl_t{
	cons_page_t *head;
	cons_page_t *bottom;
	size_t           size;
	size_t           bitmapsize;
	uintptr_t       *bitmap;
	uintptr_t       *tenure;
} cons_tbl_t;

typedef struct cons_arena_t {
	/* list of cons_tbl_t */
	array_t     *a;
}cons_arena_t;

static cons_arena_t *cons_arena;

void *valloc(size_t size) {
	void *block = malloc(size + PAGESIZE);
	if (block == NULL/* unlikely */) {
		fprintf(stderr, "OutOfMemory!!\n");
		exit(1);
	}
	if ((uintptr_t)block % PAGESIZE != 0) {
		char *t2 = (char*)((((uintptr_t)block / PAGESIZE) + 1) * PAGESIZE);
		void **p = (void**)(t2/* + size */);
		memset(t2, 0, size);
		p[0] = block;
		block = (void*)t2;
	} else {
		memset(block, 0, size);
		void **p = (void**) ((char*)block);
		p[0] = block;
	}
	return block;
}

static void page_init(cons_page_t *page) {
	size_t i;
	cons_t *cons = page->slots;
	for (i = 0; i < PAGECONSSIZE - 1; i++) {
		cons[i].cdr = &(cons[i+1]);
	}
	/* last slot in each page */
	page->slots[PAGECONSSIZE-1].cdr = page[1].slots;
}

static cons_tbl_t *new_page_table() {
	cons_tbl_t *tbl = NULL;
	tbl = (cons_tbl_t *)malloc(sizeof(cons_tbl_t));
	memset(tbl, 0, sizeof(cons_tbl_t));
	cons_page_t *page = (cons_page_t *)valloc(ARENASIZE);
	tbl->head = page;
	tbl->bottom = (cons_page_t *)(((char *)page) + ARENASIZE);
	size_t bitmapsize = (ARENASIZE/sizeof(cons_t));
	uintptr_t *bitmap = (uintptr_t *)malloc(bitmapsize);
	memset(bitmap, 0, bitmapsize);
	tbl->bitmap = bitmap;
	tbl->bitmapsize = bitmapsize;
	fprintf(stderr, "pageconssize: %zd\n", PAGECONSSIZE);
	unused_object += PAGECONSSIZE * 16;
	object_capacity += PAGECONSSIZE * 16;
	for (; page < tbl->bottom; page++) {
		fprintf(stderr ,"page: %p\n", page);
		page->h.bitmap = bitmap;
		fprintf(stderr, "bitmap%p\n", bitmap);
		bitmap += (PAGESIZE / sizeof(cons_t)) / sizeof(uintptr_t);
		fprintf(stderr, "bitmapsize: %zd, %zd\n", bitmapsize, (PAGESIZE / sizeof(cons_t)) / sizeof(uintptr_t));
		page_init(page);
	}
	/* last slot in last page of cons_tbl */
	(page-1)->slots[PAGECONSSIZE-1].cdr = NULL;
	return tbl;
}

static cons_arena_t *new_cons_arena() {
	cons_arena_t *arena = (cons_arena_t *)malloc(sizeof(cons_arena_t));
	arena->a = new_array();
	array_add(arena->a, new_page_table());
	free_list = ((cons_tbl_t*)array_get(arena->a, 0))->head->slots;
	cons_arena = arena;
}

static int cons_is_marked(cons_t *cons) {
	cons_page_t *page = (cons_page_t*)((((uintptr_t)cons) / PAGESIZE) * PAGESIZE);
	size_t offset = (((uintptr_t)cons) / sizeof(cons_t)) % (PAGESIZE / sizeof(cons_t));
	//int x = offset / (sizeof(uintptr_t));
	int x = offset / (sizeof(uintptr_t) * 8);
	fprintf(stderr, "is_marked cons: %p, page: %p, offset: %zd, x: %d shift: %lo\n", cons, page, offset, x, (offset % (sizeof(uintptr_t) * 8)));
	if (!(page->h.bitmap[x] & (uintptr_t)1 << (offset % (sizeof(uintptr_t) * 8)))) {
		page->h.bitmap[x] |= (uintptr_t)1 << (offset % (sizeof(uintptr_t) * 8));
		return 0;
	}
	return 1;
}
static void mark_stack(array_t *traced) {
	int i = 0;
	for (; i < STACKSIZE; i++) {
		if (stack_value[i] != NULL) {
			fprintf(stderr, "stack addref %p\n", stack_value[i]);
			ADDREF(stack_value[i], traced);
			CONS_TRACE(stack_value[i], traced);
		}
	}
}

static void mark_opline(array_t *traced) {
	int i = 0;
	fprintf(stderr, "mark opline\n");
	for (; i < NextIndex; i++) {
		opline_t *op = memory + i;
		switch(op->instruction) {
			case PUSH:
			case VARIABLE_PUSH:
			case MTDCHECK:
			case MTDCALL:
				ADDREF(op->op[0].cons, traced);
				break;
			case SPECIAL_MTD:
				ADDREF(op->op[0].cons, traced);
				//array_trace(op->op[1].a, traced);
				break;
			default:
				break;
		}
	}
	fprintf(stderr, "mark opline end\n");
}

static void mark_root(array_t *traced) {
	//ADDREF_NULLABLE(root, traced);
	fprintf(stderr, "current_environment %d\n", current_environment->type);
	fprintf(stderr, "root num: %zd\n", array_size(traced));
	ADDREF(current_environment, traced);
	fprintf(stderr, "root num: %zd\n", array_size(traced));
	mark_stack(traced);
	fprintf(stderr, "root num: %zd\n", array_size(traced));
	mark_func_data_table(traced);
	fprintf(stderr, "root num: %zd\n", array_size(traced));
	mark_opline(traced);
	fprintf(stderr, "root num: %zd\n", array_size(traced));
	mark_environment_list(traced);
	fprintf(stderr, "root num: %zd\n", array_size(traced));
}

static void gc_mark() {
	size_t i;
	array_t *ostack = new_array();
	cons_t *cons = NULL;
	array_t *a = cons_arena->a;
	array_t *traced = new_array();
	mark_root(traced);
	goto LOOP;
	while ((cons = (cons_t*)array_pop(ostack)) != NULL) {
		CONS_TRACE(cons, traced);
		LOOP:
		{
		cons_t *tmp = NULL;
		while((tmp = (cons_t*)array_pop(traced)) != NULL) {
		//for (i = 0; i < array_size(traced); i++) {
			//cons_t *tmp = (cons_t*)array_pop(traced);
			if (!cons_is_marked(tmp)) {
				array_add(ostack, tmp);
			}
		}
		}
	}
}
static int count = 0;
static void gc_sweep() {
	uintptr_t i, j;
	count = 0;
	cons_page_t *page;
	for (i = 0; i < array_size(cons_arena->a); i++) {
		array_t *a = cons_arena->a;
		cons_tbl_t *tbl = (cons_tbl_t *)array_get(a, i);
		for (page = tbl->head; page < tbl->bottom; page++) {
			for (j = 1; j <= PAGECONSSIZE; j++) {
				int x = j / (sizeof(uintptr_t) * 8);
				if (!(page->h.bitmap[x] & ((uintptr_t)1 << (j % (sizeof(uintptr_t) * 8))))) {
					if ((page->slots+j-1)->api) {
						CONS_FREE(page->slots + j-1);
					}
					memset(page->slots + j-1, 0, sizeof(cons_t));
					page->slots[j-1].cdr = free_list;
					free_list = &page->slots[j-1];
					unused_object++;
				} else {
					count++;
					fprintf(stderr, "survive: %p\n", page->slots + j-1);
				}
			}
		}
	}
	fprintf(stderr, "survived count %d\n", count);
}

static void clear_bitmap() {
	int i = 0;
	for (; i < array_size(cons_arena->a); i++) {
		cons_tbl_t *tbl = (cons_tbl_t*)array_get(cons_arena->a, i);
		memset(tbl->bitmap, 0, tbl->bitmapsize);
	}
}

static void gc() {
	fprintf(stderr, "root: %p\n", root);
	//fprintf(stderr, "PAGECONSSIZE %zd\n", PAGECONSSIZE);
	//fprintf(stderr, "PAGESIZE %d\n", PAGESIZE);
	fprintf(stderr, "gc()\n");
	clear_bitmap();
	gc_mark();
	gc_sweep();
	fprintf(stderr, "gc end object_capacity: %zd, unused_object: %zd\n", object_capacity, unused_object);
}

cons_t *new_cons_cell() {
	cons_t *cons = NULL;
	if (free_list == NULL) {
		gc();
		if ((object_capacity / 4) > unused_object) {
			cons_tbl_t *tbl = new_page_table();
			array_add(cons_arena->a, tbl);
			free_list = tbl->head->slots;
		}
	}
	cons = free_list;
	free_list = free_list->cdr;
	unused_object--;
	memset(cons, 0, sizeof(cons_t));
	//if ((uintptr_t)cons == 0x10080fc20) {
	//	asm("int3");
	//}
	//fprintf(stderr, "new_cons_tree() %p\n", cons);
	return cons;
}

void gc_init() {
	fprintf(stderr, "sizeof page_h_t %zd\n", sizeof(cons_page_h_t));
	new_cons_arena();
}

//int main() {
//	new_cons_arena();
//	while(1) {
//		cons_t *cons = new_cons_cell();
//		fprintf(stderr, "object_capacity: %zd, unused_object: %zd\n", object_capacity, unused_object);
//	}
//}
