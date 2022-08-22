%{
    #include "avm.h"   
    int yylex(void);
    void yyerror(char* yaccProvidedMessage); 

    void check_term ();
    expr* arithmetic_expr(expr* t, expr* t1, int t2, expr* t3);
    expr* compare_expr(expr* t, expr* t1, int t2, expr* t3);

    extern int yylineno;
    extern char* yytext;
    extern FILE* yyin;

    unsigned int scope = 0;
    unsigned int dollars_counter = 0;
    unsigned int is_member = 0;

    /* Checking for error continue, break and return */
    unsigned int continue_state = 0;
    unsigned int break_state = 0;
    unsigned int return_state = 0;

    unsigned int for_label = 0;
    unsigned int for_label_start = 0;
    unsigned int for_label_help = 0;

    int break_index = -1;
    int continue_index = -1;
    unsigned int loopcounter = 0;

    unsigned int whilelabel_true = 0;
    unsigned int iflabel_true = 0;
    unsigned int assignlabel_true = 0;
    unsigned int exprsemicolon_true = 0;
    unsigned int forlabel_true = 0;
    unsigned int returnlabel = 0;
    SymbolTableEntry* sym_temp;
%}

%start program

%union { 
    unsigned unsVal;
    int intVal; 
    double realVal; 
    char* strVal;
    struct expr* exprVal;
    struct SymbolTableEntry* symbVal;
    struct call_c* callVal;
    struct stmt_t* stmtVal;
}

%type <exprVal> lvalue member primary assignexpr call term ifprefix whilecond
%type <exprVal> objectdef const expr elist indexed indexedelem clindexedelem
%type <symbVal> funcdef funcprefix
%type <callVal> callsuffix normcall methodcall
%type <unsVal>  elseprefix whilestart M funcbody
%type <strVal>  funcname
%type <stmtVal> stmt forstmt whilestmt stmtp block ifstmt returnstmt

%token <intVal>  INTEGER 
%token <realVal> REAL
%token <strVal>  ID
%token <strVal>  STRING

%token ASSIGNMENT
%token ADD SUB SHARP SLASH PERCENT
%token EQUAL NOT_EQUAL PLUS_PLUS MINUS_MINUS LESS GREATER GREATER_OR_EQUAL LESS_OR_EQUAL
%token AND OR

%token LEFT_CURLY_BRACKET RIGHT_CURLY_BRACKET LEFT_SQUARE_BRACKET 
%token RIGHT_SQUARE_BRACKET LEFT_PARENTHESES RIGHT_PARENTHESES
%token SEMICOLON COMMA COLON DOUBLE_COLON DOT DOUBLE_DOT
%token IF ELSE WHILE FOR FUNCTION RETURN
%token <stmtVal> BREAK CONTINUE
%token NOT LOCAL_KEYWORD TRUE FALSE NIL

%right ASSIGNMENT
%left OR
%left AND
%nonassoc EQUAL NOT_EQUAL
%nonassoc GREATER GREATER_OR_EQUAL LESS LESS_OR_EQUAL
%left ADD SUB  
%left SHARP SLASH PERCENT
%right NOT PLUS_PLUS MINUS_MINUS
%left DOT DOUBLE_DOT
%left LEFT_PARENTHESES RIGHT_PARENTHESES
%left LEFT_SQUARE_BRACKET RIGHT_SQUARE_BRACKET

%%

program:  
    stmtp {
        clear_assign_labels();
        //fprintf(stderr, "program -> stmtp\n");
    };

stmtp:
    stmt {
        if ($1){
            if (size_brcont($1->contList) == 1){
                patchlabel_difzero(continue_index, $1->contList->value);
            } else {
                patch_brcont_reversed($1->contList, continue_index, loopcounter);
            }
            
            if (size_brcont($1->breakList) == 1){
                patchlabel_difzero(break_index, $1->breakList->value + 1);
            } else {
                patch_brcont($1->breakList, break_index, loopcounter);
            }

            // case of some break and cont jumps not being patched
            if (check_orphan_jump_brcont(break_index) == 1) {
                patch_brcont_last($1->breakList, break_index);
            }
            if (check_orphan_jump_brcont(continue_index) == 1) {
                patch_brcont_last_reversed($1->contList, continue_index);
            }
            free($1);
        }
        
        reset_temp_var_scope();
        //fprintf(stderr, "stmtp -> stmt\n");
    }
    |stmtp stmt {
        
        if ($2){
            if (size_brcont($2->contList) == 1){
                patchlabel_difzero(continue_index, $2->contList->value);
            } else {
                patch_brcont_reversed($2->contList, continue_index, loopcounter);
            }
            if (size_brcont($2->breakList) == 1){
                patchlabel_difzero(break_index, $2->breakList->value + 1);
            } else {
                patch_brcont($2->breakList, break_index, loopcounter);
            }
            
            // case of some break and cont jumps not being patched
            if (check_orphan_jump_brcont(break_index) == 1) {
                patch_brcont_last($2->breakList, break_index);
            }
            if (check_orphan_jump_brcont(continue_index) == 1) {
                patch_brcont_last_reversed($2->contList, continue_index);
            }
            free($2);
        }
        reset_temp_var_scope();
        //fprintf(stderr, "stmtp -> stmtp stmt\n");
    }
    ;

