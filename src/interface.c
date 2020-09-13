#include <stdio.h>
#include <stdlib.h>

#include <3dmr/render/camera_buffer_object.h>

#include "interface.h"

void update_node(struct Scene* scene, struct Node* n, void* data) {
    switch (n->type) {
        case NODE_DLIGHT:
            break;
        case NODE_PLIGHT:
            break;
        case NODE_SLIGHT:
            break;
        case NODE_CAMERA:
            camera_buffer_object_update_view_and_position(&scene->camera, MAT_CONST_CAST(n->data.camera->view));
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
    return;
}

void cursor_callback(struct Viewer* viewer, double xpos, double ypos,
                     double dx, double dy, int bl, int bm, int br,
                     void* data) {
    struct Interface* ui = data;
    Vec3 axisZ = {0, 0, 1};
    Vec3 axisX = {1, 0, 0};

    if (bl) {
        node_rotate(ui->camOrientation, axisZ, -dx / viewer->width);
        node_slew(ui->camOrientation, axisX, -dy / viewer->width);
    }
    return;
}

void close_callback(struct Viewer* viewer, void* data) {
    return;
}

static void render_stone(struct Interface* ui, enum WeiqiColor color,
                         unsigned int row, unsigned int col) {
    Mat4 model;
    Mat3 invNormal;
    float s = ui->weiqi->boardSize;
    float zScale = 0.3;
    struct Asset3D* stone;

    load_id4(model);
    load_id3(invNormal);

    model[3][0] = ui->board.gridScale * (col * (1. / (s - 1)) - 0.5);
    model[3][1] = ui->board.gridScale * (row * (1. / (s - 1)) - 0.5);
    model[3][2] = ui->board.thickness / 2. + zScale * ui->stoneRadius;
    model[2][2] = zScale;

    stone = color == W_WHITE ? &ui->wStone : &ui->bStone;
    material_use(stone->mat);
    material_set_matrices(stone->mat, model, invNormal);
    vertex_array_render(stone->va);
}

static void render_board(struct Interface* ui) {
    Mat4 model;
    Mat3 invNormal;
    unsigned int row, col, s;

    load_id4(model);
    load_id3(invNormal);
    material_use(ui->board.geom.mat);
    material_set_matrices(ui->board.geom.mat, model, invNormal);
    vertex_array_render(ui->board.geom.va);

    s = ui->weiqi->boardSize;
    for (row = 0; row < s; row++) {
        for (col = 0; col < s; col++) {
            if (ui->weiqi->board[row * s + col]) {
                render_stone(ui, ui->weiqi->board[row * s + col], row, col);
            }
        }
    }
}

void* run_interface(void* arg) {
    struct Interface* ui = arg;
    int sceneInit = 0;
    float scale = 1. / 1.1;

    ui->stoneRadius = 1. / (2. * (float)ui->weiqi->boardSize) * scale;
    camera_projection(1., 30 / 360. * 2 * M_PI, 0.001, 1000.,
                      ui->camera.projection);
    asset_init(&ui->board.geom);
    asset_init(&ui->wStone);
    asset_init(&ui->bStone);

    if (!(ui->viewer = viewer_new(640, 640, "weiqi"))) {
        fprintf(stderr, "Error: interface: can't create viewer\n");
    } else if (!(sceneInit = scene_init(&ui->scene, &ui->camera))) {
        fprintf(stderr, "Error: interface: can't init scene\n");
    } else if (!board_create(&ui->board, ui->weiqi->boardSize, scale,
                             0.59, 0.5, 0.3)) {
        fprintf(stderr, "Error: interface: can't create board\n");
    } else if (!stone_create(&ui->wStone, ui->stoneRadius, 1., 1., 1.)
            || !stone_create(&ui->bStone, ui->stoneRadius, 0.1, 0.1, 0.1)) {
        fprintf(stderr, "Error: interface: can't create stones\n");
    } else if (    !(ui->camNode = malloc(sizeof(struct Node)))
                || !(ui->camOrientation = malloc(sizeof(struct Node)))) {
        fprintf(stderr, "Error: interface: can't create cam node\n");
    } else {
        ui->viewer->callbackData = ui;
        ui->viewer->resize_callback = resize_callback;
        ui->viewer->key_callback = key_callback;
        ui->viewer->cursor_callback = cursor_callback;
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

        while (ui->running) {
            viewer_process_events(ui->viewer);
            scene_update_nodes(&ui->scene, update_node, NULL);
            uniform_buffer_send(&ui->scene.lights);
            uniform_buffer_send(&ui->scene.camera);
            render_board(ui);
            viewer_next_frame(ui->viewer);
        }
    }

    asset_free(&ui->board.geom);
    asset_free(&ui->wStone);
    asset_free(&ui->bStone);
    if (ui->viewer) viewer_free(ui->viewer);
    if (sceneInit) scene_free(&ui->scene, NULL);
    pthread_exit(NULL);
}

int interface_init(struct Interface* ui, struct Weiqi* weiqi) {
    ui->viewer = NULL;
    ui->weiqi = weiqi;
    ui->running = 1;

    if (pthread_create(&ui->thread, NULL, run_interface, ui) != 0) {
        fprintf(stderr, "Error: interface: couldn't start thread\n");
        return 0;
    }

    return 1;
}

void interface_free(struct Interface* ui) {
    ui->running = 0;
    pthread_join(ui->thread, NULL);
}

int interface_add_stone(struct Interface* ui, enum WeiqiColor color,
                        unsigned int row, unsigned int col);

int interface_del_stone(struct Interface* ui,
                        unsigned int row, unsigned int col);

int interface_get_stone(struct Interface* ui, enum WeiqiColor color,
                        unsigned int* row, unsigned int* col);