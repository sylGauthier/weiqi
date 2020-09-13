#ifndef PLAYER_H
#define PLAYER_H

#include <stdio.h>

#include "weiqi.h"

struct Player {
    FILE *in, *out;
    struct Weiqi* weiqi;
    void* data;

    int (*send_move)(struct Player* player, enum WeiqiColor color,
                     unsigned int row, unsigned int col);
    int (*get_move)(struct Player* player, enum WeiqiColor color,
                    unsigned int* row, unsigned int* col);
    int (*reset)(struct Player* player);
};

#endif
