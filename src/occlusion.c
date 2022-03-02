#include <stdio.h>

#include <3dmr/render/camera_buffer_object.h>
#include <3dmr/render/shader.h>

#include "occlusion.h"

int occlusion_init(struct Occlusion* occ, unsigned int w, unsigned int h) {
    float borderColor[] = {1., 1., 1., 1.};

    load_id4(occ->view);
    load_id4(occ->projection);
    if (!camera_buffer_object_gen(&occ->camUBO)) {
        fprintf(stderr, "Error: occlusion_init: "
                        "camera_buffer_object_gen failed\n");
        return 0;
    }
    occ->width = w;
    occ->height = h;
    glGenFramebuffers(1, &occ->fbo);
    glGenTextures(1, &occ->tex);
    if (!occ->fbo || !occ->tex) {
        occ->fbo = 0;
        occ->tex = 0;
        fprintf(stderr, "Error: occlusion_init: "
                        "genBuffers or genTextures failed\n");
        return 0;
    }
    /* setup texture parameters */
    /* bind occlusion map to its global slot, same for all programs */
    glActiveTexture(GL_TEXTURE0 + TEX_SLOT_OCCLUSIONMAP);
    glBindTexture(GL_TEXTURE_2D, occ->tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    /* bind texture to frame buffer object */
    glBindFramebuffer(GL_FRAMEBUFFER, occ->fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER,
                           GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D,
                           occ->tex,
                           0);
    glDrawBuffer(GL_FRONT);
    glReadBuffer(GL_FRONT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return 1;
}

void occlusion_bind(struct Occlusion* occ) {
    glActiveTexture(GL_TEXTURE0 + TEX_SLOT_OCCLUSIONMAP);
    glBindTexture(GL_TEXTURE_2D, occ->tex);
}

void occlusion_render_start(struct Occlusion* occ,
                            GLint viewport[4],
                            GLfloat clearColor[4]) {
    glGetIntegerv(GL_VIEWPORT, viewport);
    glGetFloatv(GL_COLOR_CLEAR_VALUE, clearColor);
    glViewport(0, 0, occ->width, occ->height);
    glClearColor(1, 1, 1, 1);
    glBindFramebuffer(GL_FRAMEBUFFER, occ->fbo);
    glClear(GL_COLOR_BUFFER_BIT);

    /* bind occlusion camera to "global" camera uniform buffer object */
    uniform_buffer_bind(&occ->camUBO, CAMERA_UBO_BINDING);
    camera_buffer_object_update_view_and_position(&occ->camUBO,
                                                  MAT_CONST_CAST(occ->view));
    camera_buffer_object_update_projection(&occ->camUBO,
                                           MAT_CONST_CAST(occ->projection));
    uniform_buffer_send(&occ->camUBO);
}

void occlusion_render_end(struct Occlusion* occ,
                          GLint viewport[4],
                          GLfloat clearColor[4]) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
}

void occlusion_free(struct Occlusion* occ) {
    uniform_buffer_del(&occ->camUBO);
    if (occ->tex) {
        glDeleteTextures(1, &occ->tex);
    }
}
