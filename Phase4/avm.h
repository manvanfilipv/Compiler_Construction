#include "tcodegen.h"

typedef struct node_param {
    struct vmarg* val;
    struct node_param* next;
} node_param;

void read_bin ();
void print_new_data ();
void expand_avm_instr ();
void start_avm ();
unsigned int execute_cycle (instruction* temp, unsigned int counter);
void print_var_table ();

double rm_min (double a);
void perform_arithm_operation (instruction* temp, char* op);
unsigned int perform_bool_operation (instruction* temp, char* op, unsigned label);

void append_param (vmarg* new_data);
void delete_param ();
void append_nested_param (vmarg* new_data);
void delete_nested_param ();
void point_args_to_formals (vmarg* arg1, node_param* p);

void print_param_list (node_param* p);

int findoffset (const char* name);
void print_table (table* temp);
void print_index (table* temp);

void objecttotalmembers (node_param* p);
void objectmemberkeys (node_param* temp);
void objectcopy (node_param* temp);
void strtonum (node_param* temp);
void square (node_param* temp);
void cos_sin (node_param* temp, unsigned type);
void mytypeof (node_param* temp);
void totalarguments (node_param* temp);
void input (node_param* temp);
void reset_libfunc_vars ();