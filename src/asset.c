#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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
                       Vec3 color) {
    int ok = 0;
    if (!(asset->solidParams = solid_material_params_new())) {
        fprintf(stderr, "Error: interface: can't create solid params\n");
    } else {
        material_param_set_vec3_constant(&asset->solidParams->color, color);
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
                     Vec3 color, float metal, float rough) {
    int ok = 0;
    if (!(asset->pbrParams = pbr_material_params_new())) {
        fprintf(stderr, "Error: interface: can't create pbr params\n");
    } else {
        material_param_set_vec3_constant(&asset->pbrParams->albedo, color);
        material_param_set_float_constant(&asset->pbrParams->metalness, metal);
        material_param_set_float_constant(&asset->pbrParams->roughness, rough);
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
                         GLuint tex, float metal, float rough) {
    int ok = 0;
    if (!(asset->pbrParams = pbr_material_params_new())) {
        fprintf(stderr, "Error: interface: can't create pbr params\n");
    } else {
        material_param_set_vec3_texture(&asset->pbrParams->albedo, tex);
        material_param_set_float_constant(&asset->pbrParams->metalness, metal);
        material_param_set_float_constant(&asset->pbrParams->roughness, rough);
        asset->mat = pbr_material_new(flags, asset->pbrParams);
        ok = !!asset->mat;
    }
    if (!ok) {
        free(asset->mat);
        free(asset->pbrParams);
    }
    return ok;
}

static int stone_create(struct Stone3D* stone, char white, float radius,
                        struct InterfaceTheme* theme) {
    struct Mesh s;
    int ok = 0;
    struct AssetParams* params = white ? &theme->wStone : &theme->bStone;

    stone->radius = radius;
    if (!make_uvsphere(&s, radius, 16, 16)) {
        fprintf(stderr, "Error: interface: can't create sphere\n");
    } else if (!(stone->geom.va = vertex_array_new(&s))) {
        fprintf(stderr, "Error: interface: can't create vertex array\n");
    } else {
        switch (theme->style) {
            case W_UI_PURE:
                ok = solid_asset(&stone->geom, s.flags, params->color);
                break;
            case W_UI_NICE:
                ok = pbr_asset(&stone->geom, s.flags,
                               params->color, params->metalness,
                               params->roughness);
                break;
            default:
                ok = solid_asset(&stone->geom, s.flags, params->color);
                break;
        }
    }
    mesh_free(&s);
    if (!ok) {
        vertex_array_free(stone->geom.va);
    }
    return ok;
}

static int board_create(struct Board3D* board, unsigned int size,
                        struct InterfaceTheme* theme) {
    struct Mesh box;
    GLuint tex;
    int ok = 0;

    board->thickness = theme->boardThickness;
    board->gridScale = theme->gridScale;
    if (!make_box(&box, 1., 1., board->thickness)) {
        fprintf(stderr, "Error: interface: can't create box\n");
    } else if (!(board->geom.va = vertex_array_new(&box))) {
        fprintf(stderr, "Error: interface: can't create vertex array\n");
    } else if (!(tex = grid_gen(size, theme->gridScale, theme->wood,
                                theme->board.color))) {
        fprintf(stderr, "Error: interface: can't create grid\n");
    } else {
        switch (theme->style) {
            case W_UI_PURE:
                ok = solid_asset_tex(&board->geom, box.flags, tex);
                break;
            case W_UI_NICE:
                ok = pbr_asset_tex(&board->geom, box.flags, tex,
                                   theme->board.metalness,
                                   theme->board.roughness);
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

static int pointer_create(struct Asset3D* pointer,
                          struct InterfaceTheme* theme) {
    struct Mesh cube;
    int ok = 0;

    if (!make_box(&cube,
                  theme->pointerSize,
                  theme->pointerSize,
                  theme->pointerSize)) {
        fprintf(stderr, "Error: interface: can't create cursor\n");
    } else if (!(pointer->va = vertex_array_new(&cube))) {
        fprintf(stderr, "Error: interface: can't create vertex array\n");
    } else {
        ok = solid_asset(pointer, cube.flags, theme->pointer.color);
    }
    mesh_free(&cube);
    if (!ok) {
        vertex_array_free(pointer->va);
    }
    return ok;
}

int assets_create(struct Board3D* board,
                  struct Stone3D* bStone,
                  struct Stone3D* wStone,
                  struct Asset3D* pointer,
                  unsigned int boardSize,
                  struct InterfaceTheme* theme) {
    float radius;
    int bc = 0, bsc = 0, wsc = 0, pc = 0;

    radius = 1. / (2. * (float)boardSize) * theme->gridScale;
    theme->pointerSize = radius / 2.;
    if (       !(bc = board_create(board, boardSize, theme))
            || !(bsc = stone_create(bStone, 0, radius, theme))
            || !(wsc = stone_create(wStone, 1, radius, theme))
            || !(pc = pointer_create(pointer, theme))) {
        if (bc) asset_free(&board->geom);
        if (bsc) asset_free(&bStone->geom);
        if (wsc) asset_free(&wStone->geom);
        return 0;
    }
    return 1;
}
