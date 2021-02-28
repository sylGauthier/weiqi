#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <3dmr/img/png.h>
#include <3dmr/render/texture.h>
#include <3dmr/font/text.h>
#include <3dmr/font/ttf.h>

#include "asset.h"
#include "utils.h"

#define W_COORD_LETTERS "ABCDEFGHJKLMNOPQRSTUVWXYZ"
#define W_COORD_NUMBERS "0123456789"

#define SCALE(b, x, y, s) \
{ \
    (b)[3 * ((y) * GRID_RES + (x))] *= (s); \
    (b)[3 * ((y) * GRID_RES + (x)) + 1] *= (s); \
    (b)[3 * ((y) * GRID_RES + (x)) + 2] *= (s); \
}

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
                     unsigned char row, unsigned char col) {
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

static int load_font(struct TTF* ttf, const char* dir, const char* name) {
    char* filename;
    char ok;

    if (!(filename = malloc(strlen(dir) + strlen(name) + 1))) return 0;
    strcpy(filename, dir);
    strcpy(filename + strlen(dir), name);

    ok = ttf_load(filename, ttf);
    free(filename);
    return ok;
}

static float sdm(unsigned char d) {
    if (d >= 127) return 0;
    else return 1. - powf((float) d / 127., 2);
}

static void buf_mask(unsigned char* buf, unsigned int x, unsigned int y,
                     unsigned char* mask, unsigned int w, unsigned int h) {
    unsigned int i, j;

    for (j = 0; j < h; j++) {
        for (i = 0; i < w; i++) {
            if (       x + i > w / 2 && y + j > h / 2
                    && x + i - w / 2 < GRID_RES && y + j - h / 2 < GRID_RES) {
                SCALE(buf, x + i - w / 2, y + j - h / 2, sdm(mask[j * w + i]));
            }
        }
    }
}

static int draw_coordinates(unsigned char* buf, unsigned int size,
                            struct InterfaceTheme* theme) {
    struct TTF ttf;
    struct Character *letters = NULL, *numbers = NULL;
    size_t numLetters, numNumbers;

    if (       !load_font(&ttf, W_DATA_DIR"/", theme->font)
            && !load_font(&ttf, W_DATA_SRC"/", theme->font)) {
        fprintf(stderr, "Error: can't load font: %s\n", theme->font);
        return 0;
    }
    if (       !ttf_load_chars(&ttf, W_COORD_LETTERS, &letters, &numLetters)
            || !ttf_load_chars(&ttf, W_COORD_NUMBERS, &numbers, &numNumbers)) {
        fprintf(stderr, "Error: can't load some chars\n");
    } else {
        unsigned int i;
        unsigned char* sdm;
        size_t w, h = GRID_RES / 60;
        for (i = 0; i < size; i++) {
            unsigned int x = (((((float) i) / ((float) (size - 1))) - 0.5)
                             * theme->gridScale + 0.5) * GRID_RES;
            unsigned int y1 = ((-0.5 / (float) (size - 1) - 0.5)
                              * theme->gridScale + 0.5) * GRID_RES;
            unsigned int y2 = ((0.5 / (float) (size - 1) + 0.5)
                              * theme->gridScale + 0.5) * GRID_RES;

            if (!(sdm = text_to_sdm_buffer(letters + i, 1, h, &w))) break;
            buf_mask(buf, x, y1, sdm, w, h);
            buf_mask(buf, x, y2, sdm, w, h);
            free(sdm);
        }
        for (i = 1; i <= size; i++) {
            unsigned int y = (((((float) i - 1) / ((float) (size - 1))) - 0.5)
                             * theme->gridScale + 0.5) * GRID_RES;
            unsigned int x1 = ((-0.5 / (float) (size - 1) - 0.5)
                              * theme->gridScale + 0.5) * GRID_RES;
            unsigned int x2 = ((0.5 / (float) (size - 1) + 0.5)
                              * theme->gridScale + 0.5) * GRID_RES;

            if (i < 10) {
                if (!(sdm = text_to_sdm_buffer(numbers + i, 1, h, &w))) break;
            } else {
                struct Character c[2];
                c[0] = numbers[i / 10];
                c[1] = numbers[i % 10];
                if (!(sdm = text_to_sdm_buffer(c, 2, h, &w))) break;
            }
            buf_mask(buf, x1, y, sdm, w, h);
            buf_mask(buf, x2, y, sdm, w, h);
            free(sdm);
        }
        while (numLetters) character_free(letters + --numLetters);
        while (numNumbers) character_free(numbers + --numNumbers);
        free(letters);
        free(numbers);
    }
    ttf_free(&ttf);
    return 0;
}

static int load_tex(const char* dir, const char* name, unsigned char** buffer) {
    GLint ralign = 0;
    unsigned int width, height, channels, ok = 0;
    char* filename;

    if (!(filename = malloc(strlen(dir) + strlen(name) + 1))) return 0;
    strcpy(filename, dir);
    strcpy(filename + strlen(dir), name);
    filename[strlen(dir) + strlen(name)] = '\0';

    glGetIntegerv(GL_UNPACK_ALIGNMENT, &ralign);
    if (png_read_file(filename, ralign,
                      &width, &height, &channels, 3, 1, buffer)) {
        if (width == GRID_RES && height == GRID_RES) {
            ok = 1;
        }
        if (!ok) free(*buffer);
    }
    free(filename);
    return ok;
}

static int load_color(Vec3 color, unsigned char** buffer) {
    unsigned int i;

    if (!(*buffer = malloc(GRID_RES * GRID_RES * 3))) return 0;

    for (i = 0; i < GRID_RES * GRID_RES; i++) {
        (*buffer)[3 * i + 0] = 255 * color[0];
        (*buffer)[3 * i + 1] = 255 * color[1];
        (*buffer)[3 * i + 2] = 255 * color[2];
    }
    return 1;
}

GLuint grid_gen(unsigned char boardSize, struct InterfaceTheme* theme) {
    unsigned char* texBuf;
    GLuint tex = 0;

    if (strcmp(theme->wood, "none")) {
        if (       !load_tex(W_DATA_DIR"/textures/", theme->wood, &texBuf)
                && !load_tex(W_DATA_SRC"/textures/", theme->wood, &texBuf)) {
            return 0;
        }
    } else {
        if (!load_color(theme->board.color, &texBuf)) return 0;
    }

    draw_grid(texBuf, boardSize, theme->gridScale);
    draw_dots(texBuf, boardSize, theme->gridScale);
    if (theme->coordinates) draw_coordinates(texBuf, boardSize, theme);
    tex = texture_load_from_uchar_buffer(texBuf, GRID_RES, GRID_RES, 3, 0);
    if (tex) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    free(texBuf);
    return tex;
}
