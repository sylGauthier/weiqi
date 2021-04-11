#ifndef CALLBACKS_H
#define CALLBACKS_H

#include <3dmr/scene/scene.h>

void update_node(struct Scene* scene, struct Node* n, void* data);
void resize_callback(struct Viewer* viewer, void* data);
void key_callback(struct Viewer* viewer, int key, int scancode, int action,
                  int mods, void* data);
void mouse_callback(struct Viewer* viewer, int button, int action, int mods,
                    void* data);
void cursor_callback(struct Viewer* viewer, double xpos, double ypos,
                     double dx, double dy, int bl, int bm, int br,
                     void* data);
void close_callback(struct Viewer* viewer, void* data);

#endif
