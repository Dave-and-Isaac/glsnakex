#pragma once
/* glut.h -- minimal GLUT stub for the GLSnake Xbox port.
 * Also provides gluOrtho2D and gluLookAt, which glsnake.c uses inside
 * #ifdef HAVE_GLUT sections where only this header is included. */

#include "rxgl_api.h"   /* GL types, constants, gluPerspective */
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Display mode bits ---- */
#define GLUT_RGBA       0x0000
#define GLUT_RGB        0x0000
#define GLUT_DOUBLE     0x0002
#define GLUT_DEPTH      0x0010
#define GLUT_ALPHA      0x0008

/* ---- Special key codes ---- */
#define GLUT_KEY_UP         101
#define GLUT_KEY_DOWN       103
#define GLUT_KEY_LEFT       100
#define GLUT_KEY_RIGHT      102
#define GLUT_KEY_HOME       106
#define GLUT_KEY_END        107
#define GLUT_KEY_PAGE_UP    104
#define GLUT_KEY_PAGE_DOWN  105

/* ---- Mouse ---- */
#define GLUT_LEFT_BUTTON    0
#define GLUT_MIDDLE_BUTTON  1
#define GLUT_RIGHT_BUTTON   2
#define GLUT_DOWN           0
#define GLUT_UP             1

/* ---- Bitmap font handles (opaque; glutBitmapCharacter is a no-op stub) ---- */
#define GLUT_BITMAP_HELVETICA_12   ((void *)0x1)
#define GLUT_BITMAP_HELVETICA_10   ((void *)0x2)
#define GLUT_BITMAP_HELVETICA_18   ((void *)0x3)
#define GLUT_BITMAP_TIMES_ROMAN_10 ((void *)0x4)
#define GLUT_BITMAP_TIMES_ROMAN_24 ((void *)0x5)
#define GLUT_BITMAP_8_BY_13        ((void *)0x6)
#define GLUT_BITMAP_9_BY_15        ((void *)0x7)

/* ---- Core GLUT ---- */
void  glutInit(int *argc, char **argv);
void  glutInitDisplayMode(unsigned int mode);
void  glutInitWindowSize(int width, int height);
int   glutCreateWindow(const char *title);
void  glutDestroyWindow(int win);
void  glutMainLoop(void);
void  glutSwapBuffers(void);
void  glutPostRedisplay(void);
void  glutFullScreen(void);
void  glutReshapeWindow(int w, int h);
void  glutPositionWindow(int x, int y);

/* ---- Timer / idle ---- */
void  glutIdleFunc(void (*func)(void));
void  glutTimerFunc(unsigned int millis, void (*func)(int value), int value);

/* ---- Callbacks ---- */
void  glutDisplayFunc(void (*func)(void));
void  glutReshapeFunc(void (*func)(int w, int h));
void  glutKeyboardFunc(void (*func)(unsigned char key, int x, int y));
void  glutSpecialFunc(void (*func)(int key, int x, int y));
void  glutMouseFunc(void (*func)(int button, int state, int x, int y));
void  glutMotionFunc(void (*func)(int x, int y));

/* ---- Bitmap text (no-op) ---- */
void  glutBitmapCharacter(void *font, int character);
int   glutBitmapLength(void *font, const unsigned char *string);

#ifdef __cplusplus
}
#endif

/* ====================================================================
 * GLU inline stubs needed by glsnake.c inside #ifdef HAVE_GLUT.
 * gluPerspective is already a static inline in rxgl_api.h.
 * ==================================================================== */

static inline void gluOrtho2D(GLdouble left, GLdouble right,
                               GLdouble bottom, GLdouble top)
{
    glOrtho(left, right, bottom, top, -1.0, 1.0);
}

static inline void gluLookAt(GLdouble eyex,    GLdouble eyey,    GLdouble eyez,
                              GLdouble centerx, GLdouble centery, GLdouble centerz,
                              GLdouble upx,     GLdouble upy,     GLdouble upz)
{
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
        (GLfloat)sx, (GLfloat)ux, (GLfloat)-fx, 0.0f,
        (GLfloat)sy, (GLfloat)uy, (GLfloat)-fy, 0.0f,
        (GLfloat)sz, (GLfloat)uz, (GLfloat)-fz, 0.0f,
        0.0f,        0.0f,         0.0f,         1.0f
    };
    glMultMatrixf(m);
    glTranslatef((GLfloat)-eyex, (GLfloat)-eyey, (GLfloat)-eyez);
}
