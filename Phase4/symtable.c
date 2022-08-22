#include "symtable.h"

unsigned int hash_counter = 0;
unsigned int HASH_TABLE_SIZE = 100;
unsigned int SCOPE_TABLE_SIZE = 10;

Stack *stack;
SymbolTableEntry *hash_table[500];
SymbolTableEntry *scope_link_table[100];
extern int scope;

formal_args* last_usefunc = NULL;
SymbolTableEntry* keep_last_func = NULL;

/* Phase 3 start of declarations */
unsigned int temp_variables_counter = 0;
unsigned programVarOffset = 0;
unsigned functionLocalOffset = 0;
unsigned formalArgOffset = 0;
unsigned scopeSpaceCounter = 1;
/* Phase 3 end of declarations */

void initialize_libraries (){
    int i = 0;
    *hash_table = malloc(HASH_TABLE_SIZE * sizeof(SymbolTableEntry));
    for(i=0; i<HASH_TABLE_SIZE; i++) hash_table[i] = NULL;
    SymbolTableEntry* temp;
    temp = insert_and_space_offset("print", 0, 0, LIBFUNC, -1, -1);
    temp = insert_and_space_offset("input", 0, 0, LIBFUNC, -1, -1);
    temp = insert_and_space_offset("objectmemberkeys", 0, 0, LIBFUNC, -1, -1);
    temp = insert_and_space_offset("objecttotalmembers", 0, 0, LIBFUNC, -1, -1);
    temp = insert_and_space_offset("objectcopy", 0, 0, LIBFUNC, -1, -1);
    temp = insert_and_space_offset("totalarguments", 0, 0, LIBFUNC, -1, -1);
    temp = insert_and_space_offset("argument", 0, 0, LIBFUNC, -1, -1);
    temp = insert_and_space_offset("typeof", 0, 0, LIBFUNC, -1, -1);
    temp = insert_and_space_offset("strtonum", 0, 0, LIBFUNC, -1, -1);
    temp = insert_and_space_offset("sqrt", 0, 0, LIBFUNC, -1, -1);
    temp = insert_and_space_offset("cos", 0, 0, LIBFUNC, -1, -1);
    temp = insert_and_space_offset("sin", 0, 0, LIBFUNC, -1, -1);
}

unsigned int check_library_collisions (char *name){
    if (!strcmp(name, "print")) return 1;
    if (!strcmp(name, "input")) return 1;
    if (!strcmp(name, "objectmemberkeys")) return 1;
    if (!strcmp(name, "objecttotalmembers")) return 1;
    if (!strcmp(name, "objectcopy")) return 1;
    if (!strcmp(name, "totalarguments")) return 1;
    if (!strcmp(name, "argument")) return 1;
    if (!strcmp(name, "typeof")) return 1;
    if (!strcmp(name, "strtonum")) return 1;
    if (!strcmp(name, "sqrt")) return 1;
    if (!strcmp(name, "cos")) return 1;
    if (!strcmp(name, "sin")) return 1;
    return 0;
}

unsigned int hash_function (){
    return (hash_counter % HASH_TABLE_SIZE);
}

void add_to_scope_link (unsigned int scope, SymbolTableEntry *new_element){
    if (scope > SCOPE_TABLE_SIZE) {
        SCOPE_TABLE_SIZE = SCOPE_TABLE_SIZE + SCOPE_TABLE_SIZE;
        *scope_link_table = realloc(*scope_link_table, SCOPE_TABLE_SIZE);
    }
    if (!scope_link_table[scope]) scope_link_table[scope] = new_element;
    else {
        SymbolTableEntry *temp = scope_link_table[scope];
        while (temp->next) temp = temp->next;
        temp->next = new_element;
    }
}

unsigned int lookup_by_specific_scope (char *name, unsigned int scope){
   if (scope_link_table[scope]){
        SymbolTableEntry *temp = scope_link_table[scope];
        while (temp) {
            if (temp->type == USERFUNC || temp->type == LIBFUNC) {
                if (!strcmp(temp->value.funcVal->name, name)) return 1;
            } else {
                if (!strcmp(temp->value.varVal->name, name)) return 1;
            }
            temp = temp->next;
        } 
    }
    return 0;
}

