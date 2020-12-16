#include <signal.h>
#include <string.h>
#include <stdlib.h>

#include "prog.h"

struct Prog prog;

int main(int argc, char** argv) {
    int ok = 1, init = 0;

    if (       prog_load_defaults(&prog)
            && prog_load_config(&prog)
            && prog_parse_args(&prog, argc, argv)
            && (init = prog_init(&prog))) {
        ok = game_run(&prog.ctx);
    } else {
        ok = 0;
    }

    if (init) game_free(&prog.ctx);
    prog_free(&prog);
    return !ok;
}
