%{
  #include "parser.h"
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
      return MULTI_COMMENT;
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
      return NESTED_INSIDE_MULTI_COMMENT;
    }
  } else {
    return MULTI_COMMENT_NEVER_CLOSED;
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
      return STRING_NEVER_CLOSED;
      flag = -1;
      break;
    }
    
  }
  if (flag == 1) return WARNING_STRING;
  else if (flag == 0) return STRING;
}

{assignment}              {return ASSIGNMENT;}
{add}                     {return ADD;}
{sub}                     {return SUB;}
{sharp}                   {return SHARP;}
{slash}                   {return SLASH;}
{percent}                 {return PERCENT;}
{equal}                   {return EQUAL;}
{not_equal}               {return NOT_EQUAL;}
{plus_plus}               {return PLUS_PLUS;}
{minus_minus}             {return MINUS_MINUS;}
{less}                    {return LESS;}
{greater}                 {return GREATER;}
{greater_or_equal}        {return GREATER_OR_EQUAL;}
{less_or_equal}           {return LESS_OR_EQUAL;}
{left_curly_brackets}     {return LEFT_CURLY_BRACKET;}
{right_curly_brackets}    {return RIGHT_CURLY_BRACKET;}
{left_square_bracket}     {return LEFT_SQUARE_BRACKET;}
{right_square_bracket}    {return RIGHT_SQUARE_BRACKET;}
{left_parentheses}        {return LEFT_PARENTHESES;}
{right_parentheses}       {return RIGHT_PARENTHESES;}
{semicolon}               {return SEMICOLON;}
{comma}                   {return COMMA;}
{colon}                   {return COLON;}
{double_colon}            {return DOUBLE_COLON;}
{dot}                     {return DOT;}
{double_dot}              {return DOUBLE_DOT;}
{if}                      {return IF;}
{else}                    {return ELSE;}
{while}                   {return WHILE;}
{for}                     {return FOR;}
{function}                {return FUNCTION;}
{return}                  {return RETURN;}
{break}                   {return BREAK;}
{continue}                {return CONTINUE;}
{and}                     {return AND;}
{not}                     {return NOT;}
{or}                      {return OR;}
{local}                   {return LOCAL_KEYWORD;}
{true}                    {return TRUE;}
{false}                   {return FALSE;}
{nil}                     {return NIL;}
{integer}                 {yylval.intVal = atoi(yytext); return INTEGER;}
{line_comment}            {return LINE_COMMENT;}
{id}                      {yylval.strVal = strdup(yytext); return ID;}
{line}                    {}
{real}                    {yylval.realVal = atof(yytext); return REAL;}
{space}                   {}
{error}                   {}

%%