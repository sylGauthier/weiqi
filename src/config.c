#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "config.h"
#include "cmd.h"

#define SET_VEC3(v, a, b, c) v[0] = (a); v[1] = (b); v[2] = (c);

static void load_default_theme(struct InterfaceTheme* theme) {
    memset(theme, 0, sizeof(*theme));
    theme->style = W_UI_NICE;
    strcpy(theme->wood, "wood.png");
    theme->ibl.enabled = 0;

    SET_VEC3(theme->backgroundColor, 0.3, 0.3, 0.3);
    SET_VEC3(theme->bStone.color, 0, 0, 0);
    SET_VEC3(theme->wStone.color, 1, 1, 1);
    SET_VEC3(theme->pointer.color, 0, 0, 0);
    SET_VEC3(theme->board.color, 0.65, 0.58, 0.29);
    SET_VEC3(theme->lmvp.color, 0.6, 0.2, 0.2);

    theme->board.roughness = 0.3;
    theme->board.metalness = 0.;
    theme->wStone.roughness = 0.3;
    theme->wStone.metalness = 0;
    theme->bStone.roughness = 0.3;
    theme->bStone.metalness = 0;

    theme->boardThickness = 0.01;
    theme->gridScale = 0.;
    theme->stoneYScale = 0.3;
    theme->pointerSize = 0.01;
    theme->fov = 60;

    theme->coordinates = 0;
    strcpy(theme->font, "font.ttf");
}

int config_load_defaults(struct Config* config) {
    memset(config, 0, sizeof(*config));
    config->boardSize = 19;
    config->handicap = 0;
    config->black.type = W_HUMAN;
    config->white.type = W_HUMAN;
    config->mode = WQ_SERVER;
    config->numEngines = 0;
    load_default_theme(&config->theme);
    return 1;
}

static int add_engine(struct Config* config,
                      const char* name,
                      const char* cmd) {
    struct Engine* e;

    if (config->numEngines >= W_NUM_ENGINES) return 0;
    e = config->engines + config->numEngines;
    if (       !(e->name = malloc(strlen(name) + 1))
            || !(e->command = malloc(strlen(cmd) + 1))) {
        free(e->name);
        return 0;
    }
    strcpy(e->name, name);
    strcpy(e->command, cmd);
    config->numEngines++;
    return 1;
}

