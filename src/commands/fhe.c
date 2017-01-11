#include "acirc.h"
#include "utils.h"

#include <stdio.h>
#include <string.h>

static int acirc_add_fhe_plaintexts(acirc *c, const char **strs, size_t n)
{
    (void) c; (void) strs; (void) n;
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
        fprintf(stderr, "error: unknown fhe command '%s'\n", cmd);
        return ACIRC_ERR;
    }
}
const acirc_command_t command_fhe = { ":fhe", acirc_add_fhe };

int acirc_add_fhe_commands(acirc_commands_t *cmds)
{
    size_t n = 1;
    cmds->commands = acirc_realloc(cmds->commands, (cmds->n + n) * sizeof(acirc_command_t));
    cmds->n += n;
    return ACIRC_OK;
}
