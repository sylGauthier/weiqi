#include <string.h>
#include <stdlib.h>

#include "prog.h"
#include "gtp.h"
#include "human.h"

static void print_help(const char* cmd) {
    printf("%s [-w|--white <human|gnugo>] [-b|--black <human|gnugo>] "
           "[-s|--size <size>] [-h|--handicap <handicap>]\n", cmd);
}

static int player_init(struct Prog* prog, struct Player* player, enum PlayerType type) {
    switch (type) {
        case W_HUMAN:
            if (!human_init(player, &prog->ctx.ui)) {
                fprintf(stderr, "Error: human player init failed\n");
                return 0;
            }
            break;
        case W_GTP_LOCAL:
            if (!gtp_local_engine_init(player, prog->gtpCmd)) {
                fprintf(stderr, "Error: GTP engine init failed\n");
                return 0;
            }
            break;
        default:
            fprintf(stderr, "Error: unsupported player backend\n");
            return 0;
    }
    return 1;
}

int prog_load_defaults(struct Prog* prog) {
    prog->boardSize = 19;
    prog->handicap = 0;
    prog->white = W_GTP_LOCAL;
    prog->black = W_HUMAN;
    prog->gtpCmd = "/usr/bin/gnugo --mode gtp";
    prog->gameFile = NULL;
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
            if (!strcmp(argv[i + 1], "human")) {
                prog->white = W_HUMAN;
            } else if (!strcmp(argv[i + 1], "gnugo")) {
                prog->white = W_GTP_LOCAL;
            } else {
                print_help(argv[0]);
                return 0;
            }
            i++;
        } else if (!strcmp(arg, "-b") || !strcmp(arg, "--black")) {
            if (i == argc - 1) {
                print_help(argv[0]);
                return 0;
            }
            if (!strcmp(argv[i + 1], "human")) {
                prog->black = W_HUMAN;
            } else if (!strcmp(argv[i + 1], "gnugo")) {
                prog->black = W_GTP_LOCAL;
            } else {
                print_help(argv[0]);
                return 0;
            }
            i++;
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
    if (prog->gameFile && !game_load_file(&prog->ctx, prog->gameFile) && 1) {
        fprintf(stderr, "Error: loading game file failed\n");
        return 0;
    } else if (    !prog->gameFile
                && !game_init(&prog->ctx, prog->boardSize, prog->handicap)) {
        fprintf(stderr, "Error: game init failed\n");
        return 0;
    }
    if (       !player_init(prog, &prog->ctx.white, prog->white)
            || !player_init(prog, &prog->ctx.black, prog->black)) {
        fprintf(stderr, "Error: player init failed\n");
        game_free(&prog->ctx);
        return 0;
    }
    return 1;
}
