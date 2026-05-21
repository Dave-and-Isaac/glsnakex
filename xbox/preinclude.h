#pragma once
/* preinclude.h -- force-included in every TU via /FI compiler flag.
 * Suppresses XDK/VS noise, pulls in xtl.h to set _WINDOWS_ guard,
 * defines HAVE_GLUT/HAVE_FTIME so glsnake.c takes the right paths,
 * and renames main() so our Xbox entry point can own that symbol. */

#pragma warning(disable: 4005 5040 4530 4996 4273 5043 4267 4244 4018 4090 4101 4189 4702 4311 4312 4133)

#ifndef _XBOX
#define _XBOX
#endif

/* xtl.h must come before any CRT header to define Win32 types. */
#include <xtl.h>
#include <time.h>   /* time_t, time(), localtime(), struct tm — used by spooky() */

/* errno_t: VS2005+ CRT has it; older XDK headers do not. */
#ifndef _ERRNO_T_DEFINED
#define _ERRNO_T_DEFINED
typedef int errno_t;
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

/* Build switches for glsnake.c */
#define HAVE_GLUT
#define HAVE_FTIME

/* Rename glsnake's main() so main_xbox.cpp can own the entry point. */
#define main snake_main

/* ----------------------------------------------------------------
 * CRT stdio redirect — same scheme as other RXDK ports.
 * Xbox libcmt maps FILE to kernel-space addresses; we replace
 * all file I/O with Win32-backed implementations in crt_compat.cpp.
 * ---------------------------------------------------------------- */
struct _iobuf;
#ifdef __cplusplus
extern "C" {
#endif
struct _iobuf *xbox_crt_fopen  (const char *, const char *);
int            xbox_crt_fclose (struct _iobuf *);
unsigned int   xbox_crt_fread  (void *, unsigned int, unsigned int, struct _iobuf *);
unsigned int   xbox_crt_fwrite (const void *, unsigned int, unsigned int, struct _iobuf *);
int            xbox_crt_fseek  (struct _iobuf *, long, int);
long           xbox_crt_ftell  (struct _iobuf *);
int            xbox_crt_feof   (struct _iobuf *);
int            xbox_crt_ferror (struct _iobuf *);
void           xbox_crt_clearerr(struct _iobuf *);
void           xbox_crt_rewind (struct _iobuf *);
int            xbox_crt_fgetc  (struct _iobuf *);
int            xbox_crt_fputc  (int, struct _iobuf *);
char          *xbox_crt_fgets  (char *, int, struct _iobuf *);
int            xbox_crt_fputs  (const char *, struct _iobuf *);
int            xbox_crt_ungetc (int, struct _iobuf *);
int            xbox_crt_setvbuf(struct _iobuf *, char *, int, unsigned int);
int            xbox_crt_fflush (struct _iobuf *);
int            xbox_crt_fprintf(struct _iobuf *, const char *, ...);
int            xbox_crt_printf (const char *, ...);
int            xbox_crt_puts   (const char *);
#ifdef __cplusplus
}
#endif

#ifdef feof
#undef feof
#endif
#ifdef ferror
#undef ferror
#endif
#ifdef clearerr
#undef clearerr
#endif
#ifdef fgetc
#undef fgetc
#endif
#ifdef fputc
#undef fputc
#endif
#ifdef getc
#undef getc
#endif
#ifdef putc
#undef putc
#endif

#define fopen(p,m)        xbox_crt_fopen(p,m)
#define fclose(f)         xbox_crt_fclose(f)
#define fread(b,e,n,f)    xbox_crt_fread(b,e,n,f)
#define fwrite(b,e,n,f)   xbox_crt_fwrite(b,e,n,f)
#define fseek(f,o,w)      xbox_crt_fseek(f,o,w)
#define ftell(f)          xbox_crt_ftell(f)
#define feof(f)           xbox_crt_feof(f)
#define ferror(f)         xbox_crt_ferror(f)
#define clearerr(f)       xbox_crt_clearerr(f)
#define rewind(f)         xbox_crt_rewind(f)
#define fgetc(f)          xbox_crt_fgetc(f)
#define fputc(c,f)        xbox_crt_fputc(c,f)
#define getc(f)           xbox_crt_fgetc(f)
#define putc(c,f)         xbox_crt_fputc(c,f)
#define fgets(b,n,f)      xbox_crt_fgets(b,n,f)
#define fputs(s,f)        xbox_crt_fputs(s,f)
#define ungetc(c,f)       xbox_crt_ungetc(c,f)
#define setvbuf(f,b,m,s)  xbox_crt_setvbuf(f,b,m,s)
#define fflush(f)         xbox_crt_fflush(f)
#define fprintf           xbox_crt_fprintf
#define printf            xbox_crt_printf
#define puts(s)           xbox_crt_puts(s)
