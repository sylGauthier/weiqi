#ifndef WEIQI_H
#define WEIQI_H

enum WeiqiError {
    W_NO_ERROR,
    W_ILLEGAL_MOVE,
    W_FORMAT_ERROR
};

enum WeiqiColor {
    W_EMPTY = 0,
    W_BLACK = 1,
    W_WHITE = 2
};

enum MoveAction {
    W_PLAY,
    W_PASS
};

struct Move {
    enum WeiqiColor color;
    enum MoveAction action;
    unsigned int row, col;

    struct Move* prev;
    struct Move* next;
};

struct Weiqi {
    char boardSize, handicap;
    char* board;
    char* liberties;
    struct StoneList** clusters;
    struct Move* history;

    unsigned int wcap;  /* stones captured by white */
    unsigned int bcap;  /* stones captured by black */
};

int weiqi_init(struct Weiqi* wq, char boardSize, char handicap);
void weiqi_free(struct Weiqi* wq);

int weiqi_move_is_valid(struct Weiqi* weiqi, enum WeiqiColor color,
                        unsigned int row, unsigned int col);

int weiqi_register_move(struct Weiqi* weiqi, enum WeiqiColor color,
                        unsigned int row, unsigned int col);

#endif
