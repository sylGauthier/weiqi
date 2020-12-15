#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "weiqi.h"
#include "list.h"

#define VERTICE_ACCESS(w, b, r, c) (w)->b[(r) * (w)->boardSize + (c)]
#define VERTICE_COLOR(w, r, c) VERTICE_ACCESS(w, board, r, c)
#define VERTICE_TMP(w, r, c) VERTICE_ACCESS(w, tmpBoard, r, c)

static void setup_handicap(struct Weiqi* weiqi) {
    char h = weiqi->handicap;
    char s = weiqi->boardSize - 1;
    char ed;

    if (h < 2) return;
    ed = weiqi->boardSize <= 12 ? 2 : 3;
    weiqi_register_move(weiqi, W_BLACK, W_PLAY, ed, ed);
    weiqi_register_move(weiqi, W_BLACK, W_PLAY, s - ed, s - ed);
    if (h >= 3)
        weiqi_register_move(weiqi, W_BLACK, W_PLAY, s - ed, ed);
    if (h >= 4)
        weiqi_register_move(weiqi, W_BLACK, W_PLAY, ed, s - ed);
    if (h % 2 && h >= 5)
        weiqi_register_move(weiqi, W_BLACK, W_PLAY, s / 2, s / 2);
    if (h >= 6) {
        weiqi_register_move(weiqi, W_BLACK, W_PLAY, s / 2, ed);
        weiqi_register_move(weiqi, W_BLACK, W_PLAY, s / 2, s - ed);
    }
    if (h >= 8) {
        weiqi_register_move(weiqi, W_BLACK, W_PLAY, ed, s / 2);
        weiqi_register_move(weiqi, W_BLACK, W_PLAY, s - ed, s / 2);
    }
}

int weiqi_init(struct Weiqi* weiqi, char s, char h) {
    char maxHandicap;

    weiqi->board = NULL;
    weiqi->tmpBoard = NULL;
    weiqi->history.first = NULL;
    weiqi->history.last = NULL;
    weiqi->gameOver = 0;
    weiqi->wcap = 0;
    weiqi->bcap = 0;
    if (s == 7) maxHandicap = 4;
    else maxHandicap = 4 + (s % 2) * 5;

    if (s < 7 || s > 25) {
        fprintf(stderr, "Error: board size must be between 7 and 25\n");
    } else if (h > maxHandicap) {
        fprintf(stderr, "Error: handicap > max handicap (%d)\n", maxHandicap);
    } else if (!(weiqi->board = calloc(s * s * sizeof(char), 1))
            || !(weiqi->tmpBoard = calloc(s * s * sizeof(char), 1))) {
        fprintf(stderr, "Error: can't allocate memory for board\n");
    } else {
        weiqi->boardSize = s;
        weiqi->handicap = h;
        setup_handicap(weiqi);
        return 1;
    }
    free(weiqi->board);
    free(weiqi->tmpBoard);
    return 0;
}

static void free_history(struct Move* history) {
    while (history) {
        void* tmp = history->prev;
        free(history);
        history = tmp;
    }
}

void weiqi_free(struct Weiqi* weiqi) {
    free(weiqi->board);
    free(weiqi->tmpBoard);
    free_history(weiqi->history.last);
}

static int stack_add_unvisited(struct Weiqi* w, struct StoneList** stack,
                               unsigned char r, unsigned char c) {
    enum WeiqiColor color = VERTICE_COLOR(w, r, c);
    unsigned char s = w->boardSize - 1;


    VERTICE_TMP(w, r, c) = 2;
    if (r && VERTICE_COLOR(w, r - 1, c) == color
          && VERTICE_TMP(w, r - 1, c) != 2) {
        if (!list_push(stack, r - 1, c)) return 0;
        VERTICE_TMP(w, r - 1, c) = 2;
    }
    if (r < s && VERTICE_COLOR(w, r + 1, c) == color
              && VERTICE_TMP(w, r + 1, c) != 2) {
        if (!list_push(stack, r + 1, c)) return 0;
        VERTICE_TMP(w, r + 1, c) = 2;
    }
    if (c && VERTICE_COLOR(w, r, c - 1) == color
          && VERTICE_TMP(w, r, c - 1) != 2) {
        if (!list_push(stack, r, c - 1)) return 0;
        VERTICE_TMP(w, r, c - 1) = 2;
    }
    if (c < s && VERTICE_COLOR(w, r, c + 1) == color
              && VERTICE_TMP(w, r, c + 1) != 2) {
        if (!list_push(stack, r, c + 1)) return 0;
        VERTICE_TMP(w, r, c + 1) = 2;
    }
    return 1;
}

