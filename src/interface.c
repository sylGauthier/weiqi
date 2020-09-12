#include <stdio.h>

#include "interface.h"

int interface_init(struct Interface* ui, struct Weiqi* weiqi) {
    int sceneInit = 0;

    camera_projection(1., 80. / 360. * 2 * M_PI, 0.001, 1000., ui->camera.projection);
    if (!(ui->viewer = viewer_new(1024, 768, "weiqi"))) {
        fprintf(stderr, "Error: interface: can't create viewer\n");
    } else if (!(sceneInit = scene_init(&ui->scene, &ui->camera))) {
        fprintf(stderr, "Error: interface: can't init scene\n");
    } else {
        return 1;
    }
    if (ui->viewer) viewer_free(ui->viewer);
    if (sceneInit) scene_free(&ui->scene, NULL);
    return 0;
}

void interface_free(struct Interface* ui) {
    viewer_free(ui->viewer);
    scene_free(&ui->scene, NULL);
}

int interface_add_stone(struct Interface* ui, enum WeiqiColor color,
                        unsigned int row, unsigned int col);

int interface_del_stone(struct Interface* ui,
                        unsigned int row, unsigned int col);

int interface_get_stone(struct Interface* ui, enum WeiqiColor color,
                        unsigned int* row, unsigned int* col);
