#include <string.h>
#include <stdlib.h>

#include "prog.h"
#include "gtp.h"
#include "human.h"

#define SET_VEC3(v, a, b, c) v[0] = (a); v[1] = (b); v[2] = (c);

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

static int player_init(struct Prog* prog, struct Player* player,
                       struct PlayerConf* conf) {
    switch (conf->type) {
        case W_HUMAN:
            if (!player_human_init(player, &prog->ctx.ui)) {
                fprintf(stderr, "Error: human player init failed\n");
                return 0;
            }
            break;
        case W_GTP_LOCAL:
            if (!player_gtp_pipe_init(player, conf->gtpCmd)) {
                fprintf(stderr, "Error: GTP engine init failed\n");
                return 0;
            }
            break;
        case W_GTP_SOCKET:
            if (!player_gtp_socket_init(player, conf->gtpCmd)) {
                fprintf(stderr, "Error: GTP socket init failed\n");
                return 0;
            }
            break;
        default:
            fprintf(stderr, "Error: unsupported player backend\n");
            return 0;
    }
    return 1;
}

static void load_default_theme(struct InterfaceTheme* theme) {
    theme->style = W_UI_NICE;
    strcpy(theme->wood, "wood.png");

    SET_VEC3(theme->bStone.color, 0, 0, 0);
    SET_VEC3(theme->wStone.color, 1, 1, 1);
    SET_VEC3(theme->pointer.color, 0, 0, 0);
    SET_VEC3(theme->board.color, 0.65, 0.58, 0.29);

    theme->board.roughness = 0.3;
    theme->board.metalness = 0.;
    theme->wStone.roughness = 0.3;
    theme->wStone.metalness = 0;
    theme->bStone.roughness = 0.3;
    theme->bStone.metalness = 0;

    theme->boardThickness = 0.01;
    theme->gridScale = 1. / 1.1;
    theme->pointerSize = 0.01;
}

int prog_load_defaults(struct Prog* prog) {
    prog->boardSize = 19;
    prog->handicap = 0;
    prog->black.type = W_HUMAN;
    prog->white.type = W_HUMAN;
    prog->gameFile = NULL;
    load_default_theme(&prog->ctx.ui.theme);
    return 1;
}

int prog_parse_args(struct Prog* prog, unsigned int argc, char** argv) {
    unsigned int i;

    for (i = 1; i < argc; i++) {
        char* arg = argv[i];

        if (!strcmp(arg, "-w") || !strcmp(arg, "--white")) {
            if (i == argc - 1) {
                print_help(argv[0]);
                return 0;
            }
            if (!config_load_player(prog, &prog->white, argv[i + 1])) {
                return 0;
            }
            i++;
        } else if (!strcmp(arg, "-b") || !strcmp(arg, "--black")) {
            if (i == argc - 1) {
                print_help(argv[0]);
                return 0;
            }
            if (!config_load_player(prog, &prog->black, argv[i + 1])) {
                return 0;
            }
            i++;
        } else if (!strcmp(arg, "-r") || !strcmp(arg, "--random")) {
            if (i >= argc - 2) {
                print_help(argv[0]);
                return 0;
            }
            if (!config_rand_assign(prog, argv[i + 1], argv[i + 2])) {
                return 0;
            }
            i += 2;
        } else if (!strcmp(arg, "-s") || !strcmp(arg, "--size")) {
            if (i == argc - 1) {
                print_help(argv[0]);
                return 0;
            }
            prog->boardSize = strtol(argv[i + 1], NULL, 10);
            i++;
        } else if (!strcmp(arg, "-h") || !strcmp(arg, "--handicap")) {
            if (i == argc - 1) {
                print_help(argv[0]);
                return 0;
            }
            prog->handicap = strtol(argv[i + 1], NULL, 10);
            i++;
        } else if (!strcmp(arg, "-l") || !strcmp(arg, "--load")) {
            if (i == argc - 1) {
                print_help(argv[0]);
                return 0;
            }
            prog->gameFile = argv[i + 1];
            i++;
        } else if (!strcmp(arg, "-i") || !strcmp(arg, "--interface")) {
            if (i == argc - 1) {
                print_help(argv[0]);
                return 0;
            }
            if (!strcmp(argv[i + 1], "pure")) {
                prog->ctx.ui.theme.style = W_UI_PURE;
            } else if (!strcmp(argv[i + 1], "nice")) {
                prog->ctx.ui.theme.style = W_UI_NICE;
            } else {
                print_help(argv[0]);
                return 0;
            }
            i++;
        } else if (!strcmp(arg, "--texture")) {
            if (i == argc - 1) {
                print_help(argv[0]);
                return 0;
            }
            strncpy(prog->ctx.ui.theme.wood, argv[i + 1],
                    sizeof(prog->ctx.ui.theme.wood) - 1);
            i++;
        } else if (!strcmp(arg, "--color")) {
            if (i + 3 >= argc) {
                print_help(argv[0]);
                return 0;
            }
            SET_VEC3(prog->ctx.ui.theme.board.color,
                     strtof(argv[i + 1], NULL),
                     strtof(argv[i + 2], NULL),
                     strtof(argv[i + 3], NULL));
            i += 3;
        } else {
            print_help(argv[0]);
            return 0;
        }
    }
    if (prog->boardSize < 5 || prog->boardSize > 25) {
        fprintf(stderr, "Error: invalid board size, we only support 5 to 25\n");
        return 0;
    }
    return 1;
}

int prog_init(struct Prog* prog) {
    int ok = 1;
    if (       !player_init(prog, &prog->ctx.white, &prog->white)
            || !player_init(prog, &prog->ctx.black, &prog->black)) {
        fprintf(stderr, "Error: player init failed\n");
        ok = 0;
    } else if (prog->gameFile && !game_load_file(&prog->ctx, prog->gameFile)) {
        fprintf(stderr, "Error: loading game file failed\n");
        ok = 0;
    } else if (    !prog->gameFile
                && !game_init(&prog->ctx, prog->boardSize, prog->handicap)) {
        fprintf(stderr, "Error: game init failed\n");
        ok = 0;
    }
    if (!ok) {
        prog->ctx.white.free(&prog->ctx.white);
        prog->ctx.black.free(&prog->ctx.black);
    }
    return ok;
}

void prog_free(struct Prog* prog) {
    unsigned int i;

    for (i = 0; i < prog->numEngines; i++) {
        free(prog->engines[i].name);
        free(prog->engines[i].command);
    }
}
