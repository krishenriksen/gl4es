#include "line.h"
#include "matrix.h"
#include "matvec.h"

//#define DEBUG
#ifdef DEBUG
#define DBG(a) a
#else
#define DBG(a)
#endif

void gl4es_glLineStipple(GLuint factor, GLushort pattern) {
    DBG(printf("glLineStipple(%d, 0x%04X)\n", factor, pattern);)
    if(glstate->list.active) {
        if (glstate->list.compiling) {
            NewStage(glstate->list.active, STAGE_LINESTIPPLE);
            glstate->list.active->linestipple_op = 1;
            glstate->list.active->linestipple_factor = factor;
            glstate->list.active->linestipple_pattern = pattern;
            return;
        } else if(glstate->list.pending) flush();
    }
    if(pattern!=glstate->linestipple.pattern || factor!=glstate->linestipple.factor || !glstate->linestipple.texture) {
        glstate->linestipple.factor = factor;
        glstate->linestipple.pattern = pattern;
        for (int i = 0; i < 16; i++) {
            glstate->linestipple.data[i] = ((pattern >> i) & 1) ? 255 : 0;
        }

        gl4es_glPushAttrib(GL_TEXTURE_BIT);
        if (! glstate->linestipple.texture)
            gl4es_glGenTextures(1, &glstate->linestipple.texture);

        gl4es_glBindTexture(GL_TEXTURE_2D, glstate->linestipple.texture);
        gl4es_glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        gl4es_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        gl4es_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        gl4es_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        gl4es_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        gl4es_glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA,
            16, 1, 0, GL_ALPHA, GL_UNSIGNED_BYTE, glstate->linestipple.data);
        gl4es_glPopAttrib();
        noerrorShim();
    }
}
void glLineStipple(GLuint factor, GLushort pattern) AliasExport("gl4es_glLineStipple");

void bind_stipple_tex() {
    gl4es_glBindTexture(GL_TEXTURE_2D, glstate->linestipple.texture);
}

GLfloat *gen_stipple_tex_coords(GLfloat *vert, int stride, int length, GLfloat* noalloctex) {
    DBG(printf("Generate stripple tex (stride=%d, noalloctex=%p):", stride, noalloctex);)
    // generate our texture coords
    GLfloat *tex = noalloctex?noalloctex:(GLfloat *)malloc(length * 4 * sizeof(GLfloat));
    GLfloat *texPos = tex;
    GLfloat *vertPos = vert;

    GLfloat x1, x2, y1, y2;
    GLfloat oldlen, len;
    oldlen = len = 0.f;
    GLfloat* mvp = getMVPMat();
    GLfloat v1[4], v2[4];
    GLfloat w = (GLfloat)glstate->raster.viewport.width;
    GLfloat h = (GLfloat)glstate->raster.viewport.height;
    if(stride==0) stride = 4; else stride/=sizeof(GLfloat);
    int texstride = noalloctex?stride:4;
    // projected coordinates here, and transform to screen pixel using viewport
    for (int i = 0; i < length / 2; i++) {
        vector_matrix(vertPos, mvp, v1);
        vertPos+=stride;
        vector_matrix(vertPos, mvp, v2);
        vertPos+=stride;
        x1=v1[0]*w; y1=v1[1]*h;
        x2=v2[0]*w; y2=v2[1]*h;
        oldlen = len;
        len = sqrtf((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1)) / (glstate->linestipple.factor * 16.f);
        DBG(printf("%f->%f\t", oldlen, len);)
        memset(texPos, 0, 4*sizeof(GLfloat));
        texPos[0] = oldlen; texPos[3] = 1.0f;
        texPos+=texstride;
        memset(texPos, 0, 4*sizeof(GLfloat));
        texPos[0] = len; texPos[3] = 1.0f;
        texPos+=texstride;
    }
    DBG(printf("\n");)
    return tex;
}
