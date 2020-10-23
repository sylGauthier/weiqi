#ifndef GTP_H
#define GTP_H

#include "player.h"

int gtp_init(struct Player* player, FILE* in, FILE* out);

int gtp_local_engine_init(struct Player* player, const char* cmd);

int gtp_send_move(struct Player* player,
                  enum WeiqiColor color, enum MoveAction action,
                  unsigned int row, unsigned int col);
int gtp_get_move(struct Player* player,
                 enum WeiqiColor color, enum MoveAction* action,
                 unsigned int* row, unsigned int* col);

int gtp_reset(struct Player* player);
void gtp_free(struct Player* player);

#endif