static int stone_config(struct Config* config, char** cmd) {
    struct AssetParams* params;

    if (!cmd[0]) {
        fprintf(stderr, "Error: config: "
                        "stone requires at least one argument\n");
        return 0;
    } else if (!strcmp(cmd[0], "black")) {
        params = &config->theme.bStone;
    } else if (!strcmp(cmd[0], "white")) {
        params = &config->theme.wStone;
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

static int board_config(struct Config* config, char** cmd) {
    struct InterfaceTheme* theme = &config->theme;

    if (!cmd[0]) {
        fprintf(stderr, "Error: config: "
                        "board requires at least one argument\n");
        return 0;
    } else if (!strcmp(cmd[0], "size")) {
        if (!cmd[1]) {
            fprintf(stderr, "Error: config: size requires one argument\n");
            return 0;
        }
        config->boardSize = strtol(cmd[1], NULL, 10);
    } else if (!strcmp(cmd[0], "coordinates")) {
        if (!cmd[1]) {
            fprintf(stderr, "Error: config: "
                            "coordinates requires one argument (on|off)\n");
            return 0;
        }
        if (!strcmp(cmd[1], "on")) theme->coordinates = 1;
        else theme->coordinates = 0;
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

char* config_find_engine(struct Config* config, const char* name) {
    unsigned int i;
    for (i = 0; i < config->numEngines; i++) {
        if (!strcmp(name, config->engines[i].name)) {
            return config->engines[i].command;
        }
    }
    return NULL;
}

static int config_load_player(struct Config* config,
                              struct PlayerConf* c,
                              const char* p) {
    if (!strcmp(p, "human")) {
        c->type = W_HUMAN;
    } else if (!strncmp(p, "socket:", strlen("socket:"))) {
        c->type = W_GTP_SOCKET;
        c->gtpCmd = p + 7;
    } else {
        c->type = W_GTP_LOCAL;
        if (!(c->gtpCmd = config_find_engine(config, p))) {
            fprintf(stderr, "Error: invalid GTP engine: %s\n", p);
            return 0;
        }
    }
    return 1;
}

static int rand_assign(struct Config* config, const char* p1, const char* p2) {
    srand(time(NULL));
    if (rand() % 2 == 0) {
        if (       !config_load_player(config, &config->white, p1)
                || !config_load_player(config, &config->black, p2)) {
            return 0;
        }
    } else {
        if (       !config_load_player(config, &config->black, p1)
                || !config_load_player(config, &config->white, p2)) {
            return 0;
        }
    }
    return 1;
}

static int player_config(struct Config* config, char** cmd) {
    struct PlayerConf* conf;

    if (!cmd[0] || !cmd[1]) {
        fprintf(stderr, "Error: config: player requires at least 1 argument\n");
        return 0;
    }
    if (!strcmp(cmd[0], "black")) {
        conf = &config->black;
    } else if (!strcmp(cmd[0], "white")) {
        conf = &config->white;
    } else if (!strcmp(cmd[0], "random")) {
        if (!cmd[2]) {
            fprintf(stderr, "Error: config: 'player random' "
                            "requires an extra 2 arguments\n");
            return 0;
        }
        return rand_assign(config, cmd[1], cmd[2]);
    } else {
        fprintf(stderr, "Error: config: player: invalid arg: %s\n", cmd[1]);
        return 0;
    }

    if (!config_load_player(config, conf, cmd[1])) {
        return 0;
    }
    return 1;
}

static int lighting_config(struct Config* config, char** cmd) {
    struct InterfaceTheme* theme = &config->theme;

    if (!cmd[0] || !cmd[1]) {
        fprintf(stderr, "Error: config: lighting needs at least 2 arguments\n");
        return 0;
    }
    if (!strcmp(cmd[0], "ibl")) {
        if (!strcmp(cmd[1], "none")) {
            theme->ibl.enabled = 0;
        } else {
            strncpy(theme->iblPath, cmd[1], sizeof(theme->iblPath) - 1);
            theme->ibl.enabled = 1;
        }
    } else {
        fprintf(stderr, "Error: config: unkown option: %s\n", cmd[1]);
        return 0;
    }
    return 1;
}

static int interface_config(struct Config* config, char** cmd) {
    if (!cmd[0] || !cmd[1]) {
        fprintf(stderr,
                "Error: config: interface needs at least 2 arguments\n");
        return 0;
    }
    if (!strcmp(cmd[0], "shading")) {
        if (!strcmp(cmd[1], "pbr")) {
            config->theme.style = W_UI_NICE;
        } else if (!strcmp(cmd[1], "solid")) {
            config->theme.style = W_UI_PURE;
        } else {
            fprintf(stderr, "Error: config: unknown shading: %s\n"
                            "Valid shadings are 'pbr' or 'solid'\n",
                            cmd[1]);
            return 0;
        }
    } else if (!strcmp(cmd[0], "fov")) {
        config->theme.fov = strtof(cmd[1], NULL);
    } else if (!strcmp(cmd[0], "background")) {
        if (!cmd[1] || !cmd[2] || !cmd[3]) {
            fprintf(stderr, "Error: config: interface: "
                            "background needs 3 arguments\n");
            return 0;
        }
        config->theme.backgroundColor[0] = strtof(cmd[1], NULL);
        config->theme.backgroundColor[1] = strtof(cmd[2], NULL);
        config->theme.backgroundColor[2] = strtof(cmd[3], NULL);
    } else {
        fprintf(stderr, "Error: config: interface: unknown command: %s\n",
                        cmd[0]);
        return 0;
    }
    return 1;
}

int config_load_config(struct Config* config) {
    FILE* f;
    char** cmd;
    char *home, *confpath;
    char ok = 1;

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
        if (!strcmp(cmd[0], "engine")) {
            if (!cmd[1] || !cmd[2]) {
                fprintf(stderr, "Error: config: 'engine' needs 2 arguments\n");
                ok = 0;
            } else {
                ok = add_engine(config, cmd[1], cmd[2]);
            }
        } else if (!strcmp(cmd[0], "stone")) {
            ok = stone_config(config, cmd + 1);
        } else if (!strcmp(cmd[0], "board")) {
            ok = board_config(config, cmd + 1);
        } else if (!strcmp(cmd[0], "handicap")) {
            if (cmd[1]) {
                config->handicap = strtol(cmd[1], NULL, 10);
                ok = 1;
            } else {
                fprintf(stderr, "Error: config: 'handicap' needs 1 argument\n");
                ok = 0;
            }
        } else if (!strcmp(cmd[0], "player")) {
            ok = player_config(config, cmd + 1);
        } else if (!strcmp(cmd[0], "lighting")) {
            ok = lighting_config(config, cmd + 1);
        } else if (!strcmp(cmd[0], "interface")) {
            ok = interface_config(config, cmd + 1);
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

static void print_help(const char* cmd) {
    printf("%s [OPTIONS]\n"
           "    -s --size <size>: board size (5-25, default 19)\n"
           "    -h --handicap <num>: number of handicaps (default 0)\n"
           "    -b --black <human|gnugo>: black player (default human)\n"
           "    -w --white <human|gnugo>: white player (default gnugo)\n"
           "    -l --load <file>: load a saved game\n"
           "    -i --interface <pure|nice>: interface theme (default nice)\n",
           cmd);
}

int config_parse_args(struct Config* config, unsigned int argc, char** argv) {
    unsigned int i;

    for (i = 1; i < argc; i++) {
        char* arg = argv[i];

        if (!strcmp(arg, "-w") || !strcmp(arg, "--white")) {
            if (i == argc - 1) {
                print_help(argv[0]);
                return 0;
            }
            if (!config_load_player(config, &config->white, argv[i + 1])) {
                return 0;
            }
            i++;
        } else if (!strcmp(arg, "-b") || !strcmp(arg, "--black")) {
            if (i == argc - 1) {
                print_help(argv[0]);
                return 0;
            }
            if (!config_load_player(config, &config->black, argv[i + 1])) {
                return 0;
            }
            i++;
        } else if (!strcmp(arg, "-r") || !strcmp(arg, "--random")) {
            if (i >= argc - 2) {
                print_help(argv[0]);
                return 0;
            }
            if (!rand_assign(config, argv[i + 1], argv[i + 2])) {
                return 0;
            }
            i += 2;
        } else if (!strcmp(arg, "-s") || !strcmp(arg, "--size")) {
            if (i == argc - 1) {
                print_help(argv[0]);
                return 0;
            }
            config->boardSize = strtol(argv[i + 1], NULL, 10);
            i++;
        } else if (!strcmp(arg, "-h") || !strcmp(arg, "--handicap")) {
            if (i == argc - 1) {
                print_help(argv[0]);
                return 0;
            }
            config->handicap = strtol(argv[i + 1], NULL, 10);
            i++;
        } else if (!strcmp(arg, "-i") || !strcmp(arg, "--interface")) {
            if (i == argc - 1) {
                print_help(argv[0]);
                return 0;
            }
            if (!strcmp(argv[i + 1], "pure")) {
                config->theme.style = W_UI_PURE;
            } else if (!strcmp(argv[i + 1], "nice")) {
                config->theme.style = W_UI_NICE;
            } else {
                print_help(argv[0]);
                return 0;
            }
            i++;
        } else if (!strcmp(arg, "-f") || !strcmp(arg, "--fov")) {
            if (i == argc - 1) {
                print_help(argv[0]);
                return 0;
            }
            config->theme.fov = strtol(argv[i + 1], NULL, 10);
            i++;
        } else if (!strcmp(arg, "--texture")) {
            if (i == argc - 1) {
                print_help(argv[0]);
                return 0;
            }
            strncpy(config->theme.wood, argv[i + 1],
                    sizeof(config->theme.wood) - 1);
            i++;
        } else if (!strcmp(arg, "--coordinates")) {
            config->theme.coordinates = 1;
        } else if (!strcmp(arg, "--color")) {
            if (i + 3 >= argc) {
                print_help(argv[0]);
                return 0;
            }
            SET_VEC3(config->theme.board.color,
                     strtof(argv[i + 1], NULL),
                     strtof(argv[i + 2], NULL),
                     strtof(argv[i + 3], NULL));
            SET_VEC3(config->theme.board.color,
                     strtof(argv[i + 1], NULL),
                     strtof(argv[i + 2], NULL),
                     strtof(argv[i + 3], NULL));
            i += 3;
        } else if (!strcmp(arg, "--client")) {
            if (i >= argc - 2) {
                print_help(argv[0]);
                return 0;
            }
            config->mode = WQ_CLIENT;
            config->sockpath = argv[i + 2];
            if (!config_load_player(config, &config->white, argv[i + 1])) {
                return 0;
            }
            i += 2;
        } else {
            print_help(argv[0]);
            return 0;
        }
    }
    if (config->boardSize < 5 || config->boardSize > 25) {
        fprintf(stderr, "Error: invalid board size, we only support 5 to 25\n");
        return 0;
    }
    return 1;
}

void config_free(struct Config* config) {
    unsigned int i;

    for (i = 0; i < config->numEngines; i++) {
        free(config->engines[i].name);
        free(config->engines[i].command);
    }
}
