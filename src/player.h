#ifndef PLAYER_H
#define PLAYER_H

#include <stdio.h>

#include "weiqi.h"
#include "interface.h"

struct Player {
    struct Weiqi* weiqi;
    enum WeiqiColor color;

    int (*init)(struct Player* player, struct Weiqi* weiqi, int color);
    int (*send_move)(struct Player* player,
                     enum WeiqiColor color, enum MoveAction action,
                     unsigned char row, unsigned char col);
    int (*get_move)(struct Player* player,
                    enum WeiqiColor color, enum MoveAction* action,
                    unsigned char* row, unsigned char* col);
    int (*reset)(struct Player* player);
    int (*undo)(struct Player* player);
    void (*free)(struct Player* player);

    void* data;
};


int player_gtp_pipe_init(struct Player* player, const char* cmd);
int player_gtp_socket_init(struct Player* player, const char* cmd);
int player_human_init(struct Player* player, struct Interface* ui);

#endif
