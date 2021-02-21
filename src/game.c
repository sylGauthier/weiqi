#include <stdlib.h>
#include <string.h>

#include "game.h"
#include "utils.h"
#include "cmd.h"

int game_init(struct GameServer* srv, char boardSize, char handicap) {
    if (boardSize < 5 || boardSize > 25) {
        fprintf(stderr, "Error: invalid board size\n");
        return 0;
    }
    if (!(weiqi_init(&srv->weiqi, boardSize, handicap))) {
        return 0;
    } else if (!srv->white.init(&srv->white, &srv->weiqi, W_WHITE)
            || !srv->black.init(&srv->black, &srv->weiqi, W_BLACK)) {
        return 0;
    } else if (!interface_init(&srv->ui, &srv->weiqi)) {
        weiqi_free(&srv->weiqi);
        return 0;
    }
    return 1;
}

void game_free(struct GameServer* srv) {
    interface_free(&srv->ui);
    weiqi_free(&srv->weiqi);
    if (srv->white.free) srv->white.free(&srv->white);
    if (srv->black.free) srv->black.free(&srv->black);
}

static int play_turn(struct GameServer* srv, enum WeiqiColor color) {
    struct Player *p1, *p2;
    unsigned char row, col;
    char move[5];
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
    if ((err = weiqi_register_move(&srv->weiqi, color, action, row, col))
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
    if (action == W_PASS) sprintf(move, "PASS");
    else move_to_str(move, row, col);
    printf("%s %s\n", color == W_WHITE ? "white" : "black", move);
    return err;
}

static void print_header(struct GameServer* srv) {
    struct Move* cur = srv->weiqi.history.first;
    char hoffset = 0;

    printf("size %d\n", srv->weiqi.boardSize);
    printf("handicap %d\n", srv->weiqi.handicap);
    printf("start\n");
    /* skip handicap placement in history */
    for (hoffset = 0; hoffset < srv->weiqi.handicap; hoffset++) {
        cur = cur->next;
    }
    while (cur) {
        char move[4];
        move_to_str(move, cur->row, cur->col);
        printf("%s %s\n", cur->color == W_WHITE ? "white" : "black", move);
        cur = cur->next;
    }
}

static int undo(struct GameServer* srv) {
    if (       srv->black.undo(&srv->black)
            && srv->white.undo(&srv->white)) {
        weiqi_undo_move(&srv->weiqi);
        return 1;
    }
    return 0;
}

int game_run(struct GameServer* srv) {
    enum WeiqiColor color;

    if (!srv->black.reset(&srv->black)) return 0;
    if (!srv->white.reset(&srv->white)) return 0;

    print_header(srv);
    if (       srv->weiqi.history.last
            && srv->weiqi.history.last->color == W_BLACK
            && play_turn(srv, W_WHITE) != W_NO_ERROR) {
        return 0;
    }

    color = W_BLACK;
    while (srv->ui.status != W_UI_QUIT) {
        enum WeiqiError err;
        err = play_turn(srv, color);
        switch (err) {
            case W_ERROR:
                return 0;
            case W_GAME_OVER:
                fprintf(stderr, "game over\n");
                interface_wait(&srv->ui);
                return 1;
            case W_UNDO_MOVE:
                if (       undo(srv)
                        && undo(srv)) {
                    continue;
                } else {
                    fprintf(stderr, "Error: can't undo\n");
                }
            default:
                color = color == W_WHITE ? W_BLACK : W_WHITE;
                break;
        }
    }
    if (!srv->ui.ok) {
        fprintf(stderr, "Error: UI crashed\n");
        return 0;
    }
    return 1;
}

int game_load_file(struct GameServer* srv, const char* name) {
    char **cmd;
    char start = 0, end = 0, boardSize = 19, handicap = 0, ok = 1;
    FILE* f;

    if (!(f = fopen(name, "r"))) {
        fprintf(stderr, "Error: can't open game file: %s", name);
        return 0;
    }
    while (!start) {
        if (!(cmd = cmd_get(f)) || !cmd[0]) {
            fprintf(stderr, "Error: unexpected end of file\n");
            return 0;
        } else if (!strcmp(cmd[0], "size")) {
            if (!cmd[1]) {
                fprintf(stderr, "Error: missing arg to 'size' command\n");
                ok = 0;
            } else {
                boardSize = strtol(cmd[1], NULL, 10);
            }
        } else if (!strcmp(cmd[0], "handicap")) {
            if (!cmd[1]) {
                fprintf(stderr, "Error: missing arg to 'handicap' command\n");
                ok = 0;
            } else {
                handicap = strtol(cmd[1], NULL, 10);
            }
        } else if (!strcmp(cmd[0], "start")) {
            start = 1;
        } else {
            fprintf(stderr, "Error: unknown command: %s\n", cmd[0]);
            ok = 0;
        }
        cmd_free(cmd);
        if (!ok) {
            fclose(f);
            return 0;
        }
    }
    if (!game_init(srv, boardSize, handicap)) {
        fprintf(stderr, "Error: game init failed\n");
        return 0;
    }
    while (!end && ok) {
        if (!(cmd = cmd_get(f)) || !cmd[0]) {
            end = 1;
        } else if (!strcmp(cmd[0], "white")) {
            if (!cmd[1]) {
                fprintf(stderr, "Error: missing arg\n");
                ok = 0;
            } else {
                unsigned char row, col;
                char pass;
                if (       !str_to_move(&row, &col, &pass, cmd[1])
                        || weiqi_register_move(&srv->weiqi, 
                                               W_WHITE, pass ? W_PASS : W_PLAY,
                                               row, col) != W_NO_ERROR) {
                    fprintf(stderr, "file: invalid move: %s\n", cmd[1]);
                    ok = 0;
                }
            }
        } else if (!strcmp(cmd[0], "black")) {
            if (!cmd[1]) {
                fprintf(stderr, "Error: missing arg\n");
                ok = 0;
            } else {
                unsigned char row, col;
                char pass;
                if (       !str_to_move(&row, &col, &pass, cmd[1])
                        || weiqi_register_move(&srv->weiqi,
                                               W_BLACK, pass ? W_PASS : W_PLAY,
                                               row, col) != W_NO_ERROR) {
                    fprintf(stderr, "file: invalid move: %s\n", cmd[1]);
                    ok = 0;
                }
            }
        } else {
            fprintf(stderr, "Error: unknown command: %s\n", cmd[0]);
            ok = 0;
        }
        cmd_free(cmd);
    }
    if (!ok) game_free(srv);
    fclose(f);
    return ok;
}
