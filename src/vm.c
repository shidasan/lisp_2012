#include <stdio.h>
#include"lisp.h"
cons_t** sp_value = NULL;
const char* instruction_tostr[] = {"PUSH", "PLUS", "MINUL", "MUL", "DIV", "GT", "GTE", "LT", "LTE", "EQ", "PLUS2", "MINUS2", "MUL2", "DIV2", "GT2", "GTE2", "LT2", "LTE2", "EQ2", "END", "JMP", "GOTO", "NGOTO", "RETURN", "NRETURN", "ARG", "NARG", "DEFUN", "SETQ", "MTDCALL", "MTDCHECK", "SPECIAL_MTD", "VARIABLE_PUSH"};
static void dump_vm() {
	opline_t *pc = memory + CurrentIndex;
	int i = 0;
	while (pc < memory + NextIndex-1) {
		fprintf(stdout, "op: %s\n", instruction_tostr[pc->instruction]);
		pc++;
	}
	fprintf(stdout, "op: END\n");
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
cons_t* vm_exec (int i , opline_t* pc, cons_t **stack_value)
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
	cons_t **sp_value = stack_value;
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
	sp_value[0] = cons;
	sp_value++;
	goto *((++pc)->instruction_ptr);

special_mtd:
	{
		cons = pc->op[0].cons;
		array = pc->op[1].a;
		func = search_func(cons->str);
		cons_t *old_environment = begin_local_scope(func);
		args_num = 0;
		sp_value[-args_num] = func->special_mtd(sp_value, 0, array);
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
			sp_value[-args_num] = func->mtd(sp_value, args_num);
		} else {
			old_environment = change_local_scope(current_environment, func->environment);
			environment_list_push(old_environment);
			opline_list = func->opline_list;
			set_args(sp_value, args_num, func);
			for (a = 0; a < array_size(opline_list); a++) {
				cons = vm_exec(2, (opline_t *)array_get(opline_list, a), sp_value+a+1);
			}
			environment_list_pop();
			sp_value[-args_num] = cons;
		}
		end_local_scope(old_environment);
		sp_value -= (args_num - 1);
		goto *((++pc)->instruction_ptr);
	}

plus:
	(sp_value[-2])->ivalue += (sp_value[-1])->ivalue;
	sp_value--;
	goto *((++pc)->instruction_ptr);

minus:
    (sp_value[-2])->ivalue -= (sp_value[-1])->ivalue;
    sp_value--;
    goto *((++pc)->instruction_ptr);

mul:
    (sp_value[-2])->ivalue *= (sp_value[-1])->ivalue;
    sp_value--;
    goto *((++pc)->instruction_ptr);

div:
    (sp_value[-2])->ivalue /= (sp_value[-1])->ivalue;
    sp_value--;
    goto *((++pc)->instruction_ptr);

funcdef:
	printf("%s\n",pc->op[1].svalue);
	return NULL;

end:
	//if (stack_value[0] != 0) {
	//	stack_value[0]->api->print(stack_value[0]);
	//}
    return stack_value[0];

push:
    //sp_value[0]->type = NUM;
	//(sp_value)[0] = new_int(pc->op[0].ivalue);
	sp_value[0] = pc->op[0].cons;
	sp_value++;
    goto *((++pc)->instruction_ptr);

plus2:
	sp_value[-1] = new_int(sp_value[-1]->ivalue + pc->op[0].ivalue);
    goto *((++pc)->instruction_ptr);

gt2:
	sp_value[-1] = new_bool(pc->op[0].ivalue > sp_value[-1]->ivalue);
    goto *((++pc)->instruction_ptr);

gt:
    ret_ptr = ((--sp_value)[0]);
	sp_value[-1] = new_bool(ret_ptr->ivalue > (sp_value)[-1]->ivalue);
    //ret_ptr->type = ( ret_ptr->ivalue > ((--sp_value)[0])->ivalue && ret_ptr->type != nil) ? T : nil;
    goto *((++pc)->instruction_ptr);

