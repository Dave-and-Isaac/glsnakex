/* help_screen.cpp -- GLSnake Xbox controls overlay.
 *
 * Renders a semi-transparent panel listing controller mappings.
 * Called from glutSwapBuffers() so it composites onto the game frame
 * before FakeSwapBuffers() presents.
 *
 * Font: IBM PC 8x8 VGA BIOS glyphs, chars 0x20-0x7F, public domain.
 * Packed into a 128x64 GL_LUMINANCE_ALPHA atlas (16 cols x 6 rows). */

#include "rxgl_api.h"
#include "help_screen.h"
#include <string.h>
#include <math.h>

static int    s_visible  = 0;
static GLuint s_font_tex = 0;

/* ---- IBM PC 8x8 VGA font, chars 0x20–0x7F (96 entries) ---- */
static const unsigned char s_font8x8[96][8] = {
    /* 0x20 ' '  */ {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    /* 0x21 '!'  */ {0x18,0x18,0x18,0x18,0x18,0x00,0x18,0x00},
    /* 0x22 '"'  */ {0x66,0x66,0x00,0x00,0x00,0x00,0x00,0x00},
    /* 0x23 '#'  */ {0x6C,0xFE,0x6C,0x6C,0x6C,0xFE,0x6C,0x00},
    /* 0x24 '$'  */ {0x10,0x38,0x60,0x38,0x0C,0x7C,0x10,0x00},
    /* 0x25 '%'  */ {0xC3,0xC6,0x0C,0x18,0x33,0x63,0xC3,0x00},
    /* 0x26 '&'  */ {0x3C,0x6C,0x38,0x76,0xDC,0xCC,0x76,0x00},
    /* 0x27 '\'' */ {0x18,0x30,0x00,0x00,0x00,0x00,0x00,0x00},
    /* 0x28 '('  */ {0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00},
    /* 0x29 ')'  */ {0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00},
    /* 0x2A '*'  */ {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00},
    /* 0x2B '+'  */ {0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00},
    /* 0x2C ','  */ {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30},
    /* 0x2D '-'  */ {0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00},
    /* 0x2E '.'  */ {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00},
    /* 0x2F '/'  */ {0x03,0x06,0x0C,0x18,0x30,0x60,0xC0,0x00},
    /* 0x30 '0'  */ {0x3C,0x66,0x6E,0x7E,0x76,0x66,0x3C,0x00},
    /* 0x31 '1'  */ {0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00},
    /* 0x32 '2'  */ {0x3C,0x66,0x06,0x1C,0x30,0x66,0x7E,0x00},
    /* 0x33 '3'  */ {0x3C,0x66,0x06,0x1C,0x06,0x66,0x3C,0x00},
    /* 0x34 '4'  */ {0x1E,0x36,0x66,0x7E,0x06,0x06,0x06,0x00},
    /* 0x35 '5'  */ {0x7E,0x60,0x7C,0x06,0x06,0x66,0x3C,0x00},
    /* 0x36 '6'  */ {0x1C,0x30,0x60,0x7C,0x66,0x66,0x3C,0x00},
    /* 0x37 '7'  */ {0x7E,0x06,0x0C,0x18,0x30,0x30,0x30,0x00},
    /* 0x38 '8'  */ {0x3C,0x66,0x66,0x3C,0x66,0x66,0x3C,0x00},
    /* 0x39 '9'  */ {0x3C,0x66,0x66,0x3E,0x06,0x0C,0x38,0x00},
    /* 0x3A ':'  */ {0x00,0x18,0x18,0x00,0x18,0x18,0x00,0x00},
    /* 0x3B ';'  */ {0x00,0x18,0x18,0x00,0x18,0x18,0x30,0x00},
    /* 0x3C '<'  */ {0x06,0x0C,0x18,0x30,0x18,0x0C,0x06,0x00},
    /* 0x3D '='  */ {0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00},
    /* 0x3E '>'  */ {0xC0,0x60,0x30,0x18,0x30,0x60,0xC0,0x00},
    /* 0x3F '?'  */ {0x3C,0x66,0x06,0x0C,0x18,0x00,0x18,0x00},
    /* 0x40 '@'  */ {0x3E,0x63,0xCF,0xDB,0xCF,0x60,0x3E,0x00},
    /* 0x41 'A'  */ {0x18,0x3C,0x66,0x7E,0x66,0x66,0x66,0x00},
    /* 0x42 'B'  */ {0x7C,0x66,0x66,0x7C,0x66,0x66,0x7C,0x00},
    /* 0x43 'C'  */ {0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0x00},
    /* 0x44 'D'  */ {0x78,0x6C,0x66,0x66,0x66,0x6C,0x78,0x00},
    /* 0x45 'E'  */ {0x7E,0x60,0x60,0x78,0x60,0x60,0x7E,0x00},
    /* 0x46 'F'  */ {0x7E,0x60,0x60,0x78,0x60,0x60,0x60,0x00},
    /* 0x47 'G'  */ {0x3C,0x66,0x60,0x6E,0x66,0x66,0x3E,0x00},
    /* 0x48 'H'  */ {0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x00},
    /* 0x49 'I'  */ {0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00},
    /* 0x4A 'J'  */ {0x1E,0x0C,0x0C,0x0C,0x6C,0x6C,0x38,0x00},
    /* 0x4B 'K'  */ {0x66,0x6C,0x78,0x70,0x78,0x6C,0x66,0x00},
    /* 0x4C 'L'  */ {0x60,0x60,0x60,0x60,0x60,0x60,0x7E,0x00},
    /* 0x4D 'M'  */ {0xC6,0xEE,0xFE,0xD6,0xC6,0xC6,0xC6,0x00},
    /* 0x4E 'N'  */ {0xC6,0xE6,0xF6,0xDE,0xCE,0xC6,0xC6,0x00},
    /* 0x4F 'O'  */ {0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00},
    /* 0x50 'P'  */ {0x7C,0x66,0x66,0x7C,0x60,0x60,0x60,0x00},
    /* 0x51 'Q'  */ {0x3C,0x66,0x66,0x66,0x76,0x3C,0x0E,0x00},
    /* 0x52 'R'  */ {0x7C,0x66,0x66,0x7C,0x6C,0x66,0x66,0x00},
    /* 0x53 'S'  */ {0x3C,0x66,0x60,0x3C,0x06,0x66,0x3C,0x00},
    /* 0x54 'T'  */ {0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00},
    /* 0x55 'U'  */ {0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x00},
    /* 0x56 'V'  */ {0x66,0x66,0x66,0x66,0x66,0x3C,0x18,0x00},
    /* 0x57 'W'  */ {0xC6,0xC6,0xC6,0xD6,0xFE,0xEE,0xC6,0x00},
    /* 0x58 'X'  */ {0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0x00},
    /* 0x59 'Y'  */ {0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x00},
    /* 0x5A 'Z'  */ {0x7E,0x06,0x0C,0x18,0x30,0x60,0x7E,0x00},
    /* 0x5B '['  */ {0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00},
    /* 0x5C '\\' */ {0xC0,0x60,0x30,0x18,0x0C,0x06,0x03,0x00},
    /* 0x5D ']'  */ {0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00},
    /* 0x5E '^'  */ {0x10,0x38,0x6C,0xC6,0x00,0x00,0x00,0x00},
    /* 0x5F '_'  */ {0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00},
    /* 0x60 '`'  */ {0x30,0x18,0x0C,0x00,0x00,0x00,0x00,0x00},
    /* 0x61 'a'  */ {0x00,0x00,0x3C,0x06,0x3E,0x66,0x3E,0x00},
    /* 0x62 'b'  */ {0x60,0x60,0x7C,0x66,0x66,0x66,0x7C,0x00},
    /* 0x63 'c'  */ {0x00,0x00,0x3C,0x60,0x60,0x60,0x3C,0x00},
    /* 0x64 'd'  */ {0x06,0x06,0x3E,0x66,0x66,0x66,0x3E,0x00},
    /* 0x65 'e'  */ {0x00,0x00,0x3C,0x66,0x7E,0x60,0x3C,0x00},
    /* 0x66 'f'  */ {0x1C,0x30,0x7C,0x30,0x30,0x30,0x30,0x00},
    /* 0x67 'g'  */ {0x00,0x00,0x3E,0x66,0x66,0x3E,0x06,0x3C},
    /* 0x68 'h'  */ {0x60,0x60,0x7C,0x66,0x66,0x66,0x66,0x00},
    /* 0x69 'i'  */ {0x18,0x00,0x38,0x18,0x18,0x18,0x3C,0x00},
    /* 0x6A 'j'  */ {0x06,0x00,0x06,0x06,0x06,0x66,0x3C,0x00},
    /* 0x6B 'k'  */ {0x60,0x60,0x66,0x6C,0x78,0x6C,0x66,0x00},
    /* 0x6C 'l'  */ {0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0x00},
    /* 0x6D 'm'  */ {0x00,0x00,0xCC,0xFE,0xD6,0xC6,0xC6,0x00},
    /* 0x6E 'n'  */ {0x00,0x00,0x7C,0x66,0x66,0x66,0x66,0x00},
    /* 0x6F 'o'  */ {0x00,0x00,0x3C,0x66,0x66,0x66,0x3C,0x00},
    /* 0x70 'p'  */ {0x00,0x00,0x7C,0x66,0x66,0x7C,0x60,0x60},
    /* 0x71 'q'  */ {0x00,0x00,0x3E,0x66,0x66,0x3E,0x06,0x06},
    /* 0x72 'r'  */ {0x00,0x00,0x7C,0x66,0x60,0x60,0x60,0x00},
    /* 0x73 's'  */ {0x00,0x00,0x3C,0x60,0x3C,0x06,0x7C,0x00},
    /* 0x74 't'  */ {0x30,0x30,0x7C,0x30,0x30,0x30,0x1C,0x00},
    /* 0x75 'u'  */ {0x00,0x00,0x66,0x66,0x66,0x66,0x3E,0x00},
    /* 0x76 'v'  */ {0x00,0x00,0x66,0x66,0x66,0x3C,0x18,0x00},
    /* 0x77 'w'  */ {0x00,0x00,0xC6,0xD6,0xD6,0xFE,0x6C,0x00},
    /* 0x78 'x'  */ {0x00,0x00,0xC6,0x6C,0x38,0x6C,0xC6,0x00},
    /* 0x79 'y'  */ {0x00,0x00,0x66,0x66,0x3E,0x06,0x3C,0x00},
    /* 0x7A 'z'  */ {0x00,0x00,0x7E,0x0C,0x18,0x30,0x7E,0x00},
    /* 0x7B '{'  */ {0x0E,0x18,0x18,0x70,0x18,0x18,0x0E,0x00},
    /* 0x7C '|'  */ {0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x00},
    /* 0x7D '}'  */ {0x70,0x18,0x18,0x0E,0x18,0x18,0x70,0x00},
    /* 0x7E '~'  */ {0x76,0xDC,0x00,0x00,0x00,0x00,0x00,0x00},
    /* 0x7F DEL  */ {0x00,0x10,0x38,0x6C,0xC6,0xFE,0x00,0x00},
};

/* ---- Font texture atlas ---- */
/* 128x64 GL_RGBA: 16 chars/row, 6 rows.
 * Texel on = {0xFF,0xFF,0xFF,0xFF}, off = {0x00,0x00,0x00,0x00}. */
#define ATLAS_W 128
#define ATLAS_H  64
static unsigned char s_atlas[ATLAS_H * ATLAS_W * 4];

static void build_font_texture(void)
{
    memset(s_atlas, 0, sizeof(s_atlas));
    for (int idx = 0; idx < 96; idx++) {
        int col = idx % 16;
        int row = idx / 16;
        for (int r = 0; r < 8; r++) {
            unsigned char byte = s_font8x8[idx][r];
            for (int b = 0; b < 8; b++) {
                int px = col * 8 + b;
                int py = row * 8 + r;
                int offset = (py * ATLAS_W + px) * 4;
                unsigned char v = ((byte >> (7 - b)) & 1) ? 0xFF : 0x00;
                s_atlas[offset]     = v;
                s_atlas[offset + 1] = v;
                s_atlas[offset + 2] = v;
                s_atlas[offset + 3] = v;
            }
        }
    }

    glGenTextures(1, &s_font_tex);
    glBindTexture(GL_TEXTURE_2D, s_font_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 ATLAS_W, ATLAS_H, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, s_atlas);
    glBindTexture(GL_TEXTURE_2D, 0);
}

/* ---- Public init/shutdown ---- */

void xbox_help_init(void)
{
    build_font_texture();
}

void xbox_help_shutdown(void)
{
    if (s_font_tex) {
        glDeleteTextures(1, &s_font_tex);
        s_font_tex = 0;
    }
}

void xbox_help_toggle(void)  { s_visible = !s_visible; }
int  xbox_help_visible(void) { return s_visible; }

/* ---- Drawing helpers ---- */

static void draw_quad_solid(float x, float y, float w, float h,
                             float r, float g, float b, float a)
{
    glColor4f(r, g, b, a);
    glBegin(GL_QUADS);
    glVertex2f(x,     y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x,     y + h);
    glEnd();
}

static void draw_circle(float cx, float cy, float radius,
                        float r, float g, float b)
{
    const int segs = 20;
    glColor4f(r, g, b, 1.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= segs; i++) {
        float a = i * 6.28318530f / segs;
        glVertex2f(cx + radius * cosf(a), cy + radius * sinf(a));
    }
    glEnd();
}

/* Draws a single char from the atlas at (x,y) top-left, in pixel-scaled coords.
 * Requires GL_TEXTURE_2D bound to s_font_tex. */
static void draw_char(unsigned char c, float x, float y, float scale,
                      float r, float g, float b)
{
    if (c < 0x20 || c > 0x7F) c = 0x20;
    int idx = c - 0x20;
    int col = idx % 16;
    int row = idx / 16;
    float u0 = col * 8.0f / ATLAS_W;
    float u1 = (col + 1) * 8.0f / ATLAS_W;
    float v0 = row * 8.0f / ATLAS_H;
    float v1 = (row + 1) * 8.0f / ATLAS_H;
    float w  = 8.0f * scale;
    float h  = 8.0f * scale;
    glColor4f(r, g, b, 1.0f);
    glBegin(GL_QUADS);
    glTexCoord2f(u0, v0); glVertex2f(x,     y);
    glTexCoord2f(u1, v0); glVertex2f(x + w, y);
    glTexCoord2f(u1, v1); glVertex2f(x + w, y + h);
    glTexCoord2f(u0, v1); glVertex2f(x,     y + h);
    glEnd();
}

static void draw_string(const char *s, float x, float y, float scale,
                        float r, float g, float b)
{
    float cx = x;
    while (*s) {
        draw_char((unsigned char)*s, cx, y, scale, r, g, b);
        cx += 8.0f * scale;
        s++;
    }
}

static float string_width(const char *s, float scale)
{
    int len = 0;
    while (*s++) len++;
    return len * 8.0f * scale;
}

/* ---- Main render ---- */

void xbox_help_render(int screen_w, int screen_h)
{
    if (!s_visible || !s_font_tex) return;

    /* Save matrices */
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    /* Top-left origin, Y increases downward */
    glOrtho(0.0, (double)screen_w, (double)screen_h, 0.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    /* Save render states we change */
    GLboolean was_depth   = glIsEnabled(GL_DEPTH_TEST);
    GLboolean was_blend   = glIsEnabled(GL_BLEND);
    GLboolean was_tex2d   = glIsEnabled(GL_TEXTURE_2D);
    GLboolean was_light   = glIsEnabled(GL_LIGHTING);
    GLboolean was_cull    = glIsEnabled(GL_CULL_FACE);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_TEXTURE_2D);

    /* Full-screen dim */
    draw_quad_solid(0, 0, (float)screen_w, (float)screen_h,
                    0.0f, 0.0f, 0.0f, 0.72f);

    /* Panel */
    float scale = screen_w / 640.0f;
    float pw = 400.0f * scale;
    float ph = 278.0f * scale;
    float px = (screen_w  - pw) * 0.5f;
    float py = (screen_h  - ph) * 0.5f;

    draw_quad_solid(px, py, pw, ph, 0.08f, 0.08f, 0.18f, 0.94f);

    /* Panel border */
    glColor4f(0.35f, 0.35f, 0.75f, 1.0f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(px,      py);
    glVertex2f(px + pw, py);
    glVertex2f(px + pw, py + ph);
    glVertex2f(px,      py + ph);
    glEnd();

    /* Title */
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, s_font_tex);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    const char *title = "GLSNAKE CONTROLS";
    float ts = 2.0f * scale;
    float tx = px + (pw - string_width(title, ts)) * 0.5f;
    float ty = py + 16.0f * scale;
    draw_string(title, tx, ty, ts, 1.0f, 1.0f, 0.2f);

    /* Divider */
    glDisable(GL_TEXTURE_2D);
    float div_y = py + 42.0f * scale;
    glColor4f(0.35f, 0.35f, 0.75f, 0.9f);
    glBegin(GL_LINES);
    glVertex2f(px + 12.0f * scale, div_y);
    glVertex2f(px + pw - 12.0f * scale, div_y);
    glEnd();

    /* Control rows */
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, s_font_tex);

    struct Row {
        float br, bg, bb;     /* button fill colour */
        const char *btn;      /* short label drawn in/near button */
        const char *action;   /* right-side description */
    };
    static const Row rows[] = {
        { 0.55f, 0.55f, 0.55f, "D",  "ROTATE MODEL"  },  /* D-pad/stick — grey  */
        { 0.20f, 0.85f, 0.20f, "A",  "NEXT MODEL"    },  /* A — green           */
        { 0.90f, 0.20f, 0.20f, "B",  "PREV MODEL"    },  /* B — red             */
        { 0.20f, 0.40f, 1.00f, "X",  "INTERACTIVE"   },  /* X — blue            */
        { 1.00f, 0.88f, 0.00f, "Y",  "PAUSE / RESUME"},  /* Y — yellow          */
    };

    float btn_r    = 11.0f * scale;
    float row_cx   = px + 36.0f * scale;           /* circle centre x    */
    float lbl_x    = px + 62.0f * scale;           /* action text start  */
    float row_y0   = div_y + 20.0f * scale + btn_r;/* centre y of row 0  */
    float row_step = 38.0f * scale;
    float rs       = 1.5f * scale;                 /* text scale         */

    for (int i = 0; i < 5; i++) {
        float cy = row_y0 + i * row_step;

        glDisable(GL_TEXTURE_2D);

        if (i == 0) {
            /* D-pad: draw a grey cross (two overlapping quads) */
            float aw = btn_r * 0.52f;   /* arm half-width  */
            float al = btn_r;            /* arm half-length */
            glColor4f(rows[0].br, rows[0].bg, rows[0].bb, 1.0f);
            glBegin(GL_QUADS);
            /* horizontal bar */
            glVertex2f(row_cx - al, cy - aw);
            glVertex2f(row_cx + al, cy - aw);
            glVertex2f(row_cx + al, cy + aw);
            glVertex2f(row_cx - al, cy + aw);
            /* vertical bar */
            glVertex2f(row_cx - aw, cy - al);
            glVertex2f(row_cx + aw, cy - al);
            glVertex2f(row_cx + aw, cy + al);
            glVertex2f(row_cx - aw, cy + al);
            glEnd();
            /* white outline around each arm tip (4 small rectangles) */
            glColor4f(1.0f, 1.0f, 1.0f, 0.45f);
            glBegin(GL_LINE_LOOP);
            glVertex2f(row_cx - aw, cy - al);
            glVertex2f(row_cx + aw, cy - al);
            glVertex2f(row_cx + aw, cy - aw);
            glVertex2f(row_cx + al, cy - aw);
            glVertex2f(row_cx + al, cy + aw);
            glVertex2f(row_cx + aw, cy + aw);
            glVertex2f(row_cx + aw, cy + al);
            glVertex2f(row_cx - aw, cy + al);
            glVertex2f(row_cx - aw, cy + aw);
            glVertex2f(row_cx - al, cy + aw);
            glVertex2f(row_cx - al, cy - aw);
            glVertex2f(row_cx - aw, cy - aw);
            glEnd();
        } else {
            /* Face button: filled circle */
            draw_circle(row_cx, cy, btn_r, rows[i].br, rows[i].bg, rows[i].bb);

            /* Thin outline */
            glColor4f(1.0f, 1.0f, 1.0f, 0.55f);
            glBegin(GL_LINE_LOOP);
            for (int k = 0; k < 20; k++) {
                float a = k * 6.28318530f / 20.0f;
                glVertex2f(row_cx + btn_r * cosf(a), cy + btn_r * sinf(a));
            }
            glEnd();

            /* Letter centred in circle */
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, s_font_tex);
            float ls = 1.5f * scale;
            float lw = string_width(rows[i].btn, ls);
            float lh = 8.0f * ls;
            draw_string(rows[i].btn,
                        row_cx - lw * 0.5f,
                        cy     - lh * 0.5f,
                        ls, 1.0f, 1.0f, 1.0f);
        }

        /* Action description */
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, s_font_tex);
        float ay = cy - 8.0f * rs * 0.5f;
        draw_string(rows[i].action, lbl_x, ay, rs, 0.9f, 0.9f, 0.9f);
    }

    /* Second divider */
    glDisable(GL_TEXTURE_2D);
    float div2_y = row_y0 + 4 * row_step + btn_r + 10.0f * scale;
    glColor4f(0.35f, 0.35f, 0.75f, 0.9f);
    glBegin(GL_LINES);
    glVertex2f(px + 12.0f * scale, div2_y);
    glVertex2f(px + pw - 12.0f * scale, div2_y);
    glEnd();

    /* Footer */
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, s_font_tex);
    const char *footer = "PRESS START TO RESUME";
    float fs   = 1.5f * scale;
    float fx   = px + (pw - string_width(footer, fs)) * 0.5f;
    float fy   = div2_y + 10.0f * scale;
    /* Pulsing alpha based on tick count */
    unsigned int t  = (unsigned int)GetTickCount();
    float pulse = 0.55f + 0.45f * sinf((float)(t % 2000) * 3.14159265f / 1000.0f);
    draw_string(footer, fx, fy, fs, 0.9f, 0.9f, 0.9f * pulse + 0.1f);

    /* Restore state */
    glDisable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (was_tex2d)  glEnable(GL_TEXTURE_2D);  else glDisable(GL_TEXTURE_2D);
    if (was_light)  glEnable(GL_LIGHTING);    else glDisable(GL_LIGHTING);
    if (was_depth)  glEnable(GL_DEPTH_TEST);  else glDisable(GL_DEPTH_TEST);
    if (was_cull)   glEnable(GL_CULL_FACE);   else glDisable(GL_CULL_FACE);
    if (was_blend)  glEnable(GL_BLEND);       else glDisable(GL_BLEND);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}