stmt:   
    expr SEMICOLON          {
        if ($1->type == 5){
            emit(assign, newexpr_constbool(1), NULL, $1, 0, yylineno);
            emit(jump, NULL, NULL, NULL, nextquad() + 3, yylineno);
            emit(assign, newexpr_constbool(0), NULL, $1, 0, yylineno);
            patch($1->trueList, $1->falseList, nextquad(), nextquad() - 2);
        } else {
            exprsemicolon_true = nextquad() + 1;
            patch($1->trueList, $1->falseList, nextquad() + 1, exprsemicolon_true);
        }
        
        $$ = make_stmt();
        //fprintf(stderr, "statement -> expr\n");
    }
    |ifstmt                 {
        $$ = $1;
        //fprintf(stderr, "statement -> ifstmt\n");
    }
    |whilestmt              {
        $$ = $1;
        //fprintf(stderr, "statement -> whilestmt\n");
    }
    |forstmt                {
        $$ = $1;
        //fprintf(stderr, "statement -> forstmt\n");
    }
    |returnstmt             {
        $$ = make_stmt();
        //fprintf(stderr, "statement -> returnstmt\n");
    }
    |BREAK SEMICOLON        {
        $1 = make_stmt();
        break_index = nextquad();

        if (!break_state) {
            yyerror("Break statement not in a function");
            exit(0);
        } else {
            emit(jump, NULL, NULL, NULL, 0, yylineno);
        }
        $$ = $1;
        //fprintf(stderr, "statement -> break;\n");
    }
    |CONTINUE SEMICOLON     {
        $1 = make_stmt();
        continue_index = nextquad();
        if (!continue_state) {
            yyerror("Continue statement not in a function");
            exit(0);
        } else {
            emit(jump, NULL, NULL, NULL, 0, yylineno);
        }
        $$ = $1;
        //fprintf(stderr, "statement -> continue;\n");
    }
    |block                  {
        $$ = make_stmt();
        //fprintf(stderr, "statement -> block;\n");
    }
    |funcdef                {
        $$ = make_stmt();
        //fprintf(stderr, "statement -> funcdef;\n");
    }
    |SEMICOLON              {
        $$ = make_stmt();
        //fprintf(stderr, "statement -> ;\n");
    };

expr:
    assignexpr {
        $$ = $1;
        //fprintf(stderr, "expression -> assignexpr\n");
    }
    |expr ADD expr {
        $$ = arithmetic_expr($$, $1, add, $3);
        //fprintf(stderr, "expression -> expr + expr\n");
    }
    |expr SUB expr {
        $$ = arithmetic_expr($$, $1, sub, $3);
        //fprintf(stderr, "expression -> expr - expr\n");
    }
    |expr SHARP expr {
        $$ = arithmetic_expr($$, $1, mul, $3);
        //fprintf(stderr, "expression -> expr * expr\n");
    }
    |expr PERCENT expr {
        $$ = arithmetic_expr($$, $1, mod, $3);
        //fprintf(stderr, "expression -> expr %% expr\n");
    }
    |expr SLASH expr {
        $$ = arithmetic_expr($$, $1, divide, $3);
        //fprintf(stderr, "expression -> expr // expr\n");
    }
    |expr GREATER {
        $1 = get_last_expr_element($1);
    } expr {
        $$ = compare_expr($$, $1, if_greater, $4);
        //fprintf(stderr, "expression -> expr > expr\n");
    }
    |expr GREATER_OR_EQUAL{
        $1 = get_last_expr_element($1);
    } expr {
        $$ = compare_expr($$, $1, if_greatereq, $4);
        //fprintf(stderr, "expression -> expr >= expr\n");
    }
    |expr LESS {
        $1 = get_last_expr_element($1);
    } expr {
        $$ = compare_expr($$, $1, if_less, $4);
        //fprintf(stderr, "expression -> expr < expr\n");
    }
    |expr LESS_OR_EQUAL {
        $1 = get_last_expr_element($1);
    } expr {
        $$ = compare_expr($$, $1, if_lesseq, $4);
        //fprintf(stderr, "expression -> expr <= expr\n");
    }
    |expr EQUAL {
        $1 = get_last_expr_element($1);
    } expr {
        $$ = compare_expr($$, $1, if_eq, $4);
        //fprintf(stderr, "expression -> expr == expr\n");
    }
    |expr NOT_EQUAL {
        $1 = get_last_expr_element($1);
    } expr {
        $$ = compare_expr($$, $1, if_neq, $4);
        //fprintf(stderr, "expression -> expr != expr\n");
    }
    |expr OR M {
        fix_not($1, or);
        if (fix_compare($1, or) == 0 && $1->type != boolexpr_e){
            $1->trueList = append($1->trueList, $3, or);
            $1->falseList = append($1->falseList, $3+1, or);
            emit(if_eq, $1, newexpr_constbool(1), NULL, 0, yylineno);
            emit(jump, NULL, NULL, NULL, 0, yylineno);
        }
    } expr {
        $$ = newexpr(boolexpr_e);
        $$->sym = newtemp();
        fix_not($5, or);
        if (fix_compare($5, or) == 0 && $5->type != boolexpr_e){
            $1->trueList = append($1->trueList, nextquad(), or);
            $1->falseList = append($1->falseList, nextquad()+1, or);
            emit(if_eq, $5, newexpr_constbool(1), NULL, 0, yylineno);
            emit(jump, NULL, NULL, NULL, 0, yylineno);
        }
        $$->trueList = merge($1->trueList, $5->trueList);
        $$->falseList = merge($1->falseList, $5->falseList);
        //fprintf(stderr, "expression -> expr || expr\n");
    }
    |expr AND M {
        fix_not($1, and);
        if (fix_compare($1, and) == 0 && $1->type != boolexpr_e){
            $1->trueList = append($1->trueList, $3, and);
            $1->falseList = append($1->falseList, $3+1, and);
            emit(if_eq, $1, newexpr_constbool(1), NULL, 0, yylineno);
            emit(jump, NULL, NULL, NULL, 0, yylineno);
        }
    } expr {
        $$ = newexpr(boolexpr_e);
        $$->sym = newtemp();
        fix_not($5, and);
        if (fix_compare($5, and) == 0 && $5->type != boolexpr_e){
            $1->trueList = append($1->trueList, nextquad(), and);
            $1->falseList = append($1->falseList, nextquad()+1, and);
            emit(if_eq, $5, newexpr_constbool(1), NULL, 0, yylineno);
            emit(jump, NULL, NULL, NULL, 0, yylineno);
        }
        $$->trueList = merge($1->trueList, $5->trueList);
        $$->falseList = merge($1->falseList, $5->falseList);
        //fprintf(stderr, "expression -> expr && expr\n");
    }  
    |term {
        $$ = $1;
        //fprintf(stderr, "expression -> term\n");
    };     

