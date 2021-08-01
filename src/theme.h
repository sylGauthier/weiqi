#ifndef THEME_H
#define THEME_H

#include <3dmr/math/linear_algebra.h>
#include <3dmr/light/ibl.h>

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
    char iblPath[128];
    struct IBL ibl;
    Vec3 backgroundColor;

    struct AssetParams bStone;
    struct AssetParams wStone;
    struct AssetParams pointer;
    struct AssetParams board;
    struct AssetParams lmvp;

    float boardThickness, gridScale;
    float stoneRadius, stoneYScale;
    float pointerSize;
    float fov;

    /* width and height of last move pointer */
    float lmvpw, lmvph;

    /* boolean for whether or not we print the coordinates on the board */
    char coordinates;
    /* font to use for coordinates */
    char font[128];
};

#endif
