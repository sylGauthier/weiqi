#include <string.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <sys/un.h>

#include "player.h"
#include "utils.h"
#include "cmd.h"
#include "pipe_proc.h"

struct GTPConnection {
    FILE *in, *out;
    const char* cmd;
};

static int gtp_get(FILE* in, char* buf, unsigned int bufSize) {
    char end[2048];
    while (bufSize && fgets(buf, bufSize, in)
                   && !(buf[0] == '\n')) {
        bufSize -= strlen(buf);
        buf += strlen(buf);
    }
    if (!bufSize && buf[0] != '\n') {
        /* If we filled up the buffer and still didn't reach
         * the end of gtp output, we just flush the rest
         */
        while (fgets(end, sizeof(end), in) && end[0] != '\n');
    }
    /* Removing last empty line */
    buf[0] = '\0';
    return bufSize;
}

static int gtp_is_happy(FILE* in) {
    char ans[256]= {0};
    if (!gtp_get(in, ans, sizeof(ans))) return 0;
    if (ans[0] == '=') return 1;
    return 0;
}

static int check_version(struct Player* gtp) {
    char ans[16] = {0};
    struct GTPConnection* c = gtp->data;
    fprintf(c->out, "protocol_version\n");
    fflush(c->out);
    gtp_get(c->in, ans, sizeof(ans) - 1);
    if (!strlen(ans)) return -1;
    if (!strcmp(ans, "= 2\n")) return 1;
    return 0;
}

static int gtp_socket_init(struct Player* player, struct Weiqi* w) {
    struct GTPConnection* c = player->data;
    int sfd, cfd;
    struct sockaddr_un addr;

    player->weiqi = w;
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
    return 0;
}

static int gtp_pipe_init(struct Player* player, struct Weiqi* w) {
    struct GTPConnection* c = player->data;
    int pid, ok = 1, err;
    char** splitcmd;

    player->weiqi = w;
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
    char ans[2048];
    struct GTPConnection* c = player->data;

    if (action == W_PASS) sprintf(move, "PASS");
    else if (!move_to_str(move, row, col)) return W_FORMAT_ERROR;
    fprintf(c->out, "play %s %s\n",
            color == W_WHITE ? "white" : "black", move);
    fflush(c->out);
    gtp_get(c->in, ans, sizeof(ans));
    if (ans[0] != '=') return W_ILLEGAL_MOVE;
    return W_NO_ERROR;
}

static int gtp_get_move(struct Player* player,
                 enum WeiqiColor color, enum MoveAction* action,
                 unsigned char* row, unsigned char* col) {
    char ans[2048], pass;
    struct GTPConnection* c = player->data;
    fprintf(c->out, "genmove %s\n",
            color == W_WHITE ? "white" : "black");
    fflush(c->out);
    gtp_get(c->in, ans, sizeof(ans));
    if (strlen(ans) < 4 || strncmp(ans, "= ", 2)) {
        fprintf(stderr, "Error: gtp: genmove returned error: %s\n", ans);
        return W_ERROR;
    }
    ans[strlen(ans) - 1] = '\0';
    if (!str_to_move(row, col, &pass, ans + 2)) {
        fprintf(stderr, "Error: format error from gtp client: %s\n", ans + 2);
        return W_ERROR;
    }
    if (pass) *action = W_PASS;
    return W_NO_ERROR;
}

static int gtp_reset(struct Player* player) {
    struct Move* cur = NULL;
    struct GTPConnection* c = player->data;
    fprintf(c->out, "clear_board\n");
    fflush(c->out);
    if (!gtp_is_happy(c->in)) {
        fprintf(stderr, "Error: GTP: can't clear board\n");
        return 0;
    }
    fprintf(c->out, "boardsize %d\n", player->weiqi->boardSize);
    fflush(c->out);
    if (!gtp_is_happy(c->in)) {
        fprintf(stderr, "Error: GTP: engine doesn't accept this board size\n");
        return 0;
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
    fprintf(c->out, "undo\n");
    fflush(c->out);
    return gtp_is_happy(c->in);
}

static void gtp_free(struct Player* player) {
    struct GTPConnection* c = player->data;
    if (c) {
        if (c->in) fclose(c->in);
        if (c->out) fclose(c->out);
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
    c->in = NULL;
    c->out = NULL;
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
