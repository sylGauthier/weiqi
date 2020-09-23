#include <string.h>

#include "gtp.h"
#include "utils.h"

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
    fprintf(gtp->out, "protocol_version\n");
    fflush(gtp->out);
    gtp_get(gtp->in, ans, sizeof(ans) - 1);
    if (!strlen(ans)) return -1;
    if (!strcmp(ans, "= 2\n")) return 1;
    return 0;
}

int gtp_init(struct Player* player, FILE* in, FILE* out) {
    int err;

    player->in = in;
    player->out = out;
    player->send_move = gtp_send_move;
    player->get_move = gtp_get_move;
    player->reset = gtp_reset;
    err = check_version(player);
    if (err < 0) {
        fprintf(stderr, "Error: GTP: engine offline\n");
        return 0;
    } else if (!err) {
        fprintf(stderr, "Error: GTP: invalid version\n");
        return 0;
    }
    return 1;
}

int gtp_send_move(struct Player* player, enum WeiqiColor color,
                  unsigned int row, unsigned int col) {
    char move[4] = {0};
    char ans[2048];

    if (!move_to_str(move, row, col)) return W_FORMAT_ERROR;
    fprintf(player->out, "play %s %s\n",
            color == W_WHITE ? "white" : "black", move);
    fflush(player->out);
    gtp_get(player->in, ans, sizeof(ans));
    if (ans[0] != '=') return W_ILLEGAL_MOVE;
    return W_NO_ERROR;
}

int gtp_get_move(struct Player* player, enum WeiqiColor color,
                 unsigned int* row, unsigned int* col) {
    char ans[2048];
    fprintf(player->out, "genmove %s\n",
            color == W_WHITE ? "white" : "black");
    fflush(player->out);
    gtp_get(player->in, ans, sizeof(ans));
    if (strlen(ans) < 4 || strncmp(ans, "= ", 2)) {
        fprintf(stderr, "Error: gtp: genmove returned error: %s\n", ans);
        return 0;
    }
    ans[strlen(ans) - 1] = '\0';
    if (!str_to_move(row, col, ans + 2)) {
        fprintf(stderr, "Error: format error from gtp client: %s\n", ans + 2);
        return 0;
    }
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
    fprintf(player->out, "clear_board\n");
    fflush(player->out);
    if (!gtp_is_happy(player->in)) return 0;
    fprintf(player->out, "boardsize %d\n", player->weiqi->boardSize);
    fflush(player->out);
    if (!gtp_is_happy(player->in)) return 0;
    if (player->weiqi->handicap >= 2) {
        fprintf(player->out, "fixed_handicap %d\n", player->weiqi->handicap);
        fflush(player->out);
        if (!gtp_is_happy(player->in)) return 0;
    }
    return 1;
}
