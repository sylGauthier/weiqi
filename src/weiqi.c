#include <stdlib.h>

#include "weiqi.h"
#include "gtp.h"
#include "human.h"

int weiqi_init(struct GameContext* ctx, char boardSize) {
    if (boardSize < 7 || boardSize > 25) {
        fprintf(stderr, "Error: board size must be between 7 and 25\n");
        return 0;
    }
    if (!(ctx->weiqi.board = calloc(boardSize * boardSize * sizeof(char), 1))) {
        fprintf(stderr, "Error: can't allocate memory for board\n");
        return 0;
    }
    ctx->weiqi.boardSize = boardSize;
    ctx->black.weiqi = &ctx->weiqi;
    ctx->white.weiqi = &ctx->weiqi;
    return 1;
}

int register_move(struct GameContext* context, enum WeiqiColor color,
                  unsigned int row, unsigned int col) {
    if (!move_is_valid(&context->weiqi, color, row, col)) return 0;
    context->weiqi.board[row * context->weiqi.boardSize + col] = color;
    return 1;
}

int move_is_valid(struct Weiqi* weiqi, enum WeiqiColor color,
                  unsigned int row, unsigned int col) {
    return row < weiqi->boardSize
        && col < weiqi->boardSize
        && !weiqi->board[row * weiqi->boardSize + col];
}

int weiqi_run(struct GameContext* ctx) {
    while (1) {
        unsigned int row, col;

        ctx->black.get_move(&ctx->black, W_BLACK, &row, &col);
        if (!register_move(ctx, W_BLACK, row, col)) {
            fprintf(stderr, "Error: black tried to play an illegal move\n");
            return 0;
        }
        ctx->white.send_move(&ctx->white, W_BLACK, row, col);

        ctx->white.get_move(&ctx->white, W_WHITE, &row, &col);
        if (!register_move(ctx, W_WHITE, row, col)) {
            fprintf(stderr, "Error: white tried to play an illegal move\n");
            return 0;
        }
        ctx->black.send_move(&ctx->black, W_WHITE, row, col);
    }
}

void weiqi_free(struct GameContext* ctx) {
    free(ctx->weiqi.board);
    if (ctx->black.in) fclose(ctx->black.in);
    if (ctx->black.out) fclose(ctx->black.out);
    if (ctx->white.in) fclose(ctx->white.in);
    if (ctx->white.out) fclose(ctx->white.out);
}
