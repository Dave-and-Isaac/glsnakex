#ifndef XBOX_GL_GLU_H
#define XBOX_GL_GLU_H
/* Provides the GLU entry points glsnake.c needs: gluOrtho2D, gluLookAt.
 * gluPerspective is already a static inline in rxgl_api.h.
 * All implementations are inline to avoid a separate link unit. */

#include "rxgl_api.h"   /* GL types, gluPerspective, math.h via rxgl_internal */
#include <math.h>

static inline void gluOrtho2D(GLdouble left, GLdouble right,
                               GLdouble bottom, GLdouble top)
{
    glOrtho(left, right, bottom, top, -1.0, 1.0);
}

static inline void gluLookAt(GLdouble eyex,    GLdouble eyey,    GLdouble eyez,
                              GLdouble centerx, GLdouble centery, GLdouble centerz,
                              GLdouble upx,     GLdouble upy,     GLdouble upz)
{
    /* Build an orthonormal view basis (f, s, u) then load as GL matrix. */
    double fx = centerx - eyex, fy = centery - eyey, fz = centerz - eyez;
    double len = sqrt(fx*fx + fy*fy + fz*fz);
    if (len > 0.0) { fx /= len; fy /= len; fz /= len; }

    double sx = fy*upz - fz*upy;
    double sy = fz*upx - fx*upz;
    double sz = fx*upy - fy*upx;
    len = sqrt(sx*sx + sy*sy + sz*sz);
    if (len > 0.0) { sx /= len; sy /= len; sz /= len; }

    double ux = sy*fz - sz*fy;
    double uy = sz*fx - sx*fz;
    double uz = sx*fy - sy*fx;

    GLfloat m[16] = {
        (GLfloat)sx,  (GLfloat)ux,  (GLfloat)-fx, 0.0f,
        (GLfloat)sy,  (GLfloat)uy,  (GLfloat)-fy, 0.0f,
        (GLfloat)sz,  (GLfloat)uz,  (GLfloat)-fz, 0.0f,
        0.0f,         0.0f,          0.0f,         1.0f
    };
    glMultMatrixf(m);
    glTranslatef((GLfloat)-eyex, (GLfloat)-eyey, (GLfloat)-eyez);
}

#endif /* XBOX_GL_GLU_H */
