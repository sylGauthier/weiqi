#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <3dmr/mesh/box.h>
#include <3dmr/mesh/uvsphere.h>

#include "asset.h"

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

int stone_create(struct Asset3D* stone, char white,
                 struct InterfaceTheme* theme) {
    struct Mesh s;
    int ok = 0;
    struct AssetParams* params = white ? &theme->wStone : &theme->bStone;

    if (!make_uvsphere(&s, theme->stoneRadius, 16, 16)) {
        fprintf(stderr, "Error: interface: can't create sphere\n");
    } else if (!(stone->va = vertex_array_new(&s))) {
        fprintf(stderr, "Error: interface: can't create vertex array\n");
    } else {
        switch (theme->style) {
            case W_UI_PURE:
                ok = solid_asset(stone, s.flags, params->color);
                break;
            case W_UI_NICE:
                ok = pbr_asset(stone, s.flags,
                               params->color, params->metalness,
                               params->roughness);
                break;
            default:
                ok = solid_asset(stone, s.flags, params->color);
                break;
        }
    }
    mesh_free(&s);
    if (!ok) {
        vertex_array_free(stone->va);
    }
    return ok;
}

int board_create(struct Asset3D* board, unsigned int size,
                 struct InterfaceTheme* theme) {
    struct Mesh box;
    GLuint tex;
    int ok = 0;

    if (!make_box(&box, 1., 1., theme->boardThickness)) {
        fprintf(stderr, "Error: interface: can't create box\n");
    } else if (!(board->va = vertex_array_new(&box))) {
        fprintf(stderr, "Error: interface: can't create vertex array\n");
    } else if (!(tex = grid_gen(size, theme))) {
        fprintf(stderr, "Error: interface: can't create grid\n");
    } else {
        switch (theme->style) {
            case W_UI_PURE:
                ok = solid_asset_tex(board, box.flags, tex);
                break;
            case W_UI_NICE:
                ok = pbr_asset_tex(board, box.flags, tex,
                                   theme->board.metalness,
                                   theme->board.roughness);
                break;
            default:
                ok = solid_asset_tex(board, box.flags, tex);
                break;
        }
    }
    mesh_free(&box);
    if (!ok) {
        vertex_array_free(board->va);
    }
    return ok;
}

int pointer_create(struct Asset3D* pointer, struct InterfaceTheme* theme) {
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

int lmvpointer_create(struct Asset3D* pointer, struct InterfaceTheme* theme) {
    struct Mesh cube;
    int ok = 0;

    if (!make_box(&cube,
                  theme->lmvpw,
                  theme->lmvph,
                  theme->stoneZScale * theme->stoneRadius * 2.2)) {
        fprintf(stderr, "Error: interface: can't create last move pointer\n");
    } else if (!(pointer->va = vertex_array_new(&cube))) {
        fprintf(stderr, "Error: interface: can't create vertex array\n");
    } else {
        ok = solid_asset(pointer, cube.flags, theme->lmvp.color);
    }
    mesh_free(&cube);
    if (!ok) {
        vertex_array_free(pointer->va);
    }
    return ok;
}
