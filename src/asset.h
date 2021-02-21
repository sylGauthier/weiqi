#ifndef BOARD_H
#define BOARD_H

#include <3dmr/material/solid.h>
#include <3dmr/material/pbr.h>

enum InterfaceStyle {
    W_UI_PURE,
    W_UI_NICE
};

struct AssetParams {
    Vec3 color;
    float roughness;
    float metalness;
};

struct InterfaceTheme {
    enum InterfaceStyle style;
    char wood[128];

    struct AssetParams bStone;
    struct AssetParams wStone;
    struct AssetParams pointer;
    struct AssetParams board;

    float boardThickness, gridScale;
    float stoneRadius;
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

int board_create(struct Asset3D* board, unsigned int size,
                 struct InterfaceTheme* theme);

int stone_create(struct Asset3D* stone, char white,
                 struct InterfaceTheme* theme);

int pointer_create(struct Asset3D* pointer, struct InterfaceTheme* theme);

void asset_init(struct Asset3D* asset);
void asset_free(struct Asset3D* asset);

#endif
