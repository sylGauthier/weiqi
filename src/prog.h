#ifndef PROG_H
#define PROG_H

#include "game.h"

enum PlayerType {
    W_HUMAN,
    W_GTP_LOCAL
};

struct Prog {
    unsigned int boardSize;
    unsigned int handicap;
    enum PlayerType white;
    enum PlayerType black;
    enum InterfaceTheme theme;

    const char* gtpCmd;
    const char* gameFile;

    struct GameContext ctx;
};

int prog_load_defaults(struct Prog* prog);
int prog_parse_args(struct Prog* prog, unsigned int argc, char** argv);
int prog_init(struct Prog* prog);

#endif
