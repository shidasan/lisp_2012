#include "lisp.h"

/* String Buffer */

string_buffer_t *new_string_buffer() {
	string_buffer_t *buffer = (string_buffer_t*)malloc(sizeof(string_buffer_t));
	buffer->capacity = STRING_BUFFER_INIT_SIZE;
	buffer->size = 0;
	buffer->str = (char*)malloc(STRING_BUFFER_INIT_SIZE);
	return buffer;
}

char *string_buffer_to_string(string_buffer_t *buffer) {
	char *res = (char*)malloc(buffer->size + 1);
	memcpy(res, buffer->str, buffer->size);
	res[buffer->size] = '\0';
	return res;
}

void string_buffer_append_s(string_buffer_t *buffer, const char *str) {
	size_t length = strlen(str);
	if (length + buffer->size > buffer->capacity) {
		size_t newcapacity = buffer->capacity * 2;
		char *tmp = (char*)malloc(newcapacity);
		memcpy(tmp, buffer->str, buffer->capacity);
		FREE(buffer->str);
		buffer->str = tmp;
		buffer->capacity = newcapacity;
	}
	memcpy(buffer->str + buffer->size, str, length);
	buffer->size += length;
}

void string_buffer_append_c(string_buffer_t *buffer, char c) {
	if (buffer->size >= buffer->capacity) {
		size_t newcapacity = buffer->capacity * 2;
		char *tmp = (char*)malloc(newcapacity);
		memcpy(tmp, buffer->str, buffer->capacity);
		FREE(buffer->str);
		buffer->str = tmp;
		buffer->capacity = newcapacity;
	}
	buffer->str[buffer->size] = c;
	buffer->size++;
}

void string_buffer_append_i(string_buffer_t *buffer, int i) {
	sprintf(buffer->str + buffer->size, "%d", i);
	string_buffer_append_s(buffer, buffer->str + buffer->size);
}

void string_buffer_append_f(string_buffer_t *buffer, float f) {
	sprintf(buffer->str + buffer->size, "%g", f);
	string_buffer_append_s(buffer, buffer->str + buffer->size);
}

void string_buffer_free(string_buffer_t *buffer) {
	FREE(buffer->str);
	FREE(buffer);
}

/* Array */

void *array_get(array_t *a, int n) {
	if (n < 0 || n >= a->size) {
		EXCEPTION("Array out of bounds!!\n");
	}
	return a->list[n];
}

val_t array_get_val(array_t *a, int n) {
	if (n < 0 || n >= a->size) {
		EXCEPTION("Array out of bounds!!\n");
	}
	return a->val[n];
}

int array_size(array_t *a) {
	return a->size;
}

void array_set(array_t *a, int n, void *v) {
	if (n < 0 || n >= a->size) {
		EXCEPTION("Array out of bounds!!\n");
	}
	a->list[n] = v;
}

void array_add(array_t *a, void *v) {
	if (a->size < a->capacity - 1) {
		a->list[a->size] = v;
		a->size++;
	} else {
		size_t newcapacity = a->capacity * 2;
		void **newlist = (void**)malloc(sizeof(void**) * newcapacity);
		memcpy(newlist, a->list, sizeof(void**) * a->capacity);
		FREE(a->list);
		a->list = newlist;
		a->capacity = newcapacity;
		a->list[a->size] = v;
		a->size++;
	}
}

