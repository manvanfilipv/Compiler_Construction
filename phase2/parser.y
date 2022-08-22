%{
    #include "hash.h"   
    int yylex(void);
    void yyerror(char* yaccProvidedMessage); 
    void check_term ();
    void check_call (char* lvalue);

    extern int yylineno;
    extern char* yytext;
    extern FILE* yyin;

    unsigned int scope = 0;
    unsigned int dollars_counter = 0;
    unsigned int state = 0;
    unsigned int is_member = 0;
    unsigned int global_case = 0;
    unsigned int right_value_check = 0;
    unsigned int is_call = 0;
%}

%start program

%union { 
    int intVal; 
    double realVal; 
    char * strVal;
}

%token 	<intVal>    INTEGER
%token 	<realVal>   REAL
%token	<strVal>    ID

%token <strVal> STRING
%token <strVal> MULTI_COMMENT
%token <strVal> MULTI_COMMENT_NEVER_CLOSED
%token <strVal> NESTED_INSIDE_MULTI_COMMENT
%token <strVal> STRING_NEVER_CLOSED
%token <strVal> WARNING_STRING
%token <strVal> ASSIGNMENT
%token <strVal> ADD
%token <strVal> SUB
%token <strVal> SHARP
%token <strVal> SLASH
%token <strVal> PERCENT
%token <strVal> EQUAL
%token <strVal> NOT_EQUAL
%token <strVal> PLUS_PLUS
%token <strVal> MINUS_MINUS
%token <strVal> LESS
%token <strVal> GREATER
%token <strVal> GREATER_OR_EQUAL
%token <strVal> LESS_OR_EQUAL
%token <strVal> LEFT_CURLY_BRACKET
%token <strVal> RIGHT_CURLY_BRACKET
%token <strVal> LEFT_SQUARE_BRACKET
%token <strVal> RIGHT_SQUARE_BRACKET
%token <strVal> LEFT_PARENTHESES
%token <strVal> RIGHT_PARENTHESES
%token <strVal> SEMICOLON
%token <strVal> COMMA
%token <strVal> COLON
%token <strVal> DOUBLE_COLON
%token <strVal> DOT
%token <strVal> DOUBLE_DOT
%token <strVal> IF
%token <strVal> ELSE
%token <strVal> WHILE
%token <strVal> FOR
%token <strVal> FUNCTION
%token <strVal> RETURN
%token <strVal> BREAK
%token <strVal> CONTINUE
%token <strVal> AND
%token <strVal> NOT 
%token <strVal> OR
%token <strVal> LOCAL_KEYWORD
%token <strVal> TRUE
%token <strVal> FALSE
%token <strVal> NIL
%token <strVal> LINE_COMMENT

%right ASSIGNMENT
%left AND OR
%nonassoc EQUAL NOT_EQUAL
%nonassoc GREATER GREATER_OR_EQUAL LESS LESS_OR_EQUAL
%left ADD SUB
%left SHARP SLASH PERCENT
%right NOT PLUS_PLUS MINUS_MINUS
%left DOT DOUBLE_DOT
%left LEFT_SQUARE_BRACKET RIGHT_SQUARE_BRACKET
%left LEFT_PARENTHESES RIGHT_PARENTHESES

%%

program:  
    stmtp {fprintf(stderr, "program -> stmtp\n");}
    |;

stmtp:
    stmt {right_value_check = 0;fprintf(stderr, "stmtp -> stmt\n");}
    |stmtp stmt {right_value_check = 0;fprintf(stderr, "stmtp -> stmtp stmt\n");}
    |error ;

stmt:   
    expr SEMICOLON          {fprintf(stderr, "statement -> expr;\n");}
    |ifstmt                 {fprintf(stderr, "statement -> ifstmt\n");}
    |whilestmt              {fprintf(stderr, "statement -> whilestmt\n");}
    |forstmt                {fprintf(stderr, "statement -> forstmt\n");}
    |returnstmt             {fprintf(stderr, "statement -> returnstmt\n");}
    |BREAK SEMICOLON        {
        if (!state) yyerror("Break statement not in a function");
        fprintf(stderr, "statement -> break;\n");
    }
    |CONTINUE SEMICOLON     {
        if (!state) yyerror("Continue statement not in a function");
        fprintf(stderr, "statement -> continue;\n");
    }
    |block                  {fprintf(stderr, "statement -> block;\n");}
    |funcdef                {fprintf(stderr, "statement -> funcdef;\n");}
    |warning                {fprintf(stderr, "statement -> warning;\n");}
    |string_comment_error   {fprintf(stderr, "statement -> string_comment_error;\n");}
    |comment                {fprintf(stderr, "statement -> comment;\n");}
    |SEMICOLON              {fprintf(stderr, "statement -> ;\n");}