unsigned int lookup_by_specific_type (char *name, enum SymbolType type){
    int i = 0;
    SymbolTableEntry *temp;
    for (i=0; i<SCOPE_TABLE_SIZE; i++){
        if (scope_link_table[i]){
            temp = scope_link_table[i];
            while (temp) {
                if (temp->type == type){
                    if (temp->type == USERFUNC || temp->type == LIBFUNC) {
                        if (!strcmp(temp->value.funcVal->name, name)) return 1;
                    } else {
                        if (!strcmp(temp->value.varVal->name, name)) return 1;
                    }
                }
                temp = temp->next;
            }
        } 
    }
    return 0;
}

int lookup_by_specific_type_and_scope (char *name, enum SymbolType type, unsigned int scope){
    if (scope_link_table[scope]){
        SymbolTableEntry *temp = scope_link_table[scope];
        while (temp) {
            if (temp->type == type){
                if (temp->type == USERFUNC || temp->type == LIBFUNC) {
                    if (!strcmp(temp->value.funcVal->name, name)) return temp->value.funcVal->line;
                } else {
                    if (!strcmp(temp->value.varVal->name, name)) return temp->value.varVal->line;
                }
            }
            temp = temp->next;
        }
    } 
    return -1;
}

SymbolTableEntry* lookup_by_specific_type_and_scope_ret (char *name, enum SymbolType type, unsigned int scope){
    if (scope_link_table[scope]){
        SymbolTableEntry *temp = scope_link_table[scope];
        while (temp) {
            if (temp->type == type){
                if (temp->type == USERFUNC || temp->type == LIBFUNC) {
                    if (!strcmp(temp->value.funcVal->name, name)) return temp;
                } else {
                    if (!strcmp(temp->value.varVal->name, name)) return temp;
                }
            }
            temp = temp->next;
        }
    } 
    return NULL;
}

SymbolTableEntry* lookup_last (char *name, unsigned int scope){
    if (scope_link_table[scope]){
        SymbolTableEntry *temp = scope_link_table[scope];
        while (temp->next) temp = temp->next;
        if (temp->type == USERFUNC || temp->type == LIBFUNC) {
            if (!strcmp(temp->value.funcVal->name, name)) return temp;
        } else {
            if (!strcmp(temp->value.varVal->name, name)) return temp;
        }
    }
    return NULL;
}

void hide (unsigned int scope){
    int i = 0;
    for (i; i< HASH_TABLE_SIZE; i++){
        if (hash_table[i]) {
            SymbolTableEntry *temp = hash_table[i];
            while (temp->next) {
                if (temp->value.funcVal) if (temp->value.funcVal->scope == scope) temp->isActive = false;
                else if (temp->value.varVal->scope == scope) temp->isActive = false;
                temp = temp->next;
            }          
        }
    }

    if (scope_link_table[scope]){
        SymbolTableEntry *temp = scope_link_table[scope];
        while (temp->next) {
            temp->isActive = false;
            temp = temp->next;
        }
    }

}

void enable (unsigned int scope){
    int i = 0;
    for (i; i< HASH_TABLE_SIZE; i++){
        if (hash_table[i]) {
            SymbolTableEntry *temp = hash_table[i];
            while (temp->next) {
                if (temp->value.funcVal) if (temp->value.funcVal->scope == scope) temp->isActive = true;
                else if (temp->value.varVal->scope == scope) temp->isActive = true;
                temp = temp->next;
            }          
        }
    }

    if (scope_link_table[scope]){
        SymbolTableEntry *temp = scope_link_table[scope];
        while (temp->next) {
            temp->isActive = true;
            temp = temp->next;
        }
    }

}

unsigned int get_status (unsigned int scope){
    if (scope_link_table[scope]){
        if (scope_link_table[scope]->isActive == true) return 1;
    }
    return 0;
}

