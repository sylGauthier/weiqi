#ifndef BOARD_H
#define BOARD_H

#include <3dmr/material/solid.h>
#include <3dmr/material/pbr.h>

enum InterfaceStyle {
    W_UI_PURE,
    W_UI_NICE
};

struct InterfaceTheme {
    enum InterfaceStyle style;
    char* wood;

    Vec3 bStoneColor;
    Vec3 wStoneColor;
    Vec3 pointerColor;

    float boardRoughness, boardMetal;
    float stoneRoughness, stoneMetal;
    float boardThickness, gridScale;
    float pointerSize;
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

int assets_create(struct Board3D* board,
                  struct Stone3D* bStone,
                  struct Stone3D* wStone,
                  struct Asset3D* pointer,
                  unsigned int boardSize,
                  struct InterfaceTheme* theme);

void asset_init(struct Asset3D* asset);
void asset_free(struct Asset3D* asset);

#endif
