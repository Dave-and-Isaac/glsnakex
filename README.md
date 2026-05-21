# GLSnake Xbox Port

An original Xbox port of [glsnake](https://github.com/jaqx0r/glsnake) — the animated
Rubik's-snake screensaver — built with RXDK and the RXGL OpenGL→Direct3D 8 shim.

Outputs a `.xbe` that runs on a softmodded original Xbox.
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

You need the following already set up on your build machine:

- **Visual Studio 2022** with the Desktop C++ workload (MSVC v143, 32-bit tools)
- **RXDK** — set the `RXDK_LIBS` environment variable to your install path (e.g. `C:\RXDK\`). Must end with a backslash.
- **RXGL** — set the `RXGL_DIR` environment variable to your RXGL root (e.g. `C:\dev\RXGL\`). Must end with a backslash.

### Environment variables

| Variable | Example | Required |
|----------|---------|----------|
| `RXDK_LIBS` | `C:\RXDK\` | Yes |
| `RXGL_DIR` | `C:\dev\RXGL\` | No — if omitted, RXGL is expected as a sibling directory named `RXGL` next to this repo |

If you have RXGL cloned alongside this repo (sibling layout below), `RXGL_DIR` is not needed:

```
your-folder\
├── glsnake-xbox\       ← this repo
└── RXGL\               ← RXGL root (sibling, no RXGL_DIR needed)
    └── RXGL\
        ├── rxgl_api.h
        └── ...
```

## Building

```
git clone https://github.com/Dave-and-Isaac/glsnake-xbox.git
```

Open `GLSnake-Xbox.sln` in Visual Studio 2022, select **Release | Win32**, and build.

After a successful build the deploy folder is ready at:

```
bin\Release\Xbox\Deploy\
    GLSnake.xbe
    data\
        models.glsnake
```

Copy the `Deploy\` folder to your Xbox and launch `GLSnake.xbe`.

> The **Debug** build links `xbdm.lib` and requires a debug-enabled kernel with
> `xbdm.xbe` running on the console.

## How it works

`glsnake.c` is compiled unmodified as C. `xbox/preinclude.h` (force-included via `/FI`)
defines `HAVE_GLUT`, `HAVE_FTIME`, and `#define main snake_main` to rename the game
entry point. `main_xbox.cpp` owns the real XDK entry point, detects the dashboard video
mode, sets up RXGL, and calls into glsnake. RXGL translates every OpenGL 1.x call into
Direct3D 8 at runtime.

## License

`glsnake.c` is GPL-2.0 © Andrew Bennetts et al — see `COPYING`.  
Xbox platform layer (`xbox/`) is original work released under MIT.