expr:
    assignexpr{}                          {fprintf(stderr, "expression -> assignexpr;\n");}
    |expr ADD expr                      {fprintf(stderr, "expr + expr\n");}
    |expr ADD ASSIGNMENT expr                      {fprintf(stderr, "expr += expr\n");}
    |expr SUB expr                      {fprintf(stderr, "expr - expr\n");}
    |expr SHARP expr                    {fprintf(stderr, "expr * expr\n");}
    |expr PERCENT expr                  {fprintf(stderr, "expr %% expr\n");}
    |expr SLASH expr                    {fprintf(stderr, "expr // expr\n");}
    |expr GREATER expr                  {fprintf(stderr, "expr > expr\n");}
    |expr GREATER_OR_EQUAL expr         {fprintf(stderr, "expr >= expr\n");}
    |expr LESS expr                     {fprintf(stderr, "expr < expr\n");}
    |expr LESS_OR_EQUAL expr            {fprintf(stderr, "expr <= expr\n");}
    |expr EQUAL expr                    {fprintf(stderr, "expr == expr\n");}
    |expr NOT_EQUAL expr                {fprintf(stderr, "expr != expr\n");}
    |expr AND expr                      {fprintf(stderr, "expr && expr\n");}
    |expr OR expr                       {fprintf(stderr, "expr || expr\n");};
    |term                               {fprintf(stderr, "expression -> term;\n");};

term:
    LEFT_PARENTHESES expr RIGHT_PARENTHESES {fprintf(stderr, "term -> (expr)\n");}
    |SUB expr                               {fprintf(stderr, "term -> -expr\n");}
    |NOT expr                               {fprintf(stderr, "term -> not expr\n");}
    |PLUS_PLUS lvalue                       {
        check_term();
        fprintf(stderr, "term -> ++lvalue\n");
    }
    |lvalue PLUS_PLUS                       {
        check_term();
        fprintf(stderr, "term -> lvalue++\n");
    }
    |MINUS_MINUS lvalue                     {
        check_term();
        fprintf(stderr, "term -> --lvalue\n");
    }
    |lvalue MINUS_MINUS                     {
        check_term();
        fprintf(stderr, "term -> lvalue--\n");
    }
    |primary                                {fprintf(stderr, "term -> primary\n");};

assignexpr:
    lvalue{
        if (is_member == 0 && global_case == 0){
            if (!check_library_collisions(yylval.strVal)) {
                if (scope == 0){
                    if (lookup_by_specific_type_and_scope(yylval.strVal, GLOBAL, 0) == -1) {    
                        if (lookup_by_specific_type_and_scope(yylval.strVal, USERFUNC, 0) == -1) insert(yylval.strVal, 0, yylineno, GLOBAL);
                        else fprintf(stderr, "\nERROR -> Using ProgramFunc %s as an lvalue: at line %d\n\n", yylval.strVal, yylineno);
                    }
                } else if (scope > 0){
                    int i = 0;
                    int found = -1;
                    if (lookup_by_specific_type_and_scope(yylval.strVal, GLOBAL, 0) != -1) found = 0;
                    for (i; i<=scope; i++){
                        if (lookup_by_specific_type_and_scope(yylval.strVal, LOCAL, i) != -1) found = i;
                        if (lookup_by_specific_type_and_scope(yylval.strVal, FORMAL, i) != -1) found = i;
                    }              
                    if (found == -1) {
                        //if (scope == 1) insert(yylval.strVal, scope, yylineno, GLOBAL); 
                        //else 
                        insert(yylval.strVal, scope, yylineno, LOCAL);                              
                    } else {
                        if (found != scope){
                            if (scope == 1){
                                if (lookup_by_specific_type_and_scope(yylval.strVal, GLOBAL, 0) == -1) fprintf(stderr, "\nERROR -> Cannot access variable %s: at line %d\n\n", yylval.strVal, yylineno);
                            } else fprintf(stderr, "\nERROR -> Cannot access variable %s: at line %d\n\n", yylval.strVal, yylineno);
                        }
                    }
                }
            } else fprintf(stderr, "\nERROR -> Using LibFunc %s as an lvalue: at line %d\n\n", yylval.strVal, yylineno);
        }      
    } ASSIGNMENT expr {fprintf(stderr, "assignexpr -> lvalue = expr\n");};

