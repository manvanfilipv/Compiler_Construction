#include "tcodegen.h"

#define EXPAND_SIZE 1024
#define CURR_SIZE (instr_total * sizeof(instruction))
#define NEW_SIZE (EXPAND_SIZE * sizeof(instruction) + CURR_SIZE)

unsigned infunction = 0;

extern quad* quads;
extern int currQuad;

instruction* instr = (instruction*) 0;
unsigned instr_total = 0;
unsigned int currInstr = 0;

unsigned int VAR_TABLE_SIZE = 100;
unsigned int NUM_TABLE_SIZE = 100;
unsigned int STR_TABLE_SIZE = 100;
unsigned int USERFUNC_TABLE_SIZE = 100;
unsigned int LIBFUNC_TABLE_SIZE = 100;

var* var_table[100];
num* num_table[100];
char* str_table[100];
userfunc* userfunc_table[100];
char* libfunc_table[100];

unsigned int total_locals = 0;
unsigned int total_locals_funcoffset = 0;

unsigned global_counter = 0;
unsigned local_counter = 0;
unsigned formal_counter = 0;

unsigned var_offset = 0;
unsigned num_offset = 0;
unsigned str_offset = 0;
unsigned userfunc_offset = 0;
unsigned libfunc_offset = 0;

unsigned insert_to_var_table (SymbolTableEntry* sym, unsigned isTable) {
    if (var_offset > VAR_TABLE_SIZE) {
        VAR_TABLE_SIZE = VAR_TABLE_SIZE + VAR_TABLE_SIZE;
        *var_table = realloc(*var_table, VAR_TABLE_SIZE * sizeof(var*));
    }
    var* variable = (var*) malloc(sizeof(var));
    variable->name = strdup(sym->name);
    variable->type = sym->type;
    if (sym->space == programvar) {
        if (sym->type == GLOBAL) {
            variable->type_offset = global_counter;
            global_counter++;
        } else if (sym->type == LOCAL){
            variable->type_offset = local_counter;
            local_counter++;
        }
        
    } else if (sym->space == formalarg) {
        variable->type_offset = formal_counter;
        formal_counter++;
    }
    if (isTable == 1) {
        variable->type_val = tableVal;
    }
    var_table[var_offset] = variable;
    var_offset++;
    return var_offset - 1;
}

unsigned insert_to_num_table (double val) {
    if (num_offset > NUM_TABLE_SIZE) {
        NUM_TABLE_SIZE = NUM_TABLE_SIZE + NUM_TABLE_SIZE;
        *num_table = realloc(*num_table, NUM_TABLE_SIZE);
    }
    num* func = (num*) malloc(sizeof(num));
    func->value = val;
    num_table[num_offset] = func;
    num_offset++;
    return num_offset - 1;
}

unsigned insert_to_str_table (char* val) {
    if (str_offset > STR_TABLE_SIZE) {
        STR_TABLE_SIZE = STR_TABLE_SIZE + STR_TABLE_SIZE;
        *str_table = realloc(*str_table, STR_TABLE_SIZE);
    }
    str_table[str_offset] = val;
    str_offset++;
    return str_offset - 1;
}

unsigned insert_to_userfunc_table (char* name, unsigned address, unsigned size, formal_args* arg) {
    if (userfunc_offset > USERFUNC_TABLE_SIZE) {
        USERFUNC_TABLE_SIZE = USERFUNC_TABLE_SIZE + USERFUNC_TABLE_SIZE;
        *userfunc_table = realloc(*userfunc_table, USERFUNC_TABLE_SIZE);
    }
    userfunc* func = (userfunc*) malloc(sizeof(userfunc));
    func->name = name;
    func->address = address;
    func->local_size = size;
    func->arg = arg;
    userfunc_table[userfunc_offset] = func;
    userfunc_offset++;
    return userfunc_offset - 1;
}

