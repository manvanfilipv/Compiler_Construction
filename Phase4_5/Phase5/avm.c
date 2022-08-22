#include "avm.h"

#define EXPAND_SIZE 1024
#define CURR_SIZE (instr_total_avm * sizeof(instruction))
#define NEW_SIZE (EXPAND_SIZE * sizeof(instruction) + CURR_SIZE)

var* Tvar_table;
num* Tnum_table;
char** Tstr_table;
userfunc* Tuserfunc_table;
char** Tlibfunc_table;

unsigned Tvar_offset = 0;
unsigned Tnum_offset = 0;
unsigned Tstr_offset = 0;
unsigned Tuserfunc_offset = 0;
unsigned Tlibfunc_offset = 0;

unsigned int libfunc_activated = 0;
num* libfunc_result;
table* libfunc_result_table;
char* str_libfunc_result;
unsigned char bool_libfunc_result;

instruction* instr_avm = (instruction*) 0;
unsigned instr_total_avm = 0;
unsigned int currInstr_avm = 0;

unsigned int loop_counter = 0;
unsigned int temp_counter = 0;
unsigned int outside_func_counter = 0;
unsigned int Tinfunction = 0;
vmarg* ret_value;

extern instruction* instr;          //Temp solution error from reading from binary
extern unsigned instr_total;        //Temp solution error from reading from binary
extern unsigned int currInstr;      //Temp solution error from reading from binary

node_param* params;
node_param* nested_params;

void start_avm () {
    instruction* temp;
    unsigned int counter = 0;
    while (counter < currInstr){
        temp = instr + counter++;
        counter = execute_cycle(temp, counter);
    }
    //print_var_table();
    //print_num_array();
    fprintf(stderr, "\n");
}

double rm_min (double a) {
    if (a == -0) return 0;
    return a;
}

void perform_arithm_operation (instruction* temp, char* op) {
    double a;
    double b;
    if (temp->result->type == global_a || temp->result->type == local_a || temp->result->type == formal_a){
        if (temp->arg1->type >= 1 && temp->arg1->type <= 3 
        && temp->arg2->type >= 1 && temp->arg2->type <= 3 ) {
            a = Tvar_table[temp->arg1->offset].val.doubleVal;
            b = Tvar_table[temp->arg2->offset].val.doubleVal; 
        } else if (temp->arg1->type >= 1 && temp->arg1->type <= 3 
        && temp->arg2->type == 4) {
            a = Tvar_table[temp->arg1->offset].val.doubleVal;
            b = Tnum_table[temp->arg2->offset].value;     
        } else if (temp->arg2->type >= 1 && temp->arg2->type <= 3 
        && temp->arg1->type == 4) {
            a = Tnum_table[temp->arg1->offset].value;
            b = Tvar_table[temp->arg2->offset].val.doubleVal;
        } else if (temp->arg1->type == 4 && temp->arg2->type == 4) {
            a = Tnum_table[temp->arg1->offset].value;
            b = Tnum_table[temp->arg2->offset].value;
        } else {
            fprintf(stderr, "Cannot do arithmetic expression when arg1,arg2 are not both of double type\n");
            exit(0);
        }

        if (!strcmp(op, "add")) {
            Tvar_table[temp->result->offset].val.doubleVal = rm_min(a + b);
        } else if (!strcmp(op, "sub")) {
            Tvar_table[temp->result->offset].val.doubleVal = rm_min(a - b);
        } else if (!strcmp(op, "mul")) {
            Tvar_table[temp->result->offset].val.doubleVal = rm_min(a * b);
        } else if (!strcmp(op, "div")) {
            if (b != 0){
                Tvar_table[temp->result->offset].val.doubleVal = rm_min(a / b);
            } else {
                fprintf(stderr, "Cannot divide with 0\n");
                exit(0);
            }
        } else if (!strcmp(op, "mod")) {
            //printf("a1: %f\n", a);
            //printf("b1: %f\n", b);
            if (b != 0){
                Tvar_table[temp->result->offset].val.doubleVal = rm_min((int)a % (int)b);
            } else {
                fprintf(stderr, "Cannot mod with 0\n");
                exit(0);
            }
        } else assert(0);
    }
}

unsigned int perform_bool_operation (instruction* temp, char* op, unsigned label) {
    double a;
    double b;
    unsigned int c;
    unsigned int d;
    //printf("arg1type: %d\n", temp->arg1->type);
    //printf("arg2type: %d\n", temp->arg2->type);
    if (temp->arg1->type >= 1 && temp->arg1->type <= 3 
    && temp->arg2->type >= 1 && temp->arg2->type <= 3 ) {
        if (Tvar_table[temp->arg1->offset].type_val == 0 && Tvar_table[temp->arg1->offset].type_val == 0){
            a = Tvar_table[temp->arg1->offset].val.doubleVal;
            b = Tvar_table[temp->arg2->offset].val.doubleVal;
            //printf("a: %f\n", a);
            //printf("b: %f\n", b);
        } else if (Tvar_table[temp->arg1->offset].type_val == 2 && Tvar_table[temp->arg1->offset].type_val == 2) {
            c = Tvar_table[temp->arg1->offset].val.boolVal;
            d = Tvar_table[temp->arg2->offset].val.boolVal;
            
        }
    } else if (temp->arg1->type >= 1 && temp->arg1->type <= 3 && temp->arg2->type == 6) {
        c = Tvar_table[temp->arg1->offset].val.boolVal;
        d = Tvar_table[temp->arg2->offset].val.boolVal;
        if (c == d) return temp->label;
        //else return label;
    } else if (temp->arg1->type >= 1 && temp->arg1->type <= 3 && temp->arg2->type == 4){
        a = Tvar_table[temp->arg1->offset].val.doubleVal;
        b = Tnum_table[temp->arg2->offset].value;
        //printf("a: %f\n", a);
        //printf("b: %f\n", b);
    } 
    else if (temp->arg1->type == 6 && temp->arg2->type == 6) {
        a = temp->arg1->offset;
        //printf("arg1: %d\n", temp->arg1->offset);
        //printf("arg2: %d\n", temp->arg2->offset);
        b = temp->arg2->offset;
    }
    // mono edw bool option
    if (!strcmp(op, "ifeq")) {
        if (a == b) return temp->label - 1;
    } else if (!strcmp(op, "ifneq")) {
        
        if (a != b) return temp->label - 1;
    } else if (!strcmp(op, "iflesseq")) {
        if (a <= b) return temp->label - 1;
    } else if (!strcmp(op, "ifgreatereq")) {
        //printf("a: %f\n", a);
        //printf("b: %f\n", b);
        if (a >= b) {
            //printf("temp->label - 1: %d\n", temp->label - 1);
            return temp->label - 1;
        }
    } else if (!strcmp(op, "ifless")) {
        
        if (a < b) return temp->label - 1;
    } else if (!strcmp(op, "ifgreater")) {
        if (a > b) return temp->label - 1;
    } else assert(0);
    return label;
}

