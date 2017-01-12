#include "acirc.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    acircref *refs;
    size_t n;
} acirc_fhe_plaintexts_t;

static int acirc_add_fhe_plaintexts(acirc *c, const char **strs, size_t n)
{
    acirc_fhe_plaintexts_t *s = acirc_calloc(1, sizeof s[0]);
    s->refs = acirc_calloc(n, sizeof s->refs[0]);
    s->n = n;
    for (size_t i = 0; i < n; ++i) {
        s->refs[i] = atoi(strs[i]);
    }
    acirc_add_extra(&c->extras, "fhe-plaintexts", s);
    return ACIRC_ERR;
}

static int acirc_add_fhe(acirc *c, const char **strs, size_t n)
{
    const char *cmd = strs[0];
    if (strcmp(cmd, "plaintexts") == 0) {
        if (n == 0) {
            fprintf(stderr, "error: invalid number of arguments to ':fhe plaintexts' command\n");
            return ACIRC_ERR;
        }
        return acirc_add_fhe_plaintexts(c, &strs[1], n - 1);
    } else {
        fprintf(stderr, "error: unknown :fhe command '%s'\n", cmd);
        return ACIRC_ERR;
    }
}
const acirc_command_t command_fhe = { ":fhe", acirc_add_fhe };

int acirc_add_fhe_commands(acirc_commands_t *cmds)
{
    cmds->commands = acirc_realloc(cmds->commands, (cmds->n + 1) * sizeof cmds->commands[0]);
    cmds->n += 1;
    cmds->commands[cmds->n - 1] = command_fhe;
    return ACIRC_OK;
}
