#ifndef BOARD_H
#define BOARD_H

#include <3dmr/material/solid.h>
#include <3dmr/material/pbr.h>

#include "theme.h"

#define GRID_RES 1024

struct Asset3D {
    struct VertexArray* va;
    struct Material* mat;
    struct SolidMaterialParams* solidParams;
    struct PBRMaterialParams* pbrParams;
};

struct Board3D {
    struct Asset3D geom;
    float gridScale;
    float thickness;
};

struct Stone3D {
    struct Asset3D geom;
    float radius;
};

GLuint grid_gen(unsigned char boardSize, struct InterfaceTheme* theme);

int board_create(struct Asset3D* board, unsigned int size,
                 struct InterfaceTheme* theme);

int stone_create(struct Asset3D* stone, char white,
                 struct InterfaceTheme* theme);

int pointer_create(struct Asset3D* pointer, struct InterfaceTheme* theme);

int lmvpointer_create(struct Asset3D* lmvp, struct InterfaceTheme* theme);

void asset_init(struct Asset3D* asset);
void asset_free(struct Asset3D* asset);

#endif