primary:
    lvalue                                      {fprintf(stderr, "primary -> lvalue\n");}
    |call                                       {fprintf(stderr, "primary -> call\n");}
    |objectdef                                  {fprintf(stderr, "primary -> objectdef\n");}
    |LEFT_PARENTHESES funcdef RIGHT_PARENTHESES {fprintf(stderr, "primary -> (funcdef)\n");}
    |const                                      {fprintf(stderr, "primary -> const\n");};

lvalue:
    ID  {
        right_value_check++;
        if (is_call) check_call(yylval.strVal);
        if (right_value_check == 2){
            check_call(yylval.strVal);
            right_value_check = 0;
        }
        is_member = 0;
        global_case = 0;
        
        fprintf(stderr, "lvalue -> ID\n");
    }
    |LOCAL_KEYWORD ID   {
        right_value_check++;
        if (check_library_collisions(yylval.strVal)) fprintf(stderr, "\nERROR -> Collision with library function: at line %d\n\n", yylineno);
        else {
            while (1){
                if (lookup_by_specific_type_and_scope(yylval.strVal, USERFUNC, scope) != -1){
                    if (get_status(scope)) fprintf(stderr, "\nERROR -> Cannot declare %s with the name of its function: at line %d. \n\n", yylval.strVal, yylineno);
                    else insert(yylval.strVal, scope, yylineno, LOCAL);
                    break;
                } 
                if (lookup_by_specific_type_and_scope(yylval.strVal, FORMAL, scope) == -1) {
                    if (!scope) insert(yylval.strVal, scope, yylineno, GLOBAL);
                    else insert(yylval.strVal, scope, yylineno, LOCAL);
                    break;
                } 
                if (lookup_by_specific_type_and_scope(yylval.strVal, LOCAL, scope) == -1) {
                    if (!scope) insert(yylval.strVal, scope, yylineno, GLOBAL);
                    else insert(yylval.strVal, scope, yylineno, LOCAL);
                    break;
                } else fprintf(stderr, "\nERROR -> Cannot redeclare local %s : at line %d. \n\n", yylval.strVal, yylineno);
                break;
            }
        }
        global_case = 0;
        is_member = 0;
        fprintf(stderr, "lvalue -> LOCAL_KEYWORD ID\n");
    }
    |DOUBLE_COLON ID    {  
        right_value_check++;
        if (!lookup_by_specific_scope(yylval.strVal, 0)) {  
            fprintf(stderr, "\nERROR -> Reference to undefined global variable %s : at line %d. \n\n", yylval.strVal, yylineno);
        }
        global_case = 1;
        is_member = 0;
        fprintf(stderr, "lvalue -> DOUBLE_COLON ID\n");
    }
    |member             {global_case = 0;is_member = 1;fprintf(stderr, "lvalue -> member\n");};

member:
    lvalue DOT ID                                           {fprintf(stderr, "member -> lvalue.id\n");}
    |lvalue{
        if (check_library_collisions(yylval.strVal)) fprintf(stderr, "\nERROR -> Using LibFunc %s as an lvalue: at line %d\n\n", yylval.strVal, yylineno);
    } LEFT_SQUARE_BRACKET expr RIGHT_SQUARE_BRACKET   {fprintf(stderr, "member -> lvalue[expr]\n");}
    |call DOT ID                                            {fprintf(stderr, "member -> call.id\n");}
    |call LEFT_SQUARE_BRACKET expr RIGHT_SQUARE_BRACKET     {fprintf(stderr, "member -> call[expr]\n");};