void print_by_scopes (){
    int i = 0;
    SymbolTableEntry *temp;
    fprintf(stderr, "\n");
    for (i=0; i<SCOPE_TABLE_SIZE; i++){
        if (scope_link_table[i]){
            fprintf(stderr, "----------------------------------------------------------------------------------------\n");
            fprintf(stderr, "|                                         SCOPE %d                                      |", i);
            fprintf(stderr, "\n----------------------------------------------------------------------------------------\n");
            temp = scope_link_table[i];
            while (temp) {
                if (temp->type == USERFUNC || temp->type == LIBFUNC) {
                    fprintf(stderr, "%-24s", temp->value.funcVal->name);
                    fprintf(stderr, "%20s", get_enum_type(temp->type));
                    fprintf(stderr, "%20s", "[line ");
                    fprintf(stderr, "%d", temp->value.funcVal->line);
                    fprintf(stderr, "%s", "]");
                    fprintf(stderr, "%15s", "[scope ");
                    fprintf(stderr, "%d", temp->value.funcVal->scope);
                    fprintf(stderr, "%s\n", "]");
                } else {
                    fprintf(stderr, "%-24s", temp->value.varVal->name);
                    fprintf(stderr, "%20s", get_enum_type(temp->type));
                    fprintf(stderr, "%20s", "[line ");
                    fprintf(stderr, "%d", temp->value.varVal->line);
                    fprintf(stderr, "%s", "]");
                    fprintf(stderr, "%15s", "[scope ");
                    fprintf(stderr, "%d", temp->value.varVal->scope);
                    fprintf(stderr, "%s\n", "]");
                }
                temp = temp->next;
            }
            
        }
    }
}

char* get_enum_type (enum SymbolType type){
    if (type == 0) return "[global variable]";
    else if (type == 1) return "[local variable]";
    else if (type == 2) return "[formal argument]";
    else if (type == 3) return "[user function]";
    else if (type == 4) return "[library function]";
    return "";
}

/* Phase 5 */

void attach_formal_usfunc (Variable* var) {
    formal_args* last = last_usefunc;
    formal_args *new_element = malloc(sizeof(formal_args));
    new_element->var = var;
    new_element->next = NULL;
    if (last_usefunc == NULL) {
        last_usefunc = new_element;
        return;
    }
    while (last->next != NULL) last = last->next;
    last->next = new_element;
}

void attach_formals () {
    // ATTACH FORMALS TO LAST USERFUNC
    if (keep_last_func) {
        keep_last_func->value.funcVal->arg = last_usefunc;
        last_usefunc = NULL;
    }
    
}

/* Phase 3 functions */

SymbolTableEntry* insert_and_space_offset (char *name, unsigned int scope, unsigned int line, enum SymbolType type, unsigned space, unsigned offset){  
    if (hash_function() >= HASH_TABLE_SIZE) {
        HASH_TABLE_SIZE = HASH_TABLE_SIZE + HASH_TABLE_SIZE;
        *hash_table = realloc(*hash_table, HASH_TABLE_SIZE);
    }
   
    SymbolTableEntry *new_element = malloc(sizeof(SymbolTableEntry));

    if(type == GLOBAL || type == LOCAL || type == FORMAL){  
        Variable *new_variable = malloc(sizeof(Variable));
        new_variable->name = strdup(name);
        new_variable->scope = scope;
        new_variable->line = line;
        new_element->value.varVal = new_variable;
        new_element->type = var_s;
        if (type == FORMAL) {
            new_element->space = formalarg;
            attach_formal_usfunc(new_variable);
        }
        else new_element->space = programvar;
    } else {
        Function *new_function = malloc(sizeof(Function));
        new_function->name = (strdup(name));
        new_function->scope = scope;
        new_function->line = line;
        
        new_element->value.funcVal = new_function;
        if (type == USERFUNC) {
            new_element->type_t = programfunc_s;
            new_element->space = functionlocal;
        }
        else new_element->type_t = libraryfunc_s;
    }
    new_element->isActive = true;
    new_element->type = type;
    new_element->next = NULL;

    new_element->name = strdup(name);
    
    new_element->line = line;
    new_element->scope = scope;
    //new_element->space = space;
    new_element->offset = offset;
    if (type == USERFUNC) {
        if (keep_last_func) {
            if (strcmp(keep_last_func->name, new_element->name)) {
                keep_last_func->value.funcVal->arg = last_usefunc;
                last_usefunc = NULL;
                keep_last_func = new_element;
            } else {
                fprintf(stderr,"same name \n");
            }
        } else {
            keep_last_func = new_element;
        }
    }
    int index = hash_function();
    if (!hash_table[index]) hash_table[index] = new_element;
    else {
        SymbolTableEntry *temp = hash_table[index];
        while (temp->next) temp = temp->next;  
        temp->next = new_element;
    }
    hash_counter++;
    add_to_scope_link(scope, new_element);
    
    return new_element;
}

