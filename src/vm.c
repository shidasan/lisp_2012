#include"lisp.h"
const char* instruction_tostr[] = {"PUSH", "MTDCALL", "MTDCHECK", "SP_MTD", "GET_V", "GET_ARG", "END", "JMP"};
static int dump_point = 0;
static void dump_vm() {
	opline_t *pc = memory + current_index;
	while (pc < memory + next_index - 1) {
		fprintf(stdout, "op%d:\t%s\t", dump_point, instruction_tostr[pc->instruction]);
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
		case JMP:
			fprintf(stdout, "op%d", pc->op[0].ivalue + dump_point + 1);
		default:
			break;
		}
		fprintf(stdout, "\n");
		pc++;
		dump_point++;
	}
	fprintf(stdout, "op%d:\tEND\n\n", dump_point);
	dump_point++;
}
void set_args(val_t *VSTACK, int ARGC, func_t *func) {
	int i = 0;
	cons_t *args = func->args.ptr;
	for (; i < ARGC; i++) {
		val_t arg = ARGS(i);
		val_t variable = args->car;
		set_variable(variable.ptr, arg, 1);
		args = args->cdr.ptr;
	}
}

val_t call_lambda(val_t *VSTACK, array_t *a) {
	val_t lambda_func = VSTACK[-1];
	lambda_env_t *env = lambda_func.ptr->env;
	val_t args = env->args;
	int length = val_length(args), i;
	array_t *lambda_data_list = lambda_func.ptr->cdr.a;
	if (length != (int)array_size(a)) {
		EXCEPTION("Argument length does not match!!\n");
	}
	cons_t *old_environment = change_local_scope(current_environment, env->environment);
	environment_list_push(old_environment);
	for (i = 0; i < length; i++) {
		val_t car = args.ptr->car;
		if (!IS_SYMBOL(car)) {
			EXCEPTION("Excepted symbol!!\n");
		}
		val_t value = vm_exec(2, memory + (uintptr_t)array_get(a, i), VSTACK+1);
		set_variable(car.ptr, value, 1);
		args = args.ptr->cdr;
	}
	val_t res;
	res.ptr = NULL;
	for (i = 0; i < (int)array_size(lambda_data_list); i++) {
		lambda_data_t *data = (lambda_data_t*)array_get(lambda_data_list, i);
		res = vm_exec(2, memory + data->opline_idx, VSTACK+1);
	}
	environment_list_pop();
	end_local_scope(old_environment);
	return res;
}

val_t call_mtd(val_t *VSTACK, int ARGC, func_t *func) {
	val_t res;
	array_t *opline_list = func->opline_list;
	int a = 0;
	cons_t *old_environment = change_local_scope(current_environment, func->environment);
	environment_list_push(old_environment);
	set_args(VSTACK, ARGC, func);
	for (; a < (int)array_size(opline_list); a++) {
		res = vm_exec(2, memory + (uintptr_t)array_get(opline_list, a), VSTACK + 1);
		if (func != NULL && FLAG_IS_MACRO(func->flag)) {
			res = eval_inner(VSTACK, res);
		}
	}
	environment_list_pop();
	end_local_scope(old_environment);
	return res;
}
val_t call_macro(val_t *VSTACK, int ARGC, func_t *func) {
	val_t res;
	array_t *opline_list = func->opline_list;
	int a = 0;
	cons_t *old_environment = change_local_scope(current_environment, func->environment);
	environment_list_push(old_environment);
	set_args(VSTACK, ARGC, func);
	array_t *list = new_array();
	for (; a < (int)array_size(opline_list); a++) {
		res = vm_exec(2, memory + (uintptr_t)array_get(opline_list, a), VSTACK + 1);
		if (func != NULL) {
			res = eval_inner(VSTACK, res);
			//codegen(res);
			//res = vm_exec(2, memory + current_index, VSTACK + 1);
			array_add_val(list, res);
		}
	}
	environment_list_pop();
	end_local_scope(old_environment);
	for (a = 0; a < array_size(list); a++) {
		res = array_get_val(list, a);
		//codegen(res);
		//res = vm_exec(2, memory + current_index, VSTACK+1);
		res = eval_inner(VSTACK, res);
	}
	array_free(list);
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
		&&jmp,
    };

    if(i == 1){
		val_t res;
		res.ptr = (cons_t *)table;
        return res;
    }
	if (0&& dump_point < next_index - 1) {
		dump_vm();
	}

    //val_t stack_value[STACKSIZE];

	val_t *esp = ebp;
    int args_num = 0; 
	val_t val;
	func_t *func = NULL;
	struct array_t *array = NULL;


    goto *(pc->instruction_ptr);

jmp:
	pc += pc->op[0].ivalue;
	goto *((++pc)->instruction_ptr);

get_arg:
	esp[0] = ebp[pc->op[0].ivalue];
	esp++;
	goto *((++pc)->instruction_ptr);

get_variable:
	{
		val = pc->op[0].val;
		val_t res = search_variable(val.ptr->str);
		if (IS_NULL(res)) {
			FMT_EXCEPTION("Variable %s not found!!\n", (void*)val.ptr->str);
		}
		esp[0] = res;
		esp++;
		goto *((++pc)->instruction_ptr);
	}

special_mtd:
	{
		val = pc->op[0].val;
		array = pc->op[1].a;
		if (IS_NULL(val)) { /* lambda function */
			esp[-1] = call_lambda(esp, array);
		} else {
			func = search_func(val.ptr->str);
			cons_t *old_environment = begin_local_scope(func);
			esp[0] = func->special_mtd(esp, array);
			esp++;
			end_local_scope(old_environment);
		}
		goto *((++pc)->instruction_ptr);
	}

mtdcheck:
	val = pc->op[0].val;
	args_num = pc->op[1].ivalue;
	func = search_func(val.ptr->str);
	if (func == NULL) {
		FMT_EXCEPTION("Function %s not found!!\n", (void*)val.ptr->str);
	}
	if (func->value != args_num) {
		if (func->value > args_num) {
			EXCEPTION("Too few arguments!!\n");
		} else {
			EXCEPTION("Too many arguments!!\n");
		}
	}
	goto *((++pc)->instruction_ptr);

mtdcall:
	{
		val = pc->op[0].val;
		cons_t *old_environment = NULL;
		args_num = pc->op[1].ivalue;
		func = search_func(val.ptr->str);
		if (func != NULL && FLAG_IS_STATIC(func->flag)) {
			old_environment = begin_local_scope(func);
			esp[-args_num] = func->mtd(esp, args_num);
			end_local_scope(old_environment);
		} else if (FLAG_IS_MACRO(func->flag)) {
			val = call_macro(esp, args_num, func);
			esp[-args_num] = val;
		} else {
			val = call_mtd(esp, args_num, func);
			esp[-args_num] = val;
		}
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


