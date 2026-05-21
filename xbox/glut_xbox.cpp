/* glut_xbox.cpp -- GLUT stub layer for the GLSnake Xbox port.
 *
 * Implements every GLUT entry point that glsnake.c calls.
 * glutMainLoop() is the Xbox game loop: it polls XInput, drives the
 * registered callbacks, and calls FakeSwapBuffers() to present.
 *
 * d3dSetMode()/wglCreateContext()/wglMakeCurrent() are called from
 * main_xbox.cpp before glutMainLoop() is entered. */

#include "rxgl_api.h"   /* must be first: xtl.h, GL types, FakeSwapBuffers */
#include "help_screen.h"

#include <string.h>
#include <stdlib.h>

/* ---- Callback table ---- */
static void (*cb_display)(void)                          = NULL;
static void (*cb_reshape)(int, int)                      = NULL;
static void (*cb_idle)(void)                             = NULL;
static void (*cb_keyboard)(unsigned char, int, int)      = NULL;
static void (*cb_special)(int, int, int)                 = NULL;
static void (*cb_mouse)(int, int, int, int)              = NULL;
static void (*cb_motion)(int, int)                       = NULL;

/* Pending timer callbacks */
#define MAX_TIMERS 8
static struct { unsigned int fire; void (*fn)(int); int val; int active; } s_timers[MAX_TIMERS];

static int s_window        = 1;
static int s_running       = 1;
static int s_width         = 640;
static int s_height        = 480;
static int s_display_width = 0;   /* non-zero overrides s_width for reshape */
static int s_display_height= 0;

/* GLUT_KEY_* values that match the constants in GL/glut.h */
#define GKEY_LEFT   100
#define GKEY_RIGHT  102
#define GKEY_UP     101
#define GKEY_DOWN   103
#define GKEY_HOME   106

/* ====================================================================
 * Core API
 * ==================================================================== */

/* Called from main_xbox.cpp after d3dSetMode to override the aspect ratio
 * seen by glsnake_reshape.  Pass logical (display) dimensions, not
 * necessarily the backbuffer size — e.g. 853×480 for widescreen 480i. */
extern "C" void xbox_glut_set_display_size(int w, int h)
{
    s_display_width  = w;
    s_display_height = h;
}

extern "C" void glutInit(int * /*argc*/, char ** /*argv*/) {}

extern "C" void glutInitDisplayMode(unsigned int /*mode*/) {}

extern "C" void glutInitWindowSize(int w, int h)
{
    s_width  = w;
    s_height = h;
}

extern "C" int glutCreateWindow(const char * /*title*/)
{
    return s_window;
}

extern "C" void glutDestroyWindow(int /*win*/)
{
    s_running = 0;
}

extern "C" void glutSwapBuffers(void)
{
    if (xbox_help_visible()) {
        int rw = s_display_width  ? s_display_width  : s_width;
        int rh = s_display_height ? s_display_height : s_height;
        xbox_help_render(rw, rh);
    }
    FakeSwapBuffers();
}

extern "C" void glutPostRedisplay(void) {}
extern "C" void glutFullScreen(void)    {}
extern "C" void glutReshapeWindow(int w, int h) { s_width = w; s_height = h; }
extern "C" void glutPositionWindow(int /*x*/, int /*y*/) {}

/* ====================================================================
 * Callback registration
 * ==================================================================== */

extern "C" void glutDisplayFunc (void (*fn)(void))                    { cb_display  = fn; }
extern "C" void glutReshapeFunc (void (*fn)(int,int))                 { cb_reshape  = fn; }
extern "C" void glutIdleFunc    (void (*fn)(void))                    { cb_idle     = fn; }
extern "C" void glutKeyboardFunc(void (*fn)(unsigned char,int,int))   { cb_keyboard = fn; }
extern "C" void glutSpecialFunc (void (*fn)(int,int,int))             { cb_special  = fn; }
extern "C" void glutMouseFunc   (void (*fn)(int,int,int,int))         { cb_mouse    = fn; }
extern "C" void glutMotionFunc  (void (*fn)(int,int))                 { cb_motion   = fn; }

