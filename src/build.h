#pragma once

#include "acirc.h"

int acirc_add_command(acirc *c, const char *cmd, const char **strs, size_t n);
int acirc_add_input(acirc *c, acircref ref, acircref id);
int acirc_add_const(acirc *c, acircref ref, int val);
int acirc_add_gate(acirc *c, acircref ref, acirc_operation op,
                   const acircref *refs, size_t n);
int acirc_add_extgate(acirc *c, acircref ref, const char *name,
                      const acircref *refs, size_t n);
