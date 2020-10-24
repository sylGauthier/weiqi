#include <stdlib.h>
#include <stdio.h>

#include <3dmr/mesh/box.h>
#include <3dmr/mesh/uvsphere.h>

#include "asset.h"
#include "grid.h"

void asset_init(struct Asset3D* asset) {
    asset->va = NULL;
    asset->mat = NULL;
    asset->solidParams = NULL;
    asset->pbrParams = NULL;
}

void asset_free(struct Asset3D* asset) {
    vertex_array_free(asset->va);
    free(asset->mat);
    free(asset->solidParams);
    free(asset->pbrParams);
}

static int solid_asset_tex(struct Asset3D* asset, enum MeshFlags flags,
                           GLuint tex) {
    int ok = 0;
    if (!(asset->solidParams = solid_material_params_new())) {
        fprintf(stderr, "Error: interface: can't create solid params\n");
    } else {
        material_param_set_vec3_texture(&asset->solidParams->color, tex);
        asset->mat = solid_material_new(flags, asset->solidParams);
        ok = !!asset->mat;
    }
    if (!ok) {
        free(asset->mat);
        free(asset->solidParams);
    }
    return ok;
}

static int solid_asset(struct Asset3D* asset, enum MeshFlags flags,
                       float r, float g, float b) {
    int ok = 0;
    if (!(asset->solidParams = solid_material_params_new())) {
        fprintf(stderr, "Error: interface: can't create solid params\n");
    } else {
        material_param_set_vec3_elems(&asset->solidParams->color, r, g, b);
        asset->mat = solid_material_new(flags, asset->solidParams);
        ok = !!asset->mat;
    }
    if (!ok) {
        free(asset->mat);
        free(asset->solidParams);
    }
    return ok;
}

static int pbr_asset(struct Asset3D* asset, enum MeshFlags flags,
                     float r, float g, float b, float metal, float roughness) {
    int ok = 0;
    if (!(asset->pbrParams = pbr_material_params_new())) {
        fprintf(stderr, "Error: interface: can't create pbr params\n");
    } else {
        material_param_set_vec3_elems(&asset->pbrParams->albedo, r, g, b);
        material_param_set_float_constant(&asset->pbrParams->metalness, metal);
        material_param_set_float_constant(&asset->pbrParams->roughness, roughness);
        asset->mat = pbr_material_new(flags, asset->pbrParams);
        ok = !!asset->mat;
    }
    if (!ok) {
        free(asset->mat);
        free(asset->pbrParams);
    }
    return ok;
}

static int pbr_asset_tex(struct Asset3D* asset, enum MeshFlags flags,
                         GLuint tex, float metal, float roughness) {
    int ok = 0;
    if (!(asset->pbrParams = pbr_material_params_new())) {
        fprintf(stderr, "Error: interface: can't create pbr params\n");
    } else {
        material_param_set_vec3_texture(&asset->pbrParams->albedo, tex);
        material_param_set_float_constant(&asset->pbrParams->metalness, metal);
        material_param_set_float_constant(&asset->pbrParams->roughness, roughness);
        asset->mat = pbr_material_new(flags, asset->pbrParams);
        ok = !!asset->mat;
    }
    if (!ok) {
        free(asset->mat);
        free(asset->pbrParams);
    }
    return ok;
}

int stone_create(struct Stone3D* stone, enum InterfaceTheme theme,
                 float radius, float r, float g, float b) {
    struct Mesh s;
    int ok = 0;

    stone->radius = radius;
    if (!make_uvsphere(&s, radius, 16, 16)) {
        fprintf(stderr, "Error: interface: can't create sphere\n");
    } else if (!(stone->geom.va = vertex_array_new(&s))) {
        fprintf(stderr, "Error: interface: can't create vertex array\n");
    } else {
        switch (theme) {
            case W_UI_PURE:
                ok = solid_asset(&stone->geom, s.flags, r, g, b);
                break;
            case W_UI_NICE:
                ok = pbr_asset(&stone->geom, s.flags, r, g, b, 0.1, 0.5);
                break;
            default:
                ok = solid_asset(&stone->geom, s.flags, r, g, b);
                break;
        }
    }
    mesh_free(&s);
    if (!ok) {
        vertex_array_free(stone->geom.va);
    }
    return ok;
}

int board_create(struct Board3D* board, enum InterfaceTheme theme,
                 unsigned int size, float gridScale) {
    struct Mesh box;
    GLuint tex;
    int ok = 0;

    board->thickness = 0.01;
    board->gridScale = gridScale;
    if (!make_box(&box, 1., 1., board->thickness)) {
        fprintf(stderr, "Error: interface: can't create box\n");
    } else if (!(board->geom.va = vertex_array_new(&box))) {
        fprintf(stderr, "Error: interface: can't create vertex array\n");
    } else if (!(tex = grid_gen(size, gridScale))) {
        fprintf(stderr, "Error: interface: can't create grid\n");
    } else {
        switch (theme) {
            case W_UI_PURE:
                ok = solid_asset_tex(&board->geom, box.flags, tex);
                break;
            case W_UI_NICE:
                ok = pbr_asset_tex(&board->geom, box.flags, tex, 0.1, 0.3);
                break;
            default:
                ok = solid_asset_tex(&board->geom, box.flags, tex);
                break;
        }
    }
    mesh_free(&box);
    if (!ok) {
        vertex_array_free(board->geom.va);
    }
    return ok;
}

int pointer_create(struct Asset3D* pointer, enum InterfaceTheme theme,
                   float size) {
    struct Mesh cube;
    int ok = 0;

    if (!make_box(&cube, size, size, size)) {
        fprintf(stderr, "Error: interface: can't create cursor\n");
    } else if (!(pointer->va = vertex_array_new(&cube))) {
        fprintf(stderr, "Error: interface: can't create vertex array\n");
    } else {
        ok = solid_asset(pointer, cube.flags, 0, 0, 0);
    }
    mesh_free(&cube);
    if (!ok) {
        vertex_array_free(pointer->va);
    }
    return ok;
}
