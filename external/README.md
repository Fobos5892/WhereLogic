# External dependencies

Large third-party binaries live in separate repositories and are cloned next to WhereLogic.

## WhereLogicOpenCV

OpenCV prebuilt for WhereLogic (Qt MinGW / MSVC). ABI-compatible with Qt 6.11 `mingw1310_64`.

```powershell
# From WhereLogic repo root — submodule (when remote is configured):
git submodule update --init external/WhereLogicOpenCV

# Or clone manually into this path:
git clone <WhereLogicOpenCV-repo-url> external/WhereLogicOpenCV
```

Build prebuilt (once per machine / after OpenCV version bump):

```powershell
cd external/WhereLogicOpenCV
powershell -ExecutionPolicy Bypass -File scripts/build_qt_mingw.ps1
```

Then in Qt Creator: **Run qmake → Rebuild All**.

Custom location (optional):

```powershell
qmake WHERELOGIC_OPENCV_ROOT=D:/libs/WhereLogicOpenCV/prebuilt
```
