#include <string.h>
#include <stdlib.h>

#include "prog.h"
#include "cmd.h"

static int add_engine(struct Prog* prog, const char* name, const char* cmd) {
    struct Engine* e;

    if (prog->numEngines >= W_NUM_ENGINES) return 0;
    e = prog->engines + prog->numEngines;
    if (       !(e->name = malloc(strlen(name) + 1))
            || !(e->command = malloc(strlen(cmd) + 1))) {
        free(e->name);
        return 0;
    }
    strcpy(e->name, name);
    strcpy(e->command, cmd);
    prog->numEngines++;
    return 1;
}

int prog_load_config(struct Prog* prog) {
    FILE* f;
    char** cmd;
    char *home, *confpath;
    char ok = 1;

    prog->numEngines = 0;
    if (!(home = getenv("HOME"))) return 0;
    if (!(confpath = malloc(strlen(home) + strlen("/.weiqi") + 1))) return 0;
    sprintf(confpath, "%s/.weiqi", home);
    if (!(f = fopen(confpath, "r"))) {
        fprintf(stderr, "Warning: no readable config file found. "
                        "Please create %s\n", confpath);
        free(confpath);
        return 1;
    }

    while ((cmd = cmd_get(f)) && ok) {
        if (!strcmp(cmd[0], "engine")){
            if (!cmd[1] || !cmd[2]) {
                fprintf(stderr, "Error: config: 'engine' needs 2 arguments\n");
                ok = 0;
            } else {
                ok = add_engine(prog, cmd[1], cmd[2]);
            }
        } else {
            fprintf(stderr, "Warning: config: ignoring unknown command: %s\n",
                    cmd[0]);
        }
        cmd_free(cmd);
    }

    if (prog->numEngines) {
        prog->white.type = W_GTP_LOCAL;
        prog->white.gtpCmd = prog->engines[0].command;
    }
    fclose(f);
    free(confpath);
    return ok;
}