static int count_liberties(struct Weiqi* w,
                           unsigned char row, unsigned char col) {
    int count = 0;
    struct StoneList* stack = NULL;
    enum WeiqiColor color = VERTICE_COLOR(w, row, col);

    if (!color) return 0;
    /* reset the liberty mask */
    memset(w->tmpBoard, 0, w->boardSize * w->boardSize);
    if (!list_push(&stack, row, col)) return -1;

    /* for all stones in the group */
    while (stack) {
        unsigned char r, c, s = w->boardSize - 1;

        list_pop(&stack, &r, &c);
        /* we check neighbours, if empty and not already counted (not masked)
         * we increment count and we mask it
         */
        if (r && !VERTICE_COLOR(w, r - 1, c) && !VERTICE_TMP(w, r - 1, c)) {
            count++;
            VERTICE_TMP(w, r - 1, c) = 1;
        }
        if (r < s && !VERTICE_COLOR(w, r + 1, c) && !VERTICE_TMP(w, r + 1, c)) {
            count++;
            VERTICE_TMP(w, r + 1, c) = 1;
        }
        if (c && !VERTICE_COLOR(w, r, c - 1) && !VERTICE_TMP(w, r, c - 1)) {
            count++;
            VERTICE_TMP(w, r, c - 1) = 1;
        }
        if (c < s && !VERTICE_COLOR(w, r, c + 1) && !VERTICE_TMP(w, r, c + 1)) {
            count++;
            VERTICE_TMP(w, r, c + 1) = 1;
        }
        if (!stack_add_unvisited(w, &stack, r, c)) {
            count = -1;
            break;
        }
    }

    list_flush(&stack);
    return count;
}

struct Pos {
    unsigned char row, col;
};

static void get_neighbours(struct Weiqi* weiqi, enum WeiqiColor color,
                           unsigned char row, unsigned char col,
                           struct Pos* friends, unsigned int* numFriends,
                           struct Pos* enemies, unsigned int* numEnemies,
                           unsigned int* numVert) {
    *numFriends = 0;
    *numEnemies = 0;
    *numVert = 0;
    if (row) {
        (*numVert)++;
        if (VERTICE_COLOR(weiqi, row - 1, col) == color) {
            friends[(*numFriends)].row = row - 1;
            friends[(*numFriends)++].col = col;
        } else if (VERTICE_COLOR(weiqi, row - 1, col)) {
            enemies[(*numEnemies)].row = row - 1;
            enemies[(*numEnemies)++].col = col;
        }
    }
    if (row < weiqi->boardSize - 1) {
        (*numVert)++;
        if (VERTICE_COLOR(weiqi, row + 1, col) == color) {
            friends[(*numFriends)].row = row + 1;
            friends[(*numFriends)++].col = col;
        } else if (VERTICE_COLOR(weiqi, row + 1, col)) {
            enemies[(*numEnemies)].row = row + 1;
            enemies[(*numEnemies)++].col = col;
        }
    }
    if (col) {
        (*numVert)++;
        if (VERTICE_COLOR(weiqi, row, col - 1) == color) {
            friends[(*numFriends)].row = row;
            friends[(*numFriends)++].col = col - 1;
        } else if (VERTICE_COLOR(weiqi, row, col - 1)) {
            enemies[(*numEnemies)].row = row;
            enemies[(*numEnemies)++].col = col - 1;
        }
    }
    if (col < weiqi->boardSize - 1) {
        (*numVert)++;
        if (VERTICE_COLOR(weiqi, row, col + 1) == color) {
            friends[(*numFriends)].row = row;
            friends[(*numFriends)++].col = col + 1;
        } else if (VERTICE_COLOR(weiqi, row, col + 1)) {
            enemies[(*numEnemies)].row = row;
            enemies[(*numEnemies)++].col = col + 1;
        }
    }
}

