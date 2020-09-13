#ifndef BOARD_H
#define BOARD_H

#include <3dmr/material/solid.h>

struct Asset3D {
    struct VertexArray* va;
    struct Material* mat;
    struct SolidMaterialParams* matParams;
};

struct Board3D {
    struct Asset3D geom;
    float gridScale;
};

int board_create(struct Board3D* board, unsigned int size, float gridScale,
                 float r, float g, float b);

int stone_create(struct Asset3D* stone, float radius,
                 float r, float g, float b);

void asset_init(struct Asset3D* asset);
void asset_free(struct Asset3D* asset);

#endif