unsigned insert_to_libfunc_table (char* val) {
    if (libfunc_offset > LIBFUNC_TABLE_SIZE) {
        LIBFUNC_TABLE_SIZE = LIBFUNC_TABLE_SIZE + LIBFUNC_TABLE_SIZE;
        *libfunc_table = realloc(*libfunc_table, LIBFUNC_TABLE_SIZE);
    }
    libfunc_table[libfunc_offset] = val;
    libfunc_offset++;
    return libfunc_offset - 1;
}

void expand_instr (){
    assert(instr_total == currInstr);
    instruction* p = (instruction*) malloc(NEW_SIZE);    
    if (instr){
        memcpy(p,instr,CURR_SIZE);
        free(instr);
    }
    instr = p;
    instr_total += EXPAND_SIZE;
}

vmarg* var_table_cases (vmarg* arg, SymbolTableEntry* sym, unsigned isTable) {
    unsigned int i = 0;
    arg->offset = -1;
    
    for (i; i<var_offset; i++){
        if (sym){
            if (!strcmp(sym->name, var_table[i]->name)) {
                arg->offset = i;
                break;
            }
        }
    }
    if (arg->offset == -1) {
        if (check_library_collisions(sym->name) == 1) {
            arg->offset = insert_to_libfunc_table(sym->name);
            arg->type = libfunc_a;
            return arg;
        } else if (check_userfunc_collision(sym->name) != -1) {
            arg->offset = check_userfunc_collision(sym->name);
            arg->type = userfunc_a;
            return arg;
        }
        arg->offset = insert_to_var_table(sym, isTable);
    }
    if (sym->space == programvar) {
        if (sym->type == GLOBAL) arg->type = global_a;
        else if (sym->type == LOCAL) {
            arg->type = local_a;
        }
    }
    else if (sym->space == functionlocal) {
        arg->type = local_a;
    }
    else if (sym->space == formalarg) {
        arg->type = formal_a;
    }
    return arg;
}

int check_userfunc_collision (char* name) {
    unsigned int i = 0;
    for (i; i<userfunc_offset; i++) {
        if (!strcmp(userfunc_table[i]->name, name)) return i;
    }
    return -1;
}

unsigned getTotallocals (unsigned start, unsigned finish) {
    unsigned i;
    unsigned counter = 0;
    for (i=start; i<finish;i++){
        if (var_table[i]->type == LOCAL) counter++;
    }
    return counter;
}

void print_formal_arguments (SymbolTableEntry* sym) {
    unsigned counter = 0;
    formal_args* temp = sym->value.funcVal->arg;
    while(temp) {
        counter++;
        fprintf(stderr, "name: %s\n", temp->var->name);
        temp = temp->next;
    }
    fprintf(stderr,"formal_args: %d\n", counter);
}

vmarg* make_operand (enum iopcode op, expr* e){
    vmarg* arg;
    unsigned int i;
    if (e == NULL) return NULL;
    
    arg = (vmarg*) malloc(sizeof(vmarg));

    //fprintf(stderr, "Type: %d\n", e->type + 1);
    switch (e->type){
        case var_e:
            arg = var_table_cases(arg, e->sym, 0);
            break;
        case programfunc_e:
            i = 0;
            arg->offset = -1;
            if (op == 20) {
                if (userfunc_offset == 0) total_locals_funcoffset = userfunc_offset;
                else total_locals_funcoffset = userfunc_offset - 1;
                
                total_locals = var_offset;
                infunction = 1;
            } else if (op == 21) {
                userfunc_table[total_locals_funcoffset]->local_size = getTotallocals(total_locals, var_offset);
                infunction = 0;
            }
            for (i; i<userfunc_offset; i++){
                if (e->sym){
                    if (!strcmp(e->sym->name, userfunc_table[i]->name)) {
                        arg->offset = i;
                        break;
                    }
                }
            }
            if (arg->offset == -1 && infunction == 1) {
                arg->offset = insert_to_userfunc_table(e->sym->name, e->sym->iaddress, e->sym->totalLocals, e->sym->value.funcVal->arg);
            }
            arg->type = userfunc_a;
            break;
        case libraryfunc_e:
            arg->offset = insert_to_libfunc_table(e->sym->name);
            arg->type = libfunc_a;
            break;
        case tableitem_e:
            arg = var_table_cases(arg, e->sym, 0);
            break;
        case arithexpr_e:
            arg = var_table_cases(arg, e->sym, 0);
            break;
        case boolexpr_e:
            arg = var_table_cases(arg, e->sym, 0);
            break;
        case assignexpr_e: 
            arg = var_table_cases(arg, e->sym, 0);
            break;
        case newtable_e:
            arg = var_table_cases(arg, e->sym, 1);
            break;
        case constnum_e:
            arg->offset = insert_to_num_table(e->numConst);
            arg->type = number_a;
            break;
        case constbool_e:
            //assert(e->boolConst);
            if (!e->boolConst) arg->offset = 0;
            else arg->offset = 1;
            arg->type = bool_a;
            break;
        case conststring_e:
            memmove(e->strConst, e->strConst+1, strlen(e->strConst));
            e->strConst[strlen(e->strConst) - 1] = '\0';
            arg->offset = insert_to_str_table(e->strConst);
            arg->type = string_a;
            break;
        case nil_e:
            arg->type = nil_a;
            break;
        default: assert(0);
    }
}