static int history_push(struct History* hist,
                        enum WeiqiColor color, enum MoveAction action,
                        unsigned char row, unsigned char col) {
    struct Move* new;

    if (!(new = malloc(sizeof(*new)))) {
        fprintf(stderr, "Error: can't create new history entry\n");
        return 0;
    }
    new->color = color;
    new->action = action;
    new->row = row;
    new->col = col;
    new->numCaptures = 0;
    new->next = NULL;
    new->prev = hist->last;
    if (new->prev) new->nmove = new->prev->nmove + 1;
    else new->nmove = 0;
    if (hist->last) hist->last->next = new;
    hist->last = new;
    if (!hist->first) hist->first = new;
    return 1;
}

static int del_group(struct Weiqi* w,
                     unsigned char row, unsigned char col) {
    struct Move* lastMove = w->history.last;
    struct StoneList* stack = NULL;
    unsigned int max = sizeof(lastMove->captures) / 2;

    if (VERTICE_COLOR(w, row, col) == W_EMPTY) return 1;
    memset(w->tmpBoard, 0, w->boardSize * w->boardSize);
    if (!list_push(&stack, row, col)) return 0;

    while (stack) {
        unsigned char r, c;
        list_pop(&stack, &r, &c);
        if (!stack_add_unvisited(w, &stack, r, c)) return 0;
        VERTICE_COLOR(w, r, c) = W_EMPTY;
        /* store capture list into the capturing move for backtracking */
        if (lastMove && lastMove->numCaptures < max) {
            lastMove->captures[lastMove->numCaptures][0] = r;
            lastMove->captures[lastMove->numCaptures][1] = c;
            lastMove->numCaptures++;
        }
    }
    return 1;
}

static int register_move(struct Weiqi* weiqi,
                         enum WeiqiColor color,
                         unsigned char row, unsigned char col) {
    struct Pos friends[4] = {0}, enemies[4] = {0};
    unsigned int numFriends = 0, numEnemies = 0, numVert = 0;
    unsigned int i;

    get_neighbours(weiqi, color, row, col, friends, &numFriends,
                   enemies, &numEnemies, &numVert);

    /* push into history, this allocates the new Move */
    if (!history_push(&weiqi->history, color, W_PLAY, row, col))
        return W_ERROR;

    /* set the stone on the vertex */
    VERTICE_COLOR(weiqi, row, col) = color;

    /* and kill neighbouring enemies if 0 liberty left */
    for (i = 0; i < numEnemies; i++) {
        int libs = count_liberties(weiqi, enemies[i].row, enemies[i].col);
        if (libs < 0) return W_ERROR;
        else if (libs == 0) {
            if (!del_group(weiqi, enemies[i].row, enemies[i].col)) {
                return W_ERROR;
            }
        }
    }

    /* update new history entry with number of stones */
    if (weiqi->history.last->prev) {
        weiqi->history.last->nstones = weiqi->history.last->prev->nstones
            + 1 - weiqi->history.last->numCaptures;
    } else {
        weiqi->history.last->nstones = 1;
    }

    switch (color) {
        case W_WHITE:
            weiqi->bcap += weiqi->history.last->numCaptures;
            break;
        case W_BLACK:
            weiqi->wcap += weiqi->history.last->numCaptures;
            break;
        default:
            break;
    }
    return W_NO_ERROR;
}

static void tmp_prev_board_state(struct Weiqi* weiqi, struct Move* move) {
    unsigned int i;

    VERTICE_TMP(weiqi, move->row, move->col) = W_EMPTY;
    for (i = 0; i < move->numCaptures; i++) {
        VERTICE_TMP(weiqi, move->captures[i][0], move->captures[i][1]) =
                    move->color == W_WHITE ? W_BLACK : W_WHITE;
    }
}

static int comp_prev_state(struct Weiqi* weiqi, struct Move* cur) {
    struct Move* last = weiqi->history.last;
    unsigned int i;
    if (cur->nstones != last->nstones) return 0;
    for (i = 0; i < weiqi->boardSize * weiqi->boardSize; i++) {
        if (weiqi->board[i] != weiqi->tmpBoard[i]) return 0;
    }
    return 1;
}

