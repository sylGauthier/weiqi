#include <string.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <3dmr/render/camera_buffer_object.h>
#include <3dmr/render/lights_buffer_object.h>

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
                Vec3 t = {0, 0, 0};
                Vec3 axisX = {1, 0, 0};
                quaternion_set_axis_angle(q, axisX, -M_PI / 2.);
                node_set_orientation(interface->camOrientation, q);

                t[2] = interface->defCamDist;
                node_set_pos(interface->camNode, t);
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

static int update_cursor_pos(struct Interface* ui, double x, double y) {
    Vec4 v = {0, 0, -1, 1}, res, plane = {0, 1, 0, 0};
    Vec3 vec, pos, boardPos;
    Mat4 invView;
    float h = ui->theme->boardThickness / 2;
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

    plane[3] = -h;
    if (!intersectlp(boardPos, pos, vec, plane)) return 0;

    boardPos[0] = boardPos[0] / ui->theme->gridScale + 0.5;
    boardPos[2] = -boardPos[2] / ui->theme->gridScale + 0.5;
    if (       boardPos[0] >= 0. && boardPos[0] <= 1.
            && boardPos[2] >= 0. && boardPos[2] <= 1.) {
        ui->cursorPos[0] = (boardPos[0] + 1. / (2. * (float) s)) * s;
        ui->cursorPos[1] = (boardPos[2] + 1. / (2. * (float) s)) * s;
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
    Vec3 axisY = {0, 1, 0};
    Vec3 axisX = {1, 0, 0};

    update_cursor_pos(ui, 2 * xpos / viewer->width - 1,
                      1. - 2 * ypos / viewer->height);
    if (br) {
        node_rotate(ui->camOrientation, axisY, -dx / viewer->width);
        node_slew(ui->camOrientation, axisX, -dy / viewer->width);
    }
    return;
}

void wheel_callback(struct Viewer* viewer, double dx, double dy, void* data) {
    struct Interface* ui = data;
    Vec3 t = {0, 0, 0};

    if (dy < 0) {
        t[2] = ui->camNode->position[2] * (1 - dy / 10.);
    } else {
        t[2] = ui->camNode->position[2] / (1 + dy / 10.);
    }
    node_set_pos(ui->camNode, t);
}

void close_callback(struct Viewer* viewer, void* data) {
    struct Interface* ui = data;

    ui->status = W_UI_QUIT;
    return;
}
