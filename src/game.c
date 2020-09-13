#include <stdlib.h>

#include "game.h"

int game_init(struct GameContext* ctx, char boardSize) {
    if (!(weiqi_init(&ctx->weiqi, boardSize))) {
        return 0;
    } else if (!interface_init(&ctx->ui, &ctx->weiqi)) {
        weiqi_free(&ctx->weiqi);
        return 0;
    }
    ctx->black.weiqi = &ctx->weiqi;
    ctx->white.weiqi = &ctx->weiqi;
    return 1;
}

void game_free(struct GameContext* ctx) {
    interface_free(&ctx->ui);
    weiqi_free(&ctx->weiqi);
    if (ctx->black.in) fclose(ctx->black.in);
    if (ctx->black.out) fclose(ctx->black.out);
    if (ctx->white.in) fclose(ctx->white.in);
    if (ctx->white.out) fclose(ctx->white.out);
}

int game_register_move(struct GameContext* ctx, enum WeiqiColor color,
                       unsigned int row, unsigned int col) {
    if (!weiqi_move_is_valid(&ctx->weiqi, color, row, col)) return 0;
    ctx->weiqi.board[row * ctx->weiqi.boardSize + col] = color;
    return 1;
}

int game_run(struct GameContext* ctx) {
    ctx->black.reset(&ctx->black);
    ctx->white.reset(&ctx->white);
    while (1) {
        unsigned int row, col;

        ctx->black.get_move(&ctx->black, W_BLACK, &row, &col);
        if (!game_register_move(ctx, W_BLACK, row, col)) {
            fprintf(stderr, "Error: black tried to play an illegal move\n");
            return 0;
        }
        if (ctx->white.send_move(&ctx->white, W_BLACK, row, col)
                != W_NO_ERROR) {
            fprintf(stderr, "Error: white doesn't like our move :(\n"
                            "We can't agree so let's quit here\n");
            return 0;
        }

        ctx->white.get_move(&ctx->white, W_WHITE, &row, &col);
        if (!game_register_move(ctx, W_WHITE, row, col)) {
            fprintf(stderr, "Error: white tried to play an illegal move\n");
            return 0;
        }
        if (ctx->black.send_move(&ctx->black, W_WHITE, row, col)
                != W_NO_ERROR) {
            fprintf(stderr, "Error: black doesn't like our move :(\n"
                            "We can't agree so let's quit here\n");
            return 0;
        }
    }
    return 1;
}
