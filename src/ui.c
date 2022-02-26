#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <pthread.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <3dnk.h>
#include <3dasset.h>
#include <3dmr/scene/scene.h>
#include <3dmr/render/camera_buffer_object.h>
#include <3dmr/render/lights_buffer_object.h>
#include <3dmr/render/shader.h>

#include "ui.h"
#include "asset.h"

struct UIPrivate {
    struct NKRenderer nkdev;
    struct nk_font_atlas atlas;
    struct nk_font* font;
    struct nk_context ctx;

    struct Viewer* viewer;
    struct Scene scene;
    struct Camera camera;
    struct Node* camNode;
    struct Node* camOrientation;
    float defCamDist;

    struct Assets assets;
    unsigned char cursorPos[2];
    char ok;
    int lastMove;  /* keep track of number of stones last time we rendered
                    * the shadow map, so we can render it only when needed
                    */
};

static void update_node(struct Scene* scene, struct Node* n, void* data) {
    switch (n->type) {
        case NODE_DLIGHT:
            break;
        case NODE_PLIGHT:
            break;
        case NODE_SLIGHT:
            break;
        case NODE_CAMERA:
            camera_buffer_object_update_view_and_position(&scene->camera,
                    MAT_CONST_CAST(n->data.camera->view));
            break;
        default:;
    }
}

static void resize_callback_run(struct Viewer* viewer, void* data) {
    struct UI* ui = data;
    struct UIPrivate* uip = ui->private;

    glViewport(0, 0, viewer->width, viewer->height);

    camera_set_ratio(((float)viewer->width) / ((float)viewer->height),
                     uip->camera.projection);

    camera_buffer_object_update_projection(&uip->scene.camera,
            MAT_CONST_CAST(uip->camera.projection));

    uniform_buffer_send(&uip->scene.camera);
}

static void resize_callback_default(struct Viewer* viewer, void* data) {
    glViewport(0, 0, viewer->width, viewer->height);
}

static void key_callback(struct Viewer* viewer,
                         int key, int scancode, int action, int mods,
                         void* data) {
    struct UI* ui = data;
    struct UIPrivate* uip = ui->private;

    if (action != GLFW_PRESS) return;

    switch (key) {
        case GLFW_KEY_P:
            if (ui->select) {
                ui->action = W_PASS;
                ui->select = 0;
            }
            break;
        case GLFW_KEY_BACKSPACE:
            if (ui->select) {
                ui->action = W_UNDO;
                ui->select = 0;
            }
        case GLFW_KEY_HOME:
            {
                Quaternion q;
                Vec3 t = {0, 0, 0};
                Vec3 axisX = {1, 0, 0};
                quaternion_set_axis_angle(q, axisX, 0);
                node_set_orientation(uip->camOrientation, q);

                t[2] = uip->defCamDist;
                node_set_pos(uip->camNode, t);
            }
        default:
            break;
    }
    return;
}

static int intersectlp(Vec3 dest, Vec3 p, Vec3 v, Vec4 plane) {
    float d1, d2, t;

    d1 = dot3(v, plane);
    d2 = dot3(p, plane);
    if (d1 == 0.) { /* if line and plane parallel... */
        if (d2 + plane[3] == 0.) { /* if base point belongs to plane... */
            /* then the line is included in the plane, so the base point is a
             * valid solution */
            memcpy(dest, p, sizeof(Vec3));
            return 1;
        }
        /* else no intersection */
        return 0;
    }
    t = -d2 / (d1 + plane[3]);
    dest[0] = p[0] + t * v[0];
    dest[1] = p[1] + t * v[1];
    dest[2] = p[2] + t * v[2];
    return 1;
}