unsigned int execute_cycle (instruction* temp, unsigned int counter) {
    //fprintf(stderr, "Counter: %d\n", counter);
    struct table* new_node;
    struct table* last;
    switch (temp->opcode) {
        case assign_v:
            if (temp->arg2 != NULL) {
                if (temp->arg2->type == global_a || temp->arg2->type == formal_a || temp->arg2->type == local_a){
                    if (!strcmp(Tvar_table[temp->arg2->offset].name, "nil")) {
                        Tvar_table[temp->result->offset].type_val = nilVal;
                        break;
                    }
                    if (Tvar_table[temp->arg2->offset].type_val == doubleVal) {
                        Tvar_table[temp->result->offset].val.doubleVal = Tvar_table[temp->arg2->offset].val.doubleVal;
                        Tvar_table[temp->result->offset].type_val = doubleVal;
                    } else if (Tvar_table[temp->arg2->offset].type_val == stringVal) {
                        Tvar_table[temp->result->offset].val.strVal= Tvar_table[temp->arg2->offset].val.strVal;
                        Tvar_table[temp->result->offset].type_val = stringVal;
                    } else if (Tvar_table[temp->arg2->offset].type_val == boolVal) {
                        Tvar_table[temp->result->offset].val.boolVal= Tvar_table[temp->arg2->offset].val.boolVal;
                        Tvar_table[temp->result->offset].type_val = boolVal;
                    } else if (Tvar_table[temp->arg2->offset].type_val == tableVal) {
                        Tvar_table[temp->result->offset].val.tableVal = Tvar_table[temp->arg2->offset].val.tableVal;
                        Tvar_table[temp->result->offset].type_val = tableVal;
                        Tvar_table[temp->arg2->offset].val.tableVal = NULL;
                    } 
                } else if (temp->arg2->type == number_a) {
                    Tvar_table[temp->result->offset].val.doubleVal = Tnum_table[temp->arg2->offset].value;
                    Tvar_table[temp->result->offset].type_val = doubleVal;
                } else if (temp->arg2->type == string_a) {
                    Tvar_table[temp->result->offset].val.strVal= Tstr_table[temp->arg2->offset];
                    Tvar_table[temp->result->offset].type_val = stringVal;
                } else if (temp->arg2->type == bool_a) {
                    Tvar_table[temp->result->offset].val.boolVal= temp->arg2->offset;
                    Tvar_table[temp->result->offset].type_val = boolVal;
                }
            }
            break;
        case add_v:
            perform_arithm_operation(temp, "add");
            break;
        case sub_v:
            perform_arithm_operation(temp, "sub");
            break;
        case mul_v:
            perform_arithm_operation(temp, "mul");
            break;
        case divide_v:
            perform_arithm_operation(temp, "div");
            break;
        case mod_v:
            perform_arithm_operation(temp, "mod");
            break;
        case and_v:
            break;
        case or_v:
            break;
        case not_v:
            break;
        case if_eq_v:
            counter = perform_bool_operation(temp, "ifeq", counter);
            break;
        case if_neq_v:
            counter = perform_bool_operation(temp, "ifneq", counter);
            break;
        case if_lesseq_v:
            counter = perform_bool_operation(temp, "iflesseq", counter);
            break;
        case if_greatereq_v:
            counter = perform_bool_operation(temp, "ifgreatereq", counter);
            break;
        case if_less_v:
            counter = perform_bool_operation(temp, "ifless", counter);
            break;
        case if_greater_v:
            counter = perform_bool_operation(temp, "ifgreater", counter);
            break;
        case call_v:
            if (temp->arg1->type == libfunc_a) {
                if (!strcmp(Tlibfunc_table[temp->arg1->offset], "print")) {
                    if (Tinfunction == 0) {
                        print_param_list(params);
                        //delete_param();
                    } else {
                        print_param_list(nested_params);
                        //delete_nested_param();
                        break;
                    }
                } else if (!strcmp(Tlibfunc_table[temp->arg1->offset], "objecttotalmembers")) {
                    objecttotalmembers(params);
                } else if (!strcmp(Tlibfunc_table[temp->arg1->offset], "objectmemberkeys")) {
                    objectmemberkeys(params);
                } else if (!strcmp(Tlibfunc_table[temp->arg1->offset], "objectcopy")) {
                    objectcopy(params);
                } else if (!strcmp(Tlibfunc_table[temp->arg1->offset], "strtonum")) {
                    strtonum(params);
                } else if (!strcmp(Tlibfunc_table[temp->arg1->offset], "sqrt")) {
                    square(params);
                } else if (!strcmp(Tlibfunc_table[temp->arg1->offset], "sin")) {
                    cos_sin(params,0);
                } else if (!strcmp(Tlibfunc_table[temp->arg1->offset], "cos")) {
                    cos_sin(params,1);
                } else if (!strcmp(Tlibfunc_table[temp->arg1->offset], "typeof")) {
                    mytypeof(params);
                } else if (!strcmp(Tlibfunc_table[temp->arg1->offset], "totalarguments")) {
                    if (Tinfunction == 1) {
                        totalarguments(params);
                    }
                    libfunc_activated = 1;
                } else if (!strcmp(Tlibfunc_table[temp->arg1->offset], "input")) {
                    input(params);
                }
                delete_param();
            } else if (temp->arg1->type == userfunc_a) {
                if (Tinfunction == 1) {
                    temp_counter = counter;
                    point_args_to_formals(temp->arg1, nested_params);
                } else {
                    outside_func_counter = counter; // gia na vgw eksw apo sinartisi 18
                    //fprintf(stderr, "out: %d\n", outside_func_counter);
                    //exit(0);
                    point_args_to_formals(temp->arg1, params);
                }
                counter = Tuserfunc_table[temp->arg1->offset].address - 1;
            }
            break;
        case param_v:
            if (Tinfunction == 1){
                append_nested_param(temp->result);
            } else {
                append_param(temp->result);
            }
            break;
        case ret_v:
            delete_nested_param();
            ret_value = temp->arg1;
            break;
        case getretval_v:
            if (ret_value) {
                if (ret_value->type >= 1 && ret_value->type <= 3) {
                    if (Tvar_table[ret_value->offset].type_val == 0) {
                        Tvar_table[temp->result->offset].val.doubleVal = Tvar_table[ret_value->offset].val.doubleVal;
                        Tvar_table[temp->result->offset].type_val = 0;
                    } else if (Tvar_table[ret_value->offset].type_val == 1) {
                        Tvar_table[temp->result->offset].val.strVal = Tvar_table[ret_value->offset].val.strVal;
                        Tvar_table[temp->result->offset].type_val = 1;
                    } else if (Tvar_table[ret_value->offset].type_val == 2) {
                        Tvar_table[temp->result->offset].val.boolVal = Tvar_table[ret_value->offset].val.boolVal;
                        Tvar_table[temp->result->offset].type_val = 2;
                    }
                } else if (ret_value->type == 4) {
                    Tvar_table[temp->result->offset].val.doubleVal = Tnum_table[ret_value->offset].value;
                    Tvar_table[temp->result->offset].type_val = 0;
                } else if (ret_value->type == 5) {
                    Tvar_table[temp->result->offset].val.strVal = Tstr_table[ret_value->offset];
                    Tvar_table[temp->result->offset].type_val = 1;
                } else if (ret_value->type == 6) {
                    Tvar_table[temp->result->offset].val.boolVal = ret_value->offset;
                    Tvar_table[temp->result->offset].type_val = 2;
                }
                ret_value = NULL;
            } else {
                if (libfunc_activated == 0) break;
                else if (libfunc_activated == 1) { 
                    //objecttotalmembers || strtonum || sqrt || sin || cos
                    if (libfunc_result) {
                        Tvar_table[temp->result->offset].type_val = doubleVal;
                        Tvar_table[temp->result->offset].val.doubleVal = libfunc_result->value;
                    } else {
                        Tvar_table[temp->result->offset].type_val = stringVal;
                        Tvar_table[temp->result->offset].val.strVal = "nil";
                    }
                } else if (libfunc_activated == 2) { //objectmemberkeys || objectcopy
                    Tvar_table[temp->result->offset].type_val = tableVal;
                    Tvar_table[temp->result->offset].val.tableVal = libfunc_result_table;
                } else if (libfunc_activated == 3) { //typeof
                    Tvar_table[temp->result->offset].type_val = stringVal;
                    Tvar_table[temp->result->offset].val.strVal = str_libfunc_result;
                } else if (libfunc_activated == 4) { //case of input (true || false)
                    Tvar_table[temp->result->offset].type_val = boolVal;
                    Tvar_table[temp->result->offset].val.boolVal = bool_libfunc_result;
                }
                reset_libfunc_vars();
                
            }
            break;
        case funcstart_v:
            Tinfunction = 1;
            loop_counter++;
            break;
        case funcend_v:
            Tinfunction = 0;
            loop_counter--;
            if (loop_counter != 0) counter = temp_counter;
            else counter = outside_func_counter;
            break;
        case tablecreate_v:
            Tvar_table[temp->result->offset].type_val = tableVal;
            break;
        case tablegetelem_v:
            //fprintf(stderr, "name: %s\t", Tvar_table[temp->arg1->offset].name);
            if (Tvar_table[temp->arg1->offset].val.tableVal) {
                
                if (temp->arg2->type >= 1 && temp->arg2->type <= 3) {
                    if (Tvar_table[temp->arg2->offset].type_val == 0) {
                        unsigned int index = (int)Tvar_table[temp->arg2->offset].val.doubleVal;
                        //fprintf(stderr, "nikos : %d\n",(int)Tvar_table[temp->arg2->offset].val.doubleVal);
                        table* temp2 = Tvar_table[temp->arg1->offset].val.tableVal;
                        while (temp2) {
                            if (temp2->index.intVal == index) {
                                if (temp2->type == 0) {
                                    
                                    Tvar_table[temp->result->offset].val.doubleVal = temp2->val.doubleVal;
                                    Tvar_table[temp->result->offset].type_val = doubleVal;
                                } else if (temp2->type == 1) {
                                    Tvar_table[temp->result->offset].val.strVal = temp2->val.strVal;
                                    Tvar_table[temp->result->offset].type_val = stringVal;
                                } else if (temp2->type == 2) {
                                    Tvar_table[temp->result->offset].val.boolVal = temp2->val.boolVal;
                                    Tvar_table[temp->result->offset].type_val = boolVal;
                                } else if (temp2->type == 3) {
                                    Tvar_table[temp->result->offset].val.tableVal = temp2->val.nested_table;
                                    Tvar_table[temp->result->offset].type_val = tableVal;
                                }
                            }
                            temp2 = temp2->next;
                        }
                    } else if (Tvar_table[temp->arg2->offset].type_val == 1) {
                        char* index = Tstr_table[temp->arg2->offset];
                        table* temp2 = Tvar_table[temp->arg1->offset].val.tableVal;
                        while (temp2) {
                            if (!strcmp(temp2->index.strVal, index)) {
                                if (temp2->type == 0) {
                                    Tvar_table[temp->result->offset].val.doubleVal = temp2->val.doubleVal;
                                    Tvar_table[temp->result->offset].type_val = doubleVal;
                                } else if (temp2->type == 1) {
                                    Tvar_table[temp->result->offset].val.strVal = temp2->val.strVal;
                                    Tvar_table[temp->result->offset].type_val = stringVal;
                                } else if (temp2->type == 2) {
                                    Tvar_table[temp->result->offset].val.boolVal = temp2->val.boolVal;
                                    Tvar_table[temp->result->offset].type_val = boolVal;
                                } else if (temp2->type == 3) {
                                    Tvar_table[temp->result->offset].val.tableVal = temp2->val.nested_table;
                                    Tvar_table[temp->result->offset].type_val = tableVal;
                                }
                            }
                            temp2 = temp2->next;
                        }
                    } else {
                        fprintf(stderr, "index cannot be string or boolean\n");
                        exit(0);
                    }
                } else if (temp->arg2->type == 4) {
                    unsigned int index = (int)Tnum_table[temp->arg2->offset].value;
                    //fprintf(stderr, "index: %d\n", index);
                    table* temp2 = Tvar_table[temp->arg1->offset].val.tableVal;
                    //int counter = 0;
                    //while (temp2) {
                    //    counter++;
                    //    if(temp2->type == 0) fprintf(stderr, "valuie : %f\t", temp2->val.doubleVal);
                    //    else if (temp2->type == 3) fprintf(stderr, "table \t");
                    //    fprintf(stderr, "\n");
                    //    temp2 = temp2->next;
                    //}
                   //fprintf(stderr, "counter: %d\n", counter);
                    //fprintf(stderr, "temp2 %d\n", temp2->type);
                    
                    while (temp2) {
                        
                        if (temp2->index.intVal == index) {
                            //fprintf(stderr, "type: %d\n", temp2->type);
                            if (temp2->type == 0) {
                                //fprintf(stderr, "type: %d\n", temp2->type);
                                Tvar_table[temp->result->offset].val.doubleVal = temp2->val.doubleVal;
                                
                                Tvar_table[temp->result->offset].type_val = doubleVal;
                            } else if (temp2->type == 1) {
                                Tvar_table[temp->result->offset].val.strVal = temp2->val.strVal;
                                Tvar_table[temp->result->offset].type_val = stringVal;
                            } else if (temp2->type == 2) {
                                Tvar_table[temp->result->offset].val.boolVal = temp2->val.boolVal;
                                Tvar_table[temp->result->offset].type_val = boolVal;
                            } else if (temp2->type == 3) {
                                
                                Tvar_table[temp->result->offset].val.tableVal = temp2->val.nested_table;
                                Tvar_table[temp->result->offset].type_val = tableVal;
                            } else {
                                assert(0);
                            }
                            break;
                        }
                        temp2 = temp2->next;
                    }
                } else if (temp->arg2->type == 5) {
                    char* index = Tstr_table[temp->arg2->offset];
                    table* temp2 = Tvar_table[temp->arg1->offset].val.tableVal;
                    while (temp2) {
                        if (!strcmp(temp2->index.strVal, index)) {
                            if (temp2->type == 0) {
                                Tvar_table[temp->result->offset].val.doubleVal = temp2->val.doubleVal;
                                Tvar_table[temp->result->offset].type_val = doubleVal;
                            } else if (temp2->type == 1) {
                                Tvar_table[temp->result->offset].val.strVal = temp2->val.strVal;
                                Tvar_table[temp->result->offset].type_val = stringVal;
                            } else if (temp2->type == 2) {
                                Tvar_table[temp->result->offset].val.boolVal = temp2->val.boolVal;
                                Tvar_table[temp->result->offset].type_val = boolVal;
                            } else if (temp2->type == 3) {
                                Tvar_table[temp->result->offset].val.tableVal = temp2->val.nested_table;
                                Tvar_table[temp->result->offset].type_val = tableVal;
                            }
                        }
                        temp2 = temp2->next;
                    }
                } 
                else {
                    fprintf(stderr, "index cannot be string or boolean\n");
                    exit(0);
                }
            } else {
                fprintf(stderr, "tableget element that is undefined\n");
                //exit(0);
            }
            break;
        case tablesetelem_v:
            new_node = (struct table*) malloc(sizeof(struct table));
            // Setting index depending on the type of arg1
            if (temp->arg1->type == 4) {
                new_node->index_type = 0;
                new_node->index.intVal = (int)Tnum_table[temp->arg1->offset].value;
            } else if (temp->arg1->type == 5) {
                new_node->index_type = 1;
                new_node->index.strVal = Tstr_table[temp->arg1->offset];
            }

            // Setting next element to NULL and last in case list contains more than one element
            new_node->next = NULL;
            last = Tvar_table[temp->result->offset].val.tableVal;
            // Checking types of arg2
            if (temp->arg2->type >= 1 && temp->arg2->type <= 3) {
                // Checking for nil
                if (!strcmp(Tvar_table[temp->arg2->offset].name, "nil")) {
                    new_node->type = stringVal;
                    new_node->val.strVal = Tvar_table[temp->arg2->offset].name;
                } else if (Tvar_table[temp->arg2->offset].type_val == 0) {
                    
                    new_node->type = doubleVal;
                    new_node->val.doubleVal = Tvar_table[temp->arg2->offset].val.doubleVal;
                } else if (Tvar_table[temp->arg2->offset].type_val == 1) {
                    new_node->type = stringVal;
                    new_node->val.strVal = Tvar_table[temp->arg2->offset].val.strVal;
                } else if (Tvar_table[temp->arg2->offset].type_val == 2) {
                    new_node->type = boolVal;
                    new_node->val.boolVal = temp->arg2->offset;
                } else if (Tvar_table[temp->arg2->offset].type_val == 3) {
                    new_node->type = tableVal;
                    new_node->val.nested_table = Tvar_table[temp->arg2->offset].val.tableVal;
                } else assert(0);
            } else if (temp->arg2->type == 4) {
                new_node->type = doubleVal;
                new_node->val.doubleVal = Tnum_table[temp->arg2->offset].value;
            } else if (temp->arg2->type == 5) {
                new_node->type = stringVal;
                new_node->val.strVal = Tstr_table[temp->arg2->offset];
            } else if (temp->arg2->type == 6) {
                new_node->type = boolVal;
                new_node->val.boolVal = temp->arg2->offset;
            } else assert(0);

            if (Tvar_table[temp->result->offset].val.tableVal == NULL) {
                Tvar_table[temp->result->offset].val.tableVal = new_node;
            } else {
                while (last->next != NULL) last = last->next;
                last->next = new_node;
            }
            break;
        case jump_v:
            counter = temp->label - 1;
            break;
        case nop_v:
            break;
        default:
            assert(0);
    }
    return counter;
}

