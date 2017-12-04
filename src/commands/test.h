#pragma once

#include "../acirc.h"

#include <stdio.h>

extern const acirc_command_t command_test;
void acirc_add_tests_to_file(const acirc_tests_t *t, size_t ninputs, size_t noutputs, FILE *f);
