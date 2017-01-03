#pragma once

#include "acirc.h"

extern uint32_t g_verbose;

void ensure_gate_space(acirc *c, acircref ref);

void * acirc_calloc(size_t nmemb, size_t size);
void * acirc_malloc(size_t size);
void * acirc_realloc(void *ptr, size_t size);

bool in_array(int x, int *ys, size_t len);
bool any_in_array(acircref *xs, int xlen, int *ys, size_t ylen);
void array_printstring_rev(int *bits, size_t n);

