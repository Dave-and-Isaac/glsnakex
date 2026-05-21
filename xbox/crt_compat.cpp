/* crt_compat.cpp -- CRT/XDK compatibility shims for GLSnake Xbox port.
 *
 * Provides symbols that VS2022-generated code emits but the XDK's libcmt
 * does not export, plus a Win32-backed file I/O layer (preinclude.h
 * redirects all fopen/fread/... calls to the xbox_crt_* functions here).
 *
 * Also provides ftime() via GetTickCount() because glsnake.c uses the
 * HAVE_FTIME path and the XDK CRT only exports _ftime / _ftime64. */

#include <xtl.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/* ---- C++14 sized deallocation (VS2022 /std:c++17 emits these) ---- */
void operator delete  (void *p, unsigned int) { free(p); }
void operator delete[](void *p, unsigned int) { free(p); }

/* ---- _ftol2_sse: VS2022 float->long helper; XDK has only _ftol2 ---- */
extern "C" __declspec(naked) long __cdecl _ftol2_sse(void)
{
    __asm {
        sub   esp, 12
        movlps qword ptr [esp], xmm0
        fld   qword ptr [esp]
        fnstcw word ptr [esp+8]
        mov   ax, word ptr [esp+8]
        or    ax, 0x0C00
        mov   word ptr [esp+10], ax
        fldcw word ptr [esp+10]
        fistp dword ptr [esp]
        fldcw word ptr [esp+8]
        mov   eax, dword ptr [esp]
        add   esp, 12
        ret
    }
}

/* ---- vsnprintf: XDK calls it _vsnprintf ---- */
extern "C" int vsnprintf(char *buf, size_t n, const char *fmt, va_list ap)
{
    return _vsnprintf(buf, n, fmt, ap);
}

/* ---- snprintf ---- */
extern "C" int snprintf(char *buf, size_t n, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int r = _vsnprintf(buf, n, fmt, ap);
    va_end(ap);
    return r;
}

/* ---- getenv: XDK stub debug-breaks; always return NULL ---- */
extern "C" char * __cdecl getenv(const char *name)
{
    (void)name;
    return NULL;
}


/* ---- _iob: prevent stray CRT dereferences from AV'ing ---- */
extern "C" {
    FILE _iob[64];
}

/* ---- errno ---- */
#ifdef errno
#undef errno
#endif
extern "C" int errno = 0;

/* ---- ftime: glsnake uses HAVE_FTIME / struct timeb.
 * xbox/sys/timeb.h defines struct timeb; XDK CRT exports _ftime but not
 * ftime; provide it here using GetTickCount() for millisecond accuracy. ---- */
#include <sys/timeb.h>

extern "C" void ftime(struct timeb *tp)
{
    if (!tp) return;
    ULONGLONG ms = (ULONGLONG)GetTickCount();
    tp->time     = (long)(ms / 1000ULL);
    tp->millitm  = (unsigned short)(ms % 1000ULL);
    tp->timezone = 0;
    tp->dstflag  = 0;
}

/* ============================================================
 * Win32-backed file I/O
 * (preinclude.h #defines fopen -> xbox_crt_fopen etc.)
 * ============================================================ */

#define XBOX_FILE_SLOTS 32

typedef struct {
    HANDLE h;
    int    eof_flag;
    int    err_flag;
    char   unget_buf;
    int    has_unget;
} xbox_fslot_t;

static xbox_fslot_t s_fslots[XBOX_FILE_SLOTS];

static int is_xbox_file(FILE *f)
{
    char  *fp   = (char *)f;
    char  *base = (char *)s_fslots;
    size_t off;
    if (!f || fp < base || fp >= base + sizeof(s_fslots)) return 0;
    off = (size_t)(fp - base);
    return (off % sizeof(xbox_fslot_t) == 0);
}

static xbox_fslot_t *to_slot(FILE *f)   { return (xbox_fslot_t *)f; }
static FILE         *from_slot(xbox_fslot_t *s) { return (FILE *)s; }