call:
    call LEFT_PARENTHESES elist RIGHT_PARENTHESES                                        {fprintf(stderr, "call -> call[elist]\n");}
    |lvalue{
        is_call = 1;
        int i = 0;
        int found_formal = -1;
        int found_local = -1;
        int found_userfunc = -1;
        while (1) {
            for (i; i<=scope; i++){
                if (lookup_by_specific_type_and_scope(yylval.strVal, LOCAL, i) != -1) found_local = i;
                if (lookup_by_specific_type_and_scope(yylval.strVal, USERFUNC, i) != -1) found_userfunc = i;
                if (lookup_by_specific_type_and_scope(yylval.strVal, FORMAL, i) != -1) found_formal = i;
            }
            if (found_local >= found_userfunc) {
                if (lookup_by_specific_type_and_scope(yylval.strVal, LIBFUNC, 0) == -1){
                    fprintf(stderr, "\nERROR -> Cannot call %s: at line %d\n\n", yylval.strVal, yylineno);
                }
                break;
            } 
            if (found_formal >= found_userfunc) {
                if (lookup_by_specific_type_and_scope(yylval.strVal, LIBFUNC, 0) == -1){
                    fprintf(stderr, "\nERROR -> Cannot call %s: at line %d\n\n", yylval.strVal, yylineno);
                }
                break;
            } 
            break;
        }
    } callsuffix                                                                   {fprintf(stderr, "call -> lvalue callsuffix\n");}
    |LEFT_PARENTHESES funcdef RIGHT_PARENTHESES LEFT_PARENTHESES elist RIGHT_PARENTHESES {fprintf(stderr, "call -> (funcdef) (elist)\n");};

callsuffix:
    normcall    {fprintf(stderr, "callsuffix -> normcall\n");}
    |methodcall {fprintf(stderr, "callsuffix -> methodcall\n");};

normcall:
    LEFT_PARENTHESES elist RIGHT_PARENTHESES {fprintf(stderr, "normcall -> (elist)\n");}
    |ID LEFT_PARENTHESES expr RIGHT_PARENTHESES {fprintf(stderr, "methodcall -> id(elist)\n");}
    |ID LEFT_PARENTHESES elist COMMA expr RIGHT_PARENTHESES {fprintf(stderr, "methodcall -> id(elist)\n");}
    |ID LEFT_PARENTHESES RIGHT_PARENTHESES {fprintf(stderr, "methodcall -> id()\n");};

methodcall:
    DOUBLE_DOT ID LEFT_PARENTHESES elist RIGHT_PARENTHESES {fprintf(stderr, "methodcall -> ..id(elist)\n");};

elist: 
    expr {fprintf(stderr, "elist -> expr\n");}
    |elist COMMA expr {fprintf(stderr, "elist -> elist, expr\n");}  
    |;

objectdef: 
    LEFT_SQUARE_BRACKET elist RIGHT_SQUARE_BRACKET     {fprintf(stderr, "objectdef -> [elist]\n");}
    |LEFT_SQUARE_BRACKET indexed RIGHT_SQUARE_BRACKET   {fprintf(stderr, "objectdef -> [indexed]\n");};

indexed:
    indexedelem                 {fprintf(stderr, "indexed -> indexedelem\n");}
    |indexed COMMA indexedelem  {fprintf(stderr, "indexed -> indexed, indexedelem\n");};

indexedelem:
    LEFT_CURLY_BRACKET expr COLON expr RIGHT_CURLY_BRACKET {fprintf(stderr, "indexedelem -> {expr:expr}\n");};

block:
    LEFT_CURLY_BRACKET{
            scope++;
            enable(scope);
            state++;
        } RIGHT_CURLY_BRACKET{
            hide(scope);
            scope--;
            state--;
        }                  {fprintf(stderr, "block -> {}\n");}
    |LEFT_CURLY_BRACKET{
        scope++;
        enable(scope);
        state++;
    } stmtp RIGHT_CURLY_BRACKET{
        hide(scope);
        scope--;
        state--;
    }           {fprintf(stderr, "block -> {stmt*}\n");};

