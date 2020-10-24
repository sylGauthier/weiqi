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

int game_init(struct GameContext* ctx, enum InterfaceTheme theme,
              char boardSize, char handicap);

int game_load_file(struct GameContext* ctx, enum InterfaceTheme theme,
                   const char* f);

void game_free(struct GameContext* ctx);

int game_run(struct GameContext* ctx);

#endif
