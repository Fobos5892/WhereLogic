// QMake workaround:
// ggml-cpu/quants.c and ggml-cpu/arch/x86/quants.c share basename `quants`,
// which causes object name collisions (`quants.o`) in qmake.
// Include x86 variant through a uniquely named wrapper.

#include "whisper.cpp/ggml/src/ggml-cpu/arch/x86/quants.c"
