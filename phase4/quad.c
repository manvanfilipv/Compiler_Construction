#include "quad.h"

quad* quads = (quad*)0;
unsigned total = 0;
unsigned int currQuad = 0;

#define EXPAND_SIZE 1024
#define CURR_SIZE (total * sizeof(quad))
#define NEW_SIZE (EXPAND_SIZE * sizeof(quad) + CURR_SIZE)

void expand (){
    assert(total == currQuad);
    quad* p = (quad*)malloc(NEW_SIZE);    
    if (quads){
      memcpy(p,quads,CURR_SIZE);
      free(quads);
    }
    quads = p;
    total += EXPAND_SIZE;
}

void emit (enum iopcode op, expr* arg1, expr* arg2, expr* result, int label, unsigned line){
    if (currQuad == total) expand();
    quad *q = quads + currQuad++;
    q->op = op;
    q->arg1 = arg1;
    q->arg2 = arg2;
    q->result = result;
    q->label = label;
    q->line = line;
}

expr* lvalue_expr (SymbolTableEntry* sym){
    assert(sym);
    expr* e = (expr*)malloc(sizeof(expr));
    memset(e,0,sizeof(expr));
    e->next = (expr*)0;
    e->sym = sym; 
    if (sym->type_t == var_s) e->type = var_e;
    else if (sym->type_t == programfunc_s) e->type = programfunc_e;
    else if (sym->type_t == libraryfunc_s) e->type = libraryfunc_e;
    else assert(0);
    return e;
}

expr* member_item (expr* lvalue, char* name, unsigned line){
    lvalue = emit_iftableitem(lvalue, line); 
    expr *item = newexpr(tableitem_e);
    item->sym = lvalue->sym;
    item->index = newexpr_conststring(name);
    return item;
}

expr* emit_iftableitem (expr* e, unsigned line){
    if (e->type == tableitem_e) {
        expr* result = newexpr(var_e);
        result->sym = newtemp();
        emit(tablegetelem, e, e->index, result, 0, line);
        return result;
    } else return e;
}

expr* newexpr (enum expr_t type){
    expr* e = (expr *) malloc(sizeof(expr));
    memset(e, 0, sizeof(expr));
    e->type = type;
    return e;
}

expr* newexpr_constnum (double i){
    expr* result = newexpr(constnum_e);
    result->numConst = i;
    return result;
}

expr* newexpr_constbool (unsigned char i){
    expr* result = newexpr(constbool_e);
    result->boolConst = i;
    return result;
}

expr* newexpr_conststring (char* i){
    expr* result = newexpr(conststring_e);
    result->strConst = i;
    return result;
}

expr* make_call (expr* lv, expr* reversed_elist, unsigned line) {
    expr* func = emit_iftableitem(lv, line);
    while (reversed_elist) {
        emit(param, NULL, NULL, reversed_elist, 0, line);
        reversed_elist = reversed_elist->next;
    }
    emit(call, func, NULL, NULL, 0, line);
    expr* result = newexpr(var_e);
    result->sym = newtemp();
    emit(getretval, NULL, NULL, result, 0, line);
    return result;
}

unsigned nextquad(){
    return currQuad; 
}

void print_quads (){
    quad* temp;
    int counter = 0;
    fprintf(stderr, "\n----------------------------------------------------------------------------\n");
    fprintf(stderr, "|                                  QUADS                                   |");
    fprintf(stderr, "\n----------------------------------------------------------------------------\n");
    fprintf(stderr, "%-12s%-12s%-12s", "Quad", "Opcode", "Result");
    fprintf(stderr, "%-12s%-12s%-12s", "Arg1", "Arg2", "Label");
    fprintf(stderr, "%-12s\n", "Line");
    while (counter < currQuad){
        temp = quads + counter++;
        if (temp->op == 19) temp->label = 0;
        fprintf(stderr, "%-12d%-12s", counter, operation_to_string[temp->op]);
        if (temp->result) print_expr_type(temp->result);
        else fprintf(stderr, "%-12s", "");
        if (temp->arg1) print_expr_type(temp->arg1);
        else fprintf(stderr, "%-12s", "");
        if (temp->arg2) print_expr_type(temp->arg2);
        else fprintf(stderr, "%-12s", "");
        if (temp->label) fprintf(stderr, "%-12d", temp->label);
        else fprintf(stderr, "%-12s", "");
        fprintf(stderr, "%-12d\n", temp->line);
    }
    fprintf(stderr, "----------------------------------------------------------------------------\n");
}

void print_expr_type (expr* e){
    assert(e);
    if (e->type >= 0 && e->type < 8) fprintf(stderr, "%-12s", e->sym->name);
    else if (e->type == 8) fprintf(stderr, "%-12f", e->numConst);
    else if (e->type == 9) {
        if (!e->boolConst) fprintf(stderr, "%-12s", "false");
        else fprintf(stderr, "%-12s", "true");
    } 
    else if (e->type == 10) fprintf(stderr, "%-12s", e->strConst);
    else if (e->type == 11) return;
}

