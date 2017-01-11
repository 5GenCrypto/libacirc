/* make yytext a pointer */
%pointer
/* not supplying yywrap() function */
%option noyywrap
%option yylineno
%option always-interactive

%x command

%{
#include "acirc.h"
#include "parse.h"
#include <stdlib.h>
%}

%%

input       { return INPUT; }
plaintext   { return PLAINTEXT; }
const       { return CONST; }

ADD|SUB|MUL|ID {
    yylval.op = acirc_str2op(yytext); 
    return GATE;
} 

\n       { return ENDL; }
[ \r\t]+                        /* ignore whitespace */
#.*\n                           /* ignore comments */

[0-9]+      { yylval.num = strdup(yytext); return NUM; }
[a-zA-Z]+   { yylval.str = strdup(yytext); return STR; }
:[a-zA-Z]+  { yylval.str = strdup(yytext); BEGIN(command); return COMMAND; }

<command>{
    [^ \r\t\n]+ { yylval.str = strdup(yytext); return STR; }
    \n { BEGIN(INITIAL); return ENDL; }
}

. { printf("Unrecognized character: %s\n", yytext); }
