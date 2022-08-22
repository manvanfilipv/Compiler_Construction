#include "symtable.h"

static const char *operation_to_string[] = {
	"assign", "add", "sub", "mul", "div", "mod", "uminus",        
    "and", "or", "not", "if_eq", "if_neq", "if_leq", "if_greq",  
    "if_less", "if_gr", "call", "param", "ret", "getretval",     
    "funcstart", "funcend", "tablecr", "tableget", "tableset", "jump"
};

enum iopcode {
    assign, add, sub, mul, divide, mod, uminus,        
    and, or, not, if_eq, if_neq, if_lesseq, if_greatereq,  
    if_less, if_greater, call, param, ret, getretval,     
    funcstart, funcend, tablecreate, tablegetelem, tablesetelem, jump, temp, nop
};

enum expr_t {
    var_e, tableitem_e, programfunc_e, libraryfunc_e,
    arithexpr_e, boolexpr_e, assignexpr_e, newtable_e,
    constnum_e, constbool_e, conststring_e, nil_e
};

typedef struct expr {
    enum expr_t type;
    SymbolTableEntry* sym;
    struct expr* index;
    double numConst;
    char* strConst;
    unsigned char boolConst;
    struct expr* next;

    struct Node* trueList;
    struct Node* falseList;
} expr;

typedef struct quad {
    enum iopcode op;
    expr* result;
    expr* arg1;
    expr* arg2;
    int label;
    unsigned line;
} quad;

typedef struct Node {
    enum iopcode op;
    enum iopcode isNot;
    unsigned value;
    struct Node* next;
} Node;

typedef struct node_brcont {
    unsigned value;
    struct node_brcont* next;
    struct node_brcont* previous;
} node_brcont;

typedef struct stmt_t {
    struct node_brcont* breakList;
    struct node_brcont* contList;
} stmt_t;

typedef struct call_c {
    expr* elist;
    unsigned char method;
    char* name;
} call_c;

expr* lvalue_expr (SymbolTableEntry* sym);
expr* member_item (expr* lvalue, char* name, unsigned line);
expr* emit_iftableitem (expr* e, unsigned line);
expr* newexpr (enum expr_t type);
expr* newexpr_constnum (double i);
expr* newexpr_constbool (unsigned char i);
expr* newexpr_conststring (char* i);
expr* make_call (expr* lv, expr* reversed_elist, unsigned line);

void print_quads ();
void print_expr_type (expr* e);
void compile_time_expression_error (expr* e);
void patchlabel (unsigned quadNo, unsigned label);
void expand();
void emit (enum iopcode op, expr* arg1, expr* arg2, expr* result, int label, unsigned line);

unsigned nextquad ();
unsigned get_quad_line (unsigned index);

expr* get_last_expr_element (expr* expr1);
Node* append (Node* head_ref, int new_data, enum iopcode op);
Node* merge (Node* l1, Node* l2);
Node* checknextor (Node* l1) ;
Node* checknextor_returnprev (Node* l1);

Node* checknextand (Node* l1);
void print (Node* l1);
void clear_labels (expr* expr1);
void fix_not (expr* expr1, enum iopcode op);
void patch_orphan_ifelsejumps (unsigned label);
void patch (Node* l1, Node* l2, unsigned falselabel, unsigned truelabel);
unsigned fix_compare (expr* expr1, enum iopcode op);

stmt_t* make_stmt ();
node_brcont* append_brcont (node_brcont* head_ref, int new_data);
node_brcont* merge_brcont (node_brcont* l1, node_brcont* l2);
void print_brcont (node_brcont* l1);
void patch_brcont (node_brcont* l1, unsigned index, unsigned loopcounter);
void patch_brcont_reversed (node_brcont* l1, unsigned index, unsigned loopcounter);
void patchlabel_difzero (unsigned quadNo, unsigned label);
unsigned size_brcont (node_brcont* l1);
unsigned check_orphan_jump_brcont (unsigned index);
void patch_brcont_last (node_brcont* l1, unsigned index);
void patch_brcont_last_reversed (node_brcont* l1, unsigned index);
unsigned checkfalse_next_or (Node* l1, unsigned counter);

void fix_temp (Node* l1);
void clear_assign_labels ();
unsigned check_op (unsigned index);