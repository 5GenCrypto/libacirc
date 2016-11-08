%code requires { #include "acirc.h" }

%{
#include "acirc.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

int yylex(void);
static void yyerror(acirc *c, const char *m){ (void) c; printf("Error! %s\n", m); }
unsigned long from_bitstring (char *s);

#define YYINITDEPTH 100000

%}

%parse-param{ acirc *c }

%union { 
    unsigned long val;
    char *str;
    acirc_operation op;
};

%token <op> GATETYPE;
%token <str> NUM
%token <val> XID YID
%token TEST INPUT GATE OUTPUT

%%

prog: line prog | line

line: test | xinput | yinput | gate1 | gate2 | output1 | output2

test: TEST NUM NUM 
{ 
    acirc_add_test(c, $2, $3); 
    free($2);
    free($3);
}

xinput: NUM INPUT XID
{ 
    acirc_add_xinput(c, atoi($1), $3);           
    free($1);
}

yinput: NUM INPUT YID NUM
{ 
    acirc_add_yinput(c, atoi($1), $3, atoi($4)); 
    free($1); 
    free($4); 
}

gate1: NUM GATE GATETYPE NUM
{
    acirc_add_gate(c, atoi($1), $3, atoi($4), -1, false);
    free($1);
    free($4);
}

gate2: NUM GATE GATETYPE NUM NUM
{
    acirc_add_gate(c, atoi($1), $3, atoi($4), atoi($5), false); 
    free($1);
    free($4);
    free($5);
}

output1: NUM OUTPUT GATETYPE NUM
{ 
    acirc_add_gate(c, atoi($1), $3, atoi($4), -1, true); 
    free($1);
    free($4);
}

output2: NUM OUTPUT GATETYPE NUM NUM
{ 
    acirc_add_gate(c, atoi($1), $3, atoi($4), atoi($5), true); 
    free($1);
    free($4);
    free($5);
}