term:
    LEFT_PARENTHESES expr RIGHT_PARENTHESES {
        $$ = $2;
        //fprintf(stderr, "term -> (expr)\n");
    }
    |SUB expr                               {
        compile_time_expression_error($2);
        expr* t = newexpr(constnum_e);
        t->numConst = -1;
        $$ = newexpr(arithexpr_e);
        $$->sym = newtemp();
        emit(mul, $2, t, $$, 0, yylineno);
        //fprintf(stderr, "term -> -expr\n");
    }
    |NOT expr   {
        $$ = newexpr(boolexpr_e);
        $$->sym = newtemp();
        if ($2->type != boolexpr_e) {
            $2->trueList = append($2->trueList, nextquad(), not);
            $2->falseList = append($2->falseList, nextquad()+1, not);
            emit(if_eq, $2, newexpr_constbool(1), NULL, 0, yylineno);
            emit(jump, NULL, NULL, NULL, 0, yylineno);
        }
        $$->trueList = $2->trueList;
        $$->falseList = $2->falseList;
        //fprintf(stderr, "term -> not expr\n");
    }
    |PLUS_PLUS lvalue                       {
        check_term();
        compile_time_expression_error($2);
        if ($2->type == tableitem_e){
            $$ = emit_iftableitem($2, yylineno);
            emit(add, $$, newexpr_constnum(1), $$, 0, yylineno);
            emit(tablesetelem, $2, $2->index, $$, 0, yylineno);
        } else {
            emit(add, $2, newexpr_constnum(1), $2, 0, yylineno);
            $$ = newexpr(arithexpr_e);
            $$->sym = newtemp();
            emit(assign, NULL, $2, $$, 0, yylineno);      
        }
        //fprintf(stderr, "term -> ++lvalue\n");
    }
    |lvalue PLUS_PLUS                       {
        check_term();
        compile_time_expression_error($1);
        $$ = newexpr(arithexpr_e);
        $$->sym = newtemp();
        if ($1->type == tableitem_e){
            expr* val = emit_iftableitem($1, yylineno);
            emit(assign, NULL, val, $$, 0, yylineno);
            emit(add, val, newexpr_constnum(1), val, 0, yylineno);
            emit(tablesetelem, $1, $1->index, val, 0, yylineno);
        } else {
            emit(assign, NULL, $1, $$, 0, yylineno);
            emit(add, $1, newexpr_constnum(1), $1, 0, yylineno);
        }
        //fprintf(stderr, "term -> lvalue++\n");
    }
    |MINUS_MINUS lvalue                     {
        check_term();
        compile_time_expression_error($2);
        if ($2->type == tableitem_e){
            $$ = emit_iftableitem($2, yylineno);
            emit(sub, $$, newexpr_constnum(1), $$, 0, yylineno);
            emit(tablesetelem, $2, $2->index, $$, 0, yylineno);
        } else {
            emit(sub, $2, newexpr_constnum(1), $2, 0, yylineno);
            $$ = newexpr(arithexpr_e);
            $$->sym = newtemp();
            emit(assign, NULL, $2, $$, 0, yylineno);      
        }
        //fprintf(stderr, "term -> --lvalue\n");
    }
    |lvalue MINUS_MINUS                     {
        check_term();
        compile_time_expression_error($1);
        $$ = newexpr(arithexpr_e);
        $$->sym = newtemp();
        if ($1->type == tableitem_e){
            expr* val = emit_iftableitem($1, yylineno);
            emit(assign, NULL, val, $$, 0, yylineno);
            emit(sub, val, newexpr_constnum(1), val, 0, yylineno);
            emit(tablesetelem, $1, $1->index, val, 0, yylineno);
        } else {
            emit(assign, NULL, $1, $$, 0, yylineno);
            emit(sub, $1, newexpr_constnum(1), $1, 0, yylineno);
        }
        //fprintf(stderr, "term -> lvalue--\n");
    }
    |primary {
        $$ = $1;
        //fprintf(stderr, "term -> primary\n");
    };

