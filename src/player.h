#ifndef PLAYER_H
#define PLAYER_H

#include <stdio.h>

#include "weiqi.h"

struct Player {
    struct Weiqi* weiqi;

    int (*send_move)(struct Player* player, enum WeiqiColor color,
                     unsigned int row, unsigned int col);
    int (*get_move)(struct Player* player, enum WeiqiColor color,
                    unsigned int* row, unsigned int* col);
    int (*reset)(struct Player* player);
    void (*free)(struct Player* player);

    void* data;
};

#endif
