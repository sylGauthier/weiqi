#ifndef UI_H
#define UI_H

#include <pthread.h>

#include "weiqi.h"
#include "config.h"

enum UIStatus {
    W_UI_STARTUP,
    W_UI_RUN,
    W_UI_CONFIG,
    W_UI_IDLE,

    W_UI_QUIT,
    W_UI_CRASH
};

struct UI {
    struct Config* config;
    struct Weiqi* weiqi;
    enum UIStatus status;
    unsigned char selectPos[2];
    char select;
    enum MoveAction action;

    pthread_t thread;
    void* private;
};

int ui_start(struct UI* ui, struct Weiqi* weiqi, struct Config* config);
int ui_stop(struct UI* ui);
int ui_config_menu(struct UI* ui);
int ui_game_start(struct UI* ui);

int ui_wait(struct UI* ui, enum UIStatus status);

int ui_get_move(struct UI* ui,
                enum WeiqiColor color, enum MoveAction* action,
                unsigned char* row, unsigned char* col);
#endif
