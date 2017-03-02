#pragma once

#include "acirc.h"
#include <stdio.h>

void acirc_add_secrets_to_file(const acirc_secrets_t *s, FILE *f);
extern const acirc_command_t command_secrets;
