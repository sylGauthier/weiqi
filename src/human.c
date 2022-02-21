#include <string.h>

#include "player.h"

static int human_send_move(struct Player* player,
                    enum WeiqiColor color, enum MoveAction action,
                    unsigned char row, unsigned char col) {
    return W_NO_ERROR;
}

static int human_get_move(struct Player* player,
                   enum WeiqiColor color, enum MoveAction* action,
                   unsigned char* row, unsigned char* col) {
    char ok = 0, status = 0;

    while (!ok) {
        status = ui_get_move(player->data, color, action, row, col);
        if (status == W_UI_QUIT) {
            return W_QUIT;
        } else if (status == W_UI_CRASH) {
            fprintf(stderr, "Error: UI crashed\n");
            return W_ERROR;
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

static int human_undo(struct Player* player) {
    return 1;
}

static int human_reset(struct Player* player) {
    return 1;
}

static int human_init(struct Player* player, struct Weiqi* weiqi, int c) {
    player->weiqi = weiqi;
    player->color = c;
    return 1;
}

static void human_free(struct Player* player) {
    return;
}

int player_human_init(struct Player* player, struct UI* ui) {
    player->data = ui;
    player->init = human_init;
    player->send_move = human_send_move;
    player->get_move = human_get_move;
    player->reset = human_reset;
    player->undo = human_undo;
    player->free = human_free;
    return 1;
}