SymbolTableEntry* lookup_by_specific_scope_and_return (char *name, unsigned int scope){
    if (scope_link_table[scope]){
        SymbolTableEntry *temp = scope_link_table[scope];
        while (temp) {
            
            if (temp->type == USERFUNC || temp->type == LIBFUNC) {
                if (!strcmp(temp->value.funcVal->name, name)) return temp;
            } else {
                if (!strcmp(temp->value.varVal->name, name)) return temp;
            }
            temp = temp->next;
        } 
    }
    return NULL;
}

SymbolTableEntry* newtemp (){
    SymbolTableEntry* temp;
    char* a = malloc(sizeof(char*));
    char* b = malloc(sizeof(char*));
    char* c = malloc(sizeof(char*));
    strcpy(b,strdup("_t"));
    sprintf(a,"%d",temp_variables_counter++);
    strcpy(c, strcat(b,a));
    temp = lookup_by_specific_scope_and_return(c, scope);
    if (temp) return temp;
    else {
        temp = insert_and_space_offset(c, scope, 0, LOCAL, currscopespace(), currscopeoffset());
        return temp; 
    }
}

void reset_temp_var_scope (){
    temp_variables_counter = 0; 
}

enum scopespace_t currscopespace (){
    if (scopeSpaceCounter) return programvar;
    else if (!(scopeSpaceCounter % 2)) return formalarg;
    else return functionlocal;
}

unsigned currscopeoffset (){
    enum scopespace_t result = currscopespace();
    if (result == programvar) return programVarOffset;
    else if (result == functionlocal) return functionLocalOffset;
    else if (result == formalarg) return formalArgOffset;
    else assert(0);
}

void inccurrscopeoffset (){
    enum scopespace_t result = currscopespace();
    if (result == programvar) ++programVarOffset;
    else if (result == functionlocal) ++functionLocalOffset;
    else if (result == formalarg) ++formalArgOffset;
    else assert(0);
}

void enterscopespace (){
    ++scopeSpaceCounter;
}

void exitscopespace (){
    assert(scopeSpaceCounter > 1); 
    --scopeSpaceCounter;
}

void resetformalargsoffset (){
    formalArgOffset = 0;
}

void resetfunctionlocalsoffset (){
    functionLocalOffset = 0; 
}

void restorecurrscopeoffset (unsigned n) {
    enum scopespace_t result = currscopespace();
    if (result == programvar) programVarOffset = n;
    else if (result == functionlocal) functionLocalOffset = n;
    else if (result == formalarg) formalArgOffset = n;
    else assert(0);
}

void push (unsigned value){
    Stack *new_element = malloc(sizeof(Stack));
    
    new_element->data = value;
    new_element->next = stack;
    stack = new_element;
}

unsigned pop_and_top() {
    unsigned result;
    if(stack != NULL){
        result = stack->data;
        Stack* temp = stack;
        stack = stack->next;
        free(temp);
    }
    return result;
}

unsigned int lookup_last_type (unsigned int scope){
    if (scope_link_table[scope]){
        SymbolTableEntry *temp = scope_link_table[scope];
        while (temp->next) temp = temp->next;
        if (temp->type == USERFUNC || temp->type == LIBFUNC) {
            return 1;
        } else {
            return 2;
        }
    }
    return 0;
}