void compile_time_expression_error(expr* e){
    if (e->type != var_e 
        && e->type != tableitem_e 
        && e->type != arithexpr_e 
        && e->type != assignexpr_e 
        && e->type != constnum_e) fprintf(stderr, "Illegal expr used in arithmetic expression\n");
}

/* Patching functions */ 

void patchlabel (unsigned quadNo, unsigned label){
    assert(quadNo < currQuad);
    quads[quadNo].label = label;
}

void clear_labels (expr* expr1){
    Node* temp = expr1->trueList;
    
    if (temp != NULL){
        
        while (temp != NULL){
            if (quads[temp->value].label < 0){
                quads[temp->value].label = 0;
            }
            temp = temp->next;
        }

        temp = expr1->falseList;
        while (temp != NULL){
            if (quads[temp->value].label < 0){
                quads[temp->value].label = 0;
            }
            temp = temp->next;
        }
    } 

}

unsigned get_quad_line (unsigned index){
    if (index < currQuad){
        return quads[index].line;
    }
    return 0;
}

Node* append (Node* head_ref, int new_data, enum iopcode op){
    struct Node* new_node = (struct Node*) malloc(sizeof(struct Node));
    struct Node* last = head_ref; 
    new_node->value = new_data;
    new_node->op = op;
    new_node->next = NULL;

    if (head_ref == NULL){
        head_ref = new_node;
        return head_ref;
    }

    while (last->next != NULL) last = last->next;
    last->next = new_node;

    return head_ref;
}

Node* merge (Node* l1, Node* l2){
    if (!l1) return l2;
    else {
        if (!l2) return l1;
        struct Node *last = l1;
        while (last->next != NULL) last = last->next;
        last->next = l2;
        return l1;
    } 
}

expr* get_last_expr_element (expr* expr1){
    Node* l1 = expr1->trueList;
    if (expr1->sym && l1){
        if (expr1->sym->name[0] == '_'){
            if (l1->next){
                while (l1->next != NULL) l1 = l1->next;
            }
            
            if (quads[l1->value].arg1){
                return quads[l1->value].arg1;
            }
        }
    }
    return expr1;
}

void fix_temp (Node* l1) {
    while (l1 != NULL) {
        if (l1->op == 26) l1->op = 8;
        l1 = l1->next;
    }
}

void clear_assign_labels () {
    quad* temp;
    int counter = 0;
    while (counter < currQuad){
        temp = quads + counter++;
        if (temp->op == 0 && temp->label != 0) temp->label = 0;
    }
}

unsigned check_op (unsigned index) {
    fprintf(stderr, "INDEX: %d", index);
    if (quads[index].op <= 10 && quads[index].op >= 15) {
        quads[index].op == 0;
        return 0;
    }
    return 1;
}

void print (Node* l1){
    while (l1 != NULL) {
        fprintf(stderr,"Op: %d \t Index: %d\n", l1->op, l1->value+1);
        l1 = l1->next;
    }
}