void array_add_val(array_t *a, val_t v) {
	if (a->size < a->capacity) {
		a->val[a->size] = v;
		a->size++;
	} else {
		size_t newcapacity = a->capacity * 2;
		void **newlist = (void**)malloc(sizeof(void**) * newcapacity);
		memcpy(newlist, a->list, sizeof(void**) * newcapacity);
		FREE(a->list);
		a->list = newlist;
		a->capacity = newcapacity;
		a->val[a->size] = v;
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

val_t array_pop_val(array_t *a) {
	if (a->size <= 0) {
		return null_val();
	}
	a->size--;
	return a->val[a->size];
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
	FREE(a->list);
	FREE(a);
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

/* allocator */

static cons_t *free_list = NULL;
static array_t *cstack_cons_cell_list = NULL;
static size_t unused_object;
static size_t object_capacity;

typedef struct cons_page_h_t {
	/* sizeof(cons_page_h_t) must be less than sizeof(cons_t) */
	void* allocated_ptr;
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
		cons[i].cdr.ptr = &(cons[i+1]);
	}
	/* last slot in each page */
	page->slots[PAGECONSSIZE-1].cdr.ptr = page[1].slots;
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
	unused_object += PAGECONSSIZE * 16;
	object_capacity += PAGECONSSIZE * 16;
	for (; page < tbl->bottom; page++) {
		page->h.bitmap = bitmap;
		bitmap += (PAGESIZE / sizeof(cons_t)) / sizeof(uintptr_t);
		page_init(page);
	}
	/* last slot in last page of cons_tbl */
	(page-1)->slots[PAGECONSSIZE-1].cdr.ptr = free_list;
	return tbl;
}

static void new_cons_arena() {
	cons_arena_t *arena = (cons_arena_t *)malloc(sizeof(cons_arena_t));
	arena->a = new_array();
	array_add(arena->a, new_page_table());
	free_list = ((cons_tbl_t*)array_get(arena->a, 0))->head->slots;
	cons_arena = arena;
}

static void free_object() {
	uintptr_t i, j;
	cons_page_t *page;
	for (i = 0; i < (uintptr_t)array_size(cons_arena->a); i++) {
		array_t *a = cons_arena->a;
		cons_tbl_t *tbl = (cons_tbl_t *)array_get(a, i);
		for (page = tbl->head; page < tbl->bottom; page++) {
			for (j = 1; j <= PAGECONSSIZE; j++) {
				if ((page->slots+j-1)->api) {
					CONS_FREE(page->slots+j-1);
				}
			}
		}
	}
}

static void free_arena() {
	int i = 0;
	for (; i < array_size(cons_arena->a); i++) {
		cons_tbl_t *tbl = array_get(cons_arena->a, i);
		free(tbl->bitmap);
		free(tbl->head->h.allocated_ptr);
		free(tbl);
	}
	array_free(cons_arena->a);
	FREE(cons_arena);
}

static void free_cons_arena() {
	free_object();
	free_arena();
}

static int mark_count = 0;
static int cons_is_marked(cons_t *cons) {
	cons_page_t *page = (cons_page_t*)((((uintptr_t)cons) / PAGESIZE) * PAGESIZE);
	size_t offset = (((uintptr_t)cons) / sizeof(cons_t)) % (PAGESIZE / sizeof(cons_t));
	int x = offset / (sizeof(uintptr_t) * 8);
	if (!(page->h.bitmap[x] & (uintptr_t)1 << (offset % (sizeof(uintptr_t) * 8)))) {
		page->h.bitmap[x] |= (uintptr_t)1 << (offset % (sizeof(uintptr_t) * 8));
		mark_count++;
		return 0;
	}
	return 1;
}
static void mark_stack(array_t *traced) {
	int i = 0;
	for (; i < STACKSIZE; i++) {
		if (stack_value[i].ptr != NULL) {
			ADDREF_VAL(stack_value[i], traced);
		}
	}
}

static void mark_opline(array_t *traced) {
	int i = 0;
	//fprintf(stderr, "mark opline next_index: %d\n", next_index);
	for (; i < next_index; i++) {
		opline_t *op = memory + i;
		switch(op->instruction) {
			case PUSH:
			case GET_VARIABLE:
			case MTDCHECK:
			case MTDCALL:
				ADDREF_VAL(op->op[0].val, traced);
				break;
			case SPECIAL_MTD:
				ADDREF_VAL(op->op[0].val, traced);
				//array_trace(op->op[1].a, traced);
				break;
			default:
				break;
		}
	}
	//fprintf(stderr, "mark opline end\n");
}

static void mark_cstack_cons_cell(array_t *traced) {
	int i = 0;
	for (; i < array_size(cstack_cons_cell_list); i++) {
		ADDREF((cons_t*)array_get(cstack_cons_cell_list, i), traced);
	}
}

static void mark_root(array_t *traced) {
	ADDREF(current_environment, traced);
	mark_cstack_cons_cell(traced);
	mark_stack(traced);
	mark_func_data_table(traced);
	mark_opline(traced);
	mark_environment_list(traced);
}

static void gc_mark() {
	array_t *ostack = new_array();
	cons_t *cons = NULL;
	array_t *traced = new_array();
	mark_root(traced);
	cons_t *tmp;
	goto LOOP;
	while ((cons = (cons_t*)array_pop(ostack)) != NULL) {
		CONS_TRACE(cons, traced);
LOOP:
		tmp = NULL;
		while((tmp = (cons_t*)array_pop(traced)) != NULL) {
			if (!cons_is_marked(tmp)) {
				array_add(ostack, tmp);
			}
		}
	}
	array_free(traced);
	array_free(ostack);
}
static void gc_sweep() {
	uintptr_t i, j;
	int count = 0;
	int marked = 0;
	cons_page_t *page;
	for (i = 0; i < (uintptr_t)array_size(cons_arena->a); i++) {
		array_t *a = cons_arena->a;
		cons_tbl_t *tbl = (cons_tbl_t *)array_get(a, i);
		for (page = tbl->head; page < tbl->bottom; page++) {
			for (j = 1; j <= PAGECONSSIZE; j++) {
				size_t x = j / (sizeof(uintptr_t) * 8);
				if (!(page->h.bitmap[x] & ((uintptr_t)1 << (j % (sizeof(uintptr_t) * 8))))) {
					if ((page->slots+j-1)->api) {
						//CONS_PRINT(page->slots+j-1);
						//fprintf(stderr, "\n");
						CONS_FREE(page->slots+j-1);
						memset((page->slots + j-1), 0, sizeof(cons_t));
						page->slots[j-1].cdr.ptr = free_list;
						free_list = &page->slots[j-1];
					}
					unused_object++;
					marked++;
				} else {
					count++;
				}
			}
		}
	}
}

static void clear_bitmap() {
	int i = 0;
	for (; i < array_size(cons_arena->a); i++) {
		cons_tbl_t *tbl = (cons_tbl_t*)array_get(cons_arena->a, i);
		memset(tbl->bitmap, 0, tbl->bitmapsize);
	}
}

static void gc() {
	clear_bitmap();
	mark_count = 0;
	gc_mark();
	gc_sweep();
}

static void expand_arena() {
	int size = array_size(cons_arena->a);
	int newsize = size * 2;
	for (; size < newsize; size++) {
		cons_tbl_t *tbl = new_page_table();
		array_add(cons_arena->a, tbl);
		free_list = tbl->head->slots;
	}
}

cons_t *new_cons_cell() {
	cons_t *cons = NULL;
	if (free_list == NULL) {
		gc();
		if ((object_capacity / 4) > unused_object) {
			fprintf(stderr, "expand arena\n");
			expand_arena();
		}
	}
	cons = free_list;
	free_list = free_list->cdr.ptr;
	unused_object--;
	memset(cons, 0, sizeof(cons_t));
	return cons;
}

void cstack_cons_cell_push(cons_t *cons) {
	array_add(cstack_cons_cell_list, cons);
}

cons_t *cstack_cons_cell_pop() {
	return array_pop(cstack_cons_cell_list);
}

void gc_init() {
	cstack_cons_cell_list = new_array();
	new_cons_arena();
}

void gc_end() {
	free_cons_arena();
}
