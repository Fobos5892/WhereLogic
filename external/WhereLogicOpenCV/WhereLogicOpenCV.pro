# Auto-build OpenCV prebuilt (Qt MinGW) before the main app.
# Included from WhereLogic.pro when CONFIG does not contain no_opencv.

TEMPLATE = aux

isEmpty(WHERELOGIC_OPENCV_ROOT) {
    OPENCV_EXTERNAL = $$PWD
} else {
    OPENCV_EXTERNAL = $$clean_path($$WHERELOGIC_OPENCV_ROOT/..)
}

OPENCV_STAMP = $$OPENCV_EXTERNAL/prebuilt/.opencv_built
OPENCV_BUILD_SCRIPT = $$shell_path($$OPENCV_EXTERNAL/scripts/build_qt_mingw.ps1)

contains(CONFIG, no_opencv) {
    message("WhereLogicOpenCV: skipped (CONFIG+=no_opencv)")
} else:!win32-g++ {
    message("WhereLogicOpenCV: auto-build is enabled for MinGW kit only")
} else:!exists($$OPENCV_BUILD_SCRIPT) {
    warning("WhereLogicOpenCV: scripts not found — clone external/WhereLogicOpenCV")
} else {
    message("WhereLogicOpenCV: prebuilt stamp $$OPENCV_STAMP")

    OPENCV_BUILD_CMD = powershell -NoProfile -ExecutionPolicy Bypass -File \"$$OPENCV_BUILD_SCRIPT\"
    contains(CONFIG, rebuild_opencv) {
        OPENCV_BUILD_CMD = $$OPENCV_BUILD_CMD -Rebuild
    }

    opencv_prebuilt.target = $$OPENCV_STAMP
    opencv_prebuilt.commands = $$OPENCV_BUILD_CMD
    opencv_prebuilt.depends = $$OPENCV_EXTERNAL/scripts/build_qt_mingw.ps1 \
                              $$OPENCV_EXTERNAL/scripts/copy_dlls.ps1 \
                              $$OPENCV_EXTERNAL/manifest.json
    QMAKE_EXTRA_TARGETS += opencv_prebuilt

    build_all.target = all
    build_all.depends = opencv_prebuilt
    QMAKE_EXTRA_TARGETS += build_all
}
