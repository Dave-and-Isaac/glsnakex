#pragma once
/* sys/timeb.h -- minimal struct timeb for HAVE_FTIME path in glsnake.c.
 * XDK does not ship a sys/timeb.h; the XDK CRT does export ftime(). */

#ifndef _TIMEB_DEFINED
#define _TIMEB_DEFINED

struct timeb {
    long           time;      /* seconds since epoch */
    unsigned short millitm;   /* milliseconds */
    short          timezone;  /* minutes west of UTC */
    short          dstflag;   /* DST in effect? */
};

#ifdef __cplusplus
extern "C" {
#endif
void ftime(struct timeb *tp);
#ifdef __cplusplus
}
#endif

#endif /* _TIMEB_DEFINED */
