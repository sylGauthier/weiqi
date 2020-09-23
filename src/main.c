#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "pipe_proc.h"
#include "game.h"
#include "gtp.h"
#include "human.h"

struct GameContext ctx;
char* gnugocmd[] = {"/usr/bin/gnugo", "--mode", "gtp", NULL};

enum PlayerType {
    W_GNUGO,
    W_HUMAN,
    W_GTP_CUSTOM
};

struct Prog {
    unsigned int boardSize;
    unsigned int handicap;
    enum PlayerType white;
    enum PlayerType black;
};

int local_gtp_engine_init(struct Player* player,
                          const char* cmd, char* const* argv) {
    FILE *in, *out;
    int pid, i;

    printf("local go engine (GTP):");
    for (i = 0; argv[i]; i++) {
        printf(" %s", argv[i]);
    }
    printf("\n");

    pid = pipe_proc(cmd, argv, &out, &in);

    if (pid < 0) {
        exit(-1);
    } else if (pid == 0) {
        return 0;
    }
    gtp_init(player, in, out);
    return 1;
}

int player_init(struct Player* player, enum PlayerType type) {
    switch (type) {
        case W_HUMAN:
            if (!human_init(player)) {
                fprintf(stderr, "human player init failed\n");
                return 0;
            }
            break;
        case W_GNUGO:
            if (!local_gtp_engine_init(player, gnugocmd[0], gnugocmd)) {
                fprintf(stderr, "gnugo init failed\n");
                return 0;
            }
            break;
        default:
            fprintf(stderr, "Error: unsupported player backend\n");
            return 0;
    }
    return 1;
}

void handle_sig_int(int sig) {
    game_free(&ctx);
    exit(0);
}

void print_help(const char* cmd) {
    printf("%s [-w|--white <human|gnugo>] [-b|--black <human|gnugo>] "
           "[-s|--size <size>] [-h|--handicap <handicap>]\n", cmd);
}

int main(int argc, char** argv) {
    int ok = 1;
    struct sigaction act;
    struct Prog prog;
    unsigned int i;

    prog.white = W_GNUGO;
    prog.black = W_HUMAN;
    prog.handicap = 0;
    prog.boardSize = 19;

    for (i = 1; i < argc; i++) {
        char* arg = argv[i];

        if (!strcmp(arg, "-w") || !strcmp(arg, "--white")) {
            if (i == argc - 1) {
                print_help(argv[0]);
                return -1;
            }
            if (!strcmp(argv[i + 1], "human")) {
                prog.white = W_HUMAN;
            } else if (!strcmp(argv[i + 1], "gnugo")) {
                prog.white = W_GNUGO;
            } else {
                print_help(argv[0]);
                return -1;
            }
            i++;
        } else if (!strcmp(arg, "-b") || !strcmp(arg, "--black")) {
            if (i == argc - 1) {
                print_help(argv[0]);
                return -1;
            }
            if (!strcmp(argv[i + 1], "human")) {
                prog.black = W_HUMAN;
            } else if (!strcmp(argv[i + 1], "gnugo")) {
                prog.black = W_GNUGO;
            } else {
                print_help(argv[0]);
                return -1;
            }
            i++;
        } else if (!strcmp(arg, "-s") || !strcmp(arg, "--size")) {
            if (i == argc - 1) {
                print_help(argv[0]);
                return -1;
            }
            prog.boardSize = strtol(argv[i + 1], NULL, 10);
            i++;
        } else if (!strcmp(arg, "-h") || !strcmp(arg, "--handicap")) {
            if (i == argc - 1) {
                print_help(argv[0]);
                return -1;
            }
            prog.handicap = strtol(argv[i + 1], NULL, 10);
            i++;
        } else {
            print_help(argv[0]);
            return -1;
        }
    }
    if (prog.boardSize < 5 || prog.boardSize > 25) {
        fprintf(stderr, "Error: invalid board size, we only support 5 to 25\n");
        return -1;
    }

    act.sa_handler = handle_sig_int;
    sigaction(SIGINT, &act, NULL);

    if (       game_init(&ctx, prog.boardSize, prog.handicap)
            && player_init(&ctx.white, prog.white)
            && player_init(&ctx.black, prog.black)) {
        game_run(&ctx);
    } else {
        ok = 0;
    }
    game_free(&ctx);
    return !ok;
}