void reset_libfunc_vars () {
    libfunc_result = NULL;
    libfunc_activated = 0;
    libfunc_result_table = NULL;
    str_libfunc_result = NULL;
}

void append_param (vmarg* new_data){
    struct node_param* new_node = (struct node_param*) malloc(sizeof(struct node_param));
    struct node_param* last = params;

    new_node->val = new_data;
    new_node->next = NULL;

    if (params == NULL){
        params = new_node;
        return;
    }

    while (last->next != NULL) last = last->next;
    last->next = new_node;
}

void append_nested_param (vmarg* new_data){
    struct node_param* new_node = (struct node_param*) malloc(sizeof(struct node_param));
    struct node_param* last = nested_params;

    new_node->val = new_data;
    new_node->next = NULL;

    if (nested_params == NULL){
        nested_params = new_node;
        return;
    }

    while (last->next != NULL) last = last->next;
    last->next = new_node;
}

void delete_param () {
    node_param* current = params;
    node_param* next = NULL;
    
    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }

    params = NULL;
}

void delete_nested_param () {
    node_param* current = nested_params;
    node_param* next = NULL;
    
    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }

    nested_params = NULL;
}

void point_args_to_formals (vmarg* arg1, node_param* p) {
    formal_args* temp = Tuserfunc_table[arg1->offset].arg;
    node_param* temp2 = p;
    unsigned counter = 0;
    unsigned counter2 = 0;
    while (temp) { //formals
        //fprintf(stderr, "varf: %s\t", temp->var->name);
        counter++;
        temp = temp->next;
    }
    while (temp2) { //arguments
        //fprintf(stderr, "var1: %f\t", Tnum_table[temp2->val->offset].value);
        counter2++;
        temp2 = temp2->next;
    }
    if (counter > counter2) {
        fprintf(stderr, "\nERROR Passing less arguments to user func\n");
        exit(0);
    } else if (counter < counter2) {
        fprintf(stderr, "\nWARNING Passing more arguments to user func\n");
        fprintf(stderr, "The rest will be ignored\n");
    }
    temp = Tuserfunc_table[arg1->offset].arg;
    temp2 = p;

    while (temp) {
        //fprintf(stderr, "arg: %s\t", temp->var->name);
        //fprintf(stderr, "formal: %s\t", Tstr_table[temp2->val->offset]);
        int off = findoffset(temp->var->name);
        //fprintf(stderr, "OFFSET: %d\n", off);
        if (off == -1) {
            temp = temp->next;
            temp2 = temp2->next;
            continue;
        }
        if (temp2->val->type >= 1 && temp2->val->type <= 3){
            if (Tvar_table[temp2->val->offset].type_val == 0){
                Tvar_table[off].val.doubleVal = Tvar_table[temp2->val->offset].val.doubleVal;
                Tvar_table[off].type_val = 0;
            } else if (Tvar_table[temp2->val->offset].type_val == 1){
                Tvar_table[off].val.strVal = Tvar_table[temp2->val->offset].val.strVal;
                Tvar_table[off].type_val = 1;
            } else if (Tvar_table[temp2->val->offset].type_val == 2){
                Tvar_table[off].val.boolVal = Tvar_table[temp2->val->offset].val.boolVal;
                Tvar_table[off].type_val = 2;
            } else if (Tvar_table[temp2->val->offset].type_val == 3){
                //printf("MANOSSSSSSSSSSSSSSSSSS: %d", temp2->val->type );
                Tvar_table[off].val.tableVal = Tvar_table[temp2->val->offset].val.tableVal;
                Tvar_table[off].type_val = 3;
            }
        } else if (temp2->val->type == 4) {
            Tvar_table[off].val.doubleVal = Tnum_table[temp2->val->offset].value;
            Tvar_table[off].type_val = 0;
        } else if (temp2->val->type == 5) {
            Tvar_table[off].val.strVal = Tstr_table[temp2->val->offset];
            Tvar_table[off].type_val = 1;
        } else if (temp2->val->type == 6) {
            Tvar_table[off].val.boolVal = temp2->val->offset;
            Tvar_table[off].type_val = 2;
        }
        temp = temp->next;
        temp2 = temp2->next;
    }

}

