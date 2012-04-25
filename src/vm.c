#include <stdio.h>
#include"lisp.h"
const char* instruction_tostr[] = {"PUSH", "MTDCALL", "MTDCHECK", "SP_MTD", "GET_V", "GET_ARG", "END"};
static void dump_vm() {
	opline_t *pc = memory + CurrentIndex;
	int i = 0;
	while (pc < memory + NextIndex-1) {
		fprintf(stdout, "op: %s\t", instruction_tostr[pc->instruction]);
		switch (pc->instruction) {
		case PUSH:
			{
			VAL_PRINT(pc->op[0].val, _buffer);
			break;
			}
		case GET_VARIABLE:
		case MTDCALL:
		case SPECIAL_MTD:
			fprintf(stdout, "%s", pc->op[0].val.ptr->str);
			break;
		default:
			break;
		}
		fprintf(stdout, "\n");
		pc++;
	}
	fprintf(stdout, "op: END\n");
	fprintf(stdout, "op: DUMP END\n\n");
}
static void set_args(val_t *VSTACK, int ARGC, func_t *func) {
	int i = 0;
	cons_t *args = func->args.ptr;
	for (; i < ARGC; i++) {
		val_t arg = ARGS(i);
		val_t variable = args->car;
		set_variable(variable.ptr, arg, 1);
		args = args->cdr.ptr;
	}
}
static val_t exec_body(val_t *VSTACK, func_t *func) {
	val_t res;
	array_t *opline_list = func->opline_list;
	int a = 0;
	for (; a < array_size(opline_list); a++) {
		res = vm_exec(2, (opline_t *)array_get(opline_list, a), VSTACK + 1);
		if (func != NULL && FLAG_IS_MACRO(func->flag)) {
			codegen(res);
			res = vm_exec(2, memory + CurrentIndex, VSTACK + 1);
		}
	}
	return res;
}
val_t vm_exec (int i , opline_t* pc, val_t *ebp)
{
    static void *table [] = {
        &&push,
		&&mtdcall,
		&&mtdcheck,
		&&special_mtd,
		&&get_variable,
		&&get_arg,
		&&end,
    };

    if( i == 1 ){
		val_t res;
		res.ptr = (cons_t *)table;
        return res;
    }

	dump_vm();

    //val_t stack_value[STACKSIZE];

    cons_t** sp_arg = NULL;
    opline_t** sp_adr = NULL;
	val_t *esp = ebp;
    //register opline_t* pc = memory + CurrentIndex;
    int a = 0, args_num = 0; 
	val_t val;
	func_t *func = NULL;
	struct array_t *array = NULL;
	struct array_t *opline_list = NULL;


    goto *(pc->instruction_ptr);

get_arg:
	esp[0] = ebp[pc->op[0].ivalue];
	esp++;
	goto *((++pc)->instruction_ptr);

get_variable:
	{
		val = pc->op[0].val;
		val_t res = search_variable(val.ptr->str);
		if (IS_NULL(res)) {
			fprintf(stderr, "variable not found!!\n");
		}
		esp[0] = res;
		esp++;
		goto *((++pc)->instruction_ptr);
	}

special_mtd:
	{
		val = pc->op[0].val;
		array = pc->op[1].a;
		func = search_func(val.ptr->str);
		cons_t *old_environment = begin_local_scope(func);
		args_num = 0;
		esp[-args_num] = func->special_mtd(esp, 0, array);
		end_local_scope(old_environment);
		goto *((++pc)->instruction_ptr);
	}

mtdcheck:
	val = pc->op[0].val;
	args_num = pc->op[1].ivalue;
	func = search_func(val.ptr->str);
	if (func == NULL) {
		fprintf(stderr, "val.ptr->str: %s\n", val.ptr->str);
		EXCEPTION("can't call method!!\n");
	}
	if (func->value != -1 && func->value != args_num) {
		EXCEPTION("argument length does not match!!\n");
	}
	goto *((++pc)->instruction_ptr);

mtdcall:
	{
		val = pc->op[0].val;
		cons_t *old_environment = NULL;
		args_num = pc->op[1].ivalue;
		func = search_func(val.ptr->str);
		if (FLAG_IS_STATIC(func->flag)) {
			old_environment = begin_local_scope(func);
			esp[-args_num] = func->mtd(esp, args_num);
		} else {
			old_environment = change_local_scope(current_environment, func->environment);
			environment_list_push(old_environment);
			opline_list = func->opline_list;
			set_args(esp, args_num, func);
			val = exec_body(esp, func);
			esp[-args_num] = val;
			environment_list_pop();
		}
		end_local_scope(old_environment);
		esp -= (args_num - 1);
		goto *((++pc)->instruction_ptr);
	}
end:
    return ebp[0];

push:
	esp[0] = pc->op[0].val;
	esp++;
    goto *((++pc)->instruction_ptr);
}