static int update_cursor_pos(struct UI* ui, double x, double y) {
    struct UIPrivate* uip = ui->private;
    Vec4 v = {0, 0, -1, 1}, res, plane = {0, 0, 1, 0};
    Vec3 vec, pos, boardPos;
    Mat4 invView;
    float s = ui->weiqi->boardSize - 1;
    struct InterfaceTheme* theme = &ui->config->themes[ui->config->curTheme];

    invert4m(invView, MAT_CONST_CAST(uip->camera.view));
    v[0] = - x * v[2] / uip->camera.projection[0][0];
    v[1] = - y * v[2] / uip->camera.projection[1][1];
    mul4mv(res, MAT_CONST_CAST(invView), v);

    pos[0] = invView[3][0];
    pos[1] = invView[3][1];
    pos[2] = invView[3][2];

    vec[0] = res[0] - pos[0];
    vec[1] = res[1] - pos[1];
    vec[2] = res[2] - pos[2];

    /* plane[3] = -h; */
    if (!intersectlp(boardPos, pos, vec, plane)) return 0;

    boardPos[0] = boardPos[0] / theme->gridScale + 0.5;
    boardPos[1] = boardPos[1] / theme->gridScale + 0.5;
    if (       boardPos[0] >= 0. && boardPos[0] <= 1.
            && boardPos[1] >= 0. && boardPos[1] <= 1.) {
        uip->cursorPos[0] = (boardPos[0] + 1. / (2. * (float) s)) * s;
        uip->cursorPos[1] = (boardPos[1] + 1. / (2. * (float) s)) * s;
    }
    return 1;
}

static void mouse_callback(struct Viewer* viewer,
                           int button, int action, int mods,
                           void* data) {
    struct UI* ui = data;
    struct UIPrivate* uip = ui->private;

    if (       button == GLFW_MOUSE_BUTTON_LEFT
            && action == GLFW_PRESS
            && ui->select) {
        memcpy(ui->selectPos, uip->cursorPos, sizeof(ui->selectPos));
        ui->action = W_PLAY;
        ui->select = 0;
    }
}

static void cursor_callback(struct Viewer* viewer, double xpos, double ypos,
                            double dx, double dy, int bl, int bm, int br,
                            void* data) {
    struct UI* ui = data;
    struct UIPrivate* uip = ui->private;
    Vec3 axisZ = {0, 0, 1};
    Vec3 axisX = {1, 0, 0};

    update_cursor_pos(ui, 2 * xpos / viewer->width - 1,
                      1. - 2 * ypos / viewer->height);
    if (br && ui->config->themes[ui->config->curTheme].fov != 0) {
        node_rotate(uip->camOrientation, axisZ, -dx / viewer->width);
        node_slew(uip->camOrientation, axisX, -dy / viewer->width);
    }
    return;
}

static void wheel_callback(struct Viewer* viewer, double dx, double dy,
                           void* data) {
    struct UI* ui = data;
    struct UIPrivate* uip = ui->private;
    Vec3 t = {0, 0, 0};

    if (ui->config->themes[ui->config->curTheme].fov != 0) {
        if (dy < 0) {
            t[2] = uip->camNode->position[2] * (1 - dy / 10.);
        } else {
            t[2] = uip->camNode->position[2] / (1 + dy / 10.);
        }
        node_set_pos(uip->camNode, t);
    }
}

static void close_callback(struct Viewer* viewer, void* data) {
    struct UI* ui = data;

    ui->status = W_UI_QUIT;
    return;
}

static void idle_loop(struct UI* ui) {
    struct UIPrivate* uip = ui->private;

    while (ui->status == W_UI_IDLE) {
        viewer_next_frame(uip->viewer);
        viewer_process_events(uip->viewer);
    }
}

static void render_board(struct UI* ui) {
    struct UIPrivate* uip = ui->private;
    struct Node* board = uip->assets.board;
    struct Geometry* geom = board->data.geometry;
    struct InterfaceTheme* theme = &ui->config->themes[ui->config->curTheme];
    Mat4 m;
    Mat3 invNormal;

    load_id4(m);
    load_id3(invNormal);
    material_use(geom->material);
    material_set_matrices(geom->material, m, invNormal);
    if (theme->shadow) {
        material_bind_shadowmaps(geom->material, &uip->assets.lights);
    }
    vertex_array_render(geom->vertexArray);
}

