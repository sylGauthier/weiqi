#include <string.h>
#include <stdlib.h>
#include <time.h>

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

static int stone_config(struct Prog* prog, char** cmd) {
    struct AssetParams* params;

    if (!cmd[0]) {
        fprintf(stderr, "Error: config: "
                        "stone requires at least one argument\n");
        return 0;
    } else if (!strcmp(cmd[0], "black")) {
        params = &prog->ctx.ui.theme.bStone;
    } else if (!strcmp(cmd[0], "white")) {
        params = &prog->ctx.ui.theme.wStone;
    } else {
        fprintf(stderr, "Error: config: stone must be followd by color\n");
        return 0;
    }
    if (!cmd[1]) {
        fprintf(stderr, "Error: config: stone: missing argument\n");
        return 0;
    } else if (!strcmp(cmd[1], "color")) {
        if (!cmd[2] || !cmd[3] || !cmd[4]) {
            fprintf(stderr, "Error: config: stone: color: missing argument\n");
            return 0;
        }
        params->color[0] = strtof(cmd[2], NULL);
        params->color[1] = strtof(cmd[3], NULL);
        params->color[2] = strtof(cmd[4], NULL);
    } else if (!strcmp(cmd[1], "metalness")) {
        if (!cmd[2]) {
            fprintf(stderr, "Error: config: stone: metalness: "
                            "missing argument\n");
            return 0;
        }
        params->metalness = strtof(cmd[2], NULL);
    } else if (!strcmp(cmd[1], "roughness")) {
        if (!cmd[2]) {
            fprintf(stderr, "Error: config: stone: roughness: "
                            "missing argument\n");
            return 0;
        }
        params->roughness = strtof(cmd[2], NULL);
    } else {
        fprintf(stderr, "Error: config: stone: unknown command: %s\n", cmd[1]);
        return 0;
    }
    return 1;
}

static int board_config(struct Prog* prog, char** cmd) {
    struct InterfaceTheme* theme = &prog->ctx.ui.theme;

    if (!cmd[0]) {
        fprintf(stderr, "Error: config: "
                        "board requires at least one argument\n");
        return 0;
    } else if (!strcmp(cmd[0], "size")) {
        if (!cmd[1]) {
            fprintf(stderr, "Error: config: size requires one argument\n");
            return 0;
        }
        prog->boardSize = strtol(cmd[1], NULL, 10);
    } else if (!strcmp(cmd[0], "texture")) {
        if (!cmd[1]) {
            fprintf(stderr, "Error: config: texture requires one argument\n");
            return 0;
        }
        strncpy(theme->wood, cmd[1], sizeof(theme->wood) - 1);
    } else if (!strcmp(cmd[0], "color")) {
        if (!cmd[1] || !cmd[2] || !cmd[3]) {
            fprintf(stderr, "Error: config: color requires 3 arguments\n");
            return 0;
        }
        theme->board.color[0] = strtof(cmd[1], NULL);
        theme->board.color[1] = strtof(cmd[2], NULL);
        theme->board.color[2] = strtof(cmd[3], NULL);
    } else if (!strcmp(cmd[0], "metalness")) {
        if (!cmd[1]) {
            fprintf(stderr, "Error: config: board: metalness: "
                            "missing argument\n");
            return 0;
        }
        theme->board.metalness = strtof(cmd[1], NULL);
    } else if (!strcmp(cmd[0], "roughness")) {
        if (!cmd[1]) {
            fprintf(stderr, "Error: config: board: roughness: "
                            "missing argument\n");
            return 0;
        }
        theme->board.roughness = strtof(cmd[1], NULL);
    } else {
        fprintf(stderr, "Error: config: board: unknown command: %s\n", cmd[0]);
        return 0;
    }
    return 1;
}

char* config_find_engine(struct Prog* prog, const char* name) {
    unsigned int i;
    for (i = 0; i < prog->numEngines; i++) {
        if (!strcmp(name, prog->engines[i].name)) {
            return prog->engines[i].command;
        }
    }
    return NULL;
}

static int player_config(struct Prog* prog, char** cmd) {
    struct PlayerConf* conf;

    if (!cmd[0] || !cmd[1]) {
        fprintf(stderr, "Error: config: player requires at least 1 argument\n");
        return 0;
    }
    if (!strcmp(cmd[0], "black")) {
        conf = &prog->black;
    } else if (!strcmp(cmd[0], "white")) {
        conf = &prog->white;
    } else if (!strcmp(cmd[0], "random")) {
        if (!cmd[2]) {
            fprintf(stderr, "Error: config: 'player random' "
                            "requires an extra 2 arguments\n");
            return 0;
        }
        return config_rand_assign(prog, cmd[1], cmd[2]);
    } else {
        fprintf(stderr, "Error: config: player: invalid arg: %s\n", cmd[1]);
        return 0;
    }

    if (!config_load_player(prog, conf, cmd[1])) {
        return 0;
    }
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
        } else if (!strcmp(cmd[0], "stone")) {
            ok = stone_config(prog, cmd + 1);
        } else if (!strcmp(cmd[0], "board")) {
            ok = board_config(prog, cmd + 1);
        } else if (!strcmp(cmd[0], "handicap")) {
            if (cmd[1]) {
                prog->handicap = strtol(cmd[1], NULL, 10);
                ok = 1;
            } else {
                fprintf(stderr, "Error: config: 'handicap' needs 1 argument\n");
                ok = 0;
            }
        } else if (!strcmp(cmd[0], "player")) {
            ok = player_config(prog, cmd + 1);
        } else {
            fprintf(stderr, "Warning: config: ignoring unknown command: %s\n",
                    cmd[0]);
        }
        cmd_free(cmd);
    }

    cmd_free(cmd);
    fclose(f);
    free(confpath);
    return ok;
}


int config_load_player(struct Prog* prog, struct PlayerConf* c, const char* p) {
    if (!strcmp(p, "human")) {
        c->type = W_HUMAN;
    } else if (!strncmp(p, "socket:", strlen("socket:"))) {
        c->type = W_GTP_SOCKET;
        c->gtpCmd = p + 7;
    } else {
        c->type = W_GTP_LOCAL;
        if (!(c->gtpCmd = config_find_engine(prog, p))) {
            fprintf(stderr, "Error: invalid GTP engine: %s\n", p);
            return 0;
        }
    }
    return 1;
}

int config_rand_assign(struct Prog* prog, const char* p1, const char* p2) {
    srand(time(NULL));
    if (rand() % 2 == 0) {
        if (       !config_load_player(prog, &prog->white, p1)
                || !config_load_player(prog, &prog->black, p2)) {
            return 0;
        }
    } else {
        if (       !config_load_player(prog, &prog->black, p1)
                || !config_load_player(prog, &prog->white, p2)) {
            return 0;
        }
    }
    return 1;
}
