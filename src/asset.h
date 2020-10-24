#ifndef BOARD_H
#define BOARD_H

#include <3dmr/material/solid.h>
#include <3dmr/material/pbr.h>

enum InterfaceTheme {
    W_UI_PURE,
    W_UI_NICE
};

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

int board_create(struct Board3D* board, enum InterfaceTheme theme,
                 unsigned int size, float gridScale);

int stone_create(struct Stone3D* stone, enum InterfaceTheme theme,
                 float radius, float r, float g, float b);

int pointer_create(struct Asset3D* pointer, enum InterfaceTheme theme,
                   float size);

void asset_init(struct Asset3D* asset);
void asset_free(struct Asset3D* asset);

#endif