void generate (quad* quad){
    instruction* t = (instruction*) malloc(sizeof(instruction));
    if (currInstr == instr_total) expand_instr();
    t = instr + currInstr++;
    t->opcode = vmop_conversion(quad->op);
    t->arg1 = make_operand(t->opcode, quad->arg1);
    t->arg2 = make_operand(t->opcode, quad->arg2);
    if (t->opcode == 25) {
        vmarg* temp = (vmarg*) malloc(sizeof(vmarg));
        temp->type = label_a;
        t->result = temp;
    } else if (t->opcode == 18){
        vmarg* temp = (vmarg*) malloc(sizeof(vmarg));
        temp->offset = t->arg1->offset;
        temp->type = retval_a;
        t->result = temp;
    }
    else {
        t->result = make_operand(t->opcode, quad->result);
    }
    
    t->srcLine = quad->line;
    t->label = quad->label;
}

enum vmopcode vmop_conversion(enum iopcode type) {
    switch (type){
        case 0: return assign_v;
        case 1: return add_v; 
        case 2: return sub_v;
        case 3: return mul_v;
        case 4: return divide_v;
        case 5: return mod_v;
        case 6: return uminus_v;
        case 7: return and_v;
        case 8: return or_v;
        case 9: return not_v;
        case 10: return if_eq_v;
        case 11: return if_neq_v;
        case 12: return if_lesseq_v;
        case 13: return if_greatereq_v;
        case 14: return if_less_v;
        case 15: return if_greater_v;
        case 16: return call_v;
        case 17: return param_v;
        case 18: return ret_v;
        case 19: return getretval_v;
        case 20: return funcstart_v;
        case 21: return funcend_v;
        case 22: return tablecreate_v;
        case 23: return tablegetelem_v;
        case 24: return tablesetelem_v;
        case 25: return jump_v;
        case 27: return nop_v;
        default: assert(0);

    }

        
}

void start_generate (){
    quad* temp;
    int counter = 0;
    while (counter < currQuad){
        temp = quads + counter++;
        generate(temp);
    }
    print_instructions();
    print_num_array();
    print_str_array();
    print_usertfunc_array();
    print_libfunc_array();
    print_var_array();
    binary();
}

void print_num_array (){
    unsigned int i = 0;
    fprintf(stderr, "\nTable of numbers:\n");
    for (i; i<num_offset; i++){
        fprintf(stderr, "\tIndex: %d \tValue: %f \n", i, num_table[i]->value);
    }
}

void print_str_array (){
    unsigned int i = 0;
    fprintf(stderr, "Table of strings:\n");
    for (i; i<str_offset; i++){
        fprintf(stderr, "\tIndex: %d \tValue: %s\n", i, str_table[i]);
    }
}

void print_usertfunc_array (){
    unsigned int i = 0;
    fprintf(stderr, "Table of userfuncs:\n");
    for (i; i<userfunc_offset; i++){
        fprintf(stderr, "\tIndex: %d \tID: %s \tAddress: %d \tLocalsize: %d\n", i, 
        userfunc_table[i]->name, 
        userfunc_table[i]->address, userfunc_table[i]->local_size);
    }
}

