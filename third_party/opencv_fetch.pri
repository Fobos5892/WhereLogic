# Optional OpenCV fetch hook — use external/WhereLogicOpenCV or WhereLogicSetup.

isEmpty(WHERELOGIC_OPENCV_ROOT) {
    OPENCV_PREBUILT = $$clean_path($$PWD/../external/WhereLogicOpenCV/prebuilt)
} else {
    OPENCV_PREBUILT = $$clean_path($$WHERELOGIC_OPENCV_ROOT)
}

!exists($$OPENCV_PREBUILT/include/opencv2/opencv.hpp) {
    message("opencv_fetch.pri: no prebuilt — clone external/WhereLogicOpenCV and run scripts/build_qt_mingw.ps1")
}
