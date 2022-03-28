#ifndef WEIQI_H
#define WEIQI_H

enum WeiqiError {
    W_NO_ERROR,
    W_ERROR,
    W_ILLEGAL_MOVE,
    W_UNDO_ERROR,
    W_FORMAT_ERROR,
    W_GAME_OVER,
    W_QUIT
};

enum WeiqiColor {
    W_EMPTY = 0,
    W_BLACK = 1,
    W_WHITE = 2
};

enum MoveAction {
    W_PLAY,
    W_PASS,
    W_UNDO
};

struct Move {
    enum WeiqiColor color;
    enum MoveAction action;
    unsigned char row, col;
    unsigned int nmove, nstones;

    unsigned char captures[256][2];
    unsigned int numCaptures;

    struct Move* prev;
    struct Move* next;
};

struct History {
    struct Move* first;
    struct Move* last;
};

struct Weiqi {
    unsigned char boardSize, handicap;
    char* board;
    char* tmpBoard;
    struct History history;
    char gameOver;

    unsigned int wcap;  /* stones captured by white */
    unsigned int bcap;  /* stones captured by black */
};

int weiqi_init(struct Weiqi* wq, char boardSize, char handicap);
void weiqi_free(struct Weiqi* wq);

int weiqi_move_is_valid(struct Weiqi* weiqi, enum WeiqiColor color,
                        unsigned char row, unsigned char col);

int weiqi_register_move(struct Weiqi* weiqi,
                        enum WeiqiColor color, enum MoveAction action,
                        unsigned char row, unsigned char col);

void weiqi_undo_move(struct Weiqi* weiqi);

int weiqi_move_to_str(char dest[8],
                      enum MoveAction action,
                      unsigned char row,
                      unsigned char col);
int weiqi_str_to_move(enum MoveAction* action,
                      unsigned char* row,
                      unsigned char* col,
                      const char* str);
#endif
