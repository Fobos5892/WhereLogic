# ==============================================================================
# External OpenCV (external/WhereLogicOpenCV)
#
# Auto-build: WhereLogic.pro → opencv_external subproject (MinGW) or qmake
# system() below when prebuilt is missing at configure time.
#
# Skip: CONFIG+=no_opencv
# Force rebuild: CONFIG+=rebuild_opencv
# ==============================================================================

isEmpty(WHERELOGIC_OPENCV_ROOT) {
    OPENCV_EXTERNAL = $$clean_path($$PWD/../external/WhereLogicOpenCV)
    OPENCV_ROOT = $$OPENCV_EXTERNAL/prebuilt
} else {
    OPENCV_ROOT = $$clean_path($$WHERELOGIC_OPENCV_ROOT)
    OPENCV_EXTERNAL = $$clean_path($$OPENCV_ROOT/..)
}

OPENCV_HEADER = $$OPENCV_ROOT/include/opencv2/opencv.hpp
OPENCV_STAMP = $$OPENCV_ROOT/.opencv_built
OPENCV_BUILD_SCRIPT = $$shell_path($$OPENCV_EXTERNAL/scripts/build_qt_mingw.ps1)
OPENCV_READY = false
OPENCV_BINDIR =

contains(CONFIG, no_opencv) {
    message("OpenCV: disabled (CONFIG+=no_opencv)")
} else:!exists($$OPENCV_EXTERNAL/scripts/build_qt_mingw.ps1) {
    message("OpenCV: external/WhereLogicOpenCV not found — CONFIG+=no_opencv")
    CONFIG += no_opencv
} else:win32-g++ {
    # Build at qmake time if prebuilt missing (Qt Creator: Build → qmake runs first)
    !exists($$OPENCV_STAMP)|contains(CONFIG, rebuild_opencv) {
        message("OpenCV: building prebuilt (Qt MinGW, may take ~15 min)...")
        OPENCV_QMAKE_BUILD = powershell -NoProfile -ExecutionPolicy Bypass -File \"$$OPENCV_BUILD_SCRIPT\"
        contains(CONFIG, rebuild_opencv) {
            OPENCV_QMAKE_BUILD = $$OPENCV_QMAKE_BUILD -Rebuild
        }
        system($$OPENCV_QMAKE_BUILD) {
            error("OpenCV build failed. Fix errors above or use CONFIG+=no_opencv.")
        }
    }

    # Ensure prebuilt exists before game links (make without re-qmake)
    !exists($$OPENCV_STAMP) {
        opencv_prebuilt.target = $$OPENCV_STAMP
        opencv_prebuilt.commands = powershell -NoProfile -ExecutionPolicy Bypass -File \"$$OPENCV_BUILD_SCRIPT\"
        QMAKE_EXTRA_TARGETS += opencv_prebuilt
        PRE_TARGETDEPS += $$OPENCV_STAMP
    }

    INCLUDEPATH += $$OPENCV_ROOT/include
    DEPENDPATH += $$OPENCV_ROOT/include

    exists($$OPENCV_ROOT/opencv_qmake.pri) {
        include($$OPENCV_ROOT/opencv_qmake.pri)
        !isEmpty(OPENCV_LIB_CORE) {
            LIBS += $$OPENCV_LIB_CORE $$OPENCV_LIB_IMGPROC $$OPENCV_LIB_IMGCODECS
            OPENCV_READY = true
        }
    }

    isEmpty(OPENCV_READY) {
        OPENCV_LIBDIR = $$OPENCV_ROOT/x64/mingw/lib
        OPENCV_CORE_CAND = $$files($$OPENCV_LIBDIR/libopencv_core*.dll.a, true)
        OPENCV_IMGPROC_CAND = $$files($$OPENCV_LIBDIR/libopencv_imgproc*.dll.a, true)
        OPENCV_IMGCODECS_CAND = $$files($$OPENCV_LIBDIR/libopencv_imgcodecs*.dll.a, true)

        OPENCV_CORE =
        for (lib, $$OPENCV_CORE_CAND) {
            contains(lib, opencv_core4) {
                OPENCV_CORE = $$lib
                break()
            }
        }
        isEmpty(OPENCV_CORE):!isEmpty(OPENCV_CORE_CAND) {
            OPENCV_CORE = $$first($$OPENCV_CORE_CAND)
        }

        OPENCV_IMGPROC =
        for (lib, $$OPENCV_IMGPROC_CAND) {
            contains(lib, opencv_imgproc4) {
                OPENCV_IMGPROC = $$lib
                break()
            }
        }
        isEmpty(OPENCV_IMGPROC):!isEmpty(OPENCV_IMGPROC_CAND) {
            OPENCV_IMGPROC = $$first($$OPENCV_IMGPROC_CAND)
        }

        OPENCV_IMGCODECS =
        for (lib, $$OPENCV_IMGCODECS_CAND) {
            contains(lib, opencv_imgcodecs4) {
                OPENCV_IMGCODECS = $$lib
                break()
            }
        }
        isEmpty(OPENCV_IMGCODECS):!isEmpty(OPENCV_IMGCODECS_CAND) {
            OPENCV_IMGCODECS = $$first($$OPENCV_IMGCODECS_CAND)
        }

        !isEmpty(OPENCV_CORE):!isEmpty(OPENCV_IMGPROC):!isEmpty(OPENCV_IMGCODECS) {
            LIBS += $$OPENCV_CORE $$OPENCV_IMGPROC $$OPENCV_IMGCODECS
            OPENCV_BINDIR = $$OPENCV_ROOT/x64/mingw/bin
            OPENCV_READY = true
        }
    }

    equals(OPENCV_READY, true) {
        isEmpty(OPENCV_BINDIR) {
            OPENCV_BINDIR = $$OPENCV_ROOT/x64/mingw/bin
        }
    } else {
        message("OpenCV MinGW: prebuilt libs missing in $$OPENCV_ROOT/x64/mingw/lib")
    }
} else:win32-msvc* {
    OPENCV_LIBDIR = $$OPENCV_ROOT/x64/msvc/lib
    !exists($$OPENCV_LIBDIR) {
        OPENCV_LIBDIR = $$OPENCV_ROOT/x64/vc16/lib
    }
    !exists($$OPENCV_LIBDIR) {
        OPENCV_LIBDIR = $$OPENCV_ROOT/lib
    }
    exists($$OPENCV_HEADER) {
        INCLUDEPATH += $$OPENCV_ROOT/include
        DEPENDPATH += $$OPENCV_ROOT/include
        OPENCV_WORLD = $$files($$OPENCV_LIBDIR/opencv_world*.lib, true)
        !isEmpty(OPENCV_WORLD) {
            LIBS += $$OPENCV_WORLD
            OPENCV_READY = true
        } else {
            OPENCV_CORE = $$files($$OPENCV_LIBDIR/opencv_core*.lib, true)
            OPENCV_IMGPROC = $$files($$OPENCV_LIBDIR/opencv_imgproc*.lib, true)
            OPENCV_IMGCODECS = $$files($$OPENCV_LIBDIR/opencv_imgcodecs*.lib, true)
            !isEmpty(OPENCV_CORE):!isEmpty(OPENCV_IMGPROC):!isEmpty(OPENCV_IMGCODECS) {
                LIBS += $$OPENCV_CORE $$OPENCV_IMGPROC $$OPENCV_IMGCODECS
                OPENCV_READY = true
            }
        }
        OPENCV_BINDIR = $$OPENCV_ROOT/x64/msvc/bin
        !exists($$OPENCV_BINDIR) {
            OPENCV_BINDIR = $$OPENCV_ROOT/x64/vc16/bin
        }
        !exists($$OPENCV_BINDIR) {
            OPENCV_BINDIR = $$OPENCV_ROOT/bin
        }
    } else {
        message("OpenCV MSVC: fetch prebuilt (scripts/fetch_ai_deps.py --id opencv_win_msvc)")
    }
} else:unix:!macx {
    exists($$OPENCV_HEADER) {
        INCLUDEPATH += $$OPENCV_ROOT/include
        OPENCV_LIBDIR = $$OPENCV_ROOT/linux/lib
        !exists($$OPENCV_LIBDIR) {
            OPENCV_LIBDIR = $$OPENCV_ROOT/lib
        }
        exists($$OPENCV_LIBDIR) {
            LIBS += -L$$OPENCV_LIBDIR -lopencv_core -lopencv_imgproc -lopencv_imgcodecs
            OPENCV_READY = true
        }
    }
} else:macx {
    exists($$OPENCV_HEADER) {
        INCLUDEPATH += $$OPENCV_ROOT/include
        OPENCV_LIBDIR = $$OPENCV_ROOT/macos/lib
        !exists($$OPENCV_LIBDIR) {
            OPENCV_LIBDIR = $$OPENCV_ROOT/lib
        }
        exists($$OPENCV_LIBDIR) {
            LIBS += -L$$OPENCV_LIBDIR -lopencv_core -lopencv_imgproc -lopencv_imgcodecs
            OPENCV_READY = true
        }
    }
}

equals(OPENCV_READY, true) {
    CONFIG -= no_opencv
    message("OpenCV enabled: $$OPENCV_ROOT")

    win32:!isEmpty(OPENCV_BINDIR):exists($$OPENCV_BINDIR) {
        OPENCV_COPY_SCRIPT = $$clean_path($$OPENCV_EXTERNAL/scripts/copy_dlls.ps1)
        CONFIG(debug, debug|release) {
            OPENCV_DLL_DEST = $$shell_path($$OUT_PWD/debug)
        } else {
            OPENCV_DLL_DEST = $$shell_path($$OUT_PWD/release)
        }
        QMAKE_POST_LINK += $$escape_expand(\\n\\t) powershell -NoProfile -ExecutionPolicy Bypass -File \"$$OPENCV_COPY_SCRIPT\" -DestDir \"$$OPENCV_DLL_DEST\"
        message("OpenCV: POST_LINK copy DLLs to $$OPENCV_DLL_DEST")
    }
} else {
    !contains(CONFIG, no_opencv) {
        CONFIG += no_opencv
        message("OpenCV: unavailable — CONFIG+=no_opencv")
    }
}