assignexpr:
    lvalue ASSIGNMENT expr {
        if ($1->type == tableitem_e){
            emit(tablesetelem, $3, $1->index, $1, 0, yylineno);
            //fprintf(stderr,"HEREEEEEEEEEEEEEEEEE");
            $$ = emit_iftableitem($1, yylineno);
            $$->type = assignexpr_e;
        } else {
            $$ = newexpr(assignexpr_e);
            /* $$->sym = newtemp(); */ $$->sym = $1->sym;
            if ($3->type == 5){ // if is boolexpr_e
                emit(assign, NULL, newexpr_constbool(1), $3, 0, yylineno);
                assignlabel_true = nextquad();
                emit(jump, NULL, NULL, NULL, nextquad() + 3, yylineno);
                emit(assign, NULL, newexpr_constbool(0), $3, 0, yylineno);
                fix_temp($3->trueList);
                fix_temp($3->falseList);
                patch($3->trueList, $3->falseList, nextquad(), assignlabel_true);
                emit(assign, NULL, $3, $1, 0, yylineno);
            } else {
                emit(assign, NULL, $3, $1, 0, yylineno);
                
            }
              
        }
        //fprintf(stderr, "assignexpr -> lvalue = expr\n");
    };

primary:
    lvalue                                      {
        $$ = emit_iftableitem($1, yylineno);
        //fprintf(stderr, "primary -> lvalue\n");
    }
    |call  {
        //fprintf(stderr, "primary -> call\n");
    }
    |objectdef {
        $$ = $1;
        //fprintf(stderr, "primary -> objectdef\n");
    }
    |LEFT_PARENTHESES funcdef RIGHT_PARENTHESES {
        $$ = newexpr(programfunc_e);
        $$->sym = $2;
        //fprintf(stderr, "primary -> (funcdef)\n");
    }
    |const {
        $$ = $1; 
        //fprintf(stderr, "primary -> const\n");
    };

lvalue:
    ID  {
        sym_temp = lookup_by_specific_scope_and_return($1, scope);
        if (!sym_temp){
            if (scope == 0){
                sym_temp = insert_and_space_offset($1, 0, yylineno, GLOBAL, currscopespace(), currscopeoffset());
                inccurrscopeoffset();
            } else {
                sym_temp = insert_and_space_offset($1, scope, yylineno, LOCAL, currscopespace(), currscopeoffset());
                inccurrscopeoffset();
            }
            
        }
        $$ = lvalue_expr(sym_temp);
        is_member = 0;
        //fprintf(stderr, "lvalue -> ID\n");
    }
    |LOCAL_KEYWORD ID {
        if (!check_library_collisions($2)){
            sym_temp = lookup_by_specific_scope_and_return($2, 0);
            if (!sym_temp){
                sym_temp = insert_and_space_offset($2, scope, yylineno, LOCAL, currscopespace(), currscopeoffset());
                inccurrscopeoffset();
            }
        } else {
            sym_temp = lookup_by_specific_scope_and_return($2, 0);
            //fprintf(stderr, "\nERROR -> collision  with LIBFUNC at line %d\n\n", yylineno);         
        }
        $$ = lvalue_expr(sym_temp);
        is_member = 0;
        //fprintf(stderr, "lvalue -> LOCAL_KEYWORD ID\n");
    }
    |DOUBLE_COLON ID {
        if (!lookup_by_specific_scope($2, 0)){
            $$ = newexpr_conststring($2);
            //fprintf(stderr, "\nERROR -> undeclared variable or function %s at line %d\n\n", $2, yylineno);
        } else {
            sym_temp = lookup_by_specific_scope_and_return($2, scope);
            if (!sym_temp) {
                sym_temp = insert_and_space_offset($2, scope, yylineno, GLOBAL, currscopespace(), currscopeoffset());
            }
            $$ = lvalue_expr(sym_temp);
        }
        
        is_member = 0;
        //fprintf(stderr, "lvalue -> DOUBLE_COLON ID\n");
    }
    |member {
        $$ = $1;
        is_member = 1;
        //fprintf(stderr, "lvalue -> member\n");
    };

member:
    lvalue DOT ID {
        $$ = member_item($1, yylval.strVal, yylineno);
        //fprintf(stderr, "member -> lvalue.id\n");
    }
    |lvalue LEFT_SQUARE_BRACKET expr RIGHT_SQUARE_BRACKET   {
        $1 = emit_iftableitem($1, yylineno);
        $$ = newexpr(tableitem_e);
        $$->sym = $1->sym;
        $$->index = $3;
        //fprintf(stderr, "member -> lvalue[expr]\n");
    }
    |call DOT ID                                            {
        //fprintf(stderr, "member -> call.id\n");
    }
    |call LEFT_SQUARE_BRACKET expr RIGHT_SQUARE_BRACKET     {
        //fprintf(stderr, "member -> call[expr]\n");
    };

