#ifndef INTERFACE_H
#define INTERFACE_H

#include <3dmr/scene/scene.h>

#include "weiqi.h"

struct Interface {
    struct Scene scene;
    struct Camera camera;
    struct Viewer* viewer;
    struct Weiqi* weiqi;
};

int interface_init(struct Interface* ui, struct Weiqi* weiqi);
void interface_free(struct Interface* ui);

int interface_add_stone(struct Interface* ui, enum WeiqiColor color,
                        unsigned int row, unsigned int col);

int interface_del_stone(struct Interface* ui,
                        unsigned int row, unsigned int col);

int interface_get_stone(struct Interface* ui, enum WeiqiColor color,
                        unsigned int* row, unsigned int* col);

#endif
