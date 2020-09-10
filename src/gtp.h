#ifndef GTP_H
#define GTP_H

#include "weiqi.h"

int gtp_init(struct Player* player, FILE* in, FILE* out);

int gtp_send_move(struct Player* player, enum WeiqiColor color,
                  unsigned int row, unsigned int col);
int gtp_get_move(struct Player* player, enum WeiqiColor color,
                 unsigned int* row, unsigned int* col);

#endif
