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

int stone_create(struct Asset3D* stone, float radius,
                 float r, float g, float b) {
    struct Mesh sphere;

    if (!make_uvsphere(&sphere, radius, 8, 8)) {
        fprintf(stderr, "Error: interface: can't create sphere\n");
        return 0;
    } else if (!(stone->va = vertex_array_new(&sphere))) {
        fprintf(stderr, "Error: interface: can't create vertex array\n");
    } else if (!(stone->matParams = solid_material_params_new())) {
        fprintf(stderr, "Error: interface: can't create solid params\n");
    } else {
        material_param_set_vec3_elems(&stone->matParams->color, r, g, b);
        stone->mat = solid_material_new(sphere.flags, stone->matParams);
        return !!stone->mat;
    }
    return 0;
}

int board_create(struct Board3D* board, unsigned int size, float gridScale,
                 float r, float g, float b) {
    struct Mesh box;
    GLuint tex;

    if (!make_box(&box, 1., 1., 0.01)) {
        fprintf(stderr, "Error: interface: can't create box\n");
        return 0;
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