void patch (Node* l1, Node* l2, unsigned falselabel, unsigned truelabel) {
    while (l1 != NULL && l2 != NULL) {
        if (l1->op == 7) { //and
            //if (quads[l1->value].label == 0){
                if (l1->next == NULL) quads[l1->value].label = truelabel;
                else {
                    if (l1->next->op == 7) {
                        quads[l1->value].label = l1->value + 3;
                    } else if (l1->next->op == 8) {
                        quads[l1->value].label = truelabel; 
                    } else if (l1->next->op == 9) {
                        if (l1->next->isNot == 7){
                            quads[l1->value].label = l1->value + 3;
                        } else if (l1->next->isNot == 8){
                            quads[l1->value].label = truelabel;
                        }
                    }
                }
            //}
            //if (quads[l2->value].label == 0){
                if (l2->next == NULL) quads[l2->value].label = falselabel;
                else {
                    if (l2->next->op == 7) {
                        Node* temp = checknextor_returnprev(l2->next);
                        if (temp) quads[l2->value].label = temp->value + 2;
                        else quads[l2->value].label = falselabel;
                    } else if (l2->next->op == 8) {
                        quads[l2->value].label = l2->value + 2;
                    } else if (l2->next->op == 9) {
                        if (l2->next->isNot == 7){
                            Node* temp = checknextor_returnprev(l2->next);
                            if (temp) quads[l2->value].label = temp->value + 2;
                            else quads[l2->value].label = falselabel;
                        } else if (l2->next->isNot == 8){
                            quads[l2->value].label = l2->value + 2;
                        }
                    }
                }
            //}
        } else if (l1->op == 8) { //or
            //if (quads[l1->value].label == 0){
                if (l1->next == NULL) quads[l1->value].label = l1->value + 3;
                else {
                    Node* temp = checknextand(l1->next);
                    if (temp) quads[l1->value].label = temp->value + 3;
                    else quads[l1->value].label = truelabel;
                }
            //}
            //if (quads[l2->value].label == 0){
                if (l2->next == NULL) quads[l2->value].label = falselabel; //jump or
                else {
                    quads[l2->value].label = l2->value + 2;
                    //if (l2->next->op == 7) {
                    //    quads[l2->value].label = l2->value + 3;
                    //} else if (l2->next->op == 8) {
                    //    quads[l2->value].label = l2->value + 3;
                    //} else if (l2->next->op == 9){
                    //
                    //}
                    //Node* temp = checknextand(l2->next);
                    //if (temp) quads[l2->value].label = temp->value;
                    //else {
                    //    quads[l2->value].label = falselabel;
                    //}
                }
            //}
        } else if (l1->op == 9) { //not
                if (quads[l1->value].arg1->type == 9){
                    if (quads[l1->value].arg1->boolConst == 0){// not false
                        //if (quads[l1->value].label == 0){
                            if (l1->next == NULL) quads[l1->value].label = truelabel; 
                            else {
                                if (l1->next->op == 7) quads[l1->value].label = l1->value + 3;
                                else if (l1->next->op == 8) quads[l1->value].label = truelabel;
                                else if (l1->next->op == 9){
                                    if (l1->isNot == 7){
                                        quads[l1->value].label = l1->value + 3;
                                    } else if (l1->isNot == 8){
                                        quads[l1->value].label = truelabel;
                                    }
                                }
                            }
                        //}
                        //if (quads[l2->value].label == 0){
                            if (l2->next == NULL) quads[l2->value].label = falselabel;
                            else {
                                if (l2->next->op == 7) {
                                    Node* temp = checknextor(l2->next);
                                    if (temp) quads[l2->value].label = temp->value - 1;
                                    else quads[l2->value].label = falselabel;
                                } else if (l2->next->op == 8) {
                                    quads[l2->value].label = l2->value + 2;
                                } else if (l2->next->op == 9){
                                    if (l2->isNot == 7){
                                        quads[l2->value].label = falselabel;
                                    } else if (l2->isNot == 8){
                                        quads[l2->value].label = l2->value + 2;
                                    }
                                }
                            }
                        //}
                    } else if (quads[l1->value].arg1->boolConst == 1){// not true
                        //if (quads[l1->value].label == 0){
                            if (l1->next == NULL) quads[l1->value].label = falselabel; 
                            else {
                                if (l1->next->op == 7 || l1->next->op == 9) quads[l1->value].label = falselabel;
                                else if (l1->next->op == 8) quads[l1->value].label = l1->value + 3;
                            }
                        //}
                        //if (quads[l2->value].label == 0){
                            if (l2->next == NULL) quads[l2->value].label = truelabel;
                            else {
                                if (l2->next->op == 7 ) {
                                    quads[l2->value].label = l2->value + 2;
                                } else if (l2->next->op == 8) {
                                    Node* temp = checknextor(l2->next);
                                    if (temp) quads[l2->value].label = temp->value;
                                    else quads[l2->value].label = truelabel;
                                } else if (l2->next->op == 9) {
                                    if (l2->isNot == 7) {
                                        quads[l2->value].label = l2->value + 2; 
                                    } else if (l2->isNot == 8) {
                                        quads[l2->value].label = truelabel;
                                    }
                                }
                            }
                        //}
                    }
                } else {
                    //if (quads[l1->value].label == 0){
                        if (l1->next == NULL) quads[l1->value].label = falselabel; // not a
                        else {
                            if (l1->next->op == 7 || l1->next->op == 9) quads[l1->value].label = falselabel;
                            else if (l1->next->op == 8) quads[l1->value].label = l1->value + 3;
                        }
                    //}
                    //if (quads[l2->value].label == 0){
                        if (l2->next == NULL) quads[l2->value].label = truelabel;
                        else {
                            if (l2->next->op == 7 ) {
                                quads[l2->value].label = l2->value + 2;
                            } else if (l2->next->op == 8) {
                                Node* temp = checknextor(l2->next);
                                if (temp) quads[l2->value].label = temp->value + 2;
                                else quads[l2->value].label = truelabel;
                            } else if (l2->next->op == 9) {
                                if (l2->isNot == 7) {
                                    quads[l2->value].label = l2->value + 2; 
                                } else if (l2->isNot == 8) {
                                    quads[l2->value].label = truelabel;
                                } 
                            }
                        }
                    //}
                }
        }
        l2 = l2->next;
        l1 = l1->next;
    }
}

