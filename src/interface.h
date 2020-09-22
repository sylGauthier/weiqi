#ifndef INTERFACE_H
#define INTERFACE_H

#include <pthread.h>

#include <3dmr/scene/scene.h>

#include "weiqi.h"
#include "asset.h"

struct Interface {
    struct Scene scene;
    struct Camera camera;
    struct Node* camNode;
    struct Node* camOrientation;

    struct Board3D board;
    struct Stone3D wStone;
    struct Stone3D bStone;

    struct Viewer* viewer;
    struct Weiqi* weiqi;

    pthread_t thread;
    char running;
};

int interface_init(struct Interface* ui, struct Weiqi* weiqi);
void interface_free(struct Interface* ui);

int interface_get_stone(struct Interface* ui, enum WeiqiColor color,
                        unsigned int* row, unsigned int* col);

#endif