void print_libfunc_array (){
    unsigned int i = 0;
    fprintf(stderr, "Table of libfuncs:\n");
    for (i; i<libfunc_offset; i++){
        fprintf(stderr, "\tIndex: %d \tValue: %s\n", i, libfunc_table[i]);
    }
}

void print_var_array (){
    unsigned int i = 0;
    fprintf(stderr, "Table of vars:\n");
    for (i; i<var_offset; i++){
        fprintf(stderr, "\tIndex: %d \tName: %s \tSymbolType: %d \tType_offset: %d \tTypeval %d\n"
        , i, var_table[i]->name, var_table[i]->type, var_table[i]->type_offset, var_table[i]->type_val);
    }
}

void print_instructions (){
    instruction* temp;
    int counter = 0;
    fprintf(stderr, "\n----------------------------------------------------------------------------------------------------------------\n");
    fprintf(stderr, "|                                                 INSTRUCTIONS                                                 |");
    fprintf(stderr, "\n----------------------------------------------------------------------------------------------------------------\n");
    while (counter < currInstr){
        temp = instr + counter++;
        //if (temp->opcode == 19) temp->label = 0;
        if (temp->opcode == 18) {
            fprintf(stderr, "#%d \tOpcode-> %s", counter, vm_operations_to_string[0]);
        } else {
            fprintf(stderr, "#%d \tOpcode-> %s", counter, vm_operations_to_string[temp->opcode]);
        }
        fprintf(stderr, "\tResult-> ");
        if (temp->result) print_vmarg(temp->result);
        fprintf(stderr, "\tArg1-> ");
        if (temp->arg1) print_vmarg(temp->arg1);
        fprintf(stderr, "\tArg2-> ");
        if (temp->arg2) print_vmarg(temp->arg2);
        fprintf(stderr, "\tLabel-> ");
        if (temp->label != 0) fprintf(stderr, "%d", temp->label);
        fprintf(stderr, "\tLine-> %d\n", temp->srcLine);
    }
    fprintf(stderr, "----------------------------------------------------------------------------------------------------------------\n");

}

void print_vmarg (vmarg* e) {
    enum vmarg_t type = e->type;
    switch (type) {
        case 0:
            fprintf(stderr, "(label)");
            break;
        case 1:
            fprintf(stderr, "%s:%d|(global)", var_table[e->offset]->name, var_table[e->offset]->type_offset);
            break;
        case 2:
            fprintf(stderr, "%s:%d|(formal)", var_table[e->offset]->name, var_table[e->offset]->type_offset);
            break;
        case 3:
            fprintf(stderr, "%s:%d|(local)", var_table[e->offset]->name, var_table[e->offset]->type_offset);
            break;
        case 4:
            fprintf(stderr, "%f:%d|(number)", num_table[e->offset]->value, e->offset);
            break;
        case 5:
            fprintf(stderr, "%s:%d|(string)", str_table[e->offset], e->offset);
            break;
        case 6:
            fprintf(stderr, "%s", "(bool)");
            break;
        case 7:
            fprintf(stderr, "%s", "(nil)");
            break;
        case 8:
            fprintf(stderr, "%s:%d|(userfunc)", userfunc_table[e->offset]->name, e->offset);
            break;
        case 9:
            fprintf(stderr, "%s:%d|(libfunc)", libfunc_table[e->offset], e->offset);
            break;
        case 10:
            fprintf(stderr, "%s", "(retval)");
            break;
        default:
            assert(0);
    }
}

int generate_number (char *s){
    unsigned int i=0;
    int sum = 0;
    while (i < strlen(s)) {
        sum += s[i];
        i++;
    }
    return sum;
}

