# Whisper integration stub.
# Remove no_whisper from CONFIG when whisper.cpp is vendored.

WHISPER_ROOT = $$PWD/whisper.cpp
WHISPER_INCLUDE = $$WHISPER_ROOT/include
GGML_INCLUDE = $$WHISPER_ROOT/ggml/include
WHISPER_SRC_DIR = $$WHISPER_ROOT/src
GGML_SRC_DIR = $$WHISPER_ROOT/ggml/src
GGML_CPU_SRC_DIR = $$GGML_SRC_DIR/ggml-cpu

exists($$WHISPER_INCLUDE/whisper.h) {
    CONFIG -= no_whisper
    INCLUDEPATH += \
        $$WHISPER_INCLUDE \
        $$GGML_INCLUDE \
        $$WHISPER_SRC_DIR \
        $$GGML_SRC_DIR \
        $$GGML_CPU_SRC_DIR
    DEFINES += WHISPER_BUILD
    DEFINES += WHISPER_VERSION=\\\"1.9.1\\\"
    DEFINES += GGML_VERSION=\\\"1.9.1\\\"
    DEFINES += GGML_COMMIT=\\\"local\\\"
    DEFINES += GGML_USE_CPU

    # MinGW: use portable scalar ggml kernels. arch/x86 quants/repack can hang
    # during whisper_encode_internal(); wrappers force GGML_CPU_GENERIC renaming.
    win32-g++ {
        DEFINES += GGML_CPU_GENERIC
        GGML_QUANTS_SRC = $$PWD/ggml_quants_generic_qmake.c
        GGML_REPACK_SRC = $$PWD/ggml_repack_generic_qmake.cpp
        message("Whisper: using portable ggml CPU kernels (GGML_CPU_GENERIC)")
    } else {
        GGML_QUANTS_SRC = $$GGML_CPU_SRC_DIR/quants.c
        GGML_REPACK_SRC = $$GGML_CPU_SRC_DIR/repack.cpp
    }

    SOURCES += \
        $$WHISPER_SRC_DIR/whisper.cpp \
        $$PWD/ggml_core_c_qmake.c \
        $$GGML_SRC_DIR/ggml-alloc.c \
        $$GGML_SRC_DIR/ggml-backend-dl.cpp \
        $$GGML_SRC_DIR/ggml-backend.cpp \
        $$GGML_SRC_DIR/ggml-backend-reg.cpp \
        $$GGML_SRC_DIR/ggml-backend-meta.cpp \
        $$GGML_SRC_DIR/ggml-threading.cpp \
        $$GGML_SRC_DIR/gguf.cpp \
        $$GGML_SRC_DIR/ggml.cpp \
        $$GGML_SRC_DIR/ggml-opt.cpp \
        $$GGML_SRC_DIR/ggml-quants.c \
        $$PWD/ggml_cpu_c_qmake.c \
        $$GGML_CPU_SRC_DIR/ggml-cpu.cpp \
        $$GGML_QUANTS_SRC \
        $$GGML_CPU_SRC_DIR/binary-ops.cpp \
        $$GGML_CPU_SRC_DIR/unary-ops.cpp \
        $$GGML_CPU_SRC_DIR/ops.cpp \
        $$GGML_CPU_SRC_DIR/vec.cpp \
        $$GGML_REPACK_SRC \
        $$GGML_CPU_SRC_DIR/traits.cpp \
        $$GGML_CPU_SRC_DIR/hbm.cpp

    !win32-g++ {
        SOURCES += \
            $$PWD/ggml_cpu_arch_x86_quants_qmake.c \
            $$PWD/ggml_cpu_arch_x86_repack_qmake.cpp
    }

    message("Whisper headers found at $$WHISPER_INCLUDE")
} else {
    message("Whisper not found — run: git submodule update --init third_party/whisper.cpp")
    message("  or: powershell -File scripts/fetch_deps.ps1 -Whisper")
}