call:
    call LEFT_PARENTHESES elist RIGHT_PARENTHESES {
        $$ = make_call($1, $3, yylineno);
        //fprintf(stderr, "call -> call[elist]\n");
    }
    |lvalue callsuffix {
        $1 = emit_iftableitem($1, yylineno);
        if ($2->method){
            expr *t = malloc(sizeof(struct expr));
            expr *last = malloc(sizeof(struct expr));
            t = $1;
            $1 = emit_iftableitem(member_item(t, $2->name, yylineno), yylineno);
            t->next = $2->elist;
            $2->elist = t;
        }
        $$ = make_call($1, $2->elist, yylineno);
        //fprintf(stderr, "call -> lvalue callsuffix\n");
    }
    |LEFT_PARENTHESES funcdef RIGHT_PARENTHESES LEFT_PARENTHESES elist RIGHT_PARENTHESES {
        expr* func = newexpr(programfunc_e);
        func->sym = $2;
        $$ = make_call(func, $5, yylineno);
        //fprintf(stderr, "call -> (funcdef) (elist)\n");
    };

callsuffix:
    normcall    {
        $$ = $1;
        //fprintf(stderr, "callsuffix -> normcall\n");
    }
    |methodcall {
        $$ = $1;
        //fprintf(stderr, "callsuffix -> methodcall\n");
    };

normcall:
    LEFT_PARENTHESES elist RIGHT_PARENTHESES {
        $$ = malloc(sizeof(struct call_c));
        $$->elist = $2;
        $$->method = 0;
        $$->name = NULL;
        //fprintf(stderr, "normcall -> (elist)\n");
    }
    |ID LEFT_PARENTHESES expr RIGHT_PARENTHESES {
        //fprintf(stderr, "normcall -> id(elist)\n");
    }
    |ID LEFT_PARENTHESES elist COMMA expr RIGHT_PARENTHESES {
        //fprintf(stderr, "normcall -> id(elist)\n");
    }
    |ID LEFT_PARENTHESES RIGHT_PARENTHESES {
        //fprintf(stderr, "normcall -> id()\n");
    };

methodcall:
    DOUBLE_DOT ID LEFT_PARENTHESES elist RIGHT_PARENTHESES {
        $$ = malloc(sizeof(struct call_c));
        $$->elist = $4;
        $$->method = 1;
        $$->name = $2;
        //fprintf(stderr, "methodcall -> ..id(elist)\n");
    };

elist: 
    expr {
        $1->next= NULL;
        $$ = $1;
        clear_labels($1);
        //fprintf(stderr, "elist -> expr\n");
    }
    |elist COMMA expr {
        expr* last = $1;
        while(last->next != NULL) last = last->next;
        last->next = $3;
        $$ = $1;
        clear_labels($3);
        //fprintf(stderr, "elist -> elist, expr\n");
    }  
    |{
        $$=NULL;
    };

objectdef: 
    LEFT_SQUARE_BRACKET elist RIGHT_SQUARE_BRACKET     {
        expr* t = newexpr(newtable_e);
        t->sym = newtemp();
        emit(tablecreate, NULL, NULL, t, 0, yylineno);
        if ($2 != NULL){
            expr* temp = $2;
            int i = 0;
            while (temp){
                emit(tablesetelem, newexpr_constnum(i++), temp, t, 0, yylineno);
                temp = temp->next;
            }
            t->index = $2;
        }
        $$ = t;
        //fprintf(stderr, "objectdef -> [elist]\n");
    }
    |LEFT_SQUARE_BRACKET indexed RIGHT_SQUARE_BRACKET   {
        expr* t = newexpr(newtable_e);
        t->sym = newtemp();
        emit(tablecreate, NULL, NULL, t, 0, yylineno);
        expr* temp = $2;
        if (temp) t->index = $2;
        while (temp){
            emit(tablesetelem, temp, temp->index, t, 0, yylineno);
            temp = temp->next;
        }
        $$ = t;
        //fprintf(stderr, "objectdef -> [indexed]\n");
    };

indexed:
    indexedelem clindexedelem {
        $$ = $1;
        $$->next = $2;
        //fprintf(stderr,"indexed -> indexedelem clindexedelem\n");
    };

clindexedelem	: COMMA indexedelem clindexedelem	{
        $$ = $2;    
        $$->next = $3;  
        //fprintf(stderr,"clindexedelem -> , indexedelem clindexedelem\n");
    }    
    |{
        $$ = NULL;
        //fprintf(stderr,"clindexedelem -> \n");
    };

indexedelem:
    LEFT_CURLY_BRACKET expr COLON expr RIGHT_CURLY_BRACKET {
        $2->index = $4;
        $$ = $2;
        //fprintf(stderr, "indexedelem -> {expr:expr}\n");
    };

