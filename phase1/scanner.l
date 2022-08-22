%{
  #include <stdio.h> 
  #include <stdlib.h>
  #include <string.h>
  #define YY_DECL int alpha_yylex(void* yylval)
  void insertToken(unsigned int line, unsigned int numToken, char* type, char* value, char* type2);
  int element_counter = 1;
  struct alpha_token_t {
      unsigned int numline;
      unsigned int numToken;
      char* type;
      char* type2;
      char* value;
      struct alpha_token_t *next; 
  };
  struct alpha_token_t* head = NULL;
%}

%option noyywrap
%option yylineno

if                    "if"
else                  "else"
while                 "while"
for                   "for"
function              "function"
return                "return"
break                 "break"
continue              "continue"
and                   "and"
not                   "not"
or                    "or"
local                 "local"
true                  "true"
false                 "false"
nil                   "nil"
left_curly_brackets   "{"
right_curly_brackets  "}"
left_square_bracket   "["
right_square_bracket  "]"
left_parentheses      "("
right_parentheses     ")"
semicolon             ";"
comma                 ","
colon                 ":"
double_colon          "::"
dot                   "."
double_dot            ".."
assignment            "="
add                   "+"
sub                   "-"
sharp                 "*"
slash                 "/"
percent               "%"
equal                 "=="
not_equal             "!="
plus_plus             "++"
minus_minus           "--"
less                  "<"
greater               ">"
greater_or_equal      ">="
less_or_equal         "<="

integer               (0[xX][0-9A-Fa-f]+)|([0-9]+)
id                    [a-zA-Z][a-zA-Z_0-9]*
line_comment          "//".*
line                  \n
real                  [0-9]*\.[0-9](e|E)?[0-9]*
space                 [\r\n \t\v]
string                "\""
multi_comment         "/*"
error                 .


%%
{multi_comment}           {
  
  int start_counter = 1;
  int end_counter = 0;
  int start_line = yylineno;
  int end_line = yylineno;
  char tmp;

  tmp = input();
  while (tmp != '\0'){
    if (tmp == '\n') end_line++;
    if (tmp == '/') {
      tmp = input();
      if (tmp == '*') start_counter++;
      else unput(tmp);
    } else if (tmp == '*'){
      tmp = input();
      if (tmp == '/'){
        end_counter++;
        if (end_counter == start_counter) {
          end_line = yylineno;
          break;
        }
      } else unput(tmp);
    }
    tmp = input();
  }

  if (end_counter == start_counter){
    if (start_counter == 1){
      char e[100] = "";
      char f[100] = "";

      sprintf(e, "%d", start_line);
      strcat(e, " - ");
      sprintf(f, "%d", end_line);
      strcat(e, f);
      insertToken(start_line, element_counter++, "MULTI-LINE COMMENT", e, "enumerated");
    } else if (start_counter >= 2){
      char a[100] = "";
      char b[100] = "";
      char c[100] = "";
      start_counter = start_counter - 1;
      
      sprintf(a, "%d",start_counter) ;
      strcat(a, " NESTED COMMENTS INSIDE A MULTI-LINE COMMENT");

      sprintf(b, "%d",start_line) ;
      strcat(b, " - ");
      sprintf(c, "%d",end_line) ;
      strcat(b, c);
      insertToken(start_line, element_counter++, a, b, "enumerated");
    }
  } else {
    insertToken(start_line, element_counter++, "UNDEFINED", "MULTI-LINE COMMENT NEVER CLOSED", "error");
  }
}