int findoffset (const char* name) {
    unsigned int i = 0;
    for (i; i<Tvar_offset; i++) {
        if (!strcmp(Tvar_table[i].name, name)) return i;
    }
    return -1;
}

void print_var_table (){
    unsigned int i = 0;
    fprintf(stderr, "\nTable of vars:\n");
    for (i; i<Tvar_offset; i++){
        fprintf(stderr, "\tIndex: %d \tName: %s \tSymbolType: %d \tType_offset: %d\tType_val %d\t", i, 
        Tvar_table[i].name, Tvar_table[i].type, Tvar_table[i].type_offset, Tvar_table[i].type_val);
        if (Tvar_table[i].type_val == doubleVal) fprintf(stderr,"doubleVal: %f", Tvar_table[i].val.doubleVal);
        else if (Tvar_table[i].type_val == stringVal) fprintf(stderr,"strVal: %s", Tvar_table[i].val.strVal);
        else if (Tvar_table[i].type_val == boolVal) fprintf(stderr,"boolVal: %d", Tvar_table[i].val.boolVal);
        fprintf(stderr, "\n");
    }
}

void print_param_list (node_param* p) {
    node_param* temp = p;
    while (temp) {
        if (temp->val->type >= 1 && temp->val->type <= 3){
            if (Tvar_table[temp->val->offset].type_val == doubleVal) {
                fprintf(stderr, "%f", Tvar_table[temp->val->offset].val.doubleVal);
            } else if (Tvar_table[temp->val->offset].type_val == stringVal) {
                fprintf(stderr, "%s", Tvar_table[temp->val->offset].val.strVal);
            } else if (Tvar_table[temp->val->offset].type_val == boolVal) {
                if (Tvar_table[temp->val->offset].val.boolVal == 0) fprintf(stderr, "false");
                else if (Tvar_table[temp->val->offset].val.boolVal == 1) fprintf(stderr, "true");
                else assert(0);
            } else if (Tvar_table[temp->val->offset].type_val == tableVal) {
                fprintf(stderr, "[ ");
                print_table(Tvar_table[temp->val->offset].val.tableVal);
                fprintf(stderr, "] ");
            } else if (Tvar_table[temp->val->offset].type_val == nilVal) {
                fprintf(stderr, "nil");
            }
        } else if (temp->val->type == number_a) {
            fprintf(stderr, "%f", Tnum_table[temp->val->offset].value);
        } else if (temp->val->type == string_a) {
            fprintf(stderr, "%s", Tstr_table[temp->val->offset]);
        } else if (temp->val->type == bool_a) {
            if (temp->val->offset == 0) fprintf(stderr, "false");
            else if (temp->val->offset == 1) fprintf(stderr, "true");
            else assert(0);
        } else if (temp->val->type == userfunc_a) {
            fprintf(stderr, "user function %d", Tuserfunc_table[temp->val->offset].address);
        } else if (temp->val->type == libfunc_a) {
            fprintf(stderr, "library function %s", Tlibfunc_table[temp->val->offset]);
        } else if (temp->val->type == nil_a) {
            fprintf(stderr, "nil");
        }
        temp = temp->next;
    }
}

