#include <stdio.h>
#include"lisp.h"
const char* instruction_tostr[] = {"PUSH", "PLUS", "MINUL", "MUL", "DIV", "GT", "GTE", "LT", "LTE", "EQ", "PLUS2", "MINUS2", "MUL2", "DIV2", "GT2", "GTE2", "LT2", "LTE2", "EQ2", "END", "JMP", "GOTO", "NGOTO", "RETURN", "NRETURN", "ARG", "NARG", "DEFUN", "SETQ", "MTDCALL", "MTDCHECK", "SPECIAL_MTD"};
static void dump_vm() {
	opline_t *pc = memory + CurrentIndex;
	int i = 0;
	while (pc < memory + NextIndex-1) {
		fprintf(stdout, "op: %s\n", instruction_tostr[pc->instruction]);
		pc++;
	}
	fprintf(stdout, "op: END\n");
}
cons_t* eval (int i , opline_t* pc, cons_t **stack_value)
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
    };

    if( i == 1 ){
        return (cons_t*)table;
    }

	dump_vm();

    opline_t* stack_adr[STACKSIZE];
    cons_t *stack_arg[STACKSIZE];
    //cons_t *stack_value[STACKSIZE];

    cons_t** sp_value = stack_value;
    cons_t** sp_arg = stack_arg;
    opline_t** sp_adr = stack_adr;
    //register opline_t* pc = memory + CurrentIndex;
    int a = 0, args_num = 0; 
    struct cons_t *a_ptr = NULL,*ret_ptr = NULL;
	cons_t *cons = NULL;
	func_t *func = NULL;
	struct array_t *array = NULL;


    goto *(pc->instruction_ptr);

special_mtd:
	fprintf(stderr, "now special_mtd\n");
	cons = pc->op[0].cons;
	array = pc->op[1].a;
	func = searchF(cons->str);
	//args_num = array_size(array);
	args_num = 0;
	//sp_value[-args_num] = func->special_mtd(sp_value, args_num, array);
	sp_value[-args_num] = func->special_mtd(sp_value, 0, array);
	//sp_value -= (args_num - 1);
	goto *((++pc)->instruction_ptr);

mtdcheck:
	cons = pc->op[0].cons;
	args_num = pc->op[1].ivalue;
	if (cons->type != FUNC) {
		fprintf(stderr, "can't call method!!\n");
		asm("int3");
	}
	func = searchF(cons->str);
	if (func->value != -1 && func->value != args_num) {
		fprintf(stderr, "argument length does not match!!\n");
		fprintf(stderr, "correct number: %d, this time: %d\n", func->value, args_num);
		asm("int3");
	}
	goto *((++pc)->instruction_ptr);
	
mtdcall:
	cons = pc->op[0].cons;
	args_num = pc->op[1].ivalue;
	func = searchF(cons->str);
	if (func->is_static) {
		sp_value[-args_num] = func->mtd(sp_value, args_num);
	}
	sp_value -= (args_num - 1);
	goto *((++pc)->instruction_ptr);

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
	fprintf(stderr, "now end\n");
	//if (stack_value[0] != 0) {
	//	stack_value[0]->api->print(stack_value[0]);
	//}
    return stack_value[0];

push:
	fprintf(stderr, "now push\n");
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


