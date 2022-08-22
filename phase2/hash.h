#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct Variable {
  const char *name;
  unsigned int scope;
  unsigned int line;
} Variable;

typedef struct Function{
  const char *name;
  unsigned int scope;
  unsigned int line;
} Function;

enum SymbolType{GLOBAL,LOCAL,FORMAL,USERFUNC,LIBFUNC};

typedef struct SymbolTableEntry{
  bool isActive;
  union{
    Variable *varVal;
    Function *funcVal;
  } value;
  enum SymbolType type;
  struct SymbolTableEntry *next;
} SymbolTableEntry;

void initialize_libraries ();
unsigned int check_library_collisions (char *name);
unsigned int hash_function ();
void insert (char *name, int scope, int line, enum SymbolType type);
void add_to_scope_link (int scope, SymbolTableEntry *new_element);
unsigned int lookup_by_specific_scope (char *name, int scope);
unsigned int lookup_by_specific_type (char *name, enum SymbolType type);
int lookup_by_specific_type_and_scope (char *name, enum SymbolType type, int scope);
unsigned int lookup_last (char *name, int scope);
void hide (int scope);
void enable (int scope);
unsigned int get_status (int scope);
void print_by_scopes ();
char* get_enum_type (enum SymbolType type);