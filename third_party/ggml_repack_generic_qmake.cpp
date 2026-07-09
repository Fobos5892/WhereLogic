// QMake/MinGW: portable ggml repack kernels (no arch/x86 SIMD).
// Wrapper ensures GGML_CPU_GENERIC is active for arch-fallback.h renaming.

#ifndef GGML_CPU_GENERIC
#define GGML_CPU_GENERIC
#endif

#include "whisper.cpp/ggml/src/ggml-cpu/repack.cpp"
