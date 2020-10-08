#include <signal.h>
#include <string.h>
#include <stdlib.h>

#include "prog.h"

struct Prog prog;

void handle_sig_int(int sig) {
    game_free(&prog.ctx);
    exit(0);
}

int main(int argc, char** argv) {
    struct sigaction act;
    int ok = 1;

    act.sa_handler = handle_sig_int;
    sigaction(SIGINT, &act, NULL);

    if (       prog_load_defaults(&prog)
            && prog_parse_args(&prog, argc, argv)
            && prog_init(&prog)) {
        ok = game_run(&prog.ctx);
    } else {
        ok = 0;
    }
    game_free(&prog.ctx);
    return !ok;
}