{string}                  {
  char tmp;
  char prev;
  int flag = 0;
  int SIZE = 1024;
  int current_size = 2;
  char* new = malloc(sizeof(char) * SIZE);
  strcat(new, "\"");
  tmp = input();
  int start_line = yylineno;
  while (tmp != '\0'){
    if (current_size - SIZE == 1) {
      SIZE = SIZE + SIZE;
      new = realloc(new, sizeof(char) * SIZE);  
    }
    if (tmp == '\\') {
      prev = tmp;
      tmp = input();
      if (tmp == 'n') strcat(new, "\n");
      else if (tmp == 't') strcat(new, "\t"); 
      else if (tmp == '\\') strcat(new, "\\");
      else if (tmp == '\"') strcat(new, "\"");
      else if (tmp == '\v') strcat(new, "\v");
      else {
        flag = 1;
        strncat(new, &prev, 1);
        strncat(new, &tmp, 1);
        current_size += 2;
      }
    } else if (tmp == '"' && prev != '\\'){
      break;
    } else {
      strncat(new, &tmp, 1);
      current_size++;
    }
    prev = tmp;
    tmp = input();
    if (tmp == '\0') {
      insertToken(start_line, element_counter++, "STRING NEVER CLOSED", new, "error");
      flag = -1;
      break;
    }
    
  }
  if (flag == 1) insertToken(start_line, element_counter++, "WARNING STRING", strcat(new, "\""), "char*");
  else if (flag == 0) insertToken(start_line, element_counter++, "STRING", strcat(new, "\""), "char*");
}

{assignment}              {insertToken(yylineno, element_counter++, "OPERATOR ASSIGNMENT", yytext, "enumerated");}
{add}                     {insertToken(yylineno, element_counter++, "OPERATOR PLUS", yytext, "enumerated");}
{sub}                     {insertToken(yylineno, element_counter++, "OPERATOR MINUS", yytext, "enumerated");}
{sharp}                   {insertToken(yylineno, element_counter++, "OPERATOR SHARP", yytext, "enumerated");}
{slash}                   {insertToken(yylineno, element_counter++, "OPERATOR SLASH", yytext, "enumerated");}
{percent}                 {insertToken(yylineno, element_counter++, "OPERATOR PERCENT", yytext, "enumerated");}
{equal}                   {insertToken(yylineno, element_counter++, "OPERATOR EQUAL", yytext, "enumerated");}
{not_equal}               {insertToken(yylineno, element_counter++, "OPERATOR NOT_EQUAL", yytext, "enumerated");}
{plus_plus}               {insertToken(yylineno, element_counter++, "OPERATOR PLUS_PLUS", yytext, "enumerated");}
{minus_minus}             {insertToken(yylineno, element_counter++, "OPERATOR MINUS_MINUS", yytext, "enumerated");}
{less}                    {insertToken(yylineno, element_counter++, "OPERATOR LESS", yytext, "enumerated");}
{greater}                 {insertToken(yylineno, element_counter++, "OPERATOR GREATER", yytext, "enumerated");}
{greater_or_equal}        {insertToken(yylineno, element_counter++, "OPERATOR GREATER_OR_EQUAL", yytext, "enumerated");}
{less_or_equal}           {insertToken(yylineno, element_counter++, "OPERATOR LESS_OR_EQUAL", yytext, "enumerated");}