static void render_stones(struct UI* ui) {
    struct UIPrivate* uip = ui->private;
    struct Node* wstone = uip->assets.wStone;
    struct Node* bstone = uip->assets.bStone;
    struct Geometry* wStoneGeom = wstone->data.geometry;
    struct Geometry* bStoneGeom = bstone->data.geometry;
    Mat4 model;
    Mat3 invNormal, tmp;
    float s = ui->weiqi->boardSize;
    unsigned char row, col, size = ui->weiqi->boardSize;
    struct InterfaceTheme* theme = &ui->config->themes[ui->config->curTheme];

    load_id4(model);

    model[3][2] = theme->stoneThickness * theme->stoneRadius;
    model[2][2] = theme->stoneThickness;

    /* compute inverse normal only once (same for all stones) */
    mat4to3(tmp, MAT_CONST_CAST(model));
    invert3m(invNormal, MAT_CONST_CAST(tmp));
    transpose3m(invNormal);

    /* render white stones first, load material only once */
    material_use(wStoneGeom->material);
    for (row = 0; row < ui->weiqi->boardSize; row++) {
        for (col = 0; col < ui->weiqi->boardSize; col++) {
            if (ui->weiqi->board[row * size + col] != W_WHITE) {
                continue;
            }
            model[3][0] = theme->gridScale * (col * (1. / (s - 1)) - 0.5);
            model[3][1] = theme->gridScale * (row * (1. / (s - 1)) - 0.5);
            material_set_matrices(wStoneGeom->material, model, invNormal);
            vertex_array_render(wStoneGeom->vertexArray);
        }
    }

    /* render black stones second, load material only once */
    material_use(bStoneGeom->material);
    for (row = 0; row < ui->weiqi->boardSize; row++) {
        for (col = 0; col < ui->weiqi->boardSize; col++) {
            if (ui->weiqi->board[row * size + col] != W_BLACK) {
                continue;
            }
            model[3][0] = theme->gridScale * (col * (1. / (s - 1)) - 0.5);
            model[3][1] = theme->gridScale * (row * (1. / (s - 1)) - 0.5);
            material_set_matrices(bStoneGeom->material, model, invNormal);
            vertex_array_render(bStoneGeom->vertexArray);
        }
    }
    return;
}

static int board_state_changed(struct UI* ui) {
    struct UIPrivate* uip = ui->private;

    if (ui->weiqi->history.last) {
        if (uip->lastMove != ui->weiqi->history.last->nmove) {
            uip->lastMove = ui->weiqi->history.last->nmove;
            return 1;
        }
        return 0;
    } else if (uip->lastMove != -1) {
        uip->lastMove = -1;
        return 1;
    }
    return 0;
}

static void render_shadowmap(struct UI* ui) {
    struct UIPrivate* uip = ui->private;
    struct Lights* l = &uip->assets.lights;
    struct InterfaceTheme* theme = &ui->config->themes[ui->config->curTheme];
    int id = 0, row, col, size = ui->weiqi->boardSize;
    float s = ui->weiqi->boardSize;
    GLint viewport[4];
    struct Node *bStone = uip->assets.bStone, *wStone = uip->assets.wStone;

    load_id4(bStone->model);
    load_id4(wStone->model);
    bStone->model[3][2] = theme->stoneThickness * theme->stoneRadius;
    bStone->model[2][2] = theme->stoneThickness;
    wStone->model[3][2] = theme->stoneThickness * theme->stoneRadius;
    wStone->model[2][2] = theme->stoneThickness;

    light_shadowmap_render_start(l, id, viewport);
    for (row = 0; row < ui->weiqi->boardSize; row++) {
        for (col = 0; col < ui->weiqi->boardSize; col++) {
            if (ui->weiqi->board[row * size + col] == W_BLACK) {
                bStone->model[3][0] = theme->gridScale
                                    * (col * (1. / (s - 1)) - 0.5);
                bStone->model[3][1] = theme->gridScale
                                    * (row * (1. / (s - 1)) - 0.5);
                light_shadowmap_render_node(bStone);
            } else if (ui->weiqi->board[row * size + col] == W_WHITE) {
                wStone->model[3][0] = theme->gridScale
                                    * (col * (1. / (s - 1)) - 0.5);
                wStone->model[3][1] = theme->gridScale
                                    * (row * (1. / (s - 1)) - 0.5);
                light_shadowmap_render_node(wStone);
            }
        }
    }
    light_shadowmap_render_end(l, viewport);
}