static int superko(struct Weiqi* weiqi, enum WeiqiColor color,
                   unsigned char row, unsigned char col) {
    enum WeiqiError err;
    struct Move* cur;
    int res = W_NO_ERROR;

    err = register_move(weiqi, color, row, col);
    if (err != W_NO_ERROR) return err;

    cur = weiqi->history.last;
    if (cur->prev && cur->prev->prev) {
        memcpy(weiqi->tmpBoard, weiqi->board, weiqi->boardSize * weiqi->boardSize);

        tmp_prev_board_state(weiqi, cur);
        cur = cur->prev;
        while (cur) {
            if (cur->nmove + 2 <= weiqi->history.last->nstones) {
                break;
            }
            if (comp_prev_state(weiqi, cur)) {
                res = W_ILLEGAL_MOVE;
                break;
            }
            tmp_prev_board_state(weiqi, cur);
            cur = cur->prev;
        }
    }
    weiqi_undo_move(weiqi);
    return res;
}

int weiqi_move_is_valid(struct Weiqi* weiqi, enum WeiqiColor color,
                        unsigned char row, unsigned char col) {
    struct Pos friends[4], enemies[4];
    unsigned int numFriends, numEnemies, numVert, i;
    char ennemyCaptured = 0, selfAlive = 0;

    get_neighbours(weiqi, color, row, col, friends, &numFriends,
                   enemies, &numEnemies, &numVert);

    if (VERTICE_COLOR(weiqi, row, col)) return W_ILLEGAL_MOVE;

    /* If the vertice is surrounded, more checks to be sure it's legal */
    if (numEnemies + numFriends == numVert) {
        /* If one of the ennemy groups has just one lib, gonna get captured */
        for (i = 0; i < numEnemies; i++) {
            int libs = count_liberties(weiqi, enemies[i].row, enemies[i].col);
            if (libs < 0) return W_ERROR;
            else if (libs == 1) {
                ennemyCaptured = 1;
                break;
            }
        }
        if (!ennemyCaptured) {
            /* If one of our groups has 2+ libs, we can play here */
            for (i = 0; i < numFriends; i++) {
                int libs = count_liberties(weiqi, friends[i].row, friends[i].col);
                if (libs < 0) return W_ERROR;
                else if (libs >= 2) {
                    selfAlive = 1;
                    break;
                }
            }
            if (!selfAlive) return W_ILLEGAL_MOVE;
        }
    }

    return superko(weiqi, color, row, col);
}

int weiqi_register_move(struct Weiqi* weiqi,
                        enum WeiqiColor color, enum MoveAction action,
                        unsigned char row, unsigned char col) {
    enum WeiqiError err;

    /* we deal with the PASS case first */
    if (action == W_PASS) {
        struct Move* last = weiqi->history.last;
        if (!history_push(&weiqi->history, color, W_PASS, 0, 0)) {
            return W_ERROR;
        }
        if (last && last->action == W_PASS) {
            weiqi->gameOver = 1;
            return W_GAME_OVER;
        }
        return W_NO_ERROR;
    }

    /* check that move is valid first */
    err = weiqi_move_is_valid(weiqi, color, row, col);
    if (err != W_NO_ERROR) return err;

    return register_move(weiqi, color, row, col);
}

void weiqi_undo_move(struct Weiqi* weiqi) {
    struct Move* mv = weiqi->history.last;

    if (mv) {
        unsigned int i;
        VERTICE_COLOR(weiqi, mv->row, mv->col) = W_EMPTY;
        for (i = 0; i < mv->numCaptures; i++) {
            VERTICE_COLOR(weiqi, mv->captures[i][0], mv->captures[i][1]) =
                mv->color == W_WHITE ? W_BLACK : W_WHITE;
        }
        switch (mv->color) {
            case W_WHITE:
                weiqi->bcap -= mv->numCaptures;
                break;
            case W_BLACK:
                weiqi->wcap -= mv->numCaptures;
                break;
            default:
                break;
        }
        weiqi->history.last = mv->prev;
        if (mv->prev) weiqi->history.last->next = NULL;
        free(mv);
    }
}
