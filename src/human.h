#ifndef HUMAN_H
#define HUMAN_H

#include "player.h"

int human_init(struct Player* player);

int human_send_move(struct Player* player, enum WeiqiColor color,
                    unsigned int row, unsigned int col);
int human_get_move(struct Player* player, enum WeiqiColor color,
                   unsigned int* row, unsigned int* col);

#endif
