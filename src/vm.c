#include <stdio.h>
#include"lisp.h"
const char* instruction_tostr[] = {"PUSH", "MTDCALL", "MTDCHECK", "SPECIAL_MTD", "GET_VARIABLE", "GET_ARG", "END"};
static void dump_vm() {
	opline_t *pc = memory + CurrentIndex;
	int i = 0;
	while (pc < memory + NextIndex-1) {
		fprintf(stdout, "op: %s\n", instruction_tostr[pc->instruction]);
		if (pc->instruction == PUSH) {
			CONS_PRINT(pc->op[0].cons);
			fprintf(stdout, "\n");
		}
		pc++;
	}
	fprintf(stdout, "op: END\n");
	fprintf(stdout, "op: DUMP END\n");
}
static void set_args(cons_t **VSTACK, int ARGC, func_t *func) {
	int i = 0;
	cons_t *args = func->args;
	for (; i < ARGC; i++) {
		cons_t *arg = ARGS(i);
		cons_t *variable = args->car;
		set_variable(variable, arg, 1);
		args = args->cdr;
	}
}
cons_t* vm_exec (int i , opline_t* pc, cons_t **ebp)
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
        return (cons_t*)table;
    }

	//dump_vm();

    //cons_t *stack_value[STACKSIZE];

    cons_t** sp_arg = NULL;
    opline_t** sp_adr = NULL;
	cons_t **esp = ebp;
    //register opline_t* pc = memory + CurrentIndex;
    int a = 0, args_num = 0; 
    struct cons_t *a_ptr = NULL,*ret_ptr = NULL;
	cons_t *cons = NULL;
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
		cons = pc->op[0].cons;
		cons_t *res = search_variable(cons->str);
		if (res == NULL) {
			fprintf(stderr, "variable not found!!\n");
		}
		esp[0] = res;
		esp++;
		goto *((++pc)->instruction_ptr);
	}

special_mtd:
	{
		cons = pc->op[0].cons;
		array = pc->op[1].a;
		func = search_func(cons->str);
		cons_t *old_environment = begin_local_scope(func);
		args_num = 0;
		esp[-args_num] = func->special_mtd(esp, 0, array);
		end_local_scope(old_environment);
		goto *((++pc)->instruction_ptr);
	}

mtdcheck:
	cons = pc->op[0].cons;
	args_num = pc->op[1].ivalue;
	func = search_func(cons->str);
	if (func == NULL) {
		fprintf(stderr, "can't call method!!\n");
		fprintf(stderr, "cons->str: %s\n", cons->str);
		asm("int3");
	}
	if (func->value != -1 && func->value != args_num) {
		fprintf(stderr, "argument length does not match!!\n");
		fprintf(stderr, "correct number: %d, this time: %d\n", func->value, args_num);
		asm("int3");
	}
	goto *((++pc)->instruction_ptr);

mtdcall:
	{
		cons = pc->op[0].cons;
		cons_t *old_environment = NULL;
		args_num = pc->op[1].ivalue;
		func = search_func(cons->str);
		if (func->is_static) {
			old_environment = begin_local_scope(func);
			esp[-args_num] = func->mtd(esp, args_num);
		} else {
			old_environment = change_local_scope(current_environment, func->environment);
			environment_list_push(old_environment);
			opline_list = func->opline_list;
			set_args(esp, args_num, func);
			for (a = 0; a < array_size(opline_list); a++) {
				cons = vm_exec(2, (opline_t *)array_get(opline_list, a), esp + 1);
			}
			esp[-args_num] = cons;
			cons_t *tmp = environment_list_pop();
		}
		end_local_scope(old_environment);
		esp -= (args_num - 1);
		goto *((++pc)->instruction_ptr);
	}
end:
    return ebp[0];

push:
	esp[0] = pc->op[0].cons;
	esp++;
    goto *((++pc)->instruction_ptr);
}