block:
    LEFT_CURLY_BRACKET{
        scope++;
        enable(scope);
    } stmtp RIGHT_CURLY_BRACKET{
        hide(scope);
        scope--;
        $$ = $3;
        //fprintf(stderr, "block -> {stmt*}\n");
    }
    | LEFT_CURLY_BRACKET{
        scope++;
        enable(scope);
    } RIGHT_CURLY_BRACKET{
        hide(scope);
        scope--;
        //fprintf(stderr, "block -> {stmt*}\n");
    };

funcname:
    ID { 
        $$ = yylval.strVal; 
        //fprintf(stderr, "funcname -> ID\n");
    }
    |  {
        char* a = malloc(sizeof(char*));
        char* b = malloc(sizeof(char*));
        strcat(b,strdup("$"));
        sprintf(a,"%d",dollars_counter);
        dollars_counter++;
        $$ = strcat(b,a);
        //fprintf(stderr, "funcname -> \n");
    };

funcprefix: 
    FUNCTION funcname{
        if (check_library_collisions($2) && !scope) {
            //fprintf(stderr, "\nERROR -> Collision with library function: at line %d\n\n", yylineno);
        } else {
            $$ = lookup_by_specific_scope_and_return($2, scope);
            if (!$$) {
                emit(jump, NULL, NULL, NULL, 0, yylineno);
                $$ = insert_and_space_offset($2, scope, yylineno, USERFUNC, currscopespace(), currscopeoffset());
                $$->iaddress = nextquad() + 1;
                emit(funcstart, NULL, NULL, lvalue_expr($$), 0, yylineno);
                push(currscopeoffset());
                enterscopespace();
                resetformalargsoffset();
            }
            else {
                int user = lookup_by_specific_type_and_scope($2, USERFUNC, scope);
                int global = lookup_by_specific_type_and_scope($2, GLOBAL, scope);
                int local = lookup_by_specific_type_and_scope($2, LOCAL, scope);
                int formal = lookup_by_specific_type_and_scope($2, FORMAL, scope);

                if (user != -1) {
                    //fprintf(stderr, "\nERROR at line %d -> Function %s already declared: at line %d\n\n", yylineno, $2, user);
                } else if (global != -1) {
                    //fprintf(stderr, "\nERROR at line %d -> Global variable %s already declared: at line %d\n\n", yylineno, $2, global); 
                } else if (local != -1 ) { 
                    //fprintf(stderr, "\nERROR at line %d -> Local variable %s already declared: at line %d\n\n", yylineno, $2, local); 
                } else if (formal != -1 ) {
                    //fprintf(stderr, "\nERROR at line %d -> Formal variable %s already declared: at line %d\n\n", yylineno, $2, formal); 
                }  
            }
        }
        //fprintf(stderr, "funcprefix -> FUNCTION funcname\n");
    };

funcargs:
    LEFT_PARENTHESES  {
        ++scope;
        return_state++;
    } idlist RIGHT_PARENTHESES {
        enterscopespace();
        resetfunctionlocalsoffset();
        scope--;
        //fprintf(stderr, "funcargs -> (idlist)\n");
    };

funcbody:
    block {
        $$ = currscopeoffset();
        exitscopespace();
        //fprintf(stderr, "funcbody -> block\n");
    };

funcdef: 
    funcprefix funcargs funcbody {
        exitscopespace();
        
        $1->totalLocals = $3;
        restorecurrscopeoffset(pop_and_top());
        $$ = $1;
        if (returnlabel){
            patchlabel(returnlabel, nextquad() + 1);
        }
        emit(funcend, NULL, NULL, lvalue_expr($1), 0, yylineno);
        patchlabel($1->iaddress-2, nextquad() + 1);
        return_state--; 
        //fprintf(stderr, "funcdef -> funcprefix funcargs funcbody\n");
    };

const:
    INTEGER {
        $$ = newexpr_constnum(yylval.intVal);
        //fprintf(stderr, "const -> INTEGER\n");
    }
    |REAL   {
        $$ = newexpr_constnum(yylval.realVal);
        //fprintf(stderr, "const -> REAL\n");
    }
    |STRING {
        $$ = newexpr_conststring(strcat(yylval.strVal,"\""));
        //fprintf(stderr, "const -> STRING\n");
    }
    |NIL    {
        //fprintf(stderr, "const -> NIL\n");
    }
    |TRUE   {
        $$ = newexpr_constbool(1);
        //fprintf(stderr, "const -> TRUE\n");
    }
    |FALSE  {
        $$ = newexpr_constbool(0);
        //fprintf(stderr, "const -> FALSE\n");
    };

idlist:
    ID {
        if (check_library_collisions(yylval.strVal)) {
            //fprintf(stderr, "\nERROR -> Formal variable %s has the same name with Library function: at line %d\n\n", yylval.strVal, yylineno);
        } else {
            sym_temp = insert_and_space_offset(yylval.strVal, scope, yylineno, FORMAL, currscopespace(), currscopeoffset());
            inccurrscopeoffset();
        } 
        
    } idlist_case_commaid {
        //fprintf(stderr, "idlist -> ID idlist_case_commaid\n");
    }
    | {
        //fprintf(stderr, "idlist -> \n");
    };

