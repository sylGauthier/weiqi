#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <3dmr/render/camera_buffer_object.h>
#include <3dmr/render/lights_buffer_object.h>

#include "interface.h"
#include "utils.h"

#define SUN_DIRECTION   {-0.3, 0, -1}
#define SUN_COLOR       {0.8, 0.8, 0.8}
#define AMBIENT_COLOR   {5, 5, 5}

void update_node(struct Scene* scene, struct Node* n, void* data) {
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

void resize_callback(struct Viewer* viewer, void* data) {
    struct Interface* interface = data;

    glViewport(0, 0, viewer->width, viewer->height);

    camera_set_ratio(((float)viewer->width) / ((float)viewer->height),
                     interface->camera.projection);

    camera_buffer_object_update_projection(&interface->scene.camera,
            MAT_CONST_CAST(interface->camera.projection));

    uniform_buffer_send(&interface->scene.camera);
}

void key_callback(struct Viewer* viewer, int key, int scancode, int action,
                  int mods, void* data) {
    struct Interface* interface = data;

    if (action != GLFW_PRESS) return;

    switch (key) {
        case GLFW_KEY_P:
            if (interface->status == W_UI_SELECT) {
                interface->pass = 1;
                interface->status = W_UI_RUN;
            }
            break;
        case GLFW_KEY_BACKSPACE:
            if (interface->status == W_UI_SELECT) {
                interface->status = W_UI_UNDO;
            }
        case GLFW_KEY_HOME:
            {
                Quaternion q;
                quaternion_load_id(q);
                node_set_orientation(interface->camOrientation, q);
            }
        default:
            break;
    }
    return;
}

static int update_cursor_pos(struct Interface* ui, double x, double y) {
    Vec4 v = {0, 0, -1, 1}, res;
    Vec3 vec, pos, boardPos;
    Mat4 invView;
    float h = ui->theme.boardThickness / 2;
    float s = ui->weiqi->boardSize - 1;

    invert4m(invView, MAT_CONST_CAST(ui->camera.view));
    v[0] = - x * v[2] / ui->camera.projection[0][0];
    v[1] = - y * v[2] / ui->camera.projection[1][1];
    mul4mv(res, MAT_CONST_CAST(invView), v);

    pos[0] = invView[3][0];
    pos[1] = invView[3][1];
    pos[2] = invView[3][2];

    vec[0] = res[0] - pos[0];
    vec[1] = res[1] - pos[1];
    vec[2] = res[2] - pos[2];

    if (vec[2] == 0.) return 0;
    if (vec[0] == 0.) boardPos[0] = res[0];
    else boardPos[0] = vec[0] * (h - pos[2]) / vec[2] + pos[0];
    if (vec[1] == 0.) boardPos[1] = res[1];
    else boardPos[1] = vec[1] * (h - pos[2]) / vec[2] + pos[1];
    boardPos[2] = h;

    boardPos[0] = boardPos[0] / ui->theme.gridScale + 0.5;
    boardPos[1] = boardPos[1] / ui->theme.gridScale + 0.5;
    if (       boardPos[0] >= 0. && boardPos[0] <= 1.
            && boardPos[1] >= 0. && boardPos[1] <= 1.) {
        ui->cursorPos[0] = (boardPos[0] + 1. / (2. * (float) s)) * s;
        ui->cursorPos[1] = (boardPos[1] + 1. / (2. * (float) s)) * s;
    }
    return 1;
}

void mouse_callback(struct Viewer* viewer, int button, int action, int mods,
                    void* data) {
    struct Interface* ui = data;

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS
            && ui->status == W_UI_SELECT) {
        memcpy(ui->selectPos, ui->cursorPos, sizeof(ui->selectPos));
        ui->status = W_UI_RUN;
    }
}

void cursor_callback(struct Viewer* viewer, double xpos, double ypos,
                     double dx, double dy, int bl, int bm, int br,
                     void* data) {
    struct Interface* ui = data;
    Vec3 axisZ = {0, 0, 1};
    Vec3 axisX = {1, 0, 0};

    update_cursor_pos(ui, 2 * xpos / viewer->width - 1,
                      1. - 2 * ypos / viewer->height);
    if (br) {
        node_rotate(ui->camOrientation, axisZ, -dx / viewer->width);
        node_slew(ui->camOrientation, axisX, -dy / viewer->width);
    }
    return;
}

void close_callback(struct Viewer* viewer, void* data) {
    struct Interface* ui = data;

    ui->status = W_UI_QUIT;
    return;
}

