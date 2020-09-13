#include "grid.h"
#include "utils.h"

#define SCALE(b, x, y, s) \
{ \
    (b)[3 * ((x) * GRID_RES + (y))] *= (s); \
    (b)[3 * ((x) * GRID_RES + (y)) + 1] *= (s); \
    (b)[3 * ((x) * GRID_RES + (y)) + 2] *= (s); \
}

/*
static const char* spots[25][10] = {
    {NULL}, {NULL}, {NULL}, {NULL}, {NULL},
    { "B2", "B4", "D2", "D4", NULL},
    { "C3", "C4", "D3", "D4", NULL},
    { "C3", "C5", "E3", "E5", NULL},
    { "C3", "F6", "F3", "F6", NULL},
    { "C3", "C7", "E5", "G3", "G7", NULL},
    { "C3", "C8", "H3", "H8", NULL},
    { "C3", "C9", "F6", "J3", "J9", NULL},
    { "D4", "D9", "J4", "J9", NULL},
    { "D4", "D7", "D10", "G4", "G7", "G10", "K4", "K7", "K10", NULL},
    { "D4", "D11", "L4", "L11", NULL},
    { "D4", "D8", "D12", "H4", "H8", "H12", "M4", "M8", "M12", NULL},
    { "D4", "D13", "N4", "N13", NULL},
    { "D4", "D9", "D14", "J4", "J9", "J14", "O4", "O9", "O14", NULL},
    { "D4", "D15", "P4", "P15", NULL},
    { "D4", "D10", "D16", "K4", "K10", "K16", "Q4", "Q10", "Q16", NULL}
    {NULL}, {NULL}, {NULL}, {NULL}, {NULL}, {NULL}
};
*/

static float circle(int i, int j, int radius) {
    float a1 = ((float)i - 0.5) * ((float)i - 0.5)
             + ((float)j - 0.5) * ((float)j - 0.5);
    float a2 = ((float)i - 0.5) * ((float)i - 0.5)
             + ((float)j + 0.5) * ((float)j + 0.5);
    float a3 = ((float)i + 0.5) * ((float)i + 0.5)
             + ((float)j - 0.5) * ((float)j - 0.5);
    float a4 = ((float)i + 0.5) * ((float)i + 0.5)
             + ((float)j + 0.5) * ((float)j + 0.5);
    float count = 0, rs = radius * radius;
    count += !!(a1 <= rs);
    count += !!(a2 <= rs);
    count += !!(a3 <= rs);
    count += !!(a4 <= rs);
    return count / 4.;
}

static void draw_dot(unsigned char* buf, unsigned int size, float scale,
                     unsigned int row, unsigned int col) {
    unsigned int x = (((((float) col) / ((float) (size - 1))) - 0.5)
                     * scale + 0.5) * GRID_RES;
    unsigned int y = (((((float) row) / ((float) (size - 1))) - 0.5)
                     * scale + 0.5) * GRID_RES;
    int i, j, radius = 3;

    for (i = -radius; i <= radius; i++) {
        for (j = -radius; j <= radius; j++) {
            float fac = 1. - circle(i, j, radius);
            SCALE(buf, x + i, y + j, fac);
        }
    }
}

static void draw_dots(unsigned char* buf, unsigned int size, float scale) {
    unsigned int nh, ed;
    if (size <= 6) return;
    if (size == 7) {
        nh = 4;
        ed = 2;
    } else {
        nh = size % 2 ? 9 : 4;
        ed = size <= 12 ? 2 : 3;
    }
    draw_dot(buf, size, scale, ed, ed );
    draw_dot(buf, size, scale, ed, size - 1 - ed);
    draw_dot(buf, size, scale, size - 1 - ed, ed);
    draw_dot(buf, size, scale, size - 1 - ed, size - 1 - ed);
    if (nh == 9) {
        draw_dot(buf, size, scale, size / 2, ed);
        draw_dot(buf, size, scale, ed, size / 2);
        draw_dot(buf, size, scale, size / 2, size / 2);
        draw_dot(buf, size, scale, size / 2, size - 1 - ed);
        draw_dot(buf, size, scale, size - 1 - ed, size / 2);
    }
}

static void draw_grid(unsigned char* buf, unsigned int size, float scale) {
    unsigned int i, j;
    unsigned int lower = ((0. - 0.5) * scale + 0.5) * GRID_RES;
    unsigned int upper = ((1. - 0.5) * scale + 0.5) * GRID_RES;

    for (i = 0; i < size; i++) {
        unsigned int n = (((((float) i) / ((float) (size - 1))) - 0.5)
                            * scale + 0.5) * GRID_RES;
        for (j = lower; j <= upper; j++) {
            SCALE(buf, n, j, 0);
            SCALE(buf, j, n, 0);
        }
    }
}

GLuint grid_gen(unsigned int boardSize, float scale) {
    unsigned char texBuf[GRID_RES * GRID_RES * 3];
    unsigned int i;

    for (i = 0; i < GRID_RES * GRID_RES; i++) {
        texBuf[3 * i + 0] = 188;
        texBuf[3 * i + 1] = 172;
        texBuf[3 * i + 2] = 82;
    }
    draw_grid(texBuf, boardSize, scale);
    draw_dots(texBuf, boardSize, scale);
    return texture_load_from_uchar_buffer(texBuf, GRID_RES, GRID_RES, 3, 0);
}
