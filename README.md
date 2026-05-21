# GLSnake Xbox Port

An original Xbox port of [glsnake](https://github.com/attah/glsnake) — the animated
Rubik's-snake screensaver — built with [RXDK](https://github.com/Team-Resurgent/RXDK)
and the [RXGL](https://github.com/Team-Resurgent/RXGL) OpenGL→Direct3D 8 shim.

Outputs a `.xbe` that runs on a softmodded or RXDK-equipped original Xbox.
Supports 480i, 480p, and 720p output depending on dashboard video settings.

## Controls

| Button | Action |
|--------|--------|
| D-Pad / Left Stick | Rotate model |
| A | Next model |
| B | Previous model |
| X | Toggle interactive mode |
| Y | Pause / Resume |
| Start | Controls overlay |
| LT + RT + Back + Black | Return to dashboard |

## Prerequisites

| Requirement | Notes |
|-------------|-------|
| **Windows** | Build machine (MSVC toolchain) |
| **Visual Studio 2022** | With MSVC v143 (Desktop C++) |
| **RXDK** | Xbox Development Kit — set `RXDK_LIBS` environment variable to your install path (e.g. `C:\RXDK\`) |

`RXDK_LIBS` must end with a backslash. The build uses `$(RXDK_LIBS)bin\patchSubsystem.exe`
and `$(RXDK_LIBS)bin\imagebld.exe` to produce the final `.xbe`.

## Building

```
git clone --recurse-submodules https://github.com/Team-Resurgent/glsnake-xbox.git
```

Open `GLSnake-Xbox.sln` in Visual Studio 2022, select **Release | Win32**, and build.

After a successful build:

```
bin\Release\Xbox\Deploy\
    GLSnake.xbe
    data\
        models.glsnake
```

Copy the `Deploy\` folder to your Xbox (via FTP, DVD, or USB depending on your setup)
and launch `GLSnake.xbe`.

> **Debug build** links against `xbdm.lib` for Xbox Debug Monitor support.
> It requires a debug-enabled Xbox kernel and xbdm.xbe running on the console.

## Project structure

```
glsnake-xbox/
├── glsnake.c           Original glsnake source (GPL-2.0)
├── COPYING             GPL-2.0 license
├── data/
│   └── models.glsnake  Snake model definitions
├── xbox/               Xbox platform layer
│   ├── main_xbox.cpp   XDK entry point, resolution detection
│   ├── glut_xbox.cpp   GLUT stub + XInput game loop
│   ├── crt_compat.cpp  CRT shims (ftime, vsnprintf, file I/O)
│   ├── help_screen.cpp Controls overlay (pure GL, embedded font)
│   ├── help_screen.h
│   ├── preinclude.h    Force-included by /FI; renames main()
│   ├── windows.h       Stub: #include <xtl.h>
│   ├── GL/             OpenGL header stubs (include rxgl_api.h)
│   └── sys/timeb.h     ftime() struct for HAVE_FTIME path
└── RXGL/               Git submodule — Team-Resurgent/RXGL
    └── RXGL/           OpenGL 1.x → Direct3D 8 shim source
```

## How it works

`glsnake.c` is compiled unmodified as C. A force-included header (`xbox/preinclude.h`)
defines `HAVE_GLUT`, `HAVE_FTIME`, and `#define main snake_main` so the game's entry
point is renamed. `main_xbox.cpp` owns the real XDK entry point, sets up D3D/RXGL,
detects the dashboard video mode (720p / 480p / 480i widescreen), and calls
`snake_main()` which runs `glutMainLoop()`.

RXGL translates every OpenGL 1.x call made by glsnake into Direct3D 8, allowing the
game to run without modification on Xbox hardware.

## License

`glsnake.c` and `COPYING` are GPL-2.0 © Andrew Bennetts et al.  
Xbox platform layer (`xbox/`) is MIT — see individual file headers.  
RXGL is covered by its own license — see the RXGL submodule.