static void render_pointer(struct UI* ui) {
    struct UIPrivate* uip = ui->private;
    struct Node* pointer = uip->assets.pointer;
    struct Geometry* geom = pointer->data.geometry;
    Mat4 m;
    Mat3 invNormal;
    float s = ui->weiqi->boardSize;
    unsigned char col = uip->cursorPos[0], row = uip->cursorPos[1];
    struct InterfaceTheme* theme = &ui->config->themes[ui->config->curTheme];

    load_id4(m);
    load_id3(invNormal);

    m[3][0] = theme->gridScale * (col * (1. / (s - 1)) - 0.5);
    m[3][1] = theme->gridScale * (row * (1. / (s - 1)) - 0.5);
    m[3][2] = 0;
    m[2][2] = 1.;

    material_use(geom->material);
    material_set_matrices(geom->material, m, invNormal);
    vertex_array_render(geom->vertexArray);
    return;
}

static void render_lmvp(struct UI* ui) {
    struct UIPrivate* uip = ui->private;
    struct Node* lmvp = uip->assets.lmvPointer;
    struct Geometry* geom = lmvp->data.geometry;
    Mat4 m;
    Mat3 invNormal;
    float s = ui->weiqi->boardSize;
    unsigned char col, row;
    struct InterfaceTheme* theme = &ui->config->themes[ui->config->curTheme];

    if (!ui->weiqi->history.last || ui->weiqi->history.last->action != W_PLAY) {
        return;
    }
    col = ui->weiqi->history.last->col;
    row = ui->weiqi->history.last->row;

    load_id4(m);
    load_id3(invNormal);

    m[3][0] = theme->gridScale * (col * (1. / (s - 1)) - 0.5);
    m[3][1] = theme->gridScale * (row * (1. / (s - 1)) - 0.5);
    m[3][2] = theme->stoneThickness * theme->stoneRadius;
    m[2][2] = 1.;

    material_use(geom->material);
    material_set_matrices(geom->material, m, invNormal);
    vertex_array_render(geom->vertexArray);
    return;
}

static int setup_camera(struct UI* ui) {
    struct UIPrivate* uip = ui->private;
    Vec3 t = {0, 0, 0};
    struct InterfaceTheme* theme = &ui->config->themes[ui->config->curTheme];

    uip->camNode = NULL;
    uip->camOrientation = NULL;
    if (       !(uip->camNode = malloc(sizeof(struct Node)))
            || !(uip->camOrientation = malloc(sizeof(struct Node)))) {
        fprintf(stderr, "Error: setup_camera: malloc failed\n");
        free(uip->camNode);
        return 0;
    }
    node_init(uip->camOrientation);
    node_init(uip->camNode);
    node_set_camera(uip->camNode, &uip->camera);
    if (       !node_add_child(&uip->scene.root, uip->camOrientation)
            || !node_add_child(uip->camOrientation, uip->camNode)) {
        return 0;
    }
    if (theme->fov > 0.0) {
        float tanFOV = tan(theme->fov / 360. * M_PI);
        t[2] = (0.5 / tanFOV + theme->boardThickness / 2.);
    } else {
        t[2] = 1;
    }
    uip->defCamDist = t[2];
    node_translate(uip->camNode, t);

    camera_set_ratio(((float)uip->viewer->width) / ((float)uip->viewer->height),
                     uip->camera.projection);

    camera_buffer_object_update_projection(&uip->scene.camera,
            MAT_CONST_CAST(uip->camera.projection));

    uniform_buffer_send(&uip->scene.camera);
    return 1;
}

