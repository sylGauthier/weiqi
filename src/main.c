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
        switch (prog.mode) {
            case WQ_SERVER:
                ok = game_run(&prog.ctx);
                game_free(&prog.ctx);
                break;
            case WQ_CLIENT:
                ok = game_client_run(&prog.client);
                game_client_free(&prog.client);
                break;
            default:
                break;
        }
    } else {
        ok = 0;
    }

    prog_free(&prog);
    return !ok;
}
