#include <string.h>

#include "gtp.h"
#include "utils.h"

int gtp_init(struct Player* player, FILE* in, FILE* out) {
    player->in = in;
    player->out = out;
    player->send_move = gtp_send_move;
    player->get_move = gtp_get_move;
    return 1;
}

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
