#pragma once

#include "../acirc.h"

#include <stdio.h>

void acirc_add_outputs_to_file(const acirc_outputs_t *o, FILE *f);
extern const acirc_command_t command_outputs;
