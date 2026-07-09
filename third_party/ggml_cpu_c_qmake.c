// QMake workaround:
// ggml-cpu.c and ggml-cpu.cpp share the same basename, which makes qmake
// generate the same object file name (ggml-cpu.o) for both translation units.
// We include ggml-cpu.c through a uniquely named wrapper to avoid object
// collisions while keeping upstream source untouched.
//
// MinGW/Windows SDK compatibility:
// Some kits don't expose THREAD_POWER_THROTTLING_* declarations used behind
// `_WIN32_WINNT >= 0x0602` in ggml-cpu.c. Lowering `_WIN32_WINNT` for this
// translation unit disables that optional optimization path while preserving
// core functionality.

#if defined(_WIN32)
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif

#include "whisper.cpp/ggml/src/ggml-cpu/ggml-cpu.c"