void objecttotalmembers (node_param* p) {
    node_param* temp = p;
    while (temp) {
        if (temp->val->type >= 1 && temp->val->type <= 3){
            if (Tvar_table[temp->val->offset].type_val == tableVal) {
                table* temp2 = Tvar_table[temp->val->offset].val.tableVal;
                unsigned counter = 0;
                while (temp2) {
                    counter++;
                    temp2 = temp2->next;
                }
                libfunc_result = malloc(sizeof(num));
                libfunc_result->value = counter;
                libfunc_activated = 1;
            } //else assert(0);
        } //else assert(0);
        temp = temp->next;
    }
}

void objectmemberkeys (node_param* temp) {
    while (temp) {
        if (temp->val->type >= 1 && temp->val->type <= 3){
            if (Tvar_table[temp->val->offset].type_val == tableVal) {
                table* temp2 = Tvar_table[temp->val->offset].val.tableVal;
                unsigned counter = 0;
                while (temp2) {
                    if (temp2->index_type == 0) {
                        temp2->type = doubleVal;
                        temp2->val.doubleVal = (double)temp2->index.intVal;
                        temp2->index.intVal = counter;
                    } else if (temp2->index_type == 1) {
                        temp2->val.strVal = temp2->index.strVal;
                        temp2->index_type = 0;
                        temp2->index.intVal = counter;
                        temp2->type = stringVal;
                    } else assert(0);
                    counter++;
                    temp2 = temp2->next;
                }
                libfunc_result_table = Tvar_table[temp->val->offset].val.tableVal;
                
            } else assert(0);
        } else assert(0);
        temp = temp->next;
    }
    libfunc_activated = 2;
}

