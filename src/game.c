#include <stdlib.h>

#include "game.h"

int game_init(struct GameContext* ctx, char boardSize, char handicap) {
    if (!(weiqi_init(&ctx->weiqi, boardSize, handicap))) {
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

static int play_turn(struct GameContext* ctx, enum WeiqiColor color) {
    struct Player *p1, *p2;
    unsigned int row, col;

    if (color == W_WHITE) {
        p1 = &ctx->white;
        p2 = &ctx->black;
    } else {
        p1 = &ctx->black;
        p2 = &ctx->white;
    }
    p1->get_move(p1, color, &row, &col);
    if (!weiqi_register_move(&ctx->weiqi, color, row, col)) {
        fprintf(stderr, "Error: %s tried to play an illegal move\n",
                color == W_WHITE ? "white" : "black");
        return 0;
    }
    if (p2->send_move(p2, color, row, col) != W_NO_ERROR) {
        fprintf(stderr, "Error: %s doesn't like our move :(\n"
                "We can't agree so let's quit here\n",
                color == W_WHITE ? "black" : "white");
        return 0;
    }
    return 1;
}

int game_run(struct GameContext* ctx) {
    ctx->black.reset(&ctx->black);
    ctx->white.reset(&ctx->white);

    if (!ctx->weiqi.handicap && !play_turn(ctx, W_BLACK)) return 0;
    while (1) {
        if (!play_turn(ctx, W_WHITE)) return 0;
        if (!play_turn(ctx, W_BLACK)) return 0;
    }
    return 1;
}
