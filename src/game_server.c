#include <stdlib.h>
#include <string.h>

#include "game.h"
#include "utils.h"
#include "cmd.h"

int game_server_init(struct GameServer* srv,
                     struct UI* ui,
                     struct Weiqi* weiqi,
                     struct Config* config) {
    int binit = 0, winit = 0;

    srv->ui = ui;
    srv->weiqi = weiqi;
    if (config->boardSize < 5 || config->boardSize > 25) {
        fprintf(stderr, "Error: invalid board size\n");
        return 0;
    } else if (!(winit = player_init(&srv->white, &config->white, ui))
            || !(binit = player_init(&srv->black, &config->black, ui))) {
        fprintf(stderr, "Error: can't init a player from conf\n");;
    } else if (!srv->white.init(&srv->white, srv->weiqi, W_WHITE)
            || !srv->black.init(&srv->black, srv->weiqi, W_BLACK)) {
        fprintf(stderr, "Error: can't init a player\n");
    } else {
        return 1;
    }
    if (winit && srv->white.free) srv->white.free(&srv->white);
    if (binit && srv->black.free) srv->black.free(&srv->black);
    return 0;
}

void game_server_free(struct GameServer* srv) {
    if (srv->white.free) srv->white.free(&srv->white);
    if (srv->black.free) srv->black.free(&srv->black);
}

static int undo(struct GameServer* srv) {
    if (       srv->black.undo(&srv->black)
            && srv->white.undo(&srv->white)) {
        weiqi_undo_move(srv->weiqi);
        return 1;
    }
    return 0;
}

static int play_turn(struct GameServer* srv, enum WeiqiColor color) {
    struct Player *p1, *p2;
    unsigned char row, col;
    enum MoveAction action = W_PLAY;
    enum WeiqiError err;

    if (color == W_WHITE) {
        p1 = &srv->white;
        p2 = &srv->black;
    } else {
        p1 = &srv->black;
        p2 = &srv->white;
    }
    if ((err = p1->get_move(p1, color, &action, &row, &col)) != W_NO_ERROR) {
        return err;
    }
    if (action == W_UNDO) {
        if (undo(srv)) {
            return W_NO_ERROR;
        } else {
            return W_UNDO_ERROR;
        }
    }
    if ((err = weiqi_register_move(srv->weiqi, color, action, row, col))
               != W_NO_ERROR) {
        switch (err) {
            case W_ILLEGAL_MOVE:
                fprintf(stderr, "Error: %s tried to play an illegal move\n",
                        color == W_WHITE ? "white" : "black");
                return W_ERROR;
            default:
                return err;
        }
    }
    if (p2->send_move(p2, color, action, row, col) != W_NO_ERROR) {
        fprintf(stderr, "Error: %s doesn't like our move :(\n"
                "We can't agree so let's quit here\n",
                color == W_WHITE ? "black" : "white");
        return W_ERROR;
    }
    return err;
}

int game_server_run(struct GameServer* srv) {
    enum WeiqiColor color;

    if (!srv->black.reset(&srv->black)) return 0;
    if (!srv->white.reset(&srv->white)) return 0;

    if (       srv->weiqi->history.last
            && srv->weiqi->history.last->color == W_BLACK
            && play_turn(srv, W_WHITE) != W_NO_ERROR) {
        return 0;
    }

    color = W_BLACK;
    while (srv->ui->status != W_UI_QUIT && srv->ui->status != W_UI_CRASH) {
        enum WeiqiError err;
        err = play_turn(srv, color);
        switch (err) {
            case W_ERROR:
                return 0;
            case W_GAME_OVER:
                fprintf(stderr, "game over\n");
                ui_wait(srv->ui, W_UI_QUIT);
                return 1;
            case W_UNDO_ERROR:
                fprintf(stderr, "Error: can't undo\n");
                break;
            default:
                color = color == W_WHITE ? W_BLACK : W_WHITE;
                break;
        }
    }
    if (srv->ui->status == W_UI_CRASH) {
        fprintf(stderr, "Error: UI crashed\n");
        return 0;
    }
    return 1;
}