static void run_loop(struct UI* ui) {
    struct UIPrivate* uip = ui->private;
    char ok = 0;
    struct InterfaceTheme* theme = &ui->config->themes[ui->config->curTheme];

    if (theme->fov == 0.) {
        camera_ortho_projection(1., 1., 0.1, 1000., uip->camera.projection);
    } else {
        camera_projection(1., theme->fov / 360. * 2 * M_PI, 0.001, 1000.,
                          uip->camera.projection);
    }

    if (theme->gridScale == 0.) {
        float s = ui->weiqi->boardSize;
        theme->gridScale = s / (s + 2);
    }

    theme->stoneRadius = 1. / (2. * (float)(ui->weiqi->boardSize))
                            * theme->gridScale;
    theme->pointerSize = theme->stoneRadius / 2.;
    /* we're fancy and make a last move pointer that's both a golden rectangle
     * and fits perfectly into a stone
     */
    {
        float phi = (1. + sqrt(5.)) / 2.;
        float l = 0.6;

        theme->lmvph = l * theme->stoneRadius;
        theme->lmvpw = phi * theme->lmvph;
    }

    if (!assets_load(&uip->assets, ui->weiqi->boardSize, theme)) {
        fprintf(stderr, "Error: can't load assets\n");
    } else if (!scene_init(&uip->scene, &uip->camera)) {
        fprintf(stderr, "Error: interface: can't init scene\n");
    } else if (!setup_camera(ui)) {
        fprintf(stderr, "Error: can't setup camera\n");
    } else {
        uniform_buffer_bind(&uip->scene.camera, CAMERA_UBO_BINDING);
        uniform_buffer_bind(&uip->scene.lights, LIGHTS_UBO_BINDING);

        /* IBL and lights don't like each other very much apparently, so only
         * upload lights if IBL is not enabled.
         */
        if (!uip->assets.ibl.enabled) {
            lights_buffer_object_update(&uip->scene.lights,
                                        &uip->assets.lights);
            uniform_buffer_send(&uip->scene.lights);
        }

        uip->viewer->key_callback = key_callback;
        uip->viewer->cursor_callback = cursor_callback;
        uip->viewer->mouse_callback = mouse_callback;
        uip->viewer->wheel_callback = wheel_callback;
        uip->viewer->resize_callback = resize_callback_run;

        uip->lastMove = -2;  /* -2: we don't know anything about the board,
                                -1: no move but we checked board state once,
                                 0: 1 move that we checked, etc
                              */
        while (ui->status == W_UI_RUN) {
            viewer_next_frame(uip->viewer);
            viewer_process_events(uip->viewer);

            scene_update_nodes(&uip->scene, update_node, NULL);
            uniform_buffer_send(&uip->scene.camera);

            if (theme->shadow && board_state_changed(ui)) {
                render_shadowmap(ui);
                uniform_buffer_bind(&uip->scene.camera, CAMERA_UBO_BINDING);
                uniform_buffer_bind(&uip->scene.lights, LIGHTS_UBO_BINDING);
            }
            render_board(ui);
            render_stones(ui);
            render_pointer(ui);
            render_lmvp(ui);
        }
        if (ui->status == W_UI_QUIT) ok = 1;
    }

    if (!ok) {
        ui->status = W_UI_CRASH;
    }
    assets_free(&uip->assets);
    scene_free(&uip->scene, NULL);
    free(uip->camNode);
    free(uip->camOrientation);
    uip->viewer->key_callback = NULL;
    uip->viewer->cursor_callback = NULL;
    uip->viewer->mouse_callback = NULL;
    uip->viewer->wheel_callback = NULL;
    uip->viewer->resize_callback = resize_callback_default;
}

