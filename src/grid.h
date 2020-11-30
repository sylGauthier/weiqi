#ifndef GRID_H
#define GRID_H

#define GRID_RES 1024

#include <3dmr/render/texture.h>
#include <3dmr/math/linear_algebra.h>

GLuint grid_gen(unsigned char boardSize, float gridScale, const char* tex,
                Vec3 color);

#endif
