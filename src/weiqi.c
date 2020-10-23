#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "weiqi.h"
#include "list.h"

#define VERTICE_ACCESS(w, b, r, c) (w)->b[(r) * (w)->boardSize + (c)]
#define VERTICE_COLOR(w, r, c) VERTICE_ACCESS(w, board, r, c)
#define VERTICE_CLUSTER(w, r, c) VERTICE_ACCESS(w, clusters, r, c)
#define VERTICE_LIB(w, r, c) VERTICE_ACCESS(w, liberties, r, c)

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
    weiqi->liberties = NULL;
    weiqi->clusters = NULL;
    weiqi->history.first = NULL;
    weiqi->history.last = NULL;
    if (s == 7) maxHandicap = 4;
    else maxHandicap = 4 + (s % 2) * 5;

    if (s < 7 || s > 25) {
        fprintf(stderr, "Error: board size must be between 7 and 25\n");
    } else if (h > maxHandicap) {
        fprintf(stderr, "Error: handicap > max handicap (%d)\n", maxHandicap);
    } else if (!(weiqi->board = calloc(s * s * sizeof(char), 1))
            || !(weiqi->liberties = calloc(s * s * sizeof(char), 1))
            || !(weiqi->clusters = calloc(s * s * sizeof(void*), 1))) {
        fprintf(stderr, "Error: can't allocate memory for board\n");
    } else {
        weiqi->boardSize = s;
        weiqi->handicap = h;
        setup_handicap(weiqi);
        return 1;
    }
    free(weiqi->board);
    free(weiqi->liberties);
    free(weiqi->clusters);
    return 0;
}

static void free_cluster(struct Weiqi* weiqi, struct StoneList* cluster) {
    while (cluster) {
        struct StoneList* tmp = cluster->next;
        VERTICE_CLUSTER(weiqi, cluster->row, cluster->col) = NULL;
        free(cluster);
        cluster = tmp;
    }
}

static void free_clusters(struct Weiqi* weiqi) {
    unsigned int row, col;

    for (row = 0; row < weiqi->boardSize; row++) {
        for (col = 0; col < weiqi->boardSize; col++) {
            struct StoneList* cur = VERTICE_CLUSTER(weiqi, row, col);

            free_cluster(weiqi, cur);
        }
    }
}

static void free_history(struct Move* history) {
    while (history) {
        void* tmp = history->prev;
        free(history);
        history = tmp;
    }
}

void weiqi_free(struct Weiqi* weiqi) {
    free_clusters(weiqi);

    free(weiqi->board);
    free(weiqi->liberties);
    free(weiqi->clusters);
    free_history(weiqi->history.last);
}

static int count_liberties(struct Weiqi* w, struct StoneList* cluster) {
    int count = 0;

    /* reset the liberty mask */
    memset(w->liberties, 0, w->boardSize * w->boardSize);

    /* for all stones in the cluster */
    while (cluster) {
        unsigned int r = cluster->row, c = cluster->col, s = w->boardSize - 1;

        /* we check neighbours, if empty and not already counted (not masked)
         * we increment count and we mask it
         */
        if (r && !VERTICE_COLOR(w, r - 1, c) && !VERTICE_LIB(w, r - 1, c)) {
            count++;
            VERTICE_LIB(w, r - 1, c) = 1;
        }
        if (r < s && !VERTICE_COLOR(w, r + 1, c) && !VERTICE_LIB(w, r + 1, c)) {
            count++;
            VERTICE_LIB(w, r + 1, c) = 1;
        }
        if (c && !VERTICE_COLOR(w, r, c - 1) && !VERTICE_LIB(w, r, c - 1)) {
            count++;
            VERTICE_LIB(w, r, c - 1) = 1;
        }
        if (c < s && !VERTICE_COLOR(w, r, c + 1) && !VERTICE_LIB(w, r, c + 1)) {
            count++;
            VERTICE_LIB(w, r, c + 1) = 1;
        }
        cluster = cluster->next;
    }
    return count;
}

static void get_clusters(struct Weiqi* weiqi, enum WeiqiColor color,
                         unsigned int row, unsigned int col,
                         struct StoneList** friends, unsigned int* numFriends,
                         struct StoneList** enemies, unsigned int* numEnemies,
                         unsigned int* numVert) {
    *numFriends = 0;
    *numEnemies = 0;
    *numVert = 0;
    if (row) {
        (*numVert)++;
        if (VERTICE_COLOR(weiqi, row - 1, col) == color) {
            friends[(*numFriends)++] = VERTICE_CLUSTER(weiqi, row - 1, col);
        } else if (VERTICE_COLOR(weiqi, row - 1, col)) {
            enemies[(*numEnemies)++] = VERTICE_CLUSTER(weiqi, row - 1, col);
        }
    }
    if (row < weiqi->boardSize - 1) {
        (*numVert)++;
        if (VERTICE_COLOR(weiqi, row + 1, col) == color) {
            friends[(*numFriends)++] = VERTICE_CLUSTER(weiqi, row + 1, col);
        } else if (VERTICE_COLOR(weiqi, row + 1, col)) {
            enemies[(*numEnemies)++] = VERTICE_CLUSTER(weiqi, row + 1, col);
        }
    }
    if (col) {
        (*numVert)++;
        if (VERTICE_COLOR(weiqi, row, col - 1) == color) {
            friends[(*numFriends)++] = VERTICE_CLUSTER(weiqi, row, col - 1);
        } else if (VERTICE_COLOR(weiqi, row, col - 1)) {
            enemies[(*numEnemies)++] = VERTICE_CLUSTER(weiqi, row, col - 1);
        }
    }
    if (col < weiqi->boardSize - 1) {
        (*numVert)++;
        if (VERTICE_COLOR(weiqi, row, col + 1) == color) {
            friends[(*numFriends)++] = VERTICE_CLUSTER(weiqi, row, col + 1);
        } else if (VERTICE_COLOR(weiqi, row, col + 1)) {
            enemies[(*numEnemies)++] = VERTICE_CLUSTER(weiqi, row, col + 1);
        }
    }
}