idlist_case_commaid:
    COMMA ID {
        if (check_library_collisions(yylval.strVal)) {
            //fprintf(stderr, "\nERROR -> Formal variable %s has the same name with Library function: at line %d\n\n", yylval.strVal, yylineno);
        } else {
            if (!lookup_last(yylval.strVal, scope)) {
                sym_temp = insert_and_space_offset(yylval.strVal, scope, yylineno, FORMAL, currscopespace(), currscopeoffset());
                inccurrscopeoffset();
            } else {
                //fprintf(stderr, "\nERROR -> Formal Variable %s already defined: at line %d\n\n", yylval.strVal, yylineno); 
            }    
        }
    } idlist_case_commaid {
        //fprintf(stderr, "idlist_case_commaid -> ,ID idlist_case_commaid\n");
    }
    | {
        //fprintf(stderr, "idlist -> \n");
    }
    ;

ifprefix:
    IF LEFT_PARENTHESES expr RIGHT_PARENTHESES {
        if (!$3->trueList){
            // e.g if (false)
            $3->trueList = append($3->trueList, nextquad(), or);
            $3->falseList = append($3->falseList, nextquad()+1, or);
            emit(if_eq, $3, newexpr_constbool(1), NULL, 0, yylineno);
            emit(jump, NULL, NULL, NULL, 0, yylineno);
        }
        fix_temp($3->trueList);
        fix_temp($3->falseList);

        iflabel_true = nextquad() + 1;
        
        $$ = $3;
        //fprintf(stderr, "ifprefix -> IF LEFT_PARENTHESES expr RIGHT_PARENTHESES\n");
    };

elseprefix:
    ELSE { 
        $$ = nextquad(); 
    };

ifstmt:     
    ifprefix stmt {
        patch($1->trueList, $1->falseList, nextquad() + 1, iflabel_true);
        //fprintf(stderr, "ifstmt -> if (expr) stmt\n");
        $$ = $2;
    }
    |ifprefix stmt elseprefix {
        emit(jump, NULL, NULL, NULL, 0, yylineno);
        patch($1->trueList, $1->falseList, nextquad() + 1, iflabel_true);
        iflabel_true = nextquad() - 1;
    } stmt {
        patch_orphan_ifelsejumps(nextquad()+1);
        patchlabel_difzero(iflabel_true - 1, nextquad()+1);
        $$ = $5;
        //fprintf(stderr, "ifstmt -> if (expr) stmt ELSE stmt\n");
    };

whilestart:
    WHILE {
        break_state++;
        continue_state++;
        loopcounter++;
        $$ = nextquad() + 1;
        //fprintf(stderr, "whilestart -> WHILE\n");
    }

whilecond:
    LEFT_PARENTHESES expr RIGHT_PARENTHESES{
        if (!$2->trueList){
            // e.g while (false) or while(a) or while (true)
            $2->trueList = append($2->trueList, nextquad(), or);
            $2->falseList = append($2->falseList, nextquad()+1, or);
            emit(if_eq, $2, newexpr_constbool(1), NULL, 0, yylineno);
            emit(jump, NULL, NULL, NULL, 0, yylineno);
        }
        fix_temp($2->trueList);
        fix_temp($2->falseList);
        $$ = $2;
        whilelabel_true = nextquad() + 1;
        //fprintf(stderr, "whilecond -> (expr)\n");
    }

whilestmt:  
    whilestart whilecond stmt {
        emit(jump, NULL, NULL, NULL, $1, get_quad_line($1));
        
        patch($2->trueList, $2->falseList, nextquad() + 1, whilelabel_true);
        //if we found a break statement
        
        if (break_index >= 0) $3->breakList = append_brcont($3->breakList, nextquad());
        if (continue_index >= 0) $3->contList = append_brcont($3->contList, $1);
        $$ = $3;
        break_state--;
        continue_state--;
        loopcounter--;
        //fprintf(stderr, "whilestmt -> WHILE (expr) stmt\n");
    };

M:
    {
        $$ = nextquad();
    }

forstmt:
    FOR LEFT_PARENTHESES {} elist SEMICOLON {
        loopcounter++;
        break_state++;
        continue_state++;
        for_label_help = nextquad() + 1;
        //fprintf(stderr, "forstmt -> for(elist; expr; elist)\n");
        //fprintf(stderr, "for_label_st: %d", for_label_help);
    } expr SEMICOLON {
        for_label_start = nextquad()-1;
        //fprintf(stderr, "for_label_st: %d", for_label_start);
        if (!$7->trueList){
            $7->trueList = append($7->trueList, nextquad(), or);
            $7->falseList = append($7->falseList, nextquad()+1, or);
            emit(if_eq, NULL, newexpr_constbool(1), $7, 0, yylineno); 
            emit(jump, NULL, NULL, NULL, 0, yylineno);
        }
        fix_temp($7->trueList);
        fix_temp($7->falseList);
        for_label = nextquad() + 1;
    } elist RIGHT_PARENTHESES {
        if (for_label_help == for_label_start) emit(jump, NULL, NULL, NULL, for_label_start, yylineno);
        else emit(jump, NULL, NULL, NULL, for_label_help, yylineno);
        patchlabel(for_label_start - 1, nextquad() + 1);
        forlabel_true = nextquad() + 1;
    } stmt {
        emit(jump, NULL, NULL, NULL, for_label, yylineno);
        patch($7->trueList, $7->falseList, nextquad() + 1, forlabel_true);
        patchlabel(for_label_start - 1, forlabel_true);
        //patchlabel(for_label_start, forlabel_true);
        //print_quads();
        //exit(0);
        if (break_index >= 0) $13->breakList = append_brcont($13->breakList, nextquad());
        if (continue_index >= 0) $13->contList = append_brcont($13->contList, for_label);
        $$ = $13;

        //for_label = 0;
        //for_label_start = 0;
        break_state--;
        continue_state--;
        loopcounter--;
        //fprintf(stderr, "forstmt -> for(elist; expr; elist) stmt\n");
    };

