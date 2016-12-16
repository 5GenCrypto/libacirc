%{
#include "acirc.h"
#include "parse.h"
#include <stdlib.h>
%}

%option noyywrap
%option always-interactive

%%

[0-9]+      { yylval.str = strdup(yytext); return STR; }

input       { return INPUT; }
plaintext   { return PLAINTEXT; }
const       { return CONST; }
output      { return OUTPUT; }

:test       { return COLON_TEST; }

ADD|SUB|MUL|ID {
    yylval.op = acirc_str2op(yytext); 
    return GATETYPE; 
} 

\n       { return NEWLINE; }
[ \r\t]+ { }
#.*\n    { }
