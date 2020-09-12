#include <stdio.h>
#include <stdlib.h>

#include "weiqi.h"

int weiqi_init(struct Weiqi* weiqi, char boardSize) {
    if (boardSize < 7 || boardSize > 25) {
        fprintf(stderr, "Error: board size must be between 7 and 25\n");
        return 0;
    }
    if (!(weiqi->board = calloc(boardSize * boardSize * sizeof(char), 1))) {
        fprintf(stderr, "Error: can't allocate memory for board\n");
        return 0;
    }
    weiqi->boardSize = boardSize;
    return 1;
}

void weiqi_free(struct Weiqi* weiqi) {
    free(weiqi->board);
}

int weiqi_move_is_valid(struct Weiqi* weiqi, enum WeiqiColor color,
                        unsigned int row, unsigned int col) {
    return row < weiqi->boardSize
        && col < weiqi->boardSize
        && !weiqi->board[row * weiqi->boardSize + col];
}
