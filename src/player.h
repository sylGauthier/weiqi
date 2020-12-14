#ifndef PLAYER_H
#define PLAYER_H

#include <stdio.h>

#include "weiqi.h"

struct Player {
    struct Weiqi* weiqi;

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

#endif