lte:
    ret_ptr = (--sp_value)[0];
    //ret_ptr->type = ( ret_ptr->ivalue <= ((--sp_value)->ivalue && ret_ptr->type != nil) ? T : nil;
    *(sp_value++) = ret_ptr;
    goto *((++pc)->instruction_ptr);

eq:
    ret_ptr = (--sp_value)[0];
    //ret_ptr->type = ( ret_ptr->ivalue == (--sp_value)->ivalue ) ? T : nil;
    *(sp_value++) = ret_ptr;
    goto *((++pc)->instruction_ptr);

jmp:
    pc = pc->op[ (--sp_value)[0]->type ].adr;
    goto *((pc)->instruction_ptr);

funccall:
    (sp_arg++)[0]->ivalue = (--sp_value)[0]->ivalue;
    *((sp_adr++)) = pc + 1;
    pc = pc->op[0].adr;
    goto *((pc)->instruction_ptr);

nfunccall:
    a = pc->op[1].ivalue;
    while (a-- != 0){
        (sp_arg++)[0]->ivalue = (--sp_value)[0]->ivalue;
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
    //sp_value->type = NUM;
    (sp_value++)[0] = sp_arg[-1];
    goto *((++pc)->instruction_ptr);

narg:
    //sp_value->type = NUM;
    (sp_value++)[0] = (sp_arg[-(pc->op[0].ivalue)]);
    goto *((++pc)->instruction_ptr);

minus2:
	sp_value[-1] = new_int(sp_value[-1]->ivalue - pc->op[0].ivalue);
    goto *((++pc)->instruction_ptr);

mul2:
	sp_value[-1] = new_int(sp_value[-1]->ivalue * pc->op[0].ivalue);
    goto *((++pc)->instruction_ptr);

div2:
	sp_value[-1] = new_int(sp_value[-1]->ivalue / pc->op[0].ivalue);
    goto *((++pc)->instruction_ptr);

gte2:
	sp_value[-1] = new_bool(pc->op[0].ivalue >= sp_value[-1]->ivalue);
    //a_ptr = (sp_value - 1);
    //a_ptr->type = ( pc->op[0].ivalue >= a_ptr->ivalue && a_ptr->type != nil) ? T : nil; 
    goto *((++pc)->instruction_ptr);

lt2:
	sp_value[-1] = new_bool(pc->op[0].ivalue < sp_value[-1]->ivalue);
    goto *((++pc)->instruction_ptr);

lte2:
	sp_value[-1] = new_bool(pc->op[0].ivalue <= sp_value[-1]->ivalue);
    //a_ptr = (sp_value - 1);
    //a_ptr->type = ( pc->op[0].ivalue <= a_ptr->ivalue && a_ptr->type != nil) ? T : nil; 
    goto *((++pc)->instruction_ptr);

eq2:
    //a_ptr = (sp_value - 1);
    //a_ptr->type = ( pc->op[0].ivalue == a_ptr->ivalue && a_ptr->type != nil) ? T : nil; 
    //a_ptr->ivalue = pc->op[0].ivalue;
    //goto *((++pc)->instruction_ptr);

setq:
    //((Variable_Data_t*)pc->op[0].adr)->value = (--sp_value)->ivalue;
    //goto *((++pc)->instruction_ptr);

gte:
    //ret_ptr = (--sp_value);
    //ret_ptr->type = ( ret_ptr->ivalue >= (--sp_value)->ivalue && ret_ptr->type != nil) ? T : nil; 
    //*(sp_value++) = *ret_ptr;
    //goto *((++pc)->instruction_ptr);

lt:
    //ret_ptr = (--sp_value);
    //ret_ptr->type = ( ret_ptr->ivalue < (--sp_value)->ivalue && ret_ptr->type != nil) ? T : nil;
    //*(sp_value++) = *ret_ptr;
    //goto *((++pc)->instruction_ptr);
	;

}


