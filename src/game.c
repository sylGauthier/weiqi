#include <stdlib.h>
#include <string.h>

#include "game.h"
#include "utils.h"

int game_init(struct GameContext* ctx, char boardSize, char handicap) {
    if (boardSize < 5 || boardSize > 25) {
        fprintf(stderr, "Error: invalid board size\n");
        return 0;
    }
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
    if (ctx->white.free) ctx->white.free(&ctx->white);
    if (ctx->black.free) ctx->black.free(&ctx->black);
}

static int play_turn(struct GameContext* ctx, enum WeiqiColor color) {
    struct Player *p1, *p2;
    unsigned int row, col;
    char move[4];

    if (color == W_WHITE) {
        p1 = &ctx->white;
        p2 = &ctx->black;
    } else {
        p1 = &ctx->black;
        p2 = &ctx->white;
    }
    if (!p1->get_move(p1, color, &row, &col)) {
        return 0;
    }
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
    move_to_str(move, row, col);
    printf("%s %s\n", color == W_WHITE ? "white" : "black", move);
    return 1;
}

static void print_header(struct GameContext* ctx) {
    struct Move* cur = ctx->weiqi.history.first;
    char hoffset = 0;

    printf("size %d\n", ctx->weiqi.boardSize);
    printf("handicap %d\n", ctx->weiqi.handicap);
    printf("start\n");
    /* skip handicap placement in history */
    for (hoffset = 0; hoffset < ctx->weiqi.handicap; hoffset++) {
        cur = cur->next;
    }
    while (cur) {
        char move[4];
        move_to_str(move, cur->row, cur->col);
        printf("%s %s\n", cur->color == W_WHITE ? "white" : "black", move);
        cur = cur->next;
    }
}

int game_run(struct GameContext* ctx) {
    if (!ctx->black.reset(&ctx->black)) return 0;
    if (!ctx->white.reset(&ctx->white)) return 0;

    print_header(ctx);
    if (       ctx->weiqi.history.last
            && ctx->weiqi.history.last->color == W_BLACK
            && !play_turn(ctx, W_WHITE)) {
        return 0;
    }

    while (ctx->ui.status != W_UI_QUIT) {
        if (!play_turn(ctx, W_BLACK)) return 0;
        if (!play_turn(ctx, W_WHITE)) return 0;
    }
    return 1;
}

static char* get_line(FILE* f) {
    char c, *res = NULL;
    unsigned int size = 0, incr = 256, cur = 0;

    while ((c = fgetc(f)) != EOF) {
        if (cur >= size) {
            char* tmp;
            size += incr;
            if (!(tmp = realloc(res, size))) {
                free(res);
                return NULL;
            }
            res = tmp;
        }
        if (c == '\n') break;
        res[cur++] = c;
    }
    if (res) {
        if (cur >= size) cur = size - 1;
        res[cur] = '\0';
    }
    return res;
}

static int get_cmd(FILE* f, char** cmd, char** arg) {
    char *str;
    unsigned int len;

    do {
        if (!(str = get_line(f))) return 0;
    } while ((len = strlen(str)) == 0);
    *cmd = str;
    if ((*arg = strchr(str, ' '))) {
        while (*arg[0] == ' ') {
            (*arg)[0] = '\0';
            (*arg)++;
        }
    }
    return 1;
}

int game_load_file(struct GameContext* ctx, const char* name) {
    char *cmd, *arg;
    char start = 0, end = 0, boardSize = 19, handicap = 0, ok = 1;
    FILE* f;

    if (!(f = fopen(name, "r"))) {
        fprintf(stderr, "Error: can't open game file: %s", name);
        return 0;
    }
    while (!start) {
        if (!get_cmd(f, &cmd, &arg)) {
            fprintf(stderr, "Error: unexpected end of file\n");
            return 0;
        } else if (!strcmp(cmd, "size")) {
            if (!arg) {
                fprintf(stderr, "Error: missing arg to 'size' command\n");
                ok = 0;
            } else {
                boardSize = strtol(arg, NULL, 10);
            }
        } else if (!strcmp(cmd, "handicap")) {
            if (!arg) {
                fprintf(stderr, "Error: missing arg to 'handicap' command\n");
                ok = 0;
            } else {
                handicap = strtol(arg, NULL, 10);
            }
        } else if (!strcmp(cmd, "start")) {
            start = 1;
        } else {
            fprintf(stderr, "Error: unknown command: %s\n", cmd);
            ok = 0;
        }
        free(cmd);
        if (!ok) {
            fclose(f);
            return 0;
        }
    }
    if (!game_init(ctx, boardSize, handicap)) {
        fprintf(stderr, "Error: game init failed\n");
        return 0;
    }
    while (!end) {
        if (!get_cmd(f, &cmd, &arg)) {
            end = 1;
        } else if (!strcmp(cmd, "white")) {
            if (!arg) {
                fprintf(stderr, "Error: missing arg\n");
                ok = 0;
                break;
            } else {
                unsigned int row, col;
                if (   !str_to_move(&row, &col, arg)
                    || !weiqi_register_move(&ctx->weiqi, W_WHITE, row, col)) {
                    ok = 0;
                    break;
                }
            }
        } else if (!strcmp(cmd, "black")) {
            if (!arg) {
                fprintf(stderr, "Error: missing arg\n");
                ok = 0;
                break;
            } else {
                unsigned int row, col;
                if (   !str_to_move(&row, &col, arg)
                    || !weiqi_register_move(&ctx->weiqi, W_BLACK, row, col)) {
                    ok = 0;
                    break;
                }
            }
        } else {
            fprintf(stderr, "Error: unknown command: %s\n", cmd);
            ok = 0;
            break;
        }
    }
    if (!ok) game_free(ctx);
    fclose(f);
    return ok;
}
