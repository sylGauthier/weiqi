#include <string.h>

#include "human.h"
#include "utils.h"

int human_init(struct Player* player) {
    player->in = NULL;
    player->out = NULL;
    player->send_move = human_send_move;
    player->get_move = human_get_move;
    return 1;
}

int human_send_move(struct Player* player, enum WeiqiColor color,
                    unsigned int row, unsigned int col) {
    char move[4] = {0};

    move_to_str(move, row, col);
    printf("%s: %s\n", color == W_WHITE ? "white" : "black", move);
    return W_NO_ERROR;
}

int human_get_move(struct Player* player, enum WeiqiColor color,
                   unsigned int* row, unsigned int* col) {
    char buf[8];
    char ok = 0;

    while (!ok) {
        printf("%s: ", color == W_WHITE ? "white" : "black");
        if (!fgets(buf, sizeof(buf), stdin)) return 0;
        buf[strlen(buf) - 1] = '\0';
        if (str_to_move(row, col, buf)) {
            if (!weiqi_move_is_valid(player->weiqi, color, *row, *col)) {
                fprintf(stderr, "invalid move\n");
            } else {
                ok = 1;
            }
        }
    }
    return ok;
}