extern "C" FILE *xbox_crt_fopen(const char *path, const char *mode)
{
    OutputDebugStringA("[xfopen] ");
    OutputDebugStringA(path ? path : "NULL");
    OutputDebugStringA("\r\n");

    int i;
    for (i = 0; i < XBOX_FILE_SLOTS; i++) {
        if (s_fslots[i].h == NULL) break;
    }
    if (i == XBOX_FILE_SLOTS) return NULL;

    xbox_fslot_t *s = &s_fslots[i];
    memset(s, 0, sizeof(*s));

    int rw = (strchr(mode, '+') != NULL);
    DWORD access = 0, share = 0, disp = 0;

    if (mode[0] == 'r') {
        access = GENERIC_READ; share = FILE_SHARE_READ; disp = OPEN_EXISTING;
        if (rw) access |= GENERIC_WRITE;
    } else if (mode[0] == 'w') {
        access = GENERIC_WRITE; disp = CREATE_ALWAYS;
        if (rw) access |= GENERIC_READ;
    } else if (mode[0] == 'a') {
        access = GENERIC_WRITE; disp = OPEN_ALWAYS;
        if (rw) access |= GENERIC_READ;
    } else {
        return NULL;
    }

    HANDLE h = CreateFileA(path, access, share, NULL, disp,
                           FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return NULL;
    if (mode[0] == 'a') SetFilePointer(h, 0, NULL, FILE_END);
    s->h = h;
    return from_slot(s);
}

extern "C" int xbox_crt_fclose(FILE *f)
{
    if (!is_xbox_file(f)) return EOF;
    xbox_fslot_t *s = to_slot(f);
    if (!s->h) return EOF;
    CloseHandle(s->h);
    memset(s, 0, sizeof(*s));
    return 0;
}

extern "C" unsigned int xbox_crt_fread(void *buf, unsigned int elem,
                                        unsigned int count, FILE *f)
{
    if (!is_xbox_file(f)) return 0;
    xbox_fslot_t *s = to_slot(f);
    if (s->eof_flag || !s->h) return 0;
    size_t total = (size_t)elem * count;
    if (total == 0) return 0;
    char  *dst = (char *)buf;
    size_t got = 0;
    if (s->has_unget && got < total) {
        dst[got++] = s->unget_buf;
        s->has_unget = 0;
    }
    if (got < total) {
        DWORD nread = 0;
        if (!ReadFile(s->h, dst + got, (DWORD)(total - got), &nread, NULL))
            s->err_flag = 1;
        got += nread;
        if (got < total) s->eof_flag = 1;
    }
    return (elem > 0) ? (unsigned int)(got / elem) : 0;
}

extern "C" unsigned int xbox_crt_fwrite(const void *buf, unsigned int elem,
                                         unsigned int count, FILE *f)
{
    if (!is_xbox_file(f)) return 0;
    xbox_fslot_t *s = to_slot(f);
    if (!s->h) return 0;
    size_t total = (size_t)elem * count;
    if (total == 0) return count;
    DWORD nwritten = 0;
    if (!WriteFile(s->h, buf, (DWORD)total, &nwritten, NULL)) {
        s->err_flag = 1;
        return 0;
    }
    return (elem > 0) ? (unsigned int)(nwritten / elem) : 0;
}

extern "C" int xbox_crt_fseek(FILE *f, long offset, int whence)
{
    if (!is_xbox_file(f)) return -1;
    xbox_fslot_t *s = to_slot(f);
    if (!s->h) return -1;
    DWORD method;
    switch (whence) {
    case SEEK_SET: method = FILE_BEGIN;   break;
    case SEEK_CUR: method = FILE_CURRENT; break;
    case SEEK_END: method = FILE_END;     break;
    default:       return -1;
    }
    s->eof_flag  = 0;
    s->has_unget = 0;
    DWORD r = SetFilePointer(s->h, offset, NULL, method);
    return (r == (DWORD)INVALID_HANDLE_VALUE) ? -1 : 0;
}

extern "C" long xbox_crt_ftell(FILE *f)
{
    if (!is_xbox_file(f)) return -1L;
    xbox_fslot_t *s = to_slot(f);
    if (!s->h) return -1L;
    DWORD pos = SetFilePointer(s->h, 0, NULL, FILE_CURRENT);
    if (pos == (DWORD)INVALID_HANDLE_VALUE) return -1L;
    if (s->has_unget && pos > 0) pos--;
    return (long)pos;
}

extern "C" int xbox_crt_feof(FILE *f)
{
    if (!is_xbox_file(f)) return 0;
    return to_slot(f)->eof_flag;
}

extern "C" int xbox_crt_ferror(FILE *f)
{
    if (!is_xbox_file(f)) return 0;
    return to_slot(f)->err_flag;
}

extern "C" void xbox_crt_clearerr(FILE *f)
{
    if (!is_xbox_file(f)) return;
    xbox_fslot_t *s = to_slot(f);
    s->eof_flag = 0;
    s->err_flag = 0;
}

extern "C" void xbox_crt_rewind(FILE *f)
{
    xbox_crt_fseek(f, 0L, SEEK_SET);
    xbox_crt_clearerr(f);
}

extern "C" int xbox_crt_fgetc(FILE *f)
{
    if (!is_xbox_file(f)) return EOF;
    xbox_fslot_t *s = to_slot(f);
    if (s->has_unget) { s->has_unget = 0; return (unsigned char)s->unget_buf; }
    if (s->eof_flag || !s->h) return EOF;
    unsigned char c; DWORD n = 0;
    ReadFile(s->h, &c, 1, &n, NULL);
    if (n == 0) { s->eof_flag = 1; return EOF; }
    return (int)c;
}

extern "C" int xbox_crt_fputc(int c, FILE *f)
{
    if (!is_xbox_file(f)) {
        char tmp[2] = { (char)c, '\0' };
        OutputDebugStringA(tmp);
        return c;
    }
    xbox_fslot_t *s = to_slot(f);
    if (!s->h) return EOF;
    unsigned char b = (unsigned char)c;
    DWORD n = 0;
    WriteFile(s->h, &b, 1, &n, NULL);
    return (n == 1) ? c : EOF;
}

extern "C" int xbox_crt_ungetc(int c, FILE *f)
{
    if (!is_xbox_file(f)) return EOF;
    xbox_fslot_t *s = to_slot(f);
    if (s->has_unget) return EOF;
    s->unget_buf = (char)c;
    s->has_unget = 1;
    s->eof_flag  = 0;
    return c;
}

extern "C" char *xbox_crt_fgets(char *buf, int n, FILE *f)
{
    if (!buf || n <= 0) return NULL;
    int i = 0;
    while (i < n - 1) {
        int c = xbox_crt_fgetc(f);
        if (c == EOF) { if (i == 0) return NULL; break; }
        buf[i++] = (char)c;
        if (c == '\n') break;
    }
    buf[i] = '\0';
    return buf;
}

extern "C" int xbox_crt_fputs(const char *str, FILE *f)
{
    if (!str) return EOF;
    size_t len = strlen(str);
    if (!is_xbox_file(f)) { OutputDebugStringA(str); return (int)len; }
    return (int)xbox_crt_fwrite(str, 1, (unsigned int)len, f);
}

extern "C" int xbox_crt_setvbuf(FILE *f, char *buf, int mode, unsigned int size)
{
    (void)f; (void)buf; (void)mode; (void)size;
    return 0;
}

extern "C" int xbox_crt_fflush(FILE *f)
{
    if (!is_xbox_file(f)) return 0;
    xbox_fslot_t *s = to_slot(f);
    if (s->h) FlushFileBuffers(s->h);
    return 0;
}

extern "C" int xbox_crt_fprintf(FILE *f, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    char buf[1024];
    int r = _vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
    va_end(ap);
    if (r < 0) r = 0;
    buf[r] = '\0';
    if (!is_xbox_file(f)) { OutputDebugStringA(buf); return r; }
    xbox_crt_fwrite(buf, 1, (unsigned int)r, f);
    return r;
}

extern "C" int xbox_crt_printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    char buf[1024];
    int r = _vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
    va_end(ap);
    if (r < 0) r = 0;
    buf[r] = '\0';
    OutputDebugStringA(buf);
    return r;
}

extern "C" int xbox_crt_puts(const char *str)
{
    OutputDebugStringA(str ? str : "(null)");
    OutputDebugStringA("\r\n");
    return 0;
}
