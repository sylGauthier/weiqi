#include <stdlib.h>
#include <stdio.h>

#include <3dmr/mesh/box.h>
#include <3dmr/mesh/uvsphere.h>

#include "asset.h"
#include "grid.h"

void asset_init(struct Asset3D* asset) {
    asset->va = NULL;
    asset->mat = NULL;
    asset->matParams = NULL;
}

void asset_free(struct Asset3D* asset) {
    vertex_array_free(asset->va);
    free(asset->mat);
    free(asset->matParams);
}

int stone_create(struct Stone3D* stone, float radius,
                 float r, float g, float b) {
    struct Mesh s;

    stone->radius = radius;
    if (!make_uvsphere(&s, radius, 16, 16)) {
        fprintf(stderr, "Error: interface: can't create sphere\n");
    } else if (!(stone->geom.va = vertex_array_new(&s))) {
        fprintf(stderr, "Error: interface: can't create vertex array\n");
    } else if (!(stone->geom.matParams = solid_material_params_new())) {
        fprintf(stderr, "Error: interface: can't create solid params\n");
    } else {
        material_param_set_vec3_elems(&stone->geom.matParams->color, r, g, b);
        stone->geom.mat = solid_material_new(s.flags, stone->geom.matParams);
        return !!stone->geom.mat;
    }
    return 0;
}

int board_create(struct Board3D* board, unsigned int size, float gridScale,
                 float r, float g, float b) {
    struct Mesh box;
    GLuint tex;

    board->thickness = 0.01;
    if (!make_box(&box, 1., 1., board->thickness)) {
        fprintf(stderr, "Error: interface: can't create box\n");
    } else if (!(board->geom.va = vertex_array_new(&box))) {
        fprintf(stderr, "Error: interface: can't create vertex array\n");
    } else if (!(board->geom.matParams = solid_material_params_new())) {
        fprintf(stderr, "Error: interface: can't create solid params\n");
    } else if (!(tex = grid_gen(size, gridScale))) {
        fprintf(stderr, "Error: interface: can't create grid\n");
    } else {
        material_param_set_vec3_texture(&board->geom.matParams->color, tex);
        board->geom.mat = solid_material_new(box.flags, board->geom.matParams);
        board->gridScale = gridScale;
        return !!board->geom.mat;
    }
    return 0;
}

int pointer_create(struct Asset3D* pointer, float size) {
    struct Mesh cube;

    if (!make_box(&cube, size, size, size)) {
        fprintf(stderr, "Error: interface: can't create cursor\n");
    } else if (!(pointer->va = vertex_array_new(&cube))) {
        fprintf(stderr, "Error: interface: can't create vertex array\n");
    } else if (!(pointer->matParams = solid_material_params_new())) {
        fprintf(stderr, "Error: interface: can't create solid params\n");
    } else {
        material_param_set_vec3_elems(&pointer->matParams->color, 0, 0, 0);
        pointer->mat = solid_material_new(cube.flags, pointer->matParams);
        return !!pointer->mat;
    }
    return 0;
}