static int move_valid(struct Weiqi* weiqi, enum WeiqiColor color,
                      unsigned int row, unsigned int col,
                      struct StoneList** friends, unsigned int numFriends,
                      struct StoneList** enemies, unsigned int numEnemies,
                      unsigned int numVert) {
    char ennemyCaptured = 0, selfAlive = 0;
    unsigned int i;

    if (VERTICE_COLOR(weiqi, row, col)) return 0;

    /* If the vertice is surrounded, more checks to be sure it's legal */
    if (numEnemies + numFriends == numVert) {
        /* If one of the ennemy clusters has just one lib, gonna get captured */
        for (i = 0; i < numEnemies; i++) {
            if (count_liberties(weiqi, enemies[i]) == 1) {
                ennemyCaptured = 1;
                break;
            }
        }
        if (!ennemyCaptured) {
            /* If one of our clusters has 2+ libs, we can play here */
            for (i = 0; i < numFriends; i++) {
                if (count_liberties(weiqi, friends[i]) >= 2) {
                    selfAlive = 1;
                    break;
                }
            }
            if (!selfAlive) return 0;
        }
    }
    return 1;
}

int weiqi_move_is_valid(struct Weiqi* weiqi, enum WeiqiColor color,
                        unsigned int row, unsigned int col) {
    struct StoneList *friends[4], *enemies[4];
    unsigned int numFriends, numEnemies, numVert;

    get_clusters(weiqi, color, row, col, friends, &numFriends,
                 enemies, &numEnemies, &numVert);
    return move_valid(weiqi, color, row, col, friends, numFriends,
                      enemies, numEnemies, numVert);
}

static void merge_cluster(struct Weiqi* weiqi, struct StoneList* dest,
                          struct StoneList* cluster) {
    struct StoneList *tail = dest;

    while (tail->next) {
        tail = tail->next;
    }
    tail->next = cluster;

    for (; cluster; cluster = cluster->next) {
        VERTICE_CLUSTER(weiqi, cluster->row, cluster->col) = dest;
    }
}

static void del_cluster(struct Weiqi* weiqi, struct StoneList* cluster) {
    while (cluster) {
        struct StoneList* tmp = cluster->next;
        VERTICE_COLOR(weiqi, cluster->row, cluster->col) = W_EMPTY;
        VERTICE_CLUSTER(weiqi, cluster->row, cluster->col) = NULL;
        free(cluster);
        cluster = tmp;
    }
}

static int history_push(struct History* hist,
                        enum WeiqiColor color, enum MoveAction action,
                        unsigned int row, unsigned int col) {
    struct Move* new;

    if (!(new = malloc(sizeof(*new)))) {
        fprintf(stderr, "Error: can't create new history entry\n");
        return 0;
    }
    new->color = color;
    new->action = action;
    new->row = row;
    new->col = col;
    new->next = NULL;
    new->prev = hist->last;
    if (hist->last) hist->last->next = new;
    hist->last = new;
    if (!hist->first) hist->first = new;
    return 1;
}

int weiqi_register_move(struct Weiqi* weiqi,
                        enum WeiqiColor color, enum MoveAction action,
                        unsigned int row, unsigned int col) {
    struct StoneList *friends[4] = {NULL}, *enemies[4] = {NULL}, *new = NULL;
    unsigned int numFriends = 0, numEnemies = 0, numVert = 0, i;

    if (action == W_PASS) {
        struct Move* last = weiqi->history.last;
        if (!history_push(&weiqi->history, color, W_PASS, 0, 0)) {
            return W_ERROR;
        }
        if (last && last->action == W_PASS) return W_GAME_OVER;
        return W_NO_ERROR;
    }

    get_clusters(weiqi, color, row, col, friends, &numFriends,
                 enemies, &numEnemies, &numVert);
    if (!move_valid(weiqi, color, row, col, friends, numFriends,
                    enemies, numEnemies, numVert)) return 0;
    if (!(new = list_new())) {
        fprintf(stderr, "Error: can't create new stone list\n");
        return W_ERROR;
    }
    new->row = row;
    new->col = col;
    VERTICE_CLUSTER(weiqi, row, col) = new;
    VERTICE_COLOR(weiqi, row, col) = color;
    for (i = 0; i < numFriends; i++) {
        int merge = 1, j;
        /* if two friends are part of the same cluster, we only merge once */
        for (j = 0; j < i && merge; j++) {
            if (friends[j] == friends[i]) merge = 0;
        }
        if (merge) merge_cluster(weiqi, new, friends[i]);
    }
    for (i = 0; i < numEnemies; i++) {
        int delete = 1, j;
        /* if two enemies are part of the same cluster, we only delete once */
        for (j = 0; j < i && delete; j++) {
            if (enemies[j] == enemies[i]) delete = 0;
        }
        if (delete && count_liberties(weiqi, enemies[i]) == 0) {
            del_cluster(weiqi, enemies[i]);
        }
    }
    if (!history_push(&weiqi->history, color, W_PLAY, row, col))
        return W_ERROR;
    return W_NO_ERROR;
}