extern "C" void glutTimerFunc(unsigned int millis, void (*fn)(int), int val)
{
    unsigned int fire = (unsigned int)GetTickCount() + millis;
    for (int i = 0; i < MAX_TIMERS; i++) {
        if (!s_timers[i].active) {
            s_timers[i].fire   = fire;
            s_timers[i].fn     = fn;
            s_timers[i].val    = val;
            s_timers[i].active = 1;
            break;
        }
    }
}

/* ====================================================================
 * Bitmap text -- no-op stubs (score display only; not needed on Xbox)
 * ==================================================================== */

extern "C" void glutBitmapCharacter(void * /*font*/, int /*character*/) {}

extern "C" int  glutBitmapLength(void * /*font*/, const unsigned char * /*str*/)
{
    return 0;
}

/* ====================================================================
 * XInput polling
 *
 * Controller mapping for glsnake:
 *   D-pad / left stick → GLUT_KEY_UP/DOWN/LEFT/RIGHT (model rotation)
 *   A button           → '.' (next model)
 *   B button           → ',' (previous model)
 *   X button           → 'i' (toggle interactive)
 *   Y button           → 'p' (pause)
 *   Start              → 'f' (fullscreen — no-op, but harmless)
 *   Back               → 'q' (quit)
 * ==================================================================== */

static HANDLE s_hDevice = INVALID_HANDLE_VALUE;

/* XGetDeviceChanges enumerates connected/disconnected pads.
 * The first call after XInitDevices reports all plugged controllers
 * as new insertions, which is how we discover the initial device. */
static void xbox_enumerate_devices(void)
{
    DWORD insertions = 0, removals = 0;
    XGetDeviceChanges(XDEVICE_TYPE_GAMEPAD, &insertions, &removals);

    if (removals && s_hDevice != INVALID_HANDLE_VALUE) {
        XInputClose(s_hDevice);
        s_hDevice = INVALID_HANDLE_VALUE;
    }

    if (s_hDevice == INVALID_HANDLE_VALUE) {
        for (DWORD port = 0; port < 4; port++) {
            if (insertions & (1 << port)) {
                XINPUT_POLLING_PARAMETERS pp;
                memset(&pp, 0, sizeof(pp));
                pp.fAutoPoll      = TRUE;
                pp.bInputInterval = 8;
                s_hDevice = XInputOpen(XDEVICE_TYPE_GAMEPAD, port,
                                       XDEVICE_NO_SLOT, &pp);
                if (s_hDevice != INVALID_HANDLE_VALUE) break;
            }
        }
    }
}

static void xbox_open_input(void)
{
    xbox_enumerate_devices();
}

static void xbox_fire_key(unsigned char c)
{
    if (cb_keyboard) cb_keyboard(c, 0, 0);
}

static void xbox_fire_special(int key)
{
    if (cb_special) cb_special(key, 0, 0);
}

