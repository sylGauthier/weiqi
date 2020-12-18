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

struct GameClient {
    struct Weiqi weiqi;
    struct Player player;
    struct Interface ui;

    FILE *in, *out;
    const char* socketPath;
};

int game_init(struct GameContext* ctx, char boardSize, char handicap);

int game_load_file(struct GameContext* ctx, const char* f);

void game_free(struct GameContext* ctx);

int game_run(struct GameContext* ctx);

int game_client_init(struct GameClient* client, const char* socketPath);
int game_client_run(struct GameClient* client);
void game_client_free(struct GameClient* client);

#endif
