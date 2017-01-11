#include "utils.h"

#include <stdio.h>
#include <stdlib.h>

uint32_t g_verbose = 0;

void ensure_gate_space(acirc *c, acircref ref)
{
    if ((size_t) ref >= c->_ref_alloc) {
        c->gates = acirc_realloc(c->gates, 2 * c->_ref_alloc * sizeof(struct acirc_gate_t));
        c->_ref_alloc *= 2;
    }
}

void * acirc_calloc(size_t nmemb, size_t size)
{
    void *ptr = calloc(nmemb, size);
    if (ptr == NULL) {
        fprintf(stderr, "[acirc_calloc] couldn't allocate %lu bytes!\n", nmemb * size);
        abort();
    }
    return ptr;
}

void * acirc_malloc(size_t size)
{
    void *ptr = malloc(size);
    if (ptr == NULL) {
        fprintf(stderr, "[acirc_malloc] couldn't allocate %lu bytes!\n", size);
        abort();
    }
    return ptr;
}

void * acirc_realloc(void *ptr, size_t size)
{
    void *ptr_ = realloc(ptr, size);
    if (ptr_ == NULL) {
        fprintf(stderr, "[acirc_realloc] couldn't reallocate %lu bytes!\n", size);
        abort();
    }
    return ptr_;
}

bool in_array(int x, int *ys, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        if (x == ys[i])
            return true;
    }
    return false;
}

bool any_in_array(acircref *xs, int xlen, int *ys, size_t ylen)
{
    for (int i = 0; i < xlen; i++) {
        if (in_array(xs[i], ys, ylen))
            return true;
    }
    return false;
}

void array_printstring_rev(int *xs, size_t n)
{
    for (int i = n-1; i >= 0; i--)
        printf("%d", xs[i]);
}