static void player_select_widget(struct nk_context* nkctx,
                                 struct UI* ui,
                                 int* color,
                                 int* selection) {
    const char* players[] = {"White", "Black", "Random"};

    nk_combobox(nkctx, players, 3, color, 35,
                nk_vec2(nk_widget_width(nkctx), 200));
    if (nk_combo_begin_label(nkctx,
                             *selection > 0
                                ? ui->config->engines[*selection - 1].name
                                : "human",
                             nk_vec2(nk_widget_width(nkctx), 200))) {
        int i;
        nk_layout_row_dynamic(nkctx, 35, 1);
        if (nk_combo_item_label(nkctx, "human", NK_TEXT_LEFT)) {
            *selection = 0;
        } else {
            for (i = 0; i < ui->config->numEngines; i++) {
                if (nk_combo_item_label(nkctx,
                                        ui->config->engines[i].name,
                                        NK_TEXT_LEFT)) {
                    *selection = i + 1;
                }
            }
        }
        nk_combo_end(nkctx);
    }
}

static int player_index_from_config(struct Config* c, struct PlayerConf* conf) {
    int i;

    switch (conf->type) {
        case W_GTP_LOCAL:
            for (i = 0; i < c->numEngines; i++) {
                if (!strcmp(c->engines[i].command, conf->gtpCmd)) {
                    return i + 1;
                }
            }
        default:
            return 0;
    }
    return 0;
}

static void player_config_from_index(struct Config* c,
                                     struct PlayerConf* conf,
                                     int idx) {
    if (idx == 0) {
        conf->type = W_HUMAN;
        return;
    }
    conf->type = W_GTP_LOCAL;
    conf->gtpCmd = c->engines[idx - 1].command;
}

static void player_select(struct nk_context* nkctx, struct UI* ui) {
    static int player1 = 0, player2 = 0, playerColor1 = 0, playerColor2 = 1;

    if (ui->config->randomPlayer) {
        playerColor1 = 2;
        playerColor2 = 2;
        player1 = player_index_from_config(ui->config, &ui->config->white);
        player2 = player_index_from_config(ui->config, &ui->config->black);
    } else if (playerColor1 == 0) {
        player1 = player_index_from_config(ui->config, &ui->config->white);
        player2 = player_index_from_config(ui->config, &ui->config->black);
    } else {
        player1 = player_index_from_config(ui->config, &ui->config->black);
        player2 = player_index_from_config(ui->config, &ui->config->white);
    }
    player_select_widget(nkctx, ui, &playerColor1, &player1);
    switch (playerColor1) {
        case 0: playerColor2 = 1; break;
        case 1: playerColor2 = 0; break;
        case 2: playerColor2 = 2; break;
        default: break;
    }
    player_select_widget(nkctx, ui, &playerColor2, &player2);
    switch (playerColor2) {
        case 0: playerColor1 = 1; break;
        case 1: playerColor1 = 0; break;
        case 2: playerColor1 = 2; break;
        default: break;
    }

    if (playerColor1 == 0) {
        player_config_from_index(ui->config, &ui->config->white, player1);
        player_config_from_index(ui->config, &ui->config->black, player2);
        ui->config->randomPlayer = 0;
    } else if (playerColor1 == 1) {
        player_config_from_index(ui->config, &ui->config->black, player1);
        player_config_from_index(ui->config, &ui->config->white, player2);
        ui->config->randomPlayer = 0;
    } else {
        player_config_from_index(ui->config, &ui->config->white, player1);
        player_config_from_index(ui->config, &ui->config->black, player2);
        ui->config->randomPlayer = 1;
    }
}

