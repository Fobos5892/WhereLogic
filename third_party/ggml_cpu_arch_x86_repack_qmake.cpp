// QMake workaround:
// ggml-cpu/repack.cpp and ggml-cpu/arch/x86/repack.cpp share basename
// `repack`, which causes object name collisions (`repack.o`) in qmake.
// Include x86 variant through a uniquely named wrapper.

#include "whisper.cpp/ggml/src/ggml-cpu/arch/x86/repack.cpp"
