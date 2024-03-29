#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <3dmr/render/texture.h>
#include <3dmr/skybox.h>
#include <3dmr/math/interp.h>
#include <3dasset.h>

#include "asset.h"

GLuint grid_gen(unsigned char boardSize, struct InterfaceTheme* theme);

static char* strmerge(const char* s1, const char* s2) {
    char* res;
    if (       strlen(s1) < (size_t)(-1) / 2 && strlen(s2) < (size_t)(-1) / 2
            && (res = malloc(strlen(s1) + strlen(s2) + 1))) {
        strcpy(res, s1);
        strcpy(res + strlen(s1), s2);
    }
    return res;
}

static int load_ibl(struct Assets* assets, struct InterfaceTheme* theme) {
    if (theme->style == W_UI_NICE && strlen(theme->iblPath)) {
        GLuint tex;
        char *path = NULL, ok = 0;

        if (!(path = strmerge(W_DATA_DIR"/textures/", theme->iblPath))) {
            fprintf(stderr, "Error: lighting: can't merge strings\n");
        } else if (!(tex = skybox_load_texture_hdr_equirect(path, 1024))) {
            fprintf(stderr, "Error: lighting: can't load IBL texture\n");
        } else {
            ok = compute_ibl(tex, 32, 1024, 5, 256, &assets->ibl);
            if (ok) {
                assets->ibl.enabled = 1;
            }
        }
        free(path);
        return ok;
    }
    return 1;
}

static int load_lights(struct Assets* assets, struct InterfaceTheme* theme) {
    struct DirectionalLight* dl = &assets->lights.directional[0];
    struct AmbientLight* a = &assets->lights.ambientLight;

    if (!light_init(&assets->lights)) return 0;

    assets->lights.numDirectionalLights = 1;
    memcpy(dl->direction, theme->sunDirection, sizeof(Vec3));
    memcpy(dl->color, theme->sunColor, sizeof(Vec3));
    memcpy(a->color, theme->ambientColor, sizeof(Vec3));

    if (theme->shadow) {
        struct ShadowMap* map;
        Vec3 pos, lookAt = {0}, up = {0, 0, 1};
        if ((dl->shadow = light_shadowmap_new(&assets->lights, 2048, 2048)) < 0) {
            fprintf(stderr, "Error: assets: could not create shadowmap\n");
            return 0;
        }
        map = &assets->lights.shadowMaps[dl->shadow];
        camera_ortho_projection(1.2, 1.2, 4, 5.3, map->projection);

        pos[0] = -dl->direction[0] * 2;
        pos[1] = -dl->direction[1] * 2;
        pos[2] = -dl->direction[2] * 2;
        normalize3(pos);
        scale3v(pos, 5.);
        camera_look_at(pos, lookAt, up, map->view);
    }

    return 1;
}

static int setup_occlusion(struct Assets* assets,
                           struct InterfaceTheme* theme) {
    if (!occlusion_init(&assets->occlusion, 1024, 1024)) {
        fprintf(stderr, "Error: setup_occlusion: occlusion_init failed\n");
    } else {
        float texbuf[128 * 128] = {0};
        unsigned int i, j;
        GLuint tex;

        for (i = 0; i < 128; i++) {
            for (j = 0; j < 128; j++) {
                float u = 2 * (float) j / 128. - 1.;
                float v = 2 * (float) i / 128. - 1.;
                float dist = sqrt(u * u + v * v);
                float l = 2, o = 1;
                texbuf[i * 128 + j] =
                    1 - (cos(M_PI - clamp(M_PI * (l * dist - o), 0, M_PI)) + 1) / 2;
            }
        }
        if (!(tex = texture_load_from_float_buffer(texbuf, 128, 128, 0, 0))) {
            fprintf(stderr, "Error: setup_occlusion: "
                            "texture_load_from_float_buffer failed\n");
            return 0;
        }
        asset_mat_overlay_color(&assets->stoneOccMat, 0, 0, 0);
        assets->stoneOccMat.params.solid.alpha.mode = ALPHA_BLEND;
        material_param_set_float_texture(
                &assets->stoneOccMat.params.solid.alpha.alpha, tex);

        if (!(assets->stoneOcclusion = asset_quad(&assets->stoneOccMat,
                                                  5 * theme->stoneRadius,
                                                  5 * theme->stoneRadius))) {
            fprintf(stderr, "Error: setup_occlusion: asset creation failed\n");
            return 0;
        }
        assets->boardMat.params.pbr.occlusionMap = assets->occlusion.tex;
        return 1;
    }
    return 0;
}