void objectcopy (node_param* temp) {
    while (temp) {
        if (temp->val->type >= 1 && temp->val->type <= 3){
            if (Tvar_table[temp->val->offset].type_val == tableVal) {
                libfunc_result_table = Tvar_table[temp->val->offset].val.tableVal;
            } else assert(0);
        } else assert(0);
        temp = temp->next;
    }
    libfunc_activated = 2;
}

void strtonum (node_param* temp) {
    char* str;
    unsigned flag = 0;
    while (temp) {
        if (temp->val->type >= 1 && temp->val->type <= 3){
            if (Tvar_table[temp->val->offset].type_val == stringVal) {
                str = Tvar_table[temp->val->offset].val.strVal;
            } else {
                //fprintf(stderr, "WARNING: Variable in strtonum is not of type String\n");
                flag = 1;
            }
        } else if (temp->val->type == 5){
            str = Tstr_table[temp->val->offset];
        } else {
            //fprintf(stderr, "WARNING: Variable in strtonum is not of type String\n");
            flag = 1;
        }
        unsigned int i = 0;
        for (i; i<strlen(str); i++) {
            if ((i == 0 && str[i] == '.') || (i == strlen(str) - 1 && str[i] == '.')) {
                //fprintf(stderr, "WARNING: Number cannot start or end with .\n");
                flag = 1; 
            } 
            if (isdigit(str[i]) == 0 && str[i] != '.') {
                //fprintf(stderr, "WARNING: Provided variable is not a number\n");
                flag = 1;
            }
        }
        if (flag == 0) {
            libfunc_result = malloc(sizeof(num));
            libfunc_result->value = strtod(str, NULL);
        }
        
        temp = temp->next;
    }
    libfunc_activated = 1;
}

void square (node_param* temp) {
    double d;
    unsigned flag = 0;
    while (temp) {
        if (temp->val->type >= 1 && temp->val->type <= 3){
            if (Tvar_table[temp->val->offset].type_val == doubleVal) {
                d = Tvar_table[temp->val->offset].val.doubleVal;
            } else {
                //fprintf(stderr, "WARNING: Variable in sqrt is not of type number\n");
                flag = 1;
            }
        } else if (temp->val->type == 4){
            d = Tnum_table[temp->val->offset].value;
        } else {
            //fprintf(stderr, "WARNING: Variable in sqrt is not of type number\n");
            flag = 1;
        }

        if (flag == 0) {
            if (d >= 0){
                libfunc_result = malloc(sizeof(num));
                libfunc_result->value = sqrt(d);
            } 
        }
        
        temp = temp->next;
    }
    libfunc_activated = 1;
}

