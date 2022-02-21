#ifndef GAME_H
#define GAME_H

#include "player.h"
#include "ui.h"

struct GameServer {
    struct Player white;
    struct Player black;

    struct UI* ui;
    struct Weiqi* weiqi;
};

struct GameClient {
    struct Player player;
    FILE *in, *out;
    const char* socketPath;

    struct UI* ui;
    struct Weiqi* weiqi;
};

int game_server_init(struct GameServer* srv,
                     struct UI* ui,
                     struct Weiqi* weiqi,
                     struct Config* config);
int game_server_run(struct GameServer* srv);
void game_server_free(struct GameServer* srv);

int game_client_init(struct GameClient* client,
                     struct UI* ui,
                     struct Weiqi* weiqi,
                     struct Config* config);
int game_client_run(struct GameClient* client);
void game_client_free(struct GameClient* client);

#endif
