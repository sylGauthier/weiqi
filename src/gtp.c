#include <string.h>
#include <stdlib.h>

#include "gtp.h"
#include "utils.h"
#include "pipe_proc.h"

struct GTPConnection {
    FILE *in, *out;
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

int gtp_init(struct Player* player, FILE* in, FILE* out) {
    int err, ok = 1;
    struct GTPConnection* c;

    if (!(c = malloc(sizeof(struct GTPConnection)))) {
        fprintf(stderr, "Error: can't create GTP data\n");
        return 0;
    }
    c->in = in;
    c->out = out;
    player->data = c;
    player->send_move = gtp_send_move;
    player->get_move = gtp_get_move;
    player->reset = gtp_reset;
    player->free = gtp_free;
    err = check_version(player);
    if (err < 0) {
        fprintf(stderr, "Error: GTP: engine offline\n");
        ok = 0;
    } else if (!err) {
        fprintf(stderr, "Error: GTP: invalid version\n");
        ok = 0;
    }
    if (!ok) {
        free(c);
        player->data = NULL;
    }
    return ok;
}

int gtp_local_engine_init(struct Player* player, const char* cmd) {
    FILE *in, *out;
    int pid, ok = 1;
    char** splitcmd;

    if (!(splitcmd = split_cmd(cmd))) {
        fprintf(stderr, "Error: can't split command\n");
        return 0;
    }

    pid = pipe_proc(splitcmd[0], splitcmd, &out, &in);

    if (pid < 0) {
        exit(-1);
    } else if (pid == 0) {
        ok = 0;
    } else {
        ok = gtp_init(player, in, out);
    }
    free(splitcmd[0]);
    free(splitcmd);
    return ok;
}

int gtp_send_move(struct Player* player,
                  enum WeiqiColor color, enum MoveAction action,
                  unsigned int row, unsigned int col) {
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

int gtp_get_move(struct Player* player,
                 enum WeiqiColor color, enum MoveAction* action,
                 unsigned int* row, unsigned int* col) {
    char ans[2048], pass;
    struct GTPConnection* c = player->data;
    fprintf(c->out, "genmove %s\n",
            color == W_WHITE ? "white" : "black");
    fflush(c->out);
    gtp_get(c->in, ans, sizeof(ans));
    if (strlen(ans) < 4 || strncmp(ans, "= ", 2)) {
        fprintf(stderr, "Error: gtp: genmove returned error: %s\n", ans);
        return 0;
    }
    ans[strlen(ans) - 1] = '\0';
    if (!str_to_move(row, col, &pass, ans + 2)) {
        fprintf(stderr, "Error: format error from gtp client: %s\n", ans + 2);
        return 0;
    }
    if (pass) *action = W_PASS;
    return 1;
}

#if 0
static void gtp_showboard(struct Player* player) {
    char ans[2048];
    fprintf(player->out, "showboard\n");
    fflush(player->out);
    gtp_get(player->in, ans, sizeof(ans));
    printf("%s\n", ans);
}
#endif

int gtp_reset(struct Player* player) {
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

void gtp_free(struct Player* player) {
    struct GTPConnection* c = player->data;
    if (c) {
        if (c->in) fclose(c->in);
        if (c->out) fclose(c->out);
        free(player->data);
    }
}
