#include <stdio.h>
#include"lisp.h"
const char* instruction_tostr[] = {"PUSH", "PLUS", "MINUL", "MUL", "DIV", "GT", "GTE", "LT", "LTE", "EQ", "PLUS2", "MINUS2", "MUL2", "DIV2", "GT2", "GTE2", "LT2", "LTE2", "EQ2", "END", "JMP", "GOTO", "NGOTO", "RETURN", "NRETURN", "ARG", "NARG", "DEFUN", "SETQ", "MTDCALL", "MTDCHECK", "SPECIAL_MTD", "VARIABLE_PUSH"};
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
cons_t* vm_exec (int i , opline_t* pc, cons_t **_stack_value)
{
    static void *table [] = {
        &&push,
        &&plus,
        &&minus,
        &&mul,
        &&div,
        &&gt,
        &&gte,
        &&lt,
        &&lte,
        &&eq,
        &&plus2,
        &&minus2,
        &&mul2,
        &&div2,
        &&gt2,
        &&gte2,
        &&lt2,
        &&lte2,
        &&eq2,
        &&end,
        &&jmp,
        &&funccall,
        &&nfunccall,
        &&Return,
        &&nReturn,
        &&arg,
        &&narg,
        &&funcdef,
        &&setq,
		&&mtdcall,
		&&mtdcheck,
		&&special_mtd,
		&&variable_push,
    };

    if( i == 1 ){
        return (cons_t*)table;
    }

	//dump_vm();

    //cons_t *stack_value[STACKSIZE];

    cons_t** sp_arg = NULL;
    opline_t** sp_adr = NULL;
	cons_t **esp = _stack_value;
    //register opline_t* pc = memory + CurrentIndex;
    int a = 0, args_num = 0; 
    struct cons_t *a_ptr = NULL,*ret_ptr = NULL;
	cons_t *cons = NULL;
	func_t *func = NULL;
	struct array_t *array = NULL;
	struct array_t *opline_list = NULL;


    goto *(pc->instruction_ptr);

variable_push:
	cons = pc->op[0].cons;
	cons = search_variable(cons->str);
	if (cons == NULL) {
		fprintf(stderr, "variable not found!!\n");
	}
	esp[0] = cons;
	esp++;
	goto *((++pc)->instruction_ptr);

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
	if (cons->type != FUNC) {
		fprintf(stderr, "can't call method!!\n");
		asm("int3");
	}
	func = search_func(cons->str);
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

plus:
	(esp[-2])->ivalue += (esp[-1])->ivalue;
	esp--;
	goto *((++pc)->instruction_ptr);

minus:
    (esp[-2])->ivalue -= (esp[-1])->ivalue;
    esp--;
    goto *((++pc)->instruction_ptr);

mul:
    (esp[-2])->ivalue *= (esp[-1])->ivalue;
    esp--;
    goto *((++pc)->instruction_ptr);

div:
    (esp[-2])->ivalue /= (esp[-1])->ivalue;
    esp--;
    goto *((++pc)->instruction_ptr);

funcdef:
	printf("%s\n",pc->op[1].svalue);
	return NULL;

end:
    return _stack_value[0];

push:
    //esp[0]->type = NUM;
	//(esp)[0] = new_int(pc->op[0].ivalue);
	esp[0] = pc->op[0].cons;
	esp++;
    goto *((++pc)->instruction_ptr);

plus2:
	esp[-1] = new_int(esp[-1]->ivalue + pc->op[0].ivalue);
    goto *((++pc)->instruction_ptr);

gt2:
	esp[-1] = new_bool(pc->op[0].ivalue > esp[-1]->ivalue);
    goto *((++pc)->instruction_ptr);

gt:
    ret_ptr = ((--esp)[0]);
	esp[-1] = new_bool(ret_ptr->ivalue > (esp)[-1]->ivalue);
    //ret_ptr->type = ( ret_ptr->ivalue > ((--esp)[0])->ivalue && ret_ptr->type != nil) ? T : nil;
    goto *((++pc)->instruction_ptr);

lte:
    ret_ptr = (--esp)[0];
    //ret_ptr->type = ( ret_ptr->ivalue <= ((--esp)->ivalue && ret_ptr->type != nil) ? T : nil;
    *(esp++) = ret_ptr;
    goto *((++pc)->instruction_ptr);

eq:
    ret_ptr = (--esp)[0];
    //ret_ptr->type = ( ret_ptr->ivalue == (--esp)->ivalue ) ? T : nil;
    *(esp++) = ret_ptr;
    goto *((++pc)->instruction_ptr);

jmp:
    pc = pc->op[ (--esp)[0]->type ].adr;
    goto *((pc)->instruction_ptr);

funccall:
    (sp_arg++)[0]->ivalue = (--esp)[0]->ivalue;
    *((sp_adr++)) = pc + 1;
    pc = pc->op[0].adr;
    goto *((pc)->instruction_ptr);

nfunccall:
    a = pc->op[1].ivalue;
    while (a-- != 0){
        (sp_arg++)[0]->ivalue = (--esp)[0]->ivalue;
    }
    *(sp_adr++) = pc + 1;
    pc = pc->op[0].adr;
    goto *((pc)->instruction_ptr);

sfunccall:
	/* TODO call runtime function */
Return:
    --sp_arg;
    pc = *(--sp_adr);
    goto *((pc)->instruction_ptr);

nReturn:
    sp_arg -= pc->op[0].ivalue;
    pc = *(--sp_adr);
    goto *((pc)->instruction_ptr);

arg:
    //esp->type = NUM;
    (esp++)[0] = sp_arg[-1];
    goto *((++pc)->instruction_ptr);

narg:
    //esp->type = NUM;
    (esp++)[0] = (sp_arg[-(pc->op[0].ivalue)]);
    goto *((++pc)->instruction_ptr);

minus2:
	esp[-1] = new_int(esp[-1]->ivalue - pc->op[0].ivalue);
    goto *((++pc)->instruction_ptr);

mul2:
	esp[-1] = new_int(esp[-1]->ivalue * pc->op[0].ivalue);
    goto *((++pc)->instruction_ptr);

div2:
	esp[-1] = new_int(esp[-1]->ivalue / pc->op[0].ivalue);
    goto *((++pc)->instruction_ptr);

gte2:
	esp[-1] = new_bool(pc->op[0].ivalue >= esp[-1]->ivalue);
    //a_ptr = (esp - 1);
    //a_ptr->type = ( pc->op[0].ivalue >= a_ptr->ivalue && a_ptr->type != nil) ? T : nil; 
    goto *((++pc)->instruction_ptr);

lt2:
	esp[-1] = new_bool(pc->op[0].ivalue < esp[-1]->ivalue);
    goto *((++pc)->instruction_ptr);

lte2:
	esp[-1] = new_bool(pc->op[0].ivalue <= esp[-1]->ivalue);
    //a_ptr = (esp - 1);
    //a_ptr->type = ( pc->op[0].ivalue <= a_ptr->ivalue && a_ptr->type != nil) ? T : nil; 
    goto *((++pc)->instruction_ptr);

eq2:
    //a_ptr = (esp - 1);
    //a_ptr->type = ( pc->op[0].ivalue == a_ptr->ivalue && a_ptr->type != nil) ? T : nil; 
    //a_ptr->ivalue = pc->op[0].ivalue;
    //goto *((++pc)->instruction_ptr);

setq:
    //((Variable_Data_t*)pc->op[0].adr)->value = (--esp)->ivalue;
    //goto *((++pc)->instruction_ptr);

gte:
    //ret_ptr = (--esp);
    //ret_ptr->type = ( ret_ptr->ivalue >= (--esp)->ivalue && ret_ptr->type != nil) ? T : nil; 
    //*(esp++) = *ret_ptr;
    //goto *((++pc)->instruction_ptr);

lt:
    //ret_ptr = (--esp);
    //ret_ptr->type = ( ret_ptr->ivalue < (--esp)->ivalue && ret_ptr->type != nil) ? T : nil;
    //*(esp++) = *ret_ptr;
    //goto *((++pc)->instruction_ptr);
	;

}


