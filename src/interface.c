#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <3dmr/render/camera_buffer_object.h>
#include <3dmr/render/lights_buffer_object.h>
#include <3dmr/skybox.h>

#include "interface.h"
#include "callbacks.h"
#include "utils.h"

#define SUN_DIRECTION   {-0.3, 0, -1}
#define SUN_COLOR       {0.8, 0.8, 0.8}
#define AMBIENT_COLOR   {5, 5, 5}

static void render_stone(struct Interface* ui, enum WeiqiColor color,
                         unsigned char row, unsigned char col) {
    Mat4 model;
    Mat3 tmp, invNormal;
    float s = ui->weiqi->boardSize;
    float yScale = ui->theme->stoneYScale;
    struct Asset3D* stone;

    load_id4(model);
    stone = color == W_WHITE ? &ui->wStone : &ui->bStone;

    model[3][0] = ui->theme->gridScale * (col * (1. / (s - 1)) - 0.5);
    model[3][1] = ui->theme->boardThickness / 2.
                    + yScale * ui->theme->stoneRadius;
    model[3][2] =  -ui->theme->gridScale * (row * (1. / (s - 1)) - 0.5);
    model[1][1] = yScale;

    mat4to3(tmp, MAT_CONST_CAST(model));
    invert3m(invNormal, MAT_CONST_CAST(tmp));
    transpose3m(invNormal);

    material_use(stone->mat);
    material_set_matrices(stone->mat, model, invNormal);
    vertex_array_render(stone->va);
}

static void render_pointer(struct Interface* ui) {
    Mat4 model;
    Mat3 invNormal;
    float s = ui->weiqi->boardSize;
    unsigned char col = ui->cursorPos[0], row = ui->cursorPos[1];

    load_id4(model);
    load_id3(invNormal);

    model[3][0] = ui->theme->gridScale * (col * (1. / (s - 1)) - 0.5);
    model[3][1] = ui->theme->boardThickness / 2.;
    model[3][2] = -ui->theme->gridScale * (row * (1. / (s - 1)) - 0.5);
    model[2][2] = 1.;

    material_use(ui->pointer.mat);
    material_set_matrices(ui->pointer.mat, model, invNormal);
    vertex_array_render(ui->pointer.va);
}

static void render_lmvpointer(struct Interface* ui) {
    Mat4 model;
    Mat3 invNormal;
    float s = ui->weiqi->boardSize;
    unsigned char col, row;

    if (!ui->weiqi->history.last || ui->weiqi->history.last->action != W_PLAY) {
        return;
    }
    col = ui->weiqi->history.last->col;
    row = ui->weiqi->history.last->row;

    load_id4(model);
    load_id3(invNormal);

    model[3][0] = ui->theme->gridScale * (col * (1. / (s - 1)) - 0.5);
    model[3][1] = ui->theme->boardThickness / 2.
                  + ui->theme->stoneYScale * ui->theme->stoneRadius;
    model[3][2] = -ui->theme->gridScale * (row * (1. / (s - 1)) - 0.5);
    model[2][2] = 1.;

    material_use(ui->lmvpointer.mat);
    material_set_matrices(ui->lmvpointer.mat, model, invNormal);
    vertex_array_render(ui->lmvpointer.va);
}

static void render_board(struct Interface* ui) {
    Mat4 model;
    Mat3 invNormal;
    unsigned char row, col, s;

    load_id4(model);
    load_id3(invNormal);

    material_use(ui->board.mat);
    material_set_matrices(ui->board.mat, model, invNormal);
    vertex_array_render(ui->board.va);

    s = ui->weiqi->boardSize;
    for (row = 0; row < s; row++) {
        for (col = 0; col < s; col++) {
            if (ui->weiqi->board[row * s + col]) {
                render_stone(ui, ui->weiqi->board[row * s + col], row, col);
            }
        }
    }
    render_lmvpointer(ui);
    render_pointer(ui);
}

static char* strmerge(const char* s1, const char* s2) {
    char* res;
    if (       strlen(s1) < (size_t)(-1) / 2 && strlen(s2) < (size_t)(-1) / 2
            && (res = malloc(strlen(s1) + strlen(s2) + 1))) {
        strcpy(res, s1);
        strcpy(res + strlen(s1), s2);
    }
    return res;
}

static int setup_lighting(struct Scene* scene, struct InterfaceTheme* theme) {
    if (theme->style == W_UI_NICE && theme->ibl.enabled) {
        GLuint tex;
        char *path1 = NULL, *path2 = NULL, ok = 0;

        if (       !(path1 = strmerge(W_DATA_DIR"/textures/", theme->iblPath))
                || !(path2 = strmerge(W_DATA_SRC"/textures/", theme->iblPath))) {
            fprintf(stderr, "Error: lighting: can't merge strings\n");
        } else if (!(tex = skybox_load_texture_hdr_equirect(path1, 1024))
                && !(tex = skybox_load_texture_hdr_equirect(path2, 1024))) {
            fprintf(stderr, "Error: lighting: can't load IBL texture\n");
        } else {
            ok = compute_ibl(tex, 32, 1024, 5, 256, &theme->ibl);
        }
        free(path1);
        free(path2);
        return ok;
    } else {
        struct DirectionalLight l = {SUN_DIRECTION, SUN_COLOR};
        struct AmbientLight a = {AMBIENT_COLOR};
        lights_buffer_object_update_dlight(&scene->lights, &l, 0);
        lights_buffer_object_update_ndlight(&scene->lights, 1);
        lights_buffer_object_update_ambient(&scene->lights, &a);
        uniform_buffer_send(&scene->lights);
    }
    return 1;
}

static int setup_camera(struct Interface* ui) {
    Vec3 t = {0, 0, 0};
    Vec3 axis = {1, 0, 0};

    node_init(ui->camOrientation);
    node_init(ui->camNode);
    node_set_camera(ui->camNode, &ui->camera);
    if (       !node_add_child(&ui->scene.root, ui->camOrientation)
            || !node_add_child(ui->camOrientation, ui->camNode)) {
        return 0;
    }
    if (ui->theme->fov > 0.0) {
        float tanFOV = tan(ui->theme->fov / 360. * M_PI);
        t[2] = (0.5 / tanFOV + ui->theme->boardThickness / 2.);
    } else {
        t[2] = 1;
    }
    node_translate(ui->camNode, t);
    node_rotate(ui->camOrientation, axis, -M_PI / 2);
    return 1;
}

static void set_title(struct Interface* ui) {
    char title[256] = "", last[64] = "";
    char* status;
    struct Move* lastm = ui->weiqi->history.last;

    if (lastm) {
        char strMove[5];
        if (lastm->action == W_PASS) {
            strcpy(strMove, "PASS");
        } else {
            move_to_str(strMove, lastm->row, lastm->col);
        }
        snprintf(last, sizeof(last), "%s %s",
                 lastm->color == W_WHITE ? "white" : "black",
                 strMove);
    }
    if (ui->weiqi->gameOver) {
        status = "game over";
    } else {
        if (!lastm) status = "black's turn";
        else status = ui->weiqi->history.last->color == W_WHITE ?
                      "black's turn" : "white's turn";
    }
    snprintf(title, sizeof(title), "weiqi | W:%d | B:%d | last move: %s | %s",
             ui->weiqi->wcap,
             ui->weiqi->bcap,
             last,
             status);
    viewer_set_title(ui->viewer, title);
}