void cos_sin (node_param* temp, unsigned type) {
    double d;
    unsigned flag = 0;
    while (temp) {
        if (temp->val->type >= 1 && temp->val->type <= 3){
            if (Tvar_table[temp->val->offset].type_val == doubleVal) {
                d = Tvar_table[temp->val->offset].val.doubleVal;
            } else {
                //fprintf(stderr, "WARNING: Variable in sqrt is not of type number\n");
                flag = 1;
            }
        } else if (temp->val->type == 4){
            d = Tnum_table[temp->val->offset].value;
        } else {
            //fprintf(stderr, "WARNING: Variable in sqrt is not of type number\n");
            flag = 1;
        }

        if (flag == 0) {
            libfunc_result = malloc(sizeof(num));
            if (type == 0) libfunc_result->value = sin(d);
            else if (type == 1) libfunc_result->value = cos(d);
        }
        
        temp = temp->next;
    }
    libfunc_activated = 1;
}

void mytypeof (node_param* temp) {
    while (temp) {
        str_libfunc_result = malloc(sizeof(char*));
        if (temp->val->type >= 1 && temp->val->type <= 3){
            if (Tvar_table[temp->val->offset].type_val == doubleVal) {
                strcpy(str_libfunc_result, "number");
            } else if (Tvar_table[temp->val->offset].type_val == stringVal) {
                strcpy(str_libfunc_result, "string");
            } else if (Tvar_table[temp->val->offset].type_val == boolVal) {
                strcpy(str_libfunc_result, "boolean");
            } else if (Tvar_table[temp->val->offset].type_val == tableVal) {
                strcpy(str_libfunc_result, "table");
            } else if (Tvar_table[temp->val->offset].type_val == nilVal) {
                strcpy(str_libfunc_result, "nil");
            } else assert(0);
        } else if (temp->val->type == 4){
            strcpy(str_libfunc_result, "number");
        } else if (temp->val->type == 5){
            strcpy(str_libfunc_result, "string");
        } else if (temp->val->type == 6){
            strcpy(str_libfunc_result, "boolean");
        } else if (temp->val->type == 7){
            strcpy(str_libfunc_result, "nil");
        } else if (temp->val->type == 8){
            strcpy(str_libfunc_result, "userfunc");
        } else if (temp->val->type == 9){
            strcpy(str_libfunc_result, "libfunc");
        } else assert(0);
        
        temp = temp->next;
    }
    
    libfunc_activated = 3;
}

void totalarguments (node_param* temp) {
    unsigned counter = 0;
    while (temp) {
        counter++;
        temp = temp->next;
    }
    libfunc_result = malloc(sizeof(num));
    libfunc_result->value = counter;
}

void input (node_param* temp) {
    char* str = malloc (sizeof(char*));
    unsigned int i = 0;
    unsigned flag = 0;
    fprintf(stderr, "Input: ");
    scanf("%s", str);
    if (!strcmp(str, "true")) {
        libfunc_activated = 4;
        bool_libfunc_result = 1;
        return;
    } else if (!strcmp(str, "false")) {
        libfunc_activated = 4;
        bool_libfunc_result = 0;
        return;
    } else if (!strcmp(str, "nil")) {
        libfunc_activated = 1;
        return;
    }
    for (i; i<strlen(str); i++) {
        if ((i == 0 && str[i] == '.') || (i == strlen(str) - 1 && str[i] == '.')) {
            libfunc_activated = 3;
            str_libfunc_result = str;
            break;
        } 
        if (isdigit(str[i]) == 0 && str[i] != '.') {
            flag = 1;
            break;
        }
    }
    if (flag == 0) {
        libfunc_activated = 1;
        libfunc_result = malloc(sizeof(num));
        libfunc_result->value = strtod(str, NULL);
    } else if (flag == 1) {
        libfunc_activated = 3;
        str_libfunc_result = str;
    }
}

