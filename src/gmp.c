#include "acirc.h"

#ifdef HAVE_GMP

#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
acirc_eval_mpz_mod_memo(acirc *c, acircref root, mpz_t *xs, mpz_t *ys,
                        const mpz_t modulus, bool *known, mpz_t *cache)
{
    if (known[root])
        return;

    const acirc_gate_t *gate = &c->gates.gates[root];
    const acirc_operation op = gate->op;
    mpz_t *rop;
    switch (op) {
    case OP_INPUT:
        mpz_init_set(cache[root], xs[gate->args[0]]);
        break;
    case OP_CONST:
        mpz_init_set(cache[root], ys[gate->args[0]]);
        break;
    case OP_ADD: case OP_SUB: case OP_MUL: {
        mpz_init(cache[root]);
        rop = &cache[root];
        for (size_t i = 0; i < gate->nargs; ++i) {
            acirc_eval_mpz_mod_memo(c, gate->args[i], xs, ys, modulus, known, cache);
        }
        if (op == OP_ADD) {
            mpz_set_ui(*rop, 0);
            for (size_t i = 0; i < gate->nargs; ++i) {
                mpz_add(*rop, *rop, cache[gate->args[i]]);
                mpz_mod(*rop, *rop, modulus);
            }
        } else if (op == OP_SUB) {
            mpz_set(*rop, cache[gate->args[0]]);
            for (size_t i = 1; i < gate->nargs; ++i) {
                mpz_sub(*rop, *rop, cache[gate->args[i]]);
                mpz_mod(*rop, *rop, modulus);
            }
        } else if (op == OP_MUL) {
            mpz_set_ui(*rop, 1);
            for (size_t i = 0; i < gate->nargs; ++i) {
                mpz_mul(*rop, *rop, cache[gate->args[i]]);
                mpz_mod(*rop, *rop, modulus);
            }
        } else abort();
        known[root] = true;
        break;
    }
    case OP_SET:
        acirc_eval_mpz_mod_memo(c, gate->args[0], xs, ys, modulus, known, cache);
        mpz_init_set(cache[root], cache[gate->args[0]]);
        break;
    default:
        abort();
    }
}

void
acirc_eval_mpz_mod(mpz_t rop, acirc *c, acircref root, mpz_t *xs, mpz_t *ys,
                   const mpz_t modulus)
{
    bool known[acirc_nrefs(c)];
    mpz_t cache[acirc_nrefs(c)];
    memset(known, '\0', sizeof known);
    acirc_eval_mpz_mod_memo(c, root, xs, ys, modulus, known, cache);
    mpz_set(rop, cache[root]);
    for (size_t i = 0; i < acirc_nrefs(c); ++i) {
        if (known[i])
            mpz_clear(cache[i]);
    }
}

static void array_printstring_rev_mpz(mpz_t *xs, size_t n)
{
    for (int i = n-1; i >= 0; i--)
        gmp_printf("%Zd", xs[i]);
}


bool acirc_ensure_mpz(acirc *c)
{
    bool ok = true;
    mpz_t xs[c->ninputs];
    mpz_t ys[c->consts.n];
    mpz_t rs[c->outputs.n];
    mpz_t modulus;
    const acirc_tests_t *tests = &c->tests;

    if (g_verbose)
        fprintf(stderr, "running acirc tests...\n");

    for (size_t i = 0; i < c->ninputs; ++i)
        mpz_init(xs[i]);
    for (size_t i = 0; i < c->consts.n; ++i)
        mpz_init_set_ui(ys[i], c->consts.buf[i]);
    for (size_t i = 0; i < c->outputs.n; ++i)
        mpz_init(rs[i]);
    mpz_init_set_ui(modulus, 23); /* XXX: why 23? */

    for (size_t test_num = 0; test_num < tests->n; test_num++) {
        bool test_ok = true;
        for (size_t i = 0; i < c->ninputs; ++i)
            mpz_set_ui(xs[i], tests->inps[test_num][i]);
        for (size_t i = 0; i < c->outputs.n; i++) {
            acirc_eval_mpz_mod(rs[i], c, c->outputs.buf[i], xs, ys, modulus);
            test_ok = test_ok && (mpz_cmp_ui(rs[i], tests->outs[test_num][i]) == 0);
        }

        if (g_verbose) {
            if (!test_ok)
                printf("\033[1;41m");
            printf("test %lu input=", test_num);
            array_printstring_rev(tests->inps[test_num], c->ninputs);
            printf(" expected=");
            array_printstring_rev(tests->outs[test_num], c->outputs.n);
            printf(" got=");
            array_printstring_rev_mpz(rs, c->outputs.n);
            if (!test_ok)
                printf("\033[0m");
            puts("");
        }

        ok = ok && test_ok;
    }
    return ok;
}

#endif
