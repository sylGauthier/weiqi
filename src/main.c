#include <signal.h>
#include <stdlib.h>

#include "pipe_proc.h"
#include "game.h"
#include "gtp.h"
#include "human.h"

struct GameContext ctx;

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
        fprintf(stderr,
                "I have failed as a child and will end myself peacefully\n");
        return 0;
    } else if (pid == 0) {
        fprintf(stderr,
                "I have failed as a father and will end myself peacefully\n");
        return 0;
    }
    gtp_init(player, in, out);
    return 1;
}

void handle_sig_int(int sig) {
    game_free(&ctx);
    exit(0);
}

int main() {
    char* gnugocmd[] = {"/usr/bin/gnugo", "--mode", "gtp", NULL};
    int ok = 1;
    struct sigaction act;

    act.sa_handler = handle_sig_int;
    sigaction(SIGINT, &act, NULL);

    game_init(&ctx, 19);

    if (!local_gtp_engine_init(&ctx.white, gnugocmd[0], gnugocmd)) {
        fprintf(stderr, "gnugo init failed\n");
        ok = 0;
    } else if (!local_gtp_engine_init(&ctx.black, gnugocmd[0], gnugocmd)) {
        fprintf(stderr, "human player init failed\n");
        ok = 0;
    } else {
        game_run(&ctx);
    }
    game_free(&ctx);
    return !ok;
}