static void xbox_poll_input(void)
{
    if (s_hDevice == INVALID_HANDLE_VALUE) return;

    XINPUT_STATE xs;
    memset(&xs, 0, sizeof(xs));
    if (XInputGetState(s_hDevice, &xs) != ERROR_SUCCESS) return;

    /* ---- Digital d-pad ---- */
    static WORD s_prev_digital = 0;
    WORD cur = xs.Gamepad.wButtons;
    WORD dn  = (WORD)( cur & ~s_prev_digital);
    s_prev_digital = cur;

    if (dn & XINPUT_GAMEPAD_DPAD_LEFT)  xbox_fire_special(GKEY_LEFT);
    if (dn & XINPUT_GAMEPAD_DPAD_RIGHT) xbox_fire_special(GKEY_RIGHT);
    if (dn & XINPUT_GAMEPAD_DPAD_UP)    xbox_fire_special(GKEY_UP);
    if (dn & XINPUT_GAMEPAD_DPAD_DOWN)  xbox_fire_special(GKEY_DOWN);

    /* ---- Analog face buttons (A/B/X/Y) ---- */
    static BYTE s_prev_ab[4] = { 0, 0, 0, 0 };
    const unsigned char ab_chars[4] = { '.', ',', 'i', 'p' };  /* A, B, X, Y */
    for (int j = 0; j < 4; j++) {
        int now  = xs.Gamepad.bAnalogButtons[j] > 100;
        int prev = s_prev_ab[j] > 100;
        if (now && !prev) xbox_fire_key(ab_chars[j]);
        s_prev_ab[j] = xs.Gamepad.bAnalogButtons[j];
    }

    /* ---- LT+RT+Back+Black = return to dashboard (standard Xbox combo) ---- */
    {
        int lt    = xs.Gamepad.bAnalogButtons[6] > 100;
        int rt    = xs.Gamepad.bAnalogButtons[7] > 100;
        int black = xs.Gamepad.bAnalogButtons[4] > 100;
        int back  = (cur & XINPUT_GAMEPAD_BACK) != 0;
        if (lt && rt && back && black)
            XLaunchNewImage(NULL, NULL);
    }

    /* ---- Start button = toggle controls overlay (also pauses/resumes game) ---- */
    static int s_prev_start = 0;
    int start_now = (cur & XINPUT_GAMEPAD_START) != 0;
    if (start_now && !s_prev_start) {
        xbox_help_toggle();
        xbox_fire_key('p');   /* 'p' toggles glsnake pause in sync with overlay */
    }
    s_prev_start = start_now;

    /* ---- Left stick → special keys ---- */
    SHORT lx = xs.Gamepad.sThumbLX;
    SHORT ly = xs.Gamepad.sThumbLY;
    static int s_lx = 0, s_ly = 0;
    int nx = (lx < -16384) ? -1 : (lx > 16384) ? 1 : 0;
    int ny = (ly < -16384) ? -1 : (ly > 16384) ? 1 : 0;
    if (nx != s_lx) {
        if (nx) xbox_fire_special((nx < 0) ? GKEY_LEFT : GKEY_RIGHT);
        s_lx = nx;
    }
    if (ny != s_ly) {
        if (ny) xbox_fire_special((ny > 0) ? GKEY_UP : GKEY_DOWN);
        s_ly = ny;
    }
}

/* ====================================================================
 * Main loop
 * ==================================================================== */

extern "C" void glutMainLoop(void)
{
    xbox_help_init();
    xbox_open_input();

    /* Clear to black before the first frame so any stray glutSwapBuffers()
     * call in glsnake's main() presents a clean buffer, not the dashboard. */
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    FakeSwapBuffers();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    FakeSwapBuffers();

    /* Fire the initial reshape with actual display dimensions. */
    {
        int rw = s_display_width  ? s_display_width  : s_width;
        int rh = s_display_height ? s_display_height : s_height;
        if (cb_reshape) cb_reshape(rw, rh);
    }

    while (s_running) {
        xbox_enumerate_devices();
        xbox_poll_input();

        /* Fire expired timers */
        unsigned int now = (unsigned int)GetTickCount();
        for (int i = 0; i < MAX_TIMERS; i++) {
            if (s_timers[i].active && (int)(now - s_timers[i].fire) >= 0) {
                void (*fn)(int) = s_timers[i].fn;
                int val         = s_timers[i].val;
                s_timers[i].active = 0;
                if (fn) fn(val);
            }
        }

        if (cb_idle)    cb_idle();
        if (cb_display) cb_display();
        /* glsnake_display() calls glutSwapBuffers() → FakeSwapBuffers() itself;
         * no second present here. */
    }

    if (s_hDevice != INVALID_HANDLE_VALUE) {
        XInputClose(s_hDevice);
        s_hDevice = INVALID_HANDLE_VALUE;
    }
    xbox_help_shutdown();
}