funcdef:
    FUNCTION  LEFT_PARENTHESES  { 
        char* a = malloc(sizeof(char*));
        char* b = malloc(sizeof(char*));
        strcat(b,strdup("$"));
        sprintf(a,"%d",dollars_counter);
        dollars_counter++;
        insert(strcat(b,a),scope,yylineno,USERFUNC); 
        ++scope;
    } idlist RIGHT_PARENTHESES{scope--;} block {fprintf(stderr, "funcdef -> FUNCTION(idlist)\n");}
    |FUNCTION ID    {
        if (check_library_collisions(yylval.strVal) && !scope) fprintf(stderr, "\nERROR -> Collision with library function: at line %d\n\n", yylineno);
        else {
            if (!lookup_by_specific_scope(yylval.strVal, scope)) insert(yylval.strVal, scope, yylineno, USERFUNC);
            else {
                int user = lookup_by_specific_type_and_scope(yylval.strVal, USERFUNC, scope);
                int global = lookup_by_specific_type_and_scope(yylval.strVal, GLOBAL, scope);
                int local = lookup_by_specific_type_and_scope(yylval.strVal, LOCAL, scope);
                int formal = lookup_by_specific_type_and_scope(yylval.strVal, FORMAL, scope);

                if (user != -1) fprintf(stderr, "\nERROR in line %d -> Function %s already declared: at line %d\n\n", yylineno, yylval.strVal, user);
                else if (global != -1) fprintf(stderr, "\nERROR in line %d -> Global variable %s already declared: at line %d\n\n", yylineno, yylval.strVal, global); 
                else if (local != -1 ) fprintf(stderr, "\nERROR in line %d -> Local variable %s already declared: at line %d\n\n", yylineno, yylval.strVal, local); 
                else if (formal != -1 ) fprintf(stderr, "\nERROR in line %d -> Formal variable %s already declared: at line %d\n\n", yylineno, yylval.strVal, formal);   
            }       
        }
    } LEFT_PARENTHESES{++scope;} idlist RIGHT_PARENTHESES{scope--;} block {fprintf(stderr, "funcdef -> FUNCTION ID(idlist)\n");};

const:
    INTEGER {fprintf(stderr, "const -> INTEGER\n");}
    |REAL   {fprintf(stderr, "const -> REAL\n");}
    |STRING {fprintf(stderr, "const -> STRING\n");}
    |NIL    {fprintf(stderr, "const -> NIL\n");}
    |TRUE   {fprintf(stderr, "const -> TRUE\n");}
    |FALSE  {fprintf(stderr, "const -> FALSE\n");};

idlist:
    ID             {
        if (check_library_collisions(yylval.strVal)) fprintf(stderr, "\nERROR -> Formal variable %s has the same name with Library function: at line %d\n\n", yylval.strVal, yylineno);
        else insert(yylval.strVal, scope, yylineno, FORMAL);        
        fprintf(stderr, "idlist -> ID\n");
    } idlist_case_commaid
    |;

idlist_case_commaid:
    idlist_case_commaid COMMA ID {
        if (check_library_collisions(yylval.strVal)) fprintf(stderr, "\nERROR -> Formal variable %s has the same name with Library function: at line %d\n\n", yylval.strVal, yylineno);
        else {
            if (!lookup_last(yylval.strVal, scope)) insert(yylval.strVal, scope, yylineno, FORMAL);
            else {
                fprintf(stderr, "\nERROR -> Formal Variable %s already defined: at line %d\n\n", yylval.strVal, yylineno); 
            }    
        }
        fprintf(stderr, "idlist -> idlist,ID\n");
    }
    |;

ifstmt:     
    IF LEFT_PARENTHESES expr RIGHT_PARENTHESES stmt {fprintf(stderr, "ifstmt -> if(expr) stmt\n");}; 
    |IF LEFT_PARENTHESES expr RIGHT_PARENTHESES stmt ELSE stmt {fprintf(stderr, "ifstmt -> if(expr) stmt ELSE stmt\n");};

whilestmt:  
    WHILE LEFT_PARENTHESES expr RIGHT_PARENTHESES stmt {fprintf(stderr, "whilestmt -> WHILE(expr) stmt\n");};

forstmt:    
    FOR LEFT_PARENTHESES elist SEMICOLON expr SEMICOLON elist RIGHT_PARENTHESES{scope--;} stmt {scope++;fprintf(stderr, "forstmt -> for(elist; expr; elist) stmt\n");};