static void theme_select(struct nk_context* nkctx, struct UI* ui) {
    nk_label(nkctx, "Theme", NK_TEXT_ALIGN_LEFT);

    if (nk_combo_begin_label(nkctx,
                             ui->config->themes[ui->config->curTheme].name,
                             nk_vec2(nk_widget_width(nkctx), 200))) {
        int i;
        nk_layout_row_dynamic(nkctx, 35, 1);
        for (i = 0; i < ui->config->numThemes; i++) {
            if (nk_combo_item_label(nkctx,
                                    ui->config->themes[i].name,
                                    NK_TEXT_LEFT)) {
                ui->config->curTheme = i;
            }
        }
        nk_combo_end(nkctx);
    }
}

static void config_ui_update(struct nk_context* nkctx, struct UI* ui) {
    struct UIPrivate* uip = ui->private;
    struct Viewer* viewer = uip->viewer;
    int w = 400, h = 600;

    nk_input_begin(nkctx);
    nk_input_motion(nkctx, viewer->cursorPos[0], viewer->cursorPos[1]);
    nk_input_button(nkctx,
                    NK_BUTTON_LEFT,
                    viewer->cursorPos[0],
                    viewer->cursorPos[1],
                    viewer->buttonPressed[0]);
    nk_input_button(nkctx,
                    NK_BUTTON_MIDDLE,
                    viewer->cursorPos[0],
                    viewer->cursorPos[1],
                    viewer->buttonPressed[1]);
    nk_input_button(nkctx,
                    NK_BUTTON_RIGHT,
                    viewer->cursorPos[0],
                    viewer->cursorPos[1],
                    viewer->buttonPressed[2]);
    nk_input_end(nkctx);

    nk_begin(nkctx, "Game configuration",
             nk_rect(viewer->width / 2 - w / 2, 100, w, h),
             NK_WINDOW_TITLE | NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR);
    nk_layout_row_dynamic(nkctx, 40, 2);

    player_select(nkctx, ui);

    nk_label(nkctx, "Board size", NK_TEXT_ALIGN_LEFT);
    nk_property_int(nkctx, "", 5, &ui->config->boardSize, 25, 1, 0.5);

    nk_label(nkctx, "Handicap", NK_TEXT_ALIGN_LEFT);
    nk_property_int(nkctx, "", 0, &ui->config->handicap, 9, 1, 0.5);

    theme_select(nkctx, ui);

    nk_layout_row_dynamic(nkctx, 40, 1);
    if (nk_button_label(nkctx, "Start game")) {
        ui->status = W_UI_IDLE;
    }
    nk_end(nkctx);
}

static void config_loop(struct UI* ui) {
    struct UIPrivate* uip = ui->private;
    struct nk_context nkctx;
    struct nk_font_config cfg;
    const void* image;
    int w, h;

    if (!tdnk_init(&uip->nkdev, uip->viewer)) goto error;

    cfg = nk_font_config(0);
    nk_font_atlas_init_default(&uip->atlas);
    nk_font_atlas_begin(&uip->atlas);
    if (!(uip->font = nk_font_atlas_add_from_file(&uip->atlas,
                                                  W_DATA_SRC "/font.ttf",
                                                  30.0f,
                                                  &cfg))) {
        fprintf(stderr, "Error: can't load font\n");
        goto error;
    } else if (!(image = nk_font_atlas_bake(&uip->atlas, &w, &h,
                                            NK_FONT_ATLAS_RGBA32))) {
        fprintf(stderr, "Error: can't bake font\n");
        goto error;
    }

    tdnk_upload_atlas(&uip->nkdev, image, w, h);
    nk_font_atlas_end(&uip->atlas,
                      nk_handle_id((int)uip->nkdev.font_tex),
                      &uip->nkdev.null);
    nk_init_default(&nkctx, &uip->font->handle);
    while (ui->status == W_UI_CONFIG) {
        viewer_next_frame(uip->viewer);
        viewer_process_events(uip->viewer);
        config_ui_update(&nkctx, ui);


        tdnk_render(&uip->nkdev,
                    &nkctx,
                    nk_vec2(1, 1),
                    NK_ANTI_ALIASING_ON);
    }
    nk_font_atlas_clear(&uip->atlas);
    nk_free(&nkctx);
    tdnk_shutdown(&uip->nkdev);
    return;

error:
    ui->status = W_UI_CRASH;
}

