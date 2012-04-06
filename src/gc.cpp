#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lisp.h"

/* Array */

typedef struct array_t {
	void **list;
	size_t size;
	size_t capacity;
}array_t;

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

array_t *new_array() {
	array_t *a = (array_t *)malloc(sizeof(array_t));
	a->capacity = 8;
	a->size = 0;
	a->list = (void**)malloc(sizeof(void*) * a->capacity);
	return a;
}

void free_array(array_t *a) {
	free(a->list);
	free(a);
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
		bzero(t2, size);
		p[0] = block;
		block = (void*)t2;
	} else {
		bzero(block, size);
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
	bzero(tbl, sizeof(cons_tbl_t));
	cons_page_t *page = (cons_page_t *)valloc(ARENASIZE);
	tbl->head = page;
	tbl->bottom = (cons_page_t *)(((char *)page) + ARENASIZE);
	size_t bitmapsize = (ARENASIZE/sizeof(cons_t));
	uintptr_t *bitmap = (uintptr_t *)malloc(bitmapsize);
	bzero(bitmap, bitmapsize);
	tbl->bitmap = bitmap;
	tbl->bitmapsize = bitmapsize;
	unused_object += PAGECONSSIZE * 16;
	object_capacity += PAGECONSSIZE * 16;
	for (; page < tbl->bottom - 1; page++) {
		page->h.bitmap = bitmap;
		bitmap += (PAGESIZE / sizeof(cons_t)) / sizeof(uintptr_t);
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

static void reftrace(cons_t *cons, array_t *traced) {

}

static int cons_is_marked(cons_t *cons) {
	cons_page_t *page = (cons_page_t*)((((uintptr_t)cons) % PAGESIZE) * PAGESIZE);
	size_t offset = (((uintptr_t)cons) / sizeof(cons_t)) % (PAGESIZE / sizeof(cons_t));
	int x = offset / (sizeof(uintptr_t) * 8);
	if (!(page->h.bitmap[x] & 1 << (offset % (sizeof(uintptr_t) * 8)))) {
		page->h.bitmap[x] |= 1 << offset;
		return 0;
	}
	return 1;
}

static void mark_root(array_t *ostack, array_t *traced) {
	/* TODO mark root */

	size_t i;
	for (i = 0; i < array_size(traced); i++) {
		cons_t *tmp = (cons_t*)array_get(traced, i);
		if (!cons_is_marked(tmp)) {
			array_add(ostack, tmp);
		}
	}
}

static void gc_mark() {
	size_t i;
	array_t *ostack = new_array();
	cons_t *cons = NULL;
	array_t *a = cons_arena->a;
	array_t *traced = new_array();
	mark_root(ostack, traced);
	while ((cons = (cons_t*)array_pop(ostack)) != NULL) {
		reftrace(cons, traced);
		for (i = 0; i < array_size(traced); i++) {
			cons_t *tmp = (cons_t*)array_get(traced, i);
			if (!cons_is_marked(tmp)) {
				array_add(ostack, tmp);
			}
		}
	}
}

static void gc_sweep() {
	size_t i, j;
	cons_page_t *page;
	for (i = 0; i < array_size(cons_arena->a); i++) {
		array_t *a = cons_arena->a;
		cons_tbl_t *tbl = (cons_tbl_t *)array_get(a, i);
		for (page = tbl->head; page < tbl->bottom-1; page++) {
			for (j = 0; j < PAGECONSSIZE; j++) {
				int x = j / (sizeof(uintptr_t) * 8);
				if (!(page->h.bitmap[x] & 1 << (j % (sizeof(uintptr_t) * 8)))) {
					page->slots[j].cdr = free_list;
					free_list = &page->slots[j];
					unused_object++;
				}
			}
		}
	}
}

static void gc() {
	gc_mark();
	gc_sweep();
}

static cons_t *new_cons_cell() {
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
	bzero(cons, sizeof(cons_t));
	return cons;
}

cons_t *new_int(int n) {
	cons_t *cons = new_cons_cell();
	cons->type = NUM;
	cons->ivalue = n;
	return cons;
}

void gc_init() {
	new_cons_arena();
}

//int main() {
//	new_cons_arena();
//	while(1) {
//		cons_t *cons = new_cons_cell();
//		fprintf(stderr, "object_capacity: %zd, unused_object: %zd\n", object_capacity, unused_object);
//	}
//}
