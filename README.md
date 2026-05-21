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

### 1. Visual Studio 2022

Install [Visual Studio 2022](https://visualstudio.microsoft.com/) with the **Desktop development with C++** workload. The 32-bit MSVC v143 tools are included automatically.

### 2. RXDK

Install RXDK, then set the `RXDK_LIBS` environment variable to your installation path.

**How to set an environment variable on Windows:**
> Press **Win + S** and search for **"Edit the system environment variables"**, then click **Environment Variables…** at the bottom of the dialog. Under *System variables* click **New** and enter the name and value. Click OK through all dialogs, then **restart Visual Studio** for the change to take effect.

Or from an **Administrator** command prompt (then restart Visual Studio):
```
setx /M RXDK_LIBS "C:\RXDK\"
```

| Variable | Example value | Notes |
|----------|--------------|-------|
| `RXDK_LIBS` | `C:\RXDK\` | Must end with a backslash |

### 3. RXGL

Clone the [RXGL](https://github.com/Team-Resurgent/RXGL) repository somewhere on your machine. The project finds it in one of two ways — use whichever fits your setup:

**Option A — sibling directory (no configuration needed)**

Clone RXGL into the same parent folder as this repo and name it `RXGL`:

```
your-folder\
├── glsnakex\     ← this repo
└── RXGL\         ← RXGL repo root
    └── RXGL\
        ├── rxgl_api.h
        └── ...
```

**Option B — set `RXGL_DIR` (clone it anywhere)**

Clone RXGL wherever you like, then set `RXGL_DIR` to its root path using the same steps as above for `RXDK_LIBS`:

```
setx /M RXGL_DIR "C:\dev\RXGL\"
```

| Variable | Example value | Notes |
|----------|--------------|-------|
| `RXGL_DIR` | `C:\dev\RXGL\` | Must end with a backslash |

## Building

```
git clone https://github.com/Dave-and-Isaac/glsnakex.git
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
