#include <string.h>

#include "human.h"

int human_init(struct Player* player, struct Interface* ui) {
    player->data = ui;
    player->send_move = human_send_move;
    player->get_move = human_get_move;
    player->reset = human_reset;
    player->undo = human_undo;
    player->free = NULL;
    return 1;
}

int human_send_move(struct Player* player,
                    enum WeiqiColor color, enum MoveAction action,
                    unsigned char row, unsigned char col) {
    return W_NO_ERROR;
}

int human_get_move(struct Player* player,
                   enum WeiqiColor color, enum MoveAction* action,
                   unsigned char* row, unsigned char* col) {
    char ok = 0, status = 0;

    while (!ok) {
        status = interface_get_move(player->data, color, action, row, col);
        if (status == W_UI_QUIT) {
            return W_QUIT;
        } else if (status == W_UI_UNDO) {
            return W_UNDO_MOVE;
        } else if (*action == W_PASS) {
            return W_NO_ERROR;
        } else if (weiqi_move_is_valid(player->weiqi, color, *row, *col)
                != W_NO_ERROR) {
            fprintf(stderr, "invalid move\n");
        } else {
            ok = 1;
        }
    }
    return W_NO_ERROR;
}

int human_undo(struct Player* player) {
    return 1;
}

int human_reset(struct Player* player) {
    return 1;
}