void read_bin () {
    FILE* file = NULL;
    instruction* temp;
    instruction* new_item;
    unsigned codesize;
    unsigned int i;
    int number;

    fprintf(stderr, "\n\n");

    file = fopen("bin.abc", "rb");
    if (!file) {
        fprintf(stderr, "Error, opening file\n");
        exit(0);
    }

    fread(&number, sizeof(int), 1, file);
    if (number != generate_number("manos eirini thanos....")){
        fprintf(stderr, "Wrong number.\n");
        exit(0);
    }

    fread(&Tstr_offset, sizeof(unsigned), 1, file);
	Tstr_table = (char**) malloc (Tstr_offset * sizeof(char*));
    i = 0;
    while (i < Tstr_offset){
        if (fread(&Tstr_table[i],sizeof(char *), 1, file) != 1){
            print_error();
        }
        i++;
    }

    fread(&Tnum_offset, sizeof(unsigned), 1, file);
	Tnum_table = (num*) malloc (Tnum_offset * sizeof(num));
    i = 0;
  	while (i < Tnum_offset){
		if (fread(&Tnum_table[i].value,sizeof(double), 1, file) != 1){
            print_error();
        }
        i++;
	}

    fread(&Tlibfunc_offset, sizeof(unsigned), 1, file);
	Tlibfunc_table = (char**) malloc (Tlibfunc_offset * sizeof(char*));
    i = 0;
  	while (i < Tlibfunc_offset){
		if (fread(&Tlibfunc_table[i],sizeof(char *), 1, file) != 1){
            print_error();
        }
        i++;
	}

    fread(&Tuserfunc_offset, sizeof(unsigned), 1, file);
	Tuserfunc_table = (userfunc*) malloc (Tuserfunc_offset * sizeof(userfunc));
    i = 0;
  	while (i < Tuserfunc_offset){
		if (fread(&Tuserfunc_table[i].name,sizeof(char *), 1, file) != 1){
            print_error();
        }
        if (fread(&Tuserfunc_table[i].address,sizeof(unsigned), 1, file) != 1){
            print_error();
        }
        if (fread(&Tuserfunc_table[i].local_size,sizeof(unsigned), 1, file) != 1){
            print_error();
        }
        if (fread(&Tuserfunc_table[i].arg,sizeof(formal_args*), 1, file) != 1){
            print_error();
        }
        i++;
	}

    fread(&Tvar_offset, sizeof(unsigned), 1, file);
	Tvar_table = (var*) malloc (Tvar_offset * sizeof(var));
    i = 0;
  	while (i < Tvar_offset){
		if (fread(&Tvar_table[i].name,sizeof(char *), 1, file) != 1){
            print_error();
        }
        if (fread(&Tvar_table[i].type,sizeof(unsigned), 1, file) != 1){
            print_error();
        }
        if (fread(&Tvar_table[i].type_offset,sizeof(unsigned), 1, file) != 1){
            print_error();
        }
        if (fread(&Tvar_table[i].val.doubleVal, sizeof(double), 1, file) != 1){
            print_error();
        }
        if (fread(&Tvar_table[i].val.strVal, sizeof(char*), 1, file) != 1){
            print_error();
        }
        if (fread(&Tvar_table[i].val.boolVal, sizeof(unsigned char), 1, file) != 1){
            print_error();
        }
        if (fread(&Tvar_table[i].val.tableVal, sizeof(table*), 1, file) != 1){
            print_error();
        }
        if (fread(&Tvar_table[i].type_val, sizeof(enum typeVal), 1, file) != 1){
            print_error();
        }
        i++;
	}

    fread(&codesize, sizeof(unsigned), 1, file);
    if (!codesize){
        fprintf(stderr, "No instructions provides!!\n");
        exit(0);
    }

    i = 0;
    while (i < codesize){
        new_item = (instruction*) malloc(sizeof(instruction));
        if (currInstr_avm == instr_total_avm) expand_avm_instr();
        new_item = instr_avm + currInstr_avm++;

        fread(&(new_item->opcode), sizeof(int), 1, file);
        
        new_item->result = (vmarg*) malloc(sizeof(vmarg));
		fread(&(new_item->result->type), sizeof(unsigned), 1, file);
		fread(&(new_item->result->offset), sizeof(unsigned), 1, file);

		new_item->arg1 = (vmarg*) malloc(sizeof(vmarg));
		fread(&(new_item->arg1->type), sizeof(unsigned), 1, file);
		fread(&(new_item->arg1->offset), sizeof(unsigned), 1, file);

        new_item->arg2 = (vmarg*) malloc(sizeof(vmarg));
		fread(&(new_item->arg2->type), sizeof(unsigned), 1, file);
		fread(&(new_item->arg2->offset), sizeof(unsigned), 1, file);

        i++;
    }
    
    start_avm();
}

void print_index (table* temp) {
    if (temp->index_type == 1) fprintf(stderr, "{%s : ", temp->index.strVal);
    else if (temp->index_type == 0) fprintf(stderr, "{%d : ", temp->index.intVal);
    else assert(0);
}

void print_table (table* temp) {
    while (temp) {
        if (temp->type == 0) {
            print_index(temp);
            fprintf(stderr, "%f}, ", temp->val.doubleVal);
        } else if (temp->type == 1) {
            if (strcmp(temp->val.strVal, "nil")) {
                print_index(temp);
                fprintf(stderr, "%s}, ", temp->val.strVal);
            }
        } else if (temp->type == 2) {
            if (temp->val.boolVal == 0) {
                print_index(temp);
                fprintf(stderr, "false}, ");
            } else if (temp->val.boolVal == 1) {
                print_index(temp);
                fprintf(stderr, "true}, ");
            } else assert(0);
        } else if (temp->type == 3) {
            print_index(temp);
            fprintf(stderr, "[ ");
            print_table(temp->val.nested_table);
            fprintf(stderr, "]}, ");
        }
        temp = temp->next;
    }
}

void print_new_data (){
    unsigned int i = 0;
    fprintf(stderr, "\nTable of numbers:\n");
    for (i; i<Tnum_offset; i++){
        fprintf(stderr, "\tIndex: %d \tValue: %f \n", i, Tnum_table[i].value);
    }

    i = 0;
    fprintf(stderr, "Table of strings:\n");
    for (i; i<Tstr_offset; i++){
        fprintf(stderr, "\tIndex: %d \tValue: %s\n", i, Tstr_table[i]);
    }

    i = 0;
    fprintf(stderr, "Table of userfuncs:\n");
    for (i; i<Tuserfunc_offset; i++){
        fprintf(stderr, "\tIndex: %d \tID: %s \tAddress: %d \tLocalsize: %d\n", i, 
        Tuserfunc_table[i].name, 
        Tuserfunc_table[i].address, Tuserfunc_table[i].local_size);
    }

    i = 0;
    fprintf(stderr, "Table of libfuncs:\n");
    for (i; i<Tlibfunc_offset; i++){
        fprintf(stderr, "\tIndex: %d \tValue: %s\n", i, Tlibfunc_table[i]);
    }

    i = 0;
    fprintf(stderr, "Table of vars:\n");
    for (i; i<Tvar_offset; i++){
        fprintf(stderr, "\tIndex: %d \tName: %s \tSymbolType: %d \tType_offset: %d\n", i, 
        Tvar_table[i].name, Tvar_table[i].type, Tvar_table[i].type_offset);
    }
}

void expand_avm_instr () {
    assert(instr_total_avm == currInstr_avm);
    instruction* p = (instruction*) malloc(NEW_SIZE);    
    if (instr_avm){
        memcpy(p,instr_avm,CURR_SIZE);
        free(instr_avm);
    }
    instr_avm = p;
    instr_total_avm += EXPAND_SIZE;
}