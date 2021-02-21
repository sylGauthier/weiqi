#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/un.h>
#include <sys/socket.h>

#include "game.h"
#include "cmd.h"
#include "utils.h"

char** gtp_cmd_get(FILE* in) {
    char** cmd;
    if (!(cmd = cmd_get(in))) {
        fprintf(stderr, "Error: server offline\n");
    }
    return cmd;
}

static int client_connect(struct GameClient* c, const char* s) {
    struct sockaddr_un addr;
    int sfd;

    c->in = NULL;
    c->out = NULL;
    if ((sfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "Error: can't create socket\n");
        return 0;
    }
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    if (strlen(s) >= sizeof(addr.sun_path)) {
        fprintf(stderr, "Error: socket path too long\n");
        return 0;
    }
    strcpy(addr.sun_path, s);

    fprintf(stderr, "Info: connecting to %s...\n", s);
    if (connect(sfd, (void*)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "Error: can't connect to socket: %s\n", s);
        return 0;
    }
    if (!(c->in = fdopen(sfd, "r")) || !(c->out = fdopen(sfd, "w"))) {
        fprintf(stderr, "Error: can't fdopen socket\n");
        return 0;
    }
    fprintf(stderr, "Info: connection successful\n");
    return 1;
}

static int client_init(struct GameClient* c) {
    unsigned char s = 19, flags = 0, ok = 1;
    char** cmd;

    while (ok && flags < 15 && (cmd = gtp_cmd_get(c->in))) {
        if (!strcmp(cmd[0], "protocol_version")) {
            fprintf(c->out, "= 2\n\n");
            flags |= 1;
        } else if (!strcmp(cmd[0], "clear_board")) {
            fprintf(c->out, "=\n\n");
            flags |= 2;
        } else if (!strcmp(cmd[0], "boardsize")) {
            if (!cmd[1]) {
                fprintf(stderr, "Error: GTP: missing argument\n");
                ok = 0;
            } else {
                s = strtol(cmd[1], NULL, 10);
                fprintf(stderr, "Info: board size = %d\n", s);
                fprintf(c->out, "=\n\n");
                flags |= 4;
            }
        } else if (!strcmp(cmd[0], "player")) {
            if (!cmd[1]) {
                fprintf(stderr, "Error: GTP: missing argument\n");
                ok = 0;
            } else {
                if (!strcmp(cmd[1], "white")) {
                    c->player.color = W_WHITE;
                } else if (!strcmp(cmd[1], "black")) {
                    c->player.color = W_BLACK;
                } else {
                    fprintf(stderr, "Error: GTP: invalid color\n");
                    return 0;
                }
                fprintf(stderr, "Info: you are playing %s\n", cmd[1]);
                fprintf(c->out, "=\n\n");
                flags |= 8;
            }
        } else {
            fprintf(stderr, "Error: unexpected GTP command\n");
            ok = 0;
        }
        fflush(c->out);
        cmd_free(cmd);
    }
    if (ok) {
        c->weiqi.boardSize = s;
        c->weiqi.handicap = 0;
    }
    return ok;
}

int game_client_init(struct GameClient* client, const char* socketPath) {
    if (!client_connect(client, socketPath)) return 0;
    if (!client_init(client)) return 0;
    if (!client->player.init(&client->player,
                             &client->weiqi,
                             client->player.color)) {
        return 0;
    }
    if (!weiqi_init(&client->weiqi,
                    client->weiqi.boardSize,
                    client->weiqi.handicap)) {
        return 0;
    }
    if (!interface_init(&client->ui, &client->weiqi)) {
        weiqi_free(&client->weiqi);
        return 0;
    }
    return 1;
}

static int client_play(struct GameClient* c, const char* color,
                       const char* move) {
    enum WeiqiColor wcol;
    enum MoveAction action;
    unsigned char row, col;
    char pass;

    if (!strcmp(color, "white")) wcol = W_WHITE;
    else wcol = W_BLACK;
    str_to_move(&row, &col, &pass, move);
    action = pass ? W_PASS : W_PLAY;

    if (weiqi_register_move(&c->weiqi, wcol, action, row, col) != W_NO_ERROR) {
        fprintf(stderr, "Error: we received an invalid move\n");
        return W_ERROR;
    }
    if (c->player.send_move(&c->player, wcol, action, row, col)
            != W_NO_ERROR) {
        fprintf(stderr, "Error: player didn't accept the move\n");
        return W_ERROR;
    }
    fprintf(c->out, "=\n\n");
    fflush(c->out);
    return W_NO_ERROR;
}

static int client_genmove(struct GameClient* c, const char* color) {
    int err;
    enum MoveAction action;
    enum WeiqiColor wcol;
    unsigned char row, col;
    char move[5];

    if (!strcmp(color, "white")) wcol = W_WHITE;
    else wcol = W_BLACK;

    err = c->player.get_move(&c->player, wcol, &action, &row, &col);
    if (err != W_NO_ERROR) return err;

    err = weiqi_register_move(&c->weiqi, wcol, action, row, col);
    if (err != W_NO_ERROR) return err;

    if (action == W_PASS) strcpy(move, "PASS");
    else move_to_str(move, row, col);

    fprintf(c->out, "= %s\n\n", move);
    fflush(c->out);
    return W_NO_ERROR;
}

int game_client_run(struct GameClient* client) {
    char** cmd;
    int ok = 1;

    while (ok && client->ui.status != W_UI_QUIT
              && (cmd = cmd_get(client->in))) {
        if (!strcmp(cmd[0], "play")) {
            if (!cmd[1] || !cmd[2]) {
                fprintf(stderr, "Error: GTP: not enough arguments\n");
                ok = 0;
            } else {
                ok = client_play(client, cmd[1], cmd[2]) == W_NO_ERROR;
            }
        } else if (!strcmp(cmd[0], "genmove")) {
            if (!cmd[1]) {
                fprintf(stderr, "Error: GTP: not enough arguments\n");
                ok = 0;
            } else {
                int err;
                err = client_genmove(client, cmd[1]);
                if (err == W_QUIT) break;
                if (err == W_ILLEGAL_MOVE) {
                    fprintf(stderr, "Error: oops, illegal move...\n");
                    ok = 0;
                }
                if (err == W_GAME_OVER) {
                    fprintf(stderr, "game over");
                    interface_wait(&client->ui);
                    break;
                }
            }
        }
        cmd_free(cmd);
    }
    if (!client->ui.ok) fprintf(stderr, "Error: UI crashed\n");
    return ok && client->ui.ok;
}

void game_client_free(struct GameClient* client) {
    interface_free(&client->ui);
    client->player.free(&client->player);
    weiqi_free(&client->weiqi);
    return;
}