returnstmt: 
    RETURN expr SEMICOLON   {
        if (!state) yyerror("Return statement not in a function");
        fprintf(stderr, "returnstmt -> RETURN expr SEMICOLON\n");
    }
    |RETURN SEMICOLON       {
        if (!state) yyerror("Return statement not in a function");
        fprintf(stderr, "returnstmt -> RETURN SEMICOLON\n");
    };	

comment:    
    LINE_COMMENT                    {fprintf(stderr, "comment -> Single line comment\n");}
    |MULTI_COMMENT                  {fprintf(stderr, "comment -> Multiline comment\n");}
    |NESTED_INSIDE_MULTI_COMMENT    {fprintf(stderr, "comment -> Multiline comment with nested comments\n");};

warning:
    WARNING_STRING                    {fprintf(stderr, "warning -> WARNING_STRING\n");};

string_comment_error:
    MULTI_COMMENT_NEVER_CLOSED      {fprintf(stderr, "string_comment_error -> MULTI_COMMENT_NEVER_CLOSED\n");}
    |STRING_NEVER_CLOSED            {fprintf(stderr, "string_comment_error -> STRING_NEVER_CLOSED\n");}

%%

void check_term () {
    if (!is_member){
        if (lookup_by_specific_type(yylval.strVal, USERFUNC)) fprintf(stderr, "\nERROR -> Using ProgramFunc %s as an lvalue: at line %d\n\n", yylval.strVal, yylineno);
        if (lookup_by_specific_type(yylval.strVal, LIBFUNC)) fprintf(stderr, "\nERROR -> Using LibFunc %s as an lvalue: at line %d\n\n", yylval.strVal, yylineno);
     } else {
        if (lookup_by_specific_type(yytext, USERFUNC)) fprintf(stderr, "\nERROR -> Using ProgramFunc %s as an lvalue: at line %d\n\n", yytext, yylineno);
        if (lookup_by_specific_type(yytext, LIBFUNC)) fprintf(stderr, "\nERROR -> Using LibFunc %s as an lvalue: at line %d\n\n", yytext, yylineno);
     }
}

void yyerror (char* yaccProvidedMessage){
	fprintf(stderr, "\nERROR -> %s: at line %d, before token %s\n\n", yaccProvidedMessage, yylineno, yytext);
}

void check_call (char* lvalue){
    if (!check_library_collisions(lvalue)) {
        if (scope == 0){
            if (lookup_by_specific_type_and_scope(lvalue, GLOBAL, 0) == -1) {    
                if (lookup_by_specific_type_and_scope(lvalue, USERFUNC, 0) == -1) insert(lvalue, 0, yylineno, GLOBAL);                     
            }
        } else if (scope > 0){
            int i = 0;
            int found = -1;
            if (lookup_by_specific_type_and_scope(lvalue, GLOBAL, 0) != -1) found = 0;
            for (i; i<=scope; i++){
                if (lookup_by_specific_type_and_scope(lvalue, LOCAL, i) != -1) found = i;
                if (lookup_by_specific_type_and_scope(lvalue, FORMAL, i) != -1) found = i;
            }              
            if (found == -1) {
                //if (scope == 1) insert(lvalue, scope, yylineno, GLOBAL); 
                //else 
                for (i=0; i<=scope; i++){
                    if (lookup_by_specific_type_and_scope(lvalue, USERFUNC, i) != -1) found = i;
                }  
                if (found == -1) insert(lvalue, scope, yylineno, LOCAL);                              
            } else {
                if (found != scope){
                    if (scope == 1){
                        if (lookup_by_specific_type_and_scope(lvalue, GLOBAL, 0) == -1) fprintf(stderr, "\nERROR -> Cannot access variable %s: at line %d\n\n", yylval.strVal, yylineno);
                    } else fprintf(stderr, "\nERROR -> Cannot access variable %s: at line %d\n\n", lvalue, yylineno);
                }
            }
        }
    }
}

int main(int argc, char** argv){
    if (argc > 1){
        if (!(yyin=fopen(argv[1],"r") )){
            fprintf(stderr,"Cannot read file: %s\n",argv[1]);
            return 1;
        }
    } else yyin=stdin;

    initialize_libraries();
    yyparse();  
    print_by_scopes();

    return 0;   
}