returnstmt: 
    RETURN expr SEMICOLON   {
        if (!return_state) {
            yyerror("Return statement not in a function");
            exit(0);
        }
        else {	
            emit(ret, $2, NULL, NULL, 0, yylineno);	
            $$ = make_stmt();	
            returnlabel = nextquad();
            emit(jump, NULL, NULL, NULL, 0, yylineno);	
        }
        //fprintf(stderr, "returnstmt -> RETURN expr SEMICOLON\n");
    }
    |RETURN SEMICOLON       {
        if (!return_state) {
            yyerror("Return statement not in a function");
            exit(0);
        }
        else {	
            emit(assign, NULL, NULL, NULL, 0, yylineno);	
            $$ = make_stmt();
            returnlabel = nextquad();
            emit(jump, NULL, NULL, NULL, 0, yylineno);	
        }
        //fprintf(stderr, "returnstmt -> RETURN SEMICOLON\n");
    };	



%%

void check_term () {
    if (!is_member){
        if (lookup_by_specific_type(yylval.strVal, USERFUNC))   {
            //fprintf(stderr, "\nERROR -> Using ProgramFunc %s as an lvalue: at line %d\n\n", yylval.strVal, yylineno);
        }
        if (lookup_by_specific_type(yylval.strVal, LIBFUNC))    {
            //fprintf(stderr, "\nERROR -> Using LibFunc %s as an lvalue: at line %d\n\n", yylval.strVal, yylineno);
        }
     } else {
        if (lookup_by_specific_type(yytext, USERFUNC)) {
            //fprintf(stderr, "\nERROR -> Using ProgramFunc %s as an lvalue: at line %d\n\n", yytext, yylineno);
        }
        if (lookup_by_specific_type(yytext, LIBFUNC))  {
            //fprintf(stderr, "\nERROR -> Using LibFunc %s as an lvalue: at line %d\n\n", yytext, yylineno);
        }
     }
}

expr* arithmetic_expr(expr* t, expr* t1, int t2, expr* t3){
    compile_time_expression_error(t1);
    compile_time_expression_error(t3);
    if (t1->type == constnum_e && t3->type == constnum_e){
        int result;
        if (t2 == 1) result = t1->numConst + t3->numConst;
        else if (t2 == 2) result = t1->numConst - t3->numConst;
        else if (t2 == 3) {
            
            result = (int)t1->numConst * (int)t3->numConst;
        }
        else if (t2 == 4) {
            if ((int)t3->numConst == 0) {
                //fprintf(stderr, "x DIV 0 NOT PERMITTED\n");
                exit(0);
            }
            result = t1->numConst / t3->numConst;
        }
        else if (t2 == 5) {
            if ((int)t3->numConst == 0) {
                //fprintf(stderr, "x MOD 0 NOT PERMITTED\n");
                exit(0);
            }
            result = (int)t1->numConst % (int)t3->numConst;
        }
        t = newexpr(arithexpr_e);
        t->sym = newtemp();
    } else {
        t = newexpr(arithexpr_e);
        t->sym = newtemp();
    }

    emit(t2, t1, t3, t, 0, yylineno);
    return t;
}

expr* compare_expr(expr* t, expr* t1, int t2, expr* t3){
    /* Τotal Expression Assessment */
    t->type = boolexpr_e;
    t->trueList = append(t->trueList, nextquad(), temp);
    t->falseList = append(t->falseList, nextquad()+1, temp);
    emit(t2, t1, t3, NULL, 0, yylineno);
    emit(jump, NULL, NULL, NULL, 0, yylineno);
    return t;
}

void yyerror (char* yaccProvidedMessage){
	//fprintf(stderr, "\nERROR -> %s: at line %d, before token %s\n\n", yaccProvidedMessage, yylineno, yytext);
}

int main (int argc, char** argv){
    if (argc > 1){
        if (!(yyin = fopen(argv[1], "r"))){
            //fprintf(stderr,"Cannot read file: %s\n",argv[1]);
            return 1;
        }
    } else yyin = stdin;

    initialize_libraries();
    yyparse();
    attach_formals();
    //print_by_scopes();    /* Phase 2 */
    print_quads();          /* Phase 3 */
    start_generate();       /* Phase 4 */
    read_bin();             /* Phase 5 */

    return 0;   
}