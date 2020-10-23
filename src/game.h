#ifndef GAME_H
#define GAME_H

#include "player.h"
#include "interface.h"

struct GameContext {
    struct Weiqi weiqi;
    struct Player white;
    struct Player black;
    struct Interface ui;
};

int game_init(struct GameContext* ctx, char boardSize, char handicap);
void game_free(struct GameContext* ctx);

int game_run(struct GameContext* ctx);
int game_load_file(struct GameContext* ctx, const char* f);

#endif