void binary () {
    FILE* file = NULL;
    instruction* temp;
    unsigned temp_instr = 0;
    unsigned int i;
    int number = generate_number("manos eirini thanos....");

    file = fopen("bin.abc", "wb");
    if (!file) {
        fprintf(stderr, "Error, opening file\n");
        exit(0);
    }

    fwrite(&number, sizeof(int), 1, file);

    if (fwrite(&str_offset, sizeof(unsigned), 1, file) != 1){
        print_error();
    }
    i = 0;
    while (i < str_offset){
        if (fwrite(&str_table[i], sizeof(char*), 1, file) != 1){
            print_error();
        }
        i++;
    }

    if (fwrite(&num_offset, sizeof(unsigned), 1, file) != 1){
        print_error();
    }
    i = 0;
    while (i < num_offset){
        if (fwrite(&num_table[i]->value, sizeof(double), 1, file) != 1){
            print_error();
        }
        i++;
    }

    if (fwrite(&libfunc_offset, sizeof(unsigned), 1, file) != 1){
        print_error();
    }
    i = 0;
    while (i < libfunc_offset){
        if (fwrite(&libfunc_table[i], sizeof(char*), 1, file) != 1){
            print_error();
        }
        i++;
    }

    if (fwrite(&userfunc_offset, sizeof(unsigned), 1, file) != 1){
        print_error();
    }
    i = 0;
    while (i < userfunc_offset){
        if (fwrite(&userfunc_table[i]->name, sizeof(char*), 1, file) != 1){
            print_error();
        }
        if (fwrite(&userfunc_table[i]->address, sizeof(unsigned), 1, file) != 1){
            print_error();
        }
        if (fwrite(&userfunc_table[i]->local_size, sizeof(unsigned), 1, file) != 1){
            print_error();
        }
        if (fwrite(&userfunc_table[i]->arg, sizeof(formal_args*), 1, file) != 1){
            print_error();
        }
        i++;
    }

    if (fwrite(&var_offset, sizeof(unsigned), 1, file) != 1){
        print_error();
    }

    i = 0;
    while (i < var_offset){
        if (fwrite(&var_table[i]->name, sizeof(char*), 1, file) != 1){
            print_error();
        }
        if (fwrite(&var_table[i]->type, sizeof(unsigned), 1, file) != 1){
            print_error();
        }
        if (fwrite(&var_table[i]->type_offset, sizeof(unsigned), 1, file) != 1){
            print_error();
        }
        if (fwrite(&var_table[i]->val.doubleVal, sizeof(double), 1, file) != 1){
            print_error();
        }
        if (fwrite(&var_table[i]->val.strVal, sizeof(char*), 1, file) != 1){
            print_error();
        }
        if (fwrite(&var_table[i]->val.boolVal, sizeof(unsigned char), 1, file) != 1){
            print_error();
        }
        if (fwrite(&var_table[i]->val.tableVal, sizeof(table*), 1, file) != 1){
            print_error();
        }
        if (fwrite(&var_table[i]->type_val, sizeof(enum typeVal), 1, file) != 1){
            print_error();
        }
        i++;
    }

    fwrite(&currInstr, sizeof(unsigned), 1, file);
    
    for (i = 0; i < currInstr; i++) {
        temp = instr + i;
        
        if (fwrite(&(temp->opcode), sizeof(int), 1, file) != 1) {
			print_error();
        }
        if (temp->result) {
            if (fwrite(&(temp->result->type),sizeof(unsigned), 1, file) != 1) {
				print_error();
            }
			if (fwrite(&(temp->result->offset),sizeof(unsigned), 1, file) != 1) {
				print_error();
            }
        }
        if (temp->arg1) {
            if (fwrite(&(temp->arg1->type),sizeof(unsigned), 1, file) != 1) {
				print_error();
            }
			if (fwrite(&(temp->arg1->offset),sizeof(unsigned), 1, file) != 1) {
				print_error();
            }
        }
        if (temp->arg2) {
            if (fwrite(&(temp->arg2->type),sizeof(unsigned), 1, file) != 1) {
				print_error();
            }
			if (fwrite(&(temp->arg2->offset),sizeof(unsigned), 1, file) != 1) {
				print_error();
            }
        }
    }

    if (file) fclose(file);

}

void print_error (){
    fprintf(stderr, "Error, while writing in the Binary file.\n");
    exit(0);
}