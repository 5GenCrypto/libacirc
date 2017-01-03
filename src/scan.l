
%option noyywrap yylineno
%option always-interactive

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
[ \r\t]+ { }
#.*\n    { }

[0-9]+      { yylval.num = strdup(yytext); return NUM; }
[a-zA-Z]+   { yylval.str = strdup(yytext); return STR; }
:[a-zA-Z]+  { yylval.cmd = strdup(yytext); return COMMAND; }

. printf("Unrecognized character: %s\n", yytext);
