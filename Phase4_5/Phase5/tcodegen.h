#include "quad.h"

static const char *vm_operations_to_string[] = {
	"assign", "add", "sub", "mul", "div", "mod", "uminus",        
    "and", "or", "not", "if_eq", "if_neq", "if_leq", "if_greq",  
    "if_less", "if_gr", "call", "pusharg", "ret", "getretval",     
    "funcstart", "funcend", "tablecr", "tableget", "tableset", "jump"
};

enum vmopcode {	
    assign_v, add_v, sub_v, mul_v, divide_v, mod_v, uminus_v,        
    and_v, or_v, not_v, if_eq_v, if_neq_v, if_lesseq_v, if_greatereq_v,  
    if_less_v, if_greater_v, call_v, param_v, ret_v, getretval_v,     
    funcstart_v, funcend_v, tablecreate_v, tablegetelem_v, tablesetelem_v, jump_v, nop_v
};

enum vmarg_t {
    label_a,    global_a,   formal_a,
    local_a,    number_a,   string_a,
    bool_a,     nil_a,      userfunc_a,
    libfunc_a,  retval_a
};

enum typeVal {
    doubleVal, stringVal, boolVal, tableVal, nilVal
};

enum indexVal {
    intIndex, stringIndex
};

typedef struct vmarg { 
    enum vmarg_t type;
    unsigned offset;
} vmarg;

typedef struct instruction {
    enum vmopcode opcode;
    vmarg* result;
    vmarg* arg1;
    vmarg* arg2;
    unsigned srcLine;
    unsigned label;
} instruction;

typedef struct userfunc {
    char* name;
    unsigned address;
    unsigned local_size;
    formal_args* arg;
} userfunc;

typedef struct num {
    double value;
} num;

typedef struct table {
    enum indexVal index_type;
    union {
        unsigned intVal;
        char* strVal;
    } index;
    enum typeVal type;
    union {
        double doubleVal;
        char* strVal;
        unsigned char boolVal;
        struct table* nested_table;
    } val;
    struct table* next;
} table;

typedef struct var {
    char* name;
    enum typeVal type_val;
    union {
        double doubleVal;
        char* strVal;
        unsigned char boolVal;
        table* tableVal;
    } val; 
    enum SymbolType type;
    unsigned type_offset;
} var;

void generate (quad* quad);
void expand_instr ();

vmarg* make_operand (enum iopcode op, expr* e);

void start_generate ();
void print_instructions ();
void print_vmarg (vmarg* e);

void print_num_array ();
void print_str_array ();
void print_usertfunc_array ();
void print_libfunc_array ();
void print_var_array ();

unsigned insert_to_var_table (SymbolTableEntry* sym, unsigned isTable);
unsigned insert_to_num_table (double val);
unsigned insert_to_str_table (char* val);
unsigned insert_to_userfunc_table (char* name, unsigned address, unsigned size, formal_args* arg);
unsigned insert_to_libfunc_table (char* val);

int generate_number (char *s);
unsigned getTotallocals (unsigned start, unsigned finish);
void binary ();
void print_error ();
enum vmopcode vmop_conversion(enum iopcode type);
void print_formal_arguments(SymbolTableEntry* sym);
int check_userfunc_collision (char* name);

vmarg* var_table_cases (vmarg* arg, SymbolTableEntry* sym, unsigned isTable);

//if (temp->arg1->type == 4) {
            //    table* p = Tvar_table[temp->result->offset].val.tableVal;
            //    while (p) {
            //        if (p->index.intVal == new_node->index.intVal) {
            //            p->type = new_node->type;
            //            if (new_node->type == 0) {
            //                p->val.doubleVal = new_node->val.doubleVal;
            //            } else if (new_node->type == 1) {
            //                p->val.strVal = new_node->val.strVal;
            //            } else if (new_node->type == 2) {
            //                p->val.boolVal = new_node->val.boolVal;
            //            }
            //            break;
            //        }
            //        p = p->next;
            //    }
            //}
            //else if (temp->arg1->type == 5) {
            //    table* p = Tvar_table[temp->result->offset].val.tableVal;
            //    while (p) {
            //        if (p->index.strVal){
            //            if (!strcmp(p->index.strVal, new_node->index.strVal)) {
            //                p->type = new_node->type;
            //                if (new_node->type == 0) {
            //                    p->val.doubleVal = new_node->val.doubleVal;
            //                } else if (new_node->type == 1) {
            //                    p->val.strVal = new_node->val.strVal;
            //                } else if (new_node->type == 2) {
            //                    p->val.boolVal = new_node->val.boolVal;
            //                }
            //                break;
            //            }
            //        }
            //        p = p->next;
            //    }
            //    break;
            ////}