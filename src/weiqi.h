#ifndef WEIQI_H
#define WEIQI_H

#include <stdio.h>

enum WeiqiError {
    W_NO_ERROR,
    W_ILLEGAL_MOVE,
    W_PASS,
    W_FORMAT_ERROR
};

enum WeiqiColor {
    W_EMPTY = 0,
    W_BLACK = 1,
    W_WHITE = 2
};

struct Weiqi {
    char boardSize;
    char* board;

    unsigned int wcap;  /* stones captured by white */
    unsigned int bcap;  /* stones captured by black */
};

struct Player {
    FILE *in, *out;
    struct Weiqi* weiqi;
    void* data;

    int (*send_move)(struct Player* player, enum WeiqiColor color,
                     unsigned int row, unsigned int col);
    int (*get_move)(struct Player* player, enum WeiqiColor color,
                    unsigned int* row, unsigned int* col);
};

struct GameContext {
    struct Weiqi weiqi;
    struct Player white;
    struct Player black;
};

int weiqi_init(struct GameContext* ctx, char boardSize);
int weiqi_run(struct GameContext* ctx);
void weiqi_free(struct GameContext* ctx);

int register_move(struct GameContext* ctx, enum WeiqiColor color,
                  unsigned int row, unsigned int col);
int move_is_valid(struct Weiqi* weiqi, enum WeiqiColor color,
                  unsigned int row, unsigned int col);

#endif
