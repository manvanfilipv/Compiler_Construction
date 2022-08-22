#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>

enum SymbolType{GLOBAL ,LOCAL, FORMAL, USERFUNC, LIBFUNC};
/* Phase enum variables start of declaration */
enum scopespace_t {programvar, functionlocal, formalarg};

enum symbol_t {var_s, programfunc_s, libraryfunc_s};
/* Phase enum variables end of declaration */

typedef struct Stack {
  unsigned data;
  struct Stack *next;
} Stack;

typedef struct Variable {
  char *name;
  unsigned int scope;
  unsigned int line;
} Variable;

typedef struct formal_args {
  Variable* var;
  struct formal_args* next;
} formal_args; 

typedef struct Function{
  const char *name;
  unsigned int scope;
  unsigned int line;
  struct formal_args* arg;
} Function;

typedef struct SymbolTableEntry{
  bool isActive;
  union{
    Variable *varVal;
    Function *funcVal;
  } value;  
  /* Phase 3 start of attributes */
  enum symbol_t type_t;
  char* name;
  enum scopespace_t space;
  unsigned offset;
  unsigned scope;
  unsigned line;
  unsigned totalLocals;
  unsigned iaddress;
  /* Phase 3 end of attributes */ 
  enum SymbolType type;
  struct SymbolTableEntry *next;
} SymbolTableEntry;

void initialize_libraries ();
unsigned int check_library_collisions (char *name);
unsigned int hash_function ();
void add_to_scope_link (unsigned int scope, SymbolTableEntry *new_element);
unsigned int lookup_by_specific_scope (char *name, unsigned int scope);
unsigned int lookup_by_specific_type (char *name, enum SymbolType type);
int lookup_by_specific_type_and_scope (char *name, enum SymbolType type, unsigned int scope);
SymbolTableEntry* lookup_last (char *name, unsigned int scope);
void hide (unsigned int scope);
void enable (unsigned int scope);
unsigned int get_status (unsigned int scope);
void print_by_scopes ();
char* get_enum_type (enum SymbolType type);

/* Phase 3 */

SymbolTableEntry* lookup_by_specific_scope_and_return (char *name, unsigned int scope);
SymbolTableEntry* insert_and_space_offset (char *name, unsigned int scope, unsigned int line, enum SymbolType type, unsigned space, unsigned offset);
SymbolTableEntry* newtemp ();
void reset_temp_var_scope ();
void resetformalargsoffset ();
void resetfunctionlocalsoffset ();
enum scopespace_t currscopespace ();
unsigned currscopeoffset ();
void inccurrscopeoffset ();
void enterscopespace ();
void exitscopespace ();
void restorecurrscopeoffset (unsigned n);
void push (unsigned value);
unsigned pop_and_top();
unsigned int lookup_last_type (unsigned int scope);
SymbolTableEntry* lookup_by_specific_type_and_scope_ret (char *name, enum SymbolType type, unsigned int scope);

void attach_formal_usfunc (Variable* var);
void attach_formals();