void* run_interface(void* arg) {
    struct Interface* ui = arg;
    int sceneInit = 0, bc = 0, bsc = 0, wsc = 0, pc = 0, lmpc = 0;

    if (ui->theme->fov == 0.) {
        camera_ortho_projection(1., 1., 0.1, 1000., ui->camera.projection);
    } else {
        camera_projection(1., ui->theme->fov / 360. * 2 * M_PI, 0.001, 1000.,
                          ui->camera.projection);
    }
    asset_init(&ui->board);
    asset_init(&ui->wStone);
    asset_init(&ui->bStone);
    asset_init(&ui->pointer);
    asset_init(&ui->lmvpointer);

    if (ui->theme->gridScale == 0.) {
        float s = ui->weiqi->boardSize;
        ui->theme->gridScale = s / (s + 2);
    }

    ui->theme->stoneRadius = 1. / (2. * (float)(ui->weiqi->boardSize))
                            * ui->theme->gridScale;
    ui->theme->pointerSize = ui->theme->stoneRadius / 2.;
    /* we're fancy and make a last move pointer that's both a golden rectangle
     * and fits perfectly into a stone
     */
    {
        float phi = (1. + sqrt(5.)) / 2.;
        float l = 0.6;

        ui->theme->lmvph = l * ui->theme->stoneRadius;
        ui->theme->lmvpw = phi * ui->theme->lmvph; 
    }

    ui->ok = 0;
    if (!(ui->viewer = viewer_new(640, 640, "weiqi"))) {
        fprintf(stderr, "Error: interface: can't create viewer\n");
    } else if (!(sceneInit = scene_init(&ui->scene, &ui->camera))) {
        fprintf(stderr, "Error: interface: can't init scene\n");
    } else if (!setup_lighting(&ui->scene, ui->theme)) {
        fprintf(stderr, "Error: interface: can't setup lighting\n");
    } else if (!(bc = board_create(&ui->board,
                                   ui->weiqi->boardSize,
                                   ui->theme))) {
        fprintf(stderr, "Error: interface: can't create board\n");
    } else if (!(bsc = stone_create(&ui->bStone, 0, ui->theme))) {
        fprintf(stderr, "Error: interface: can't create black stone\n");
    } else if (!(wsc = stone_create(&ui->wStone, 1, ui->theme))) {
        fprintf(stderr, "Error: interface: can't create white stone\n");
    } else if (!(pc = pointer_create(&ui->pointer, ui->theme))) {
        fprintf(stderr, "Error: interface: can't create pointer\n");
    } else if (!(lmpc = lmvpointer_create(&ui->lmvpointer, ui->theme))) {
        fprintf(stderr, "Error: interface: can't create last move pointer\n");
    } else if (    !(ui->camNode = malloc(sizeof(struct Node)))
                || !(ui->camOrientation = malloc(sizeof(struct Node)))) {
        fprintf(stderr, "Error: interface: can't create cam node\n");
    } else if (!setup_camera(ui)) {
        fprintf(stderr, "Error: interface: can't setup camera\n");
    } else {
        ui->viewer->callbackData = ui;
        ui->viewer->resize_callback = resize_callback;
        ui->viewer->key_callback = key_callback;
        ui->viewer->cursor_callback = cursor_callback;
        ui->viewer->mouse_callback = mouse_callback;
        ui->viewer->close_callback = close_callback;

        ui->ok = 1;
        glClearColor(ui->theme->backgroundColor[0],
                     ui->theme->backgroundColor[1],
                     ui->theme->backgroundColor[2],
                     0);
        while (ui->status != W_UI_QUIT) {
            viewer_process_events(ui->viewer);
            scene_update_nodes(&ui->scene, update_node, NULL);
            uniform_buffer_send(&ui->scene.camera);
            render_board(ui);
            viewer_next_frame(ui->viewer);
            set_title(ui);
        }
    }
    /* in case of error, ui->status is not W_QUIT so the main thread won't know
     * that the UI crashed, hence we set it to QUIT here.
     */
    ui->status = W_UI_QUIT;
    if (bc) asset_free(&ui->board);
    if (bsc) asset_free(&ui->bStone);
    if (wsc) asset_free(&ui->wStone);
    if (pc) asset_free(&ui->pointer);
    if (lmpc) asset_free(&ui->lmvpointer);
    if (ui->viewer) viewer_free(ui->viewer);
    if (sceneInit) scene_free(&ui->scene, NULL);
    free(ui->camNode);
    free(ui->camOrientation);
    pthread_exit(NULL);
}

int interface_init(struct Interface* ui,
                   struct Weiqi* weiqi,
                   struct InterfaceTheme* theme) {
    ui->theme = theme;
    ui->viewer = NULL;
    ui->weiqi = weiqi;
    ui->status = W_UI_RUN;
    ui->camNode = NULL;
    ui->camOrientation = NULL;
    ui->cursorPos[0] = 0;

    ui->cursorPos[1] = 0;
    ui->selectPos[0] = 0;
    ui->selectPos[1] = 0;
    ui->pass = 0;

    if (pthread_create(&ui->thread, NULL, run_interface, ui) != 0) {
        fprintf(stderr, "Error: interface: couldn't start thread\n");
        return 0;
    }

    return 1;
}

void interface_free(struct Interface* ui) {
    ui->status = W_UI_QUIT;
    pthread_join(ui->thread, NULL);
}

void interface_wait(struct Interface* ui) {
    while (ui->status != W_UI_QUIT) {
        struct timespec t;
        t.tv_nsec = 10000000;
        t.tv_sec = 0;
        nanosleep(&t, NULL);
    }
}

int interface_get_move(struct Interface* ui,
                       enum WeiqiColor color, enum MoveAction* action,
                       unsigned char* row, unsigned char* col) {
    ui->status = W_UI_SELECT;

    while (ui->status == W_UI_SELECT) {
        struct timespec t;
        t.tv_nsec = 10000000;
        t.tv_sec = 0;
        nanosleep(&t, NULL);
    }
    if (ui->pass) {
        *action = W_PASS;
        ui->pass = 0;
        return ui->status;
    }
    *action = W_PLAY;
    *col = ui->selectPos[0];
    *row = ui->selectPos[1];
    return ui->status;
}