Node* checknextand (Node* l1) {
    while (l1 != NULL){
        if (l1->op == 7) return l1;
        l1 = l1->next;
    }
    return NULL;
}

Node* checknextor (Node* l1) {
    while (l1 != NULL){
        if (l1->op == 8) return l1;
        l1 = l1->next;
    }
    return NULL;
}

Node* checknextor_returnprev (Node* l1) {
    Node* prev = NULL;
    while (l1 != NULL){
        if (l1->op == 8) return prev;
        prev = l1;
        l1 = l1->next;
    }
    return NULL;
}

unsigned fix_compare (expr* expr1, enum iopcode op){
    unsigned result = 0;
    Node* temp = expr1->trueList;
    if (temp != NULL){
        while (temp->next != NULL){
            temp = temp->next;
        }
        if (temp->op == 26){ //temp
            temp->op = op;
            result = 1;
        }

        temp = expr1->falseList;
        while (temp->next != NULL){
            temp = temp->next;
        }
        if (temp->op == 26){ //temp
            temp->op = op;
            result = 1;
        }
    }
    return result;
}

void fix_not (expr* expr1, enum iopcode op){
    Node* temp = expr1->trueList;
    while (temp != NULL){
        if (temp->op == 9 && temp->isNot == 0){
            temp->isNot = op;
        }
        temp = temp->next;
    }
    
    temp = expr1->falseList;
    while (temp != NULL){
        if (temp->op == 9 && temp->isNot == 0){
            temp->isNot = op;
        }
        temp = temp->next;
    }
}

void patch_orphan_ifelsejumps (unsigned label){
    quad* temp;
    int counter = 0;
    while (counter < currQuad){
        temp = quads + counter++;
        if (temp->op == 25){
            if (temp->label == 0) temp->label = label;
        }
    }
}

// Break and Continue functions

stmt_t* make_stmt () {
    struct stmt_t* new = (struct stmt_t*) malloc(sizeof(struct stmt_t));
    new->breakList = NULL;
    new->contList = NULL;
    return new;
}

node_brcont* append_brcont (node_brcont* head_ref, int new_data){
    struct node_brcont* new_node = (struct node_brcont*) malloc(sizeof(struct node_brcont));
    struct node_brcont* last = head_ref; 
    new_node->value = new_data;
    new_node->next = NULL;

    if (head_ref == NULL){
        new_node->previous = NULL;
        head_ref = new_node;
        return head_ref;
    }

    while (last->next != NULL) last = last->next;
    last->next = new_node;
    new_node->previous = last;

    return head_ref;
}

node_brcont* merge_brcont (node_brcont* l1, node_brcont* l2){
    if (!l1) return l2;
    else {
        if (!l2) return l1;
        struct node_brcont *last = l1;
        while (last->next != NULL) last = last->next;
        last->next = l2;
        l2->previous = l1;
        return l1;
    } 
}

void print_brcont (node_brcont* l1){
    while (l1 != NULL) {
        fprintf(stderr,"Data %d\n", l1->value);
        l1 = l1->next;
    }
}

void patch_brcont (node_brcont* l1, unsigned index, unsigned loopcounter){
    int counter = 1;
    while (l1 != NULL) {
        if (loopcounter == counter){
            patchlabel(index, l1->value);
        }
        counter++;
        l1 = l1->next;
    }
}

void patch_brcont_last (node_brcont* l1, unsigned index){
    if (index < currQuad){
        if (l1){
            while (l1->next != NULL) {
                l1 = l1->next;
            }
            patchlabel(index, l1->value);
        }
    }
}

void patch_brcont_last_reversed (node_brcont* l1, unsigned index){
    if (index < currQuad){
        if (l1){
            while (l1->next != NULL) {
                l1 = l1->next;
            }
            l1 = l1->previous;
            
            patchlabel(index, l1->value);
        }
    }
}

void patch_brcont_reversed (node_brcont* l1, unsigned index, unsigned loopcounter){
    int counter = 1;
    if (l1){
        while (l1->next != NULL) {
            l1= l1->next;
        }
        
        while (l1 != NULL) {
            if (loopcounter == counter){
                patchlabel(index, l1->value);
            }
            counter++;
            l1 = l1->previous;
        }
    }
}

unsigned size_brcont (node_brcont* l1){
    unsigned int size = 0;
    
    while (l1 != NULL) {
        size++;
        l1 = l1->next;
    }
    return size;
}

void patchlabel_difzero (unsigned quadNo, unsigned label){
    assert(quadNo < currQuad);
    if (!quads[quadNo].label) quads[quadNo].label = label;
}

unsigned check_orphan_jump_brcont (unsigned index){
    if (index < currQuad){
        if (!quads[index].label) return 1;
    }
    return 0;
}