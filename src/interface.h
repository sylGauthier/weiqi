#ifndef INTERFACE_H
#define INTERFACE_H

#include <pthread.h>

#include <3dmr/scene/scene.h>

#include "weiqi.h"
#include "asset.h"

enum InterfaceStatus {
    W_UI_QUIT = 0,
    W_UI_RUN,
    W_UI_SELECT
};

struct Interface {
    struct Scene scene;
    struct Camera camera;
    struct Node* camNode;
    struct Node* camOrientation;

    struct Board3D board;
    struct Stone3D wStone;
    struct Stone3D bStone;
    struct Asset3D pointer;

    struct Viewer* viewer;
    struct Weiqi* weiqi;

    unsigned int cursorPos[2];
    unsigned int selectPos[2];
    char pass;

    enum InterfaceStatus status;
    enum InterfaceTheme theme;

    pthread_t thread;
};

int interface_init(struct Interface* ui, enum InterfaceTheme theme,
                   struct Weiqi* weiqi);

void interface_free(struct Interface* ui);
void interface_wait(struct Interface* ui);

int interface_get_move(struct Interface* ui,
                       enum WeiqiColor color, enum MoveAction* action,
                       unsigned int* row, unsigned int* col);

#endif
