#ifndef OCCLUSION_H
#define OCCLUSION_H

#include <GL/glew.h>

#include <3dmr/math/linear_algebra.h>

struct Occlusion {
    GLuint fbo;
    GLuint tex;
    unsigned int width, height;
    Mat4 projection;
    Mat4 view;
    struct UniformBuffer camUBO;
};

int occlusion_init(struct Occlusion* occ, unsigned int w, unsigned int h);
void occlusion_bind(struct Occlusion* occ);
void occlusion_render_start(struct Occlusion* occ,
                            GLint viewport[4],
                            GLfloat clearColor[4]);
void occlusion_render_end(struct Occlusion* occ,
                          GLint viewport[4],
                          GLfloat clearColor[4]);
void occlusion_free(struct Occlusion* occ);

#endif
