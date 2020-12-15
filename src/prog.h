#ifndef PROG_H
#define PROG_H

#include "game.h"

#define W_NUM_ENGINES 32

enum PlayerType {
    W_HUMAN,
    W_GTP_LOCAL
};

struct Engine {
    char* name;
    char* command;
};

struct PlayerConf {
    enum PlayerType type;
    char* gtpCmd;
};

struct Prog {
    unsigned char boardSize;
    unsigned char handicap;
    struct PlayerConf white;
    struct PlayerConf black;

    const char* gtpCmd;
    const char* gameFile;

    struct GameContext ctx;
    struct Engine engines[W_NUM_ENGINES];
    unsigned int numEngines;
};

char* config_find_engine(struct Prog* prog, const char* name);
int config_load_player(struct Prog* prog, struct PlayerConf* c, const char* p);
int config_rand_assign(struct Prog* prog, const char* p1, const char* p2);
int prog_load_defaults(struct Prog* prog);
int prog_load_config(struct Prog* prog);
int prog_parse_args(struct Prog* prog, unsigned int argc, char** argv);
int prog_init(struct Prog* prog);
void prog_free(struct Prog* prog);

#endif
