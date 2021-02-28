#ifndef BOARD_H
#define BOARD_H

#include <3dmr/material/solid.h>
#include <3dmr/material/pbr.h>

#define GRID_RES 1024

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
    struct AssetParams lmvp;

    float boardThickness, gridScale;
    float stoneRadius, stoneZScale;
    float pointerSize;

    /* width and height of last move pointer */
    float lmvpw, lmvph;

    /* boolean for whether or not we print the coordinates on the board */
    char coordinates;
    /* font to use for coordinates */
    char font[128];
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
