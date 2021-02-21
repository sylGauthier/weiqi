#ifndef INTERFACE_H
#define INTERFACE_H

#include <pthread.h>

#include <3dmr/scene/scene.h>

#include "weiqi.h"
#include "asset.h"

enum InterfaceStatus {
    W_UI_QUIT = 0,
    W_UI_RUN,
    W_UI_UNDO,
    W_UI_SELECT
};

struct Interface {
    struct InterfaceTheme theme;

    struct Scene scene;
    struct Camera camera;
    struct Node* camNode;
    struct Node* camOrientation;

    struct Asset3D board;
    struct Asset3D wStone;
    struct Asset3D bStone;
    struct Asset3D pointer;
    struct Asset3D lmvpointer;

    struct Viewer* viewer;
    struct Weiqi* weiqi;

    unsigned char cursorPos[2];
    unsigned char selectPos[2];
    char pass;

    enum InterfaceStatus status;
    char ok;

    pthread_t thread;
};

int interface_init(struct Interface* ui, struct Weiqi* weiqi);

void interface_free(struct Interface* ui);
void interface_wait(struct Interface* ui);

int interface_get_move(struct Interface* ui,
                       enum WeiqiColor color, enum MoveAction* action,
                       unsigned char* row, unsigned char* col);

#endif
