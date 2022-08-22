#include "hash.h"

int hash_counter = 0;
int HASH_TABLE_SIZE = 100;
int SCOPE_TABLE_SIZE = 10;
int user_function_counter = 0;

SymbolTableEntry *hash_table[100];
SymbolTableEntry *scope_link_table[10];

void initialize_libraries (){
    int i = 0;
    *hash_table = malloc(HASH_TABLE_SIZE * sizeof(SymbolTableEntry));
    for(i=0; i<HASH_TABLE_SIZE; i++) hash_table[i] = NULL;
    
    insert("print", 0, 0, LIBFUNC);
    insert("input", 0, 0, LIBFUNC);
    insert("objectmemberkeys", 0, 0, LIBFUNC);
    insert("objecttotalmembers", 0, 0, LIBFUNC);
    insert("objectcopy", 0, 0, LIBFUNC);
    insert("totalarguments", 0, 0, LIBFUNC);
    insert("argument", 0, 0, LIBFUNC);
    insert("typeof", 0, 0, LIBFUNC);
    insert("strtonum", 0, 0, LIBFUNC);
    insert("sqrt", 0, 0, LIBFUNC);
    insert("cos", 0, 0, LIBFUNC);
    insert("sin", 0, 0, LIBFUNC);
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

void insert (char *name, int scope, int line, enum SymbolType type){  
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
    } else {
        Function *new_function = malloc(sizeof(Function));
        new_function->name = (strdup(name));
        new_function->scope = scope;
        new_function->line = line;
        new_element->value.funcVal = new_function;
    }
    new_element->isActive = true;
    new_element->type = type;
    new_element->next = NULL;

    int index = hash_function();
    if (!hash_table[index]) hash_table[index] = new_element;
    else {
        SymbolTableEntry *temp = hash_table[index];
        while (temp->next) temp = temp->next;  
        temp->next = new_element;
    }
    hash_counter++;
    add_to_scope_link(scope, new_element);
}

void add_to_scope_link (int scope, SymbolTableEntry *new_element){
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

unsigned int lookup_by_specific_scope (char *name, int scope){
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

int lookup_by_specific_type_and_scope (char *name, enum SymbolType type, int scope){
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

unsigned int lookup_last (char *name, int scope){
    if (scope_link_table[scope]){
        SymbolTableEntry *temp = scope_link_table[scope];
        while (temp->next) temp = temp->next;
        if (temp->type == USERFUNC || temp->type == LIBFUNC) {
            if (!strcmp(temp->value.funcVal->name, name)) return 1;
        } else {
            if (!strcmp(temp->value.varVal->name, name)) return 1;
        }
    }
    return 0;
}

void hide (int scope){
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

void enable (int scope){
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

unsigned int get_status (int scope){
    if (scope_link_table[scope]){
        if (scope_link_table[scope]->isActive == true) return 1;
    }
    return 1;
}

void print_by_scopes (){
    int i = 0;
    SymbolTableEntry *temp;
    for (i=0; i<SCOPE_TABLE_SIZE; i++){
        if (scope_link_table[i]){
            fprintf(stderr, "-----------------------------SCOPE %d-----------------------------\n", i);
            temp = scope_link_table[i];
            while (temp) {
                if (temp->type == USERFUNC || temp->type == LIBFUNC) fprintf(stderr, "\"%s\" %s (line %d) (scope %d)\n",temp->value.funcVal->name, get_enum_type(temp->type), temp->value.funcVal->line, temp->value.funcVal->scope);
                else fprintf(stderr, "\"%s\" %s (line %d) (scope %d)\n",temp->value.varVal->name, get_enum_type(temp->type), temp->value.varVal->line, temp->value.varVal->scope);
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