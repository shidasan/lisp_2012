#include"lisp.h"
#include"config.h"
int current_index, next_index;
char* strtmp;
char* str;
void** table;
static char *(*myreadline)(const char *);
static int (*myadd_history)(const char *);
val_t stack_value[STACKSIZE];
val_t *esp = 0;

static int add_history(const char *line) {
	(void)line;
	return 0;
}

static void init_first() {
	srand((unsigned)time(NULL));
	memory = (opline_t*)malloc(sizeof(opline_t) * INSTSIZE);
	memset(memory, 0, sizeof(opline_t) * INSTSIZE);
	inst_size = INSTSIZE;
	current_index = next_index = 0;
}

static char *str_join(char *tmptmpstr, char *leftover) {
	char *tmpstr = (char*)malloc(sizeof(char) * (strlen(tmptmpstr) + strlen(leftover) + 1));
	memcpy(tmpstr, leftover, strlen(leftover));
	memcpy(tmpstr + strlen(leftover), tmptmpstr, sizeof(tmpstr));
	tmpstr[strlen(tmptmpstr) + strlen(leftover)] = '\0';
	return tmpstr;
}

void set_static_mtds() {
	int i = 0;
	while (static_mtds[i].mtd != NULL || static_mtds[i].special_mtd != NULL) {
		static_mtd_data *data = static_mtds + i;
		set_static_func(data);
		i++;
	}
}

int get_split_point(char *src, int start) {
	int res = start, level = 0;
	while (src[res] != '\0') {
		if (src[res] == '(') {
			level++;
		}
		if (src[res] == ')') {
			level--;
			if (level == 0) {
				return res;
			}
		}
		res++;
	}
	if (level > 0) {
		return -1;
	}
	return strlen(src);
}
static int is_unexpected_input(char *str) {
	return (strcmp(str, " ") != 0 && strcmp(str, "\n") != 0 && strcmp(str, "\0") != 0);
}

void print_return_value(val_t val) {
	VAL_PRINT(val, _buffer);
}
void exec(int using_readline) {
	jmp_buf buf;
	if (loop_frame_list == NULL) {
		loop_frame_list = new_array();
	}
	val_t null_value;
	null_value.ptr = NULL;
	loop_frame_push(&buf, null_value);
	if (setjmp(buf) == 0) {
		val_t val = vm_exec(2, memory + current_index, stack_value);
		if (!IS_NULL(val) && using_readline) {
			print_return_value(val);
			printf("\n");
		}
		loop_frame_t *frame = loop_frame_pop();
		FREE(frame);
	} else {
		cstack_cons_cell_clear();
		environment_clear();
	}
}
char *split_and_exec(int argc, char **args, char *tmpstr) {
	(void)args;
	int prev_point = 0;
	int next_point = 0;
	char *leftover = NULL;
	while ((next_point = get_split_point(tmpstr, prev_point)) != -1) {
		str = (char*)malloc(next_point - prev_point + 2);
		memcpy(str, tmpstr + prev_point, next_point - prev_point + 1);
		str[next_point - prev_point + 1] = '\0';
		prev_point = next_point + 1;
		if (strncmp(str,"exit", 3) == 0){
			printf("bye\n");
			free(str);
			free(tmpstr);
			exit(0);
		}
		int status;
		if (strlen(str) > 0) {
			status = -1;
			if (strlen(str) > 0 && is_unexpected_input(str)) {
				status = parse_program(str);
			}
			myadd_history(str);
			if (status == 0){
				exec(argc == 1);
			}
		}
		free(str);
		if (next_point == (int)strlen(tmpstr)) {
			break;
		}
	}
	/* copy leftover */
	if (next_point == -1) {
		leftover = (char*)malloc(strlen(tmpstr) - prev_point + 2);
		memcpy(leftover, tmpstr + prev_point ,strlen(tmpstr) - prev_point + 1);
		leftover[strlen(tmpstr) - prev_point] = ' ';
		leftover[strlen(tmpstr) - prev_point + 1] = '\0';
	}
	return leftover;
}
void shell_file(int argc, char **args, FILE* file) {
	int file_capacity = INIT_FILE_SIZE;
	int file_size = 0;
	char *tmpstr = (char*)malloc(file_capacity);
	while (1) {
		if (file_size == file_capacity - 1){
			int newcapacity = file_capacity * 2;
			strtmp = (char*)malloc(newcapacity);
			strncpy(strtmp, tmpstr, file_capacity);
			free(tmpstr);
			tmpstr = strtmp;
			file_capacity = newcapacity;
		}
		if ((tmpstr[file_size] = fgetc(file)) == EOF){
			tmpstr[file_size] = '\0';
			fclose(file);
			break;
		}
		//if (tmpstr[file_size] == '\n') {
		//	tmpstr[file_size] = '\n';
		//}
		file_size++;
	}
	split_and_exec(argc, args, tmpstr);
	FREE(tmpstr);
}
void shell_readline(int argc, char **args) {
	int file_capacity = INIT_FILE_SIZE;
	char *tmpstr = (char*)malloc(file_capacity);
	char *leftover = NULL;
	while (1) {
		init_opline();
		if (leftover != NULL) {
			char *tmptmpstr = myreadline("    ");
			tmpstr = str_join(tmptmpstr, leftover);
			free(leftover);
			leftover = NULL;
			free(tmptmpstr);
			tmptmpstr = NULL;
		} else {
			tmpstr = myreadline(">>> ");
		}
		if (strcmp(tmpstr, "exit") == 0) {
			break;
		}
		leftover = split_and_exec(argc, args, tmpstr);
	}
	FREE(tmpstr);
}

static void exception_init() {
	loop_frame_list = new_array();
}

int shell (int argc, char* args[])
{
	FILE* file = NULL;
	table = (void**)(vm_exec(1, NULL, NULL).ptr);
	gc_init();
	new_func_data_table();
	new_global_environment();
	exception_init();
	if (argc > 1){
		file = fopen(args[1],"r");
		if (!file) {
			EXCEPTION("Script not found!!\n");
			exit(1);
		}
	}
	init_first();
	set_static_mtds();
	void *handler = dlopen("libreadline" K_OSDLLEXT, RTLD_LAZY);
	void *f = (handler != NULL) ? dlsym(handler, "readline") : NULL;
	myreadline = (f != NULL) ? (char* (*)(const char*))f : NULL;
	f = (handler != NULL) ? dlsym(handler, "add_history") : NULL;
	myadd_history = (f != NULL) ? (int (*)(const char*))f : add_history;
	int i = 0;
	while (bootstrap_functions[i] != NULL) {
		split_and_exec(2, args, (char*)bootstrap_functions[i]);
		i++;
	}
	if (argc > 1){
		shell_file(argc, args, file);
	} else {
		shell_readline(argc, args);
	}
	func_data_table_free();
	opline_free();
	gc_end();
	return 0;
}