{left_curly_brackets}     {insertToken(yylineno, element_counter++, "PUNCUATION LEFT_CURLY_BRACKET", yytext, "enumerated");}
{right_curly_brackets}    {insertToken(yylineno, element_counter++, "PUNCUATION RIGHT_CURLY_BRACKET", yytext, "enumerated");}
{left_square_bracket}     {insertToken(yylineno, element_counter++, "PUNCUATION LEFT_SQUARE_BRACKET", yytext, "enumerated");}
{right_square_bracket}    {insertToken(yylineno, element_counter++, "PUNCUATION RIGHT_SQUARE_BRACKET", yytext, "enumerated");}
{left_parentheses}        {insertToken(yylineno, element_counter++, "PUNCUATION LEFT_PARENTHESES", yytext, "enumerated");}
{right_parentheses}       {insertToken(yylineno, element_counter++, "PUNCUATION RIGHT_PARENTHESES", yytext, "enumerated");}
{semicolon}               {insertToken(yylineno, element_counter++, "PUNCUATION SEMICOLON", yytext, "enumerated");}
{comma}                   {insertToken(yylineno, element_counter++, "PUNCUATION COMMA", yytext, "enumerated");}
{colon}                   {insertToken(yylineno, element_counter++, "PUNCUATION COLON", yytext, "enumerated");}
{double_colon}            {insertToken(yylineno, element_counter++, "PUNCUATION DOUBLE_COLON", yytext, "enumerated");}
{dot}                     {insertToken(yylineno, element_counter++, "PUNCUATION DOT", yytext, "enumerated");}
{double_dot}              {insertToken(yylineno, element_counter++, "PUNCUATION DOUBLE_DOT", yytext, "enumerated");}
{if}                      {insertToken(yylineno, element_counter++, "KEYWORD IF", yytext, "enumerated");}
{else}                    {insertToken(yylineno, element_counter++, "KEYWORD ELSE", yytext, "enumerated");}
{while}                   {insertToken(yylineno, element_counter++, "KEYWORD WHILE", yytext, "enumerated");}
{for}                     {insertToken(yylineno, element_counter++, "KEYWORD FOR", yytext, "enumerated");}
{function}                {insertToken(yylineno, element_counter++, "KEYWORD FUNCTION", yytext, "enumerated");}
{return}                  {insertToken(yylineno, element_counter++, "KEYWORD RETURN", yytext, "enumerated");}
{break}                   {insertToken(yylineno, element_counter++, "KEYWORD BREAK", yytext, "enumerated");}
{continue}                {insertToken(yylineno, element_counter++, "KEYWORD CONTINUE", yytext, "enumerated");}
{and}                     {insertToken(yylineno, element_counter++, "KEYWORD AND", yytext, "enumerated");}
{not}                     {insertToken(yylineno, element_counter++, "KEYWORD NOT", yytext, "enumerated");}
{or}                      {insertToken(yylineno, element_counter++, "KEYWORD OR", yytext, "enumerated");}
{local}                   {insertToken(yylineno, element_counter++, "KEYWORD LOCAL", yytext, "enumerated");}
{true}                    {insertToken(yylineno, element_counter++, "KEYWORD TRUE", yytext, "enumerated");}
{false}                   {insertToken(yylineno, element_counter++, "KEYWORD FALSE", yytext, "enumerated");}
{nil}                     {insertToken(yylineno, element_counter++, "KEYWORD NIL", yytext, "enumerated");}

{integer}                 {insertToken(yylineno, element_counter++, "INTCONST", yytext, "integer");}
{line_comment}            {insertToken(yylineno, element_counter++, "BLOCK_COMMENT", "//", "enumerated");}
{id}                      {insertToken(yylineno, element_counter++, "ID", yytext, "char*");}
{line}                    {}
{real}                    {insertToken(yylineno, element_counter++, "REALCONST", yytext, "real");}
{space}                   {/*insertToken(yylineno, element_counter++, "SPACE", yytext, "enumerated");*/}
{error}                   {insertToken(yylineno, element_counter++, "UNDEFINED", yytext, "error");}

%%

void print_lexical() {
  struct alpha_token_t *tmp = head;
  printf("---------------------------------     \tLEXICAL ANALYSIS\t     ---------------------------------\n");
  while(tmp != NULL) {
    printf("Line:%d\t \t#%d\t Type:%s\t \tValue:%s  <-%s\n", tmp->numline, tmp->numToken, tmp->type, tmp->value, tmp->type2);
    tmp = tmp->next;
  }
}

void insertToken(unsigned int line, unsigned int numToken, char* type, char* value, char* type2){
  struct alpha_token_t* newtoken = (struct alpha_token_t*) malloc(sizeof(struct alpha_token_t));
  struct alpha_token_t* last = head;
  newtoken->numline = line;
  newtoken->numToken = numToken;
  newtoken->type2 = strdup(type2);
  newtoken->type = strdup(type);
  newtoken->value = strdup(value);
  newtoken->next = NULL;
  if (head == NULL){
    head = newtoken;
    return;
  } 
  while (last->next != NULL) last = last->next;  
  last->next = newtoken;
  return;
}

int main (int argc, char** argv) {
    if (argc > 1){
        if (!(yyin = fopen(argv[1], "r"))) {
            fprintf(stderr, "Cannot read file: %s\n", argv[1]);
            return 1;
        }
    } else yyin = stdin;
    alpha_yylex(NULL);
    print_lexical();
    return 0;
}