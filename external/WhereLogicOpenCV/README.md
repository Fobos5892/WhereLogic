# WhereLogicOpenCV

External OpenCV package for [WhereLogic](https://github.com/your-org/WhereLogic).  
Clone separately into `WhereLogic/external/WhereLogicOpenCV`.

Built with **the same MinGW as Qt 6.11** (`C:\Qt\Tools\mingw1310_64`) so runtime DLLs match Qt Multimedia and the rest of the app. Do **not** use MSYS2 OpenCV binaries — they are ABI-incompatible.

## Layout

```
WhereLogicOpenCV/
  manifest.json
  scripts/
    build_qt_mingw.ps1    # build prebuilt from sources
    copy_dlls.ps1         # copy runtime DLLs next to exe (called from qmake POST_LINK)
  prebuilt/               # gitignored — produced by build script
    include/opencv2/opencv.hpp
    x64/mingw/lib/*.dll.a
    x64/mingw/bin/*.dll
    x64/msvc/             # optional MSVC kit
```

## Quick start

```powershell
# Requires: Qt 6.11 with Tools (CMake, Ninja, mingw1310_64)
powershell -ExecutionPolicy Bypass -File scripts/build_qt_mingw.ps1
```

~15 minutes on first run (downloads OpenCV 4.13 sources into `.build/`).

## Use as git submodule

From WhereLogic root:

```powershell
git submodule add <this-repo-url> external/WhereLogicOpenCV
git submodule update --init --recursive
```

## Modules

Only what WhereLogic needs: `core`, `imgproc`, `imgcodecs` (image masks in round 5).

## MSVC kit

For MSVC Qt kit, fetch Windows prebuilt via WhereLogic `scripts/fetch_ai_deps.py --id opencv_win_msvc` into `prebuilt/x64/msvc/` (see WhereLogic README).