int assets_load(struct Assets* assets,
                unsigned char boardSize,
                struct InterfaceTheme* theme) {
    GLuint gridTex;

    memset(assets, 0, sizeof(*assets));

    if (!load_ibl(assets, theme)) {
        fprintf(stderr, "Error: assets_load: load_ibl failed\n");
        return 0;
    }

    if (!(gridTex = grid_gen(boardSize, theme))) {
        fprintf(stderr, "Error: assets_load: grid_gen failed\n");
        return 0;
    }

    asset_mat_solid_color(&assets->pointerMat, theme->pointer.color[0],
                                               theme->pointer.color[1],
                                               theme->pointer.color[2]);
    asset_mat_solid_color(&assets->lmvPointerMat, theme->lmvp.color[0],
                                                  theme->lmvp.color[1],
                                                  theme->lmvp.color[2]);
    if (theme->style == W_UI_PURE) {
        asset_mat_solid_color(&assets->wStoneMat, theme->wStone.color[0],
                                                  theme->wStone.color[1],
                                                  theme->wStone.color[2]);
        asset_mat_solid_color(&assets->bStoneMat, theme->bStone.color[0],
                                                  theme->bStone.color[1],
                                                  theme->bStone.color[2]);
        asset_mat_solid_texid(&assets->boardMat, gridTex);
        /* just as a safeguard since other parts of the UI might use those
         * values, and we are not going to load shadows and occlusion
         */
        theme->shadow = 0;
        theme->occlusion = 0;
    } else if (theme->style == W_UI_NICE) {
        asset_mat_pbr_color(&assets->wStoneMat, theme->wStone.color[0],
                                                theme->wStone.color[1],
                                                theme->wStone.color[2],
                                                theme->wStone.metalness,
                                                theme->wStone.roughness);
        asset_mat_pbr_color(&assets->bStoneMat, theme->bStone.color[0],
                                                theme->bStone.color[1],
                                                theme->bStone.color[2],
                                                theme->bStone.metalness,
                                                theme->bStone.roughness);
        asset_mat_pbr_texid(&assets->boardMat, gridTex,
                                               theme->board.metalness,
                                               theme->board.roughness);
        if (assets->ibl.enabled) {
            assets->bStoneMat.params.pbr.ibl = &assets->ibl;
            assets->wStoneMat.params.pbr.ibl = &assets->ibl;
            assets->boardMat.params.pbr.ibl = &assets->ibl;
        }
        if (theme->occlusion && !setup_occlusion(assets, theme)) {
            fprintf(stderr, "Error: setup_occlusion failed\n");
            return 0;
        }
    }
    if (       !(assets->board = asset_quad(&assets->boardMat, 1, 1))
            || !(assets->wStone = asset_icosphere(&assets->wStoneMat,
                                                  theme->stoneRadius, 4))
            || !(assets->bStone = asset_icosphere(&assets->bStoneMat,
                                                  theme->stoneRadius, 4))
            || !(assets->pointer = asset_box(&assets->pointerMat,
                                             theme->pointerSize,
                                             theme->pointerSize,
                                             theme->pointerSize))
            || !(assets->lmvPointer = asset_box(&assets->lmvPointerMat,
                                                theme->lmvpw,
                                                theme->lmvph,
                                                theme->stoneThickness
                                                * theme->stoneRadius * 2.2))) {
        fprintf(stderr, "Error: assets_load: could not create an asset\n");
    } else if (!load_lights(assets, theme)) {
        fprintf(stderr, "Error: assets_load: load_lights failed\n");
    } else {
        if (!assets->ibl.enabled && theme->shadow) {
            assets->bStone->hasShadow = 1;
            assets->wStone->hasShadow = 1;
        }
        return 1;
    }
    assets_free(assets);
    return 0;
}

static void free_asset(struct Node* n) {
    if (n) {
        struct Geometry* g = n->data.geometry;
        vertex_array_free(g->vertexArray);
        free(g->material);
        free(g);
        free(n);
    }
}

void assets_free(struct Assets* assets) {
    free_asset(assets->board);
    free_asset(assets->wStone);
    free_asset(assets->bStone);
    free_asset(assets->pointer);
    free_asset(assets->lmvPointer);
    free_asset(assets->stoneOcclusion);
    if (assets->occlusion.tex) {
        occlusion_free(&assets->occlusion);
    }
    return;
}
