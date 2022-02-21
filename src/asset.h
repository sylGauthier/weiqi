#ifndef BOARD_H
#define BOARD_H

#include <3dmr/material/solid.h>
#include <3dmr/material/pbr.h>

#include "theme.h"

#define GRID_RES 1024

struct Assets {
    struct Node* board;
    struct Node* wStone;
    struct Node* bStone;
    struct Node* pointer;
    struct Node* lmvPointer;

    struct MaterialConfig boardMat;
    struct MaterialConfig wStoneMat;
    struct MaterialConfig bStoneMat;
    struct MaterialConfig pointerMat;
    struct MaterialConfig lmvPointerMat;

    struct Lights lights;
    struct IBL ibl;
};

int assets_load(struct Assets* assets,
                unsigned char boardSize,
                struct InterfaceTheme* theme);
void assets_free(struct Assets* assets);

#endif
