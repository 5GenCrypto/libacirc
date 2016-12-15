%code requires { #include "acirc.h" }

/* C declarations */

%{

#include "acirc.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

int yylex(void);
static void yyerror(acirc *c, const char *m)
{
    (void) c;
    printf("Error! %s\n", m);
}
unsigned long from_bitstring (char *s);

#define YYINITDEPTH 100000

struct ll_node {
    int data;
    struct ll_node *next;
};

struct ll {
    struct ll_node *start;
    struct ll_node *end;
    size_t length;
};

%}

/* Bison declarations */

%parse-param{ acirc *c }

%union { 
    acircref val;
    char *str;
    acirc_operation op;
    struct ll *ll;
};

%token NEWLINE
%token COLON_TEST INPUT PLAINTEXT CONST GATE OUTPUT
%token  <str>           STR
%token  <op>            GATETYPE
%type   <ll>            wirelist

/* Grammar rules */

%%

prog:           | line prog
        ;

line:           test | input | input_plaintext | const_ | gate | output
        ;

test:           COLON_TEST STR STR NEWLINE
                {
                    acirc_add_test(c, $2, $3);
                    free($2);
                    free($3);
                }
        ;

input:          STR INPUT NEWLINE
                {
                    acirc_add_input(c, atoi($1));
                    free($1);
                }
        ;

input_plaintext:
                STR INPUT PLAINTEXT NEWLINE
                {
                    acirc_add_input_plaintext(c, atoi($1));
                    free($1);
                }
        ;

const_:         STR CONST STR NEWLINE
                {
                    acirc_add_const(c, atoi($1), atoi($3));
                    free($1);
                    free($3);
                }
        ;

wirelist:       /* empty */
                {
                    struct ll *list = calloc(1, sizeof(struct ll));
                    list->start = NULL;
                    list->end = NULL;
                    $$ = list;
                }
        |       wirelist STR
                {
                    struct ll *list = $1;
                    struct ll_node *node = calloc(1, sizeof(struct ll_node));
                    node->data = atoi($2);
                    if (list->start == NULL) {
                        list->start = node;
                        list->end = node;
                    } else {
                        list->end->next = node;
                        list->end = node;
                    }
                list->length++;
                $$ = list;
                free($2);
                }
                ;

gate:           STR GATETYPE wirelist NEWLINE
{
    struct ll *list = $3;
    struct ll_node *node = list->start;
    acircref refs[list->length];
    for (size_t i = 0; i < list->length; ++i) {
        struct ll_node *tmp;
        refs[i] = node->data;
        tmp = node->next;
        free(node);
        node = tmp;
    }
    acirc_add_gate(c, atoi($1), $2, refs, list->length);
    free(list);
    free($1);
}
                ;

output:         STR OUTPUT GATETYPE wirelist NEWLINE
                {
                    struct ll *list = $4;
                    struct ll_node *node = list->start;
                    acircref refs[list->length];
                    for (size_t i = 0; i < list->length; ++i) {
                        struct ll_node *tmp;
                        refs[i] = node->data;
                        tmp = node->next;
                        free(node);
                        node = tmp;
                    }
                    acirc_add_gate(c, atoi($1), $3, refs, list->length);
                    acirc_add_output(c, atoi($1));
                    free(list);
                    free($1);
                }
                ;

%%
