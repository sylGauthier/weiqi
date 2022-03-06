#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/un.h>

#include "player.h"
#include "utils.h"
#include "cmd.h"

struct GTPConnection {
    int in, out;
    const char* cmd;
};

static char msg[2048];

static int gtp_get_cmd(int in, char* buf, size_t size) {
    char c[2];
    if (!read_line(in, buf, size)) return 0;
    if (!read_line(in, c, 2)) return 0;
    return c[0] == '\0';
}

static int gtp_is_happy(int in) {
    char ans[256]= {0};
    if (!gtp_get_cmd(in, ans, sizeof(ans))) return 0;
    if (ans[0] == '=') return 1;
    return 0;
}

static int check_version(struct Player* gtp) {
    char ans[16] = {0};
    struct GTPConnection* c = gtp->data;

    if (!write_str(c->out, "protocol_version\n")) return -1;
    if (!gtp_get_cmd(c->in, ans, sizeof(ans) - 1)) return -1;
    if (!strlen(ans)) return -1;
    if (!strcmp(ans, "= 2")) return 1;
    return 0;
}

static int gtp_socket_init(struct Player* player, struct Weiqi* w, int color) {
    struct GTPConnection* c = player->data;
    int sfd, cfd, err;
    struct sockaddr_un addr;

    player->weiqi = w;
    player->color = color;
    if (strlen(c->cmd) >= sizeof(addr.sun_path)) {
        fprintf(stderr, "Error: socket path too long\n");
        return 0;
    }
    if ((sfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "Error: can't create socket\n");
        return 0;
    }
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, c->cmd);
    if (bind(sfd, (void*)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "Error: can't bind to socket\n");
        return 0;
    }
    if (listen(sfd, 0) < 0) {
        fprintf(stderr, "Error: can't listen to socket");
        return 0;
    }
    fprintf(stderr, "Info: waiting for player to connect...\n");
    if ((cfd = accept(sfd, NULL, NULL)) < 0) {
        fprintf(stderr, "Error: socket connection failed\n");
        return 0;
    }
    fprintf(stderr, "Info: connection successful\n");
    c->in = cfd;
    c->out = cfd;
    err = check_version(player);
    if (err < 0) {
        fprintf(stderr, "Error: player offline\n");
        return 0;
    } else if (!err) {
        fprintf(stderr, "Error: socket protocol: wrong version\n");
        return 0;
    }
    return 1;
}

static int gtp_pipe_init(struct Player* player, struct Weiqi* w, int color) {
    struct GTPConnection* c = player->data;
    int pid, ok = 1, err;
    char** splitcmd;

    player->weiqi = w;
    player->color = color;
    fprintf(stderr, "Info: gtp engine: %s\n", c->cmd);
    if (!(splitcmd = cmd_split(c->cmd))) {
        fprintf(stderr, "Error: can't split command\n");
        return 0;
    }

    pid = pipe_proc(splitcmd[0], splitcmd, &c->out, &c->in);

    if (pid < 0) {
        exit(-1);
    } else if (pid == 0) {
        ok = 0;
    } else {
        err = check_version(player);
        if (err < 0) {
            fprintf(stderr, "Error: GTP: engine offline\n");
            ok = 0;
        } else if (!err) {
            fprintf(stderr, "Error: GTP: invalid version\n");
            ok = 0;
        }
    }
    cmd_free(splitcmd);
    return ok;
}

static int gtp_send_move(struct Player* player,
                  enum WeiqiColor color, enum MoveAction action,
                  unsigned char row, unsigned char col) {
    char move[5] = {0};
    struct GTPConnection* c = player->data;

    if (!weiqi_move_to_str(move, action, row, col)) return W_FORMAT_ERROR;
    sprintf(msg, "play %s %s\n", color == W_WHITE ? "white" : "black", move);
    write_str(c->out, msg);
    if (!gtp_is_happy(c->in)) return W_ILLEGAL_MOVE;
    return W_NO_ERROR;
}

static int gtp_get_move(struct Player* player,
                 enum WeiqiColor color, enum MoveAction* action,
                 unsigned char* row, unsigned char* col) {
    char ans[2048];
    struct GTPConnection* c = player->data;
    sprintf(msg, "genmove %s\n", color == W_WHITE ? "white" : "black");
    write_str(c->out, msg);
    gtp_get_cmd(c->in, ans, sizeof(ans));
    if (strlen(ans) < 4 || strncmp(ans, "= ", 2)) {
        fprintf(stderr, "Error: gtp: genmove returned error: %s\n", ans);
        return W_ERROR;
    }
    if (!weiqi_str_to_move(action, row, col, ans + 2)) {
        fprintf(stderr, "Error: format error from gtp client: %s\n", ans + 2);
        return W_ERROR;
    }
    return W_NO_ERROR;
}

static int gtp_reset(struct Player* player) {
    struct Move* cur = NULL;
    struct GTPConnection* c = player->data;
    write_str(c->out, "clear_board\n");
    if (!gtp_is_happy(c->in)) {
        fprintf(stderr, "Error: GTP: can't clear board\n");
        return 0;
    }
    sprintf(msg, "boardsize %d\n", player->weiqi->boardSize);
    write_str(c->out, msg);
    if (!gtp_is_happy(c->in)) {
        fprintf(stderr, "Error: GTP: engine doesn't accept this board size\n");
        return 0;
    }
    sprintf(msg, "player %s\n", player->color == W_WHITE ? "white" : "black");
    write_str(c->out, msg);
    if (!gtp_is_happy(c->in)) {
        fprintf(stderr, "Info: GTP player is ignoring color assignment\n");
    }

    cur = player->weiqi->history.first;
    while (cur) {
        if (gtp_send_move(player, cur->color, cur->action, cur->row, cur->col)
                != W_NO_ERROR)
            return 0;
        cur = cur->next;
    }
    return 1;
}

static int gtp_undo(struct Player* player) {
    struct GTPConnection* c = player->data;
    write_str(c->out, "undo\n");
    return gtp_is_happy(c->in);
}

static void gtp_free(struct Player* player) {
    struct GTPConnection* c = player->data;
    if (c) {
        if (c->in >= 0) close(c->in);
        if (c->out >= 0) close(c->out);
        free(player->data);
    }
}

static void gtp_socket_free(struct Player* player) {
    struct GTPConnection* c = player->data;
    gtp_free(player);
    remove(c->cmd);
}

int player_gtp_pipe_init(struct Player* player, const char* cmd) {
    struct GTPConnection* c;

    if (!(c = malloc(sizeof(struct GTPConnection)))) {
        fprintf(stderr, "Error: can't create GTP data\n");
        return 0;
    }
    c->in = -1;
    c->out = -1;
    c->cmd = cmd;
    player->data = c;
    player->init = gtp_pipe_init;
    player->send_move = gtp_send_move;
    player->get_move = gtp_get_move;
    player->undo = gtp_undo;
    player->reset = gtp_reset;
    player->free = gtp_free;
    return 1;
}

int player_gtp_socket_init(struct Player* player, const char* cmd) {
    if (player_gtp_pipe_init(player, cmd)) {
        player->init = gtp_socket_init;
        player->free = gtp_socket_free;
        return 1;
    }
    return 0;
}