static void* do_start_ui(void* data) {
    struct UI* ui = data;
    struct UIPrivate uip = {0};
    struct InterfaceTheme* theme = &ui->config->themes[ui->config->curTheme];

    ui->private = &uip;

    if (!(uip.viewer = viewer_new(640, 640, "weiqi"))) {
        fprintf(stderr, "Error: interface: can't create viewer\n");
        ui->status = W_UI_CRASH;
    } else {
        uip.viewer->callbackData = ui;
        uip.viewer->close_callback = close_callback;
        uip.viewer->resize_callback = resize_callback_default;

        glClearColor(theme->backgroundColor[0],
                     theme->backgroundColor[1],
                     theme->backgroundColor[2],
                     0);
        ui->status = W_UI_IDLE;
        while (ui->status != W_UI_QUIT) {
            switch (ui->status) {
                case W_UI_IDLE:
                    idle_loop(ui);
                    break;
                case W_UI_RUN:
                    run_loop(ui);
                    break;
                case W_UI_CONFIG:
                    config_loop(ui);
                    break;
                default:
                    goto exit;
            }
        }
    }
    viewer_free(uip.viewer);
exit:
    pthread_exit(NULL);
}

/* following functions are to be called from the master thread */

int ui_wait(struct UI* ui, enum UIStatus status) {
    while (ui->status != status) {
        struct timespec t;
        t.tv_nsec = 10000000;
        t.tv_sec = 0;
        nanosleep(&t, NULL);
        switch (ui->status) {
            case W_UI_QUIT:
                return W_QUIT;
            case W_UI_CRASH:
                return W_ERROR;
            default:
                break;
        }
    }
    return W_NO_ERROR;
}

int ui_start(struct UI* ui, struct Weiqi* weiqi, struct Config* config) {
    memset(ui, 0, sizeof(*ui));
    ui->config = config;
    ui->weiqi = weiqi;
    ui->private = NULL;
    ui->status = W_UI_STARTUP;

    if (pthread_create(&ui->thread, NULL, do_start_ui, ui) != 0) {
        fprintf(stderr, "Error: interface: couldn't start thread\n");
        return 0;
    }

    return ui_wait(ui, W_UI_IDLE);
}

int ui_stop(struct UI* ui) {
    ui->status = W_UI_QUIT;
    pthread_join(ui->thread, NULL);
    return W_NO_ERROR;
}

int ui_config_menu(struct UI* ui) {
    ui->status = W_UI_CONFIG;

    while (ui->status == W_UI_CONFIG) {
        struct timespec t;
        t.tv_nsec = 10000000;
        t.tv_sec = 0;
        nanosleep(&t, NULL);
    }
    return W_NO_ERROR;
}

int ui_game_start(struct UI* ui) {
    switch (ui->status) {
        case W_UI_QUIT:
            return W_QUIT;
        case W_UI_CRASH:
            return W_ERROR;
        default:
            break;
    }
    ui->status = W_UI_RUN;
    return W_NO_ERROR;
}

int ui_get_move(struct UI* ui,
                enum WeiqiColor color, enum MoveAction* action,
                unsigned char* row, unsigned char* col) {
    ui->select = 1;
    while (ui->select) {
        struct timespec t;
        t.tv_nsec = 10000000;
        t.tv_sec = 0;
        nanosleep(&t, NULL);
        
        switch (ui->status) {
            case W_UI_QUIT:
            case W_UI_CRASH:
                return ui->status;
            default:
                break;
        }
    }
    *action = ui->action;
    *col = ui->selectPos[0];
    *row = ui->selectPos[1];
    return ui->status;
}
