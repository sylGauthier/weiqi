#include "pipe_proc.h"
#include "game.h"
#include "gtp.h"
#include "human.h"

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

int main() {
    struct GameContext ctx;
    char* gnugocmd[] = {"/usr/bin/gnugo", "--mode", "gtp", NULL};
    int ok = 1;

    game_init(&ctx, 19);

    if (!local_gtp_engine_init(&ctx.white, gnugocmd[0], gnugocmd)) {
        fprintf(stderr, "gnugo init failed\n");
        ok = 0;
    } else if (!human_init(&ctx.black)) {
        fprintf(stderr, "human player init failed\n");
        ok = 0;
    } else {
        game_run(&ctx);
    }
    game_free(&ctx);
    return !ok;
}
