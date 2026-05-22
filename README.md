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

## Just want to play it?

Download the latest release zip, unzip it, FTP the `glsnakex` folder to your Xbox, and run `default.xbe`.

## Prerequisites

### 1. Visual Studio 2022

Install [Visual Studio 2022](https://visualstudio.microsoft.com/) with the **Desktop development with C++** workload. The 32-bit MSVC v143 tools are included automatically.

### 2. RXDK

Install RXDK. The installer sets the `RXDK_LIBS` environment variable automatically — nothing else to do.

### 3. RXGL

Clone [RXGL](https://github.com/Team-Resurgent/RXGL) into the **same parent folder** as this repo:

```
your-folder\
├── glsnakex\     ← this repo
└── RXGL\         ← RXGL repo root
    └── RXGL\
        ├── rxgl_api.h
        └── ...
```

If you cloned RXGL somewhere else, run **`setup.bat`** in the root of this repo.
It will ask for your RXGL path and configure everything automatically.

## Building

```
git clone https://github.com/Dave-and-Isaac/glsnakex.git
```

Open `GLSnake-Xbox.sln` in Visual Studio 2022, select **Release | Win32**, and build.

After a successful build the output is at:

```
bin\Release\Xbox\
    Build\
        GLSnake-Xbox\
            default.xbe
            data\
                models.glsnake
    XISO\
        GLSnake-Xbox.iso
```

FTP the `GLSnake-Xbox\` folder to your Xbox and launch `default.xbe`, or burn / mount `GLSnake-Xbox.iso`.

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