static void render_stone(struct Interface* ui, enum WeiqiColor color,
                         unsigned char row, unsigned char col) {
    Mat4 model;
    Mat3 invNormal;
    float s = ui->weiqi->boardSize;
    float zScale = ui->theme.stoneZScale;
    struct Asset3D* stone;

    load_id4(model);
    load_id3(invNormal);
    stone = color == W_WHITE ? &ui->wStone : &ui->bStone;

    model[3][0] = ui->theme.gridScale * (col * (1. / (s - 1)) - 0.5);
    model[3][1] = ui->theme.gridScale * (row * (1. / (s - 1)) - 0.5);
    model[3][2] = ui->theme.boardThickness / 2.
                  + zScale * ui->theme.stoneRadius;
    model[2][2] = zScale;

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

    model[3][0] = ui->theme.gridScale * (col * (1. / (s - 1)) - 0.5);
    model[3][1] = ui->theme.gridScale * (row * (1. / (s - 1)) - 0.5);
    model[3][2] = ui->theme.boardThickness / 2.;
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

    model[3][0] = ui->theme.gridScale * (col * (1. / (s - 1)) - 0.5);
    model[3][1] = ui->theme.gridScale * (row * (1. / (s - 1)) - 0.5);
    model[3][2] = ui->theme.boardThickness / 2.
                  + ui->theme.stoneZScale * ui->theme.stoneRadius;
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

static void setup_lighting(struct Scene* scene) {
    struct DirectionalLight l = {SUN_DIRECTION, SUN_COLOR};
    struct AmbientLight a = {AMBIENT_COLOR};
    lights_buffer_object_update_dlight(&scene->lights, &l, 0);
    lights_buffer_object_update_ndlight(&scene->lights, 1);
    lights_buffer_object_update_ambient(&scene->lights, &a);
    uniform_buffer_send(&scene->lights);
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

    camera_projection(1., 30 / 360. * 2 * M_PI, 0.001, 1000.,
                      ui->camera.projection);
    asset_init(&ui->board);
    asset_init(&ui->wStone);
    asset_init(&ui->bStone);
    asset_init(&ui->pointer);
    asset_init(&ui->lmvpointer);

    ui->theme.stoneRadius = 1. / (2. * (float)(ui->weiqi->boardSize))
                            * ui->theme.gridScale;
    ui->theme.pointerSize = ui->theme.stoneRadius / 2.;

    /* we're fancy and make a last move pointer that's both a golden rectangle
     * and fits perfectly into a stone
     */
    {
        float phi = (1. + sqrt(5.)) / 2.;
        float l = 0.6;

        ui->theme.lmvph = l * ui->theme.stoneRadius;
        ui->theme.lmvpw = phi * ui->theme.lmvph; 
    }

    if (!(ui->viewer = viewer_new(640, 640, "weiqi"))) {
        fprintf(stderr, "Error: interface: can't create viewer\n");
    } else if (!(sceneInit = scene_init(&ui->scene, &ui->camera))) {
        fprintf(stderr, "Error: interface: can't init scene\n");
    } else if (!(bc = board_create(&ui->board,
                                   ui->weiqi->boardSize,
                                   &ui->theme))) {
        fprintf(stderr, "Error: interface: can't create board\n");
    } else if (!(bsc = stone_create(&ui->bStone, 0, &ui->theme))) {
        fprintf(stderr, "Error: interface: can't create black stone\n");
    } else if (!(wsc = stone_create(&ui->wStone, 1, &ui->theme))) {
        fprintf(stderr, "Error: interface: can't create white stone\n");
    } else if (!(pc = pointer_create(&ui->pointer, &ui->theme))) {
        fprintf(stderr, "Error: interface: can't create pointer\n");
    } else if (!(lmpc = lmvpointer_create(&ui->lmvpointer, &ui->theme))) {
        fprintf(stderr, "Error: interface: can't create last move pointer\n");
    } else if (    !(ui->camNode = malloc(sizeof(struct Node)))
                || !(ui->camOrientation = malloc(sizeof(struct Node)))) {
        fprintf(stderr, "Error: interface: can't create cam node\n");
    } else {
        ui->viewer->callbackData = ui;
        ui->viewer->resize_callback = resize_callback;
        ui->viewer->key_callback = key_callback;
        ui->viewer->cursor_callback = cursor_callback;
        ui->viewer->mouse_callback = mouse_callback;
        ui->viewer->close_callback = close_callback;

        {
            Vec3 t = {0, 0, 2};
            node_init(ui->camOrientation);
            node_init(ui->camNode);
            node_set_camera(ui->camNode, &ui->camera);
            node_add_child(&ui->scene.root, ui->camOrientation);
            node_add_child(ui->camOrientation, ui->camNode);
            node_translate(ui->camNode, t);
        }
        setup_lighting(&ui->scene);

        while (ui->status != W_UI_QUIT) {
            viewer_process_events(ui->viewer);
            scene_update_nodes(&ui->scene, update_node, NULL);
            uniform_buffer_send(&ui->scene.camera);
            render_board(ui);
            viewer_next_frame(ui->viewer);
            set_title(ui);
        }
    }

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

int interface_init(struct Interface* ui, struct Weiqi* weiqi) {
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
