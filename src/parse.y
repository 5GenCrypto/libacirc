/* %define api.pure */
%code requires { #include "acirc.h" }
%parse-param { acirc *c }
%define parse.error verbose

/* C declarations */

%{
#include "acirc.h"
#include "build.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

extern int yylineno;
extern int yylex(void);
void yyerror(const acirc *c, const char *m)
{
    (void) c;
    fprintf(stderr, "error (line %d): %s\n", yylineno, m);
}

#define YYINITDEPTH 100000

struct ll_node {
    struct ll_node *next;
    char *data;
};

struct ll {
    struct ll_node *start;
    struct ll_node *end;
    size_t length;
};

%}

/* Bison declarations */

%union { 
    acircref val;
    char *num;
    char *str;
    acirc_operation op;
    struct ll *ll;
};

%token ENDL
%token INPUT CONST
%token  <str>           COMMAND
%token  <num>           NUM
%token  <str>           STR
%token  <op>            GATE
%type   <ll>            numlist
%type   <ll>            strlist

/* Grammar rules */

%%

prog:
        |       line prog
                ;

line:           command | input | const | gate
                ;

command:        COMMAND strlist ENDLS
                {
                    int ret;
                    struct ll *list = $2;
                    struct ll_node *node = list->start;
                    char *strs[list->length];
                    for (size_t i = 0; i < list->length; ++i) {
                        struct ll_node *tmp;
                        strs[i] = node->data;
                        tmp = node->next;
                        free(node);
                        node = tmp;
                    }
                    ret = acirc_add_command(c, $1, strs, list->length);
                    for (size_t i = 0; i < list->length; ++i) {
                        free(strs[i]);
                    }
                    free(list);
                    free($1);
                    if (ret == ACIRC_ERR)
                        YYABORT;
                }
                ;


input:          NUM INPUT NUM ENDLS
                {
                    acirc_add_input(c, atoi($1), atoi($3));
                    free($1);
                    free($3);
                }
                ;

const:          NUM CONST NUM ENDLS
                {
                    acirc_add_const(c, atoi($1), atoi($3));
                    free($1);
                    free($3);
                }
        ;

strlist:        /* empty */
                {
                    struct ll *list = calloc(1, sizeof(struct ll));
                    list->start = list->end = NULL;
                    $$ = list;
                }
        |       strlist STR
                {
                    struct ll *list = $1;
                    struct ll_node *node = calloc(1, sizeof(struct ll_node) + sizeof(char *));
                    node->data = $2;
                    if (list->start == NULL) {
                        list->start = node;
                        list->end = node;
                    } else {
                        list->end->next = node;
                        list->end = node;
                    }
                    list->length++;
                    $$ = list;
                }
        |       numlist
        ;

numlist:       /* empty */
                {
                    struct ll *list = calloc(1, sizeof(struct ll));
                    list->start = list->end = NULL;
                    $$ = list;
                }
        |       numlist NUM
                {
                    struct ll *list = $1;
                    struct ll_node *node = calloc(1, sizeof(struct ll_node) + sizeof(int));
                    node->data = $2;
                    if (list->start == NULL) {
                        list->start = node;
                        list->end = node;
                    } else {
                        list->end->next = node;
                        list->end = node;
                    }
                    list->length++;
                    $$ = list;
                }
                ;

gate:           NUM GATE numlist ENDLS
                {
                    struct ll *list = $3;
                    struct ll_node *node = list->start;
                    acircref refs[list->length];
                    for (size_t i = 0; i < list->length; ++i) {
                        struct ll_node *tmp;
                        refs[i] = atoi(node->data);
                        tmp = node->next;
                        free(node->data);
                        free(node);
                        node = tmp;
                    }
                    acirc_add_gate(c, atoi($1), $2, refs, list->length);
                    free(list);
                    free($1);
                }
                ;

extgate:        NUM STR numlist ENDLS
                {
                    struct ll *list = $3;
                    struct ll_node *node = list->start;
                    acircref refs[list->length];
                    for (size_t i = 0; i < list->length; ++i) {
                        struct ll_node *tmp;
                        refs[i] = atoi(node->data);
                        tmp = node->next;
                        free(node->data);
                        free(node);
                        node = tmp;
                    }
                    acirc_add_extgate(c, atoi($1), $2, refs, list->length);
                    free(list);
                    free($1);
                }
        ;

ENDLS:          ENDLS ENDL
        |       ENDL
        ;

%%
