/* main_xbox.cpp -- XDK entry point for the GLSnake Xbox port.
 *
 * Sets up the drive mapping, initialises D3D/RXGL, then calls
 * snake_main() (glsnake's main() renamed via preinclude.h).
 * snake_main() calls glutMainLoop() which does not return until
 * the user presses Back; we then spin forever as required by Xbox. */

/* preinclude.h (force-included by /FI) defines:  #define main snake_main
 * We must undef it here so that our own void __cdecl main(void) stays
 * as the real XDK entry point. */
#ifdef main
#undef main
#endif

#include "rxgl_api.h"   /* must be first: xtl.h, GL types */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* snake_main is glsnake's main(), renamed by #define in preinclude.h. */
extern "C" int snake_main(int argc, char **argv);

/* Declared in glut_xbox.cpp */
extern "C" void xbox_glut_set_display_size(int w, int h);

/* ---- Crash / diagnostic log --------------------------------------------- */
static void Log(const char *msg)
{
    OutputDebugStringA(msg);
    OutputDebugStringA("\r\n");

    static const char *paths[] = {
        "Z:\\glsnake.log", "E:\\glsnake.log",
        "F:\\glsnake.log", "D:\\glsnake.log", NULL
    };
    HANDLE h = INVALID_HANDLE_VALUE;
    for (int i = 0; paths[i] && h == INVALID_HANDLE_VALUE; i++)
        h = CreateFileA(paths[i], GENERIC_WRITE, 0, NULL,
                        OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return;
    SetFilePointer(h, 0, NULL, FILE_END);
    DWORD n = 0;
    WriteFile(h, msg, (DWORD)lstrlenA(msg), &n, NULL);
    WriteFile(h, "\r\n", 2, &n, NULL);
    CloseHandle(h);
}

/* ---- Xbox kernel types for drive-letter mounting ------------------------- */
extern "C" {
    typedef struct {
        unsigned short Length;
        unsigned short MaximumLength;
        char *Buffer;
    } XBOX_STRING;

    extern XBOX_STRING XeImageFileName;
    long __stdcall IoCreateSymbolicLink(XBOX_STRING *LinkName, XBOX_STRING *DeviceName);
    long __stdcall IoDeleteSymbolicLink(XBOX_STRING *LinkName);
}

static bool s_mounted_t = false;

static long xmount(const char *letter, const char *device_path)
{
    char link_buf[32];
    _snprintf(link_buf, sizeof(link_buf) - 1, "\\??\\%s:", letter);
    link_buf[sizeof(link_buf) - 1] = '\0';
    XBOX_STRING link_str = { (unsigned short)strlen(link_buf),
                              (unsigned short)(strlen(link_buf) + 1), link_buf };
    XBOX_STRING tgt_str  = { (unsigned short)strlen(device_path),
                              (unsigned short)(strlen(device_path) + 1), (char *)device_path };
    IoDeleteSymbolicLink(&link_str);
    return IoCreateSymbolicLink(&link_str, &tgt_str);
}

static void xumount(const char *letter)
{
    char link_buf[32];
    _snprintf(link_buf, sizeof(link_buf) - 1, "\\??\\%s:", letter);
    link_buf[sizeof(link_buf) - 1] = '\0';
    XBOX_STRING link_str = { (unsigned short)strlen(link_buf),
                              (unsigned short)(strlen(link_buf) + 1), link_buf };
    IoDeleteSymbolicLink(&link_str);
}

/* Skip 4 backslash-delimited components (\Device\HarddiskN\PartitionN\)
   and probe candidate drive letters for the relative tail. */
static bool device_dir_to_drive(const char *dev_dir, char *out, int out_size)
{
    const char *rel = dev_dir;
    int slashes = 0;
    while (*rel && slashes < 4) { if (*rel++ == '\\') slashes++; }

    const char *drives[] = { "F:", "E:", "C:", "G:", "H:", NULL };
    for (int i = 0; drives[i]; i++) {
        char try_path[MAX_PATH];
        _snprintf(try_path, sizeof(try_path) - 1, "%s\\%s", drives[i], rel);
        DWORD attr = GetFileAttributesA(try_path);
        if (attr != (DWORD)-1 && (attr & FILE_ATTRIBUTE_DIRECTORY)) {
            _snprintf(out, out_size - 1, "%s", try_path);
            return true;
        }
    }
    return false;
}

/* ---- XDK entry point ---------------------------------------------------- */
void __cdecl main(void)
{
    Log("glsnake: start");

    /* ---- Locate the XBE directory and map it to a drive letter ----------- */
    char game_dir[MAX_PATH];
    {
        char xbe_dir[MAX_PATH];
        unsigned short len = XeImageFileName.Length;
        if (len > 0 && len < (unsigned short)(sizeof(xbe_dir) - 1)) {
            memcpy(xbe_dir, XeImageFileName.Buffer, len);
            xbe_dir[len] = '\0';
            char *sep = xbe_dir + len;
            while (sep > xbe_dir && sep[-1] != '\\') sep--;
            if (sep > xbe_dir) sep[-1] = '\0';
        } else {
            strncpy(xbe_dir,
                    "\\Device\\Harddisk0\\Partition1\\Devkit\\XDKSAMPLES\\GLSnake",
                    sizeof(xbe_dir) - 1);
            xbe_dir[sizeof(xbe_dir) - 1] = '\0';
        }
        Log("xbe_dir:"); Log(xbe_dir);

        char drive_dir[MAX_PATH];
        if (device_dir_to_drive(xbe_dir, drive_dir, sizeof(drive_dir))) {
            _snprintf(game_dir, sizeof(game_dir) - 1, "%s", drive_dir);
        } else {
            long st = xmount("T", xbe_dir);
            char stbuf[64];
            _snprintf(stbuf, sizeof(stbuf) - 1,
                      "IoCreateSymbolicLink(T)=0x%08lx", (unsigned long)st);
            Log(stbuf);
            s_mounted_t = true;
            _snprintf(game_dir, sizeof(game_dir) - 1, "T:\\");
        }
        Log("game_dir:"); Log(game_dir);
    }

    /* ---- XInput device registration (must precede any XInputOpen call) ---- */
    {
        XDEVICE_PREALLOC_TYPE devTypes[] = {
            { XDEVICE_TYPE_GAMEPAD, 4 },
        };
        XInitDevices(1, devTypes);
    }
    Log("glsnake: XInitDevices OK");

    /* ---- Choose resolution from dashboard video settings ---- */
    DWORD vf = XGetVideoFlags();
    int d3d_w, d3d_h, d3d_vmode;
    int logical_w, logical_h;

    if ((vf & XC_VIDEO_FLAGS_HDTV_720p) && XGetAVPack() == XC_AV_PACK_HDTV) {
        /* True 720p — RXGL will set PROGRESSIVE | WIDESCREEN */
        d3d_w = 1280; d3d_h = 720; d3d_vmode = 1;
        logical_w = 1280; logical_h = 720;
        Log("glsnake: mode 1280x720 (720p)");
    } else if ((vf & XC_VIDEO_FLAGS_HDTV_480p) && XGetAVPack() == XC_AV_PACK_HDTV) {
        /* 480p progressive — widescreen flag applied by RXGL if set */
        d3d_w = 640; d3d_h = 480; d3d_vmode = 1;
        logical_w = (vf & XC_VIDEO_FLAGS_WIDESCREEN) ? 853 : 640;
        logical_h = 480;
        Log("glsnake: mode 640x480 progressive");
    } else {
        /* 480i — RXGL adds WIDESCREEN flag if user has it set */
        d3d_w = 640; d3d_h = 480; d3d_vmode = 0;
        logical_w = (vf & XC_VIDEO_FLAGS_WIDESCREEN) ? 853 : 640;
        logical_h = 480;
        Log("glsnake: mode 640x480 interlaced");
    }

    /* ---- D3D / RXGL context ---- */
    d3dSetMode(d3d_w, d3d_h, 32, 24, d3d_vmode);
    wglCreateContext();
    wglMakeCurrent();
    Log("glsnake: RXGL context OK");

    /* Tell the GLUT layer what logical display aspect to use for the camera. */
    xbox_glut_set_display_size(logical_w, logical_h);

    /* ---- Run glsnake ---- */
    const char *argv_stub[] = { "glsnake.xbe" };
    snake_main(1, (char **)argv_stub);

    Log("glsnake: returned");

    if (s_mounted_t) { xumount("T"); s_mounted_t = false; }

    for (;;) {}   /* Xbox must never return from main */